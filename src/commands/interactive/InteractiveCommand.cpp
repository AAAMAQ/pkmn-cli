#include "commands/interactive/InteractiveCommand.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include "app/CommandCatalog.hpp"
#include "app/CommandRouter.hpp"
#include "app/ExitCode.hpp"
#include "commands/interactive/options/Workflow.hpp"

namespace pkmn::cli::commands::interactive {
namespace {
namespace fs = std::filesystem;
using options::Answers;
using options::EmitKind;
using options::Field;
using options::InputKind;
using options::Workflow;

enum class Nav { Stay, Back, Quit, Main, Browser, Inspect, Compare };
struct Reply { Nav nav = Nav::Stay; std::string value; };

std::string Trim(std::string s) {
  auto nonspace = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), nonspace));
  s.erase(std::find_if(s.rbegin(), s.rend(), nonspace).base(), s.end());
  return s;
}
std::string Lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}
bool Starts(std::string_view s, std::string_view prefix) {
  return s.substr(0, prefix.size()) == prefix;
}
Reply Read(std::istream &in, std::ostream &out, std::string_view prompt) {
  out << prompt;
  out.flush();
  std::string value;
  if (!std::getline(in, value)) return {Nav::Quit, {}};
  value = Trim(std::move(value));
  const auto low = Lower(value);
  if (low == "q" || low == "quit" || low == "exit") return {Nav::Quit, {}};
  if (low == "b" || low == "back") return {Nav::Back, {}};
  return {Nav::Stay, std::move(value)};
}
bool Help(const std::string &s) { return s == "?" || Lower(s) == "help"; }
bool Yes(const std::string &s) {
  const auto v = Lower(s); return v == "yes" || v == "y" || v == "1";
}
bool No(const std::string &s) {
  const auto v = Lower(s); return v == "no" || v == "n" || v == "2";
}
std::optional<std::size_t> Number(const std::string &s) {
  std::size_t value=0;
  const auto [end,error]=std::from_chars(s.data(),s.data()+s.size(),value);
  if (error!=std::errc{} || end!=s.data()+s.size()) return std::nullopt;
  return value;
}
std::string Get(const Answers &a, std::string_view key) {
  auto it = a.find(std::string(key)); return it == a.end() ? "" : it->second;
}
bool Visible(const Field &f, const Answers &a) {
  return f.visibleWhenKey.empty() || Get(a, f.visibleWhenKey) == f.visibleWhenValue;
}
void ClearHidden(const Workflow &w, Answers &a) {
  for (const auto &f : w.fields) if (!Visible(f, a)) a.erase(f.key);
}
std::vector<std::string> Lines(const std::string &s) {
  std::vector<std::string> result;
  std::istringstream stream(s);
  std::string line;
  while (std::getline(stream, line)) if (!line.empty()) result.push_back(line);
  return result;
}
std::vector<std::string> Words(const std::string &s) {
  std::vector<std::string> result;
  std::istringstream stream(s);
  std::string word;
  while (stream >> word) result.push_back(word);
  return result;
}
std::vector<std::string> Arguments(const Workflow &w, const Answers &a) {
  if (w.buildArguments) return w.buildArguments(a);
  auto argv = Words(w.path);
  for (const auto &f : w.fields) {
    if (!Visible(f, a) || f.emit == EmitKind::None) continue;
    const auto value = Get(a, f.key);
    if (value.empty()) continue;
    if (f.emit == EmitKind::Switch) {
      if (Yes(value)) argv.push_back(f.option);
    } else if (f.emit == EmitKind::TokenChoice) argv.push_back(value);
    else if (f.emit == EmitKind::Option) {
      argv.push_back(f.option); argv.push_back(value);
    } else if (f.kind == InputKind::FileList) {
      for (const auto &file : Lines(value)) argv.push_back(file);
    } else argv.push_back(value);
  }
  return argv;
}
std::string Quote(const std::string &s) {
  if (s.find_first_of(" \t\n'\"\\") == std::string::npos) return s;
  std::string out = "'";
  for (char c : s) { if (c == '\'') out += "'\\''"; else out += c; }
  return out + "'";
}
std::string Command(const std::vector<std::string> &argv) {
  std::string s = "pkmn";
  for (const auto &part : argv) s += " " + Quote(part);
  return s;
}
const Workflow *Find(const std::string &path) {
  for (const auto &w : options::AllWorkflows()) if (w.path == path) return &w;
  return nullptr;
}
fs::path Normalize(const std::string &raw) {
  std::string s = raw;
  if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') ||
                         (s.front() == '\'' && s.back() == '\'')))
    s = s.substr(1, s.size()-2);
  std::string decoded;
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '\\' && i+1 < s.size() &&
        (s[i+1] == ' ' || s[i+1] == '\\' || s[i+1] == '(' ||
         s[i+1] == ')' || s[i+1] == '"' || s[i+1] == '\'')) ++i;
    decoded.push_back(s[i]);
  }
  if (decoded == "~" || Starts(decoded, "~/")) {
    const char *home = std::getenv("HOME");
    if (!home) throw std::invalid_argument("Your home folder is unavailable; enter an absolute path.");
    decoded = std::string(home) + decoded.substr(1);
  } else if (Starts(decoded, "~")) {
    throw std::invalid_argument("Use ~/ for your home folder or enter an absolute path.");
  }
  if (decoded.empty()) throw std::invalid_argument("Enter a path.");
  std::error_code ec;
  auto path = fs::absolute(fs::path(decoded), ec);
  if (ec) throw std::invalid_argument("That path cannot be resolved: " + ec.message());
  return path.lexically_normal();
}
bool DashAllowed(const Workflow &w, const Field &f) {
  if (!Starts(w.path, "rjson ")) return false;
  if (f.kind == InputKind::ExistingFile)
    return w.path == "rjson inspect" || w.path == "rjson validate" ||
           w.path == "rjson generate" || w.path == "rjson reconstruct";
  return f.kind == InputKind::OutputFile &&
         (w.path == "rjson generate" || w.path == "rjson reconstruct");
}
std::string ExpectedExtension(const Field &f) {
  const auto q = Lower(f.question);
  if (q.find("save") != std::string::npos) return ".sav";
  if (q.find("json") != std::string::npos || q.find("session") != std::string::npos ||
      q.find("manifest") != std::string::npos) return ".json";
  return "";
}
Reply Browse(std::istream &in, std::ostream &out, fs::path folder, const Field &f) {
  bool all = false;
  std::size_t page = 0;
  const auto extension = ExpectedExtension(f);
  for (;;) {
    if (!fs::is_directory(folder)) {
      out << "That folder does not exist. Enter another folder or B to go back.\n";
      auto r = Read(in, out, "Folder path: ");
      if (r.nav != Nav::Stay) return r;
      try { folder = Normalize(r.value); } catch (const std::exception &e) { out << e.what() << '\n'; }
      continue;
    }
    std::vector<fs::path> entries;
    std::error_code ec;
    for (fs::directory_iterator it(folder, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
      std::error_code typeError;
      const auto path = it->path();
      if (it->is_directory(typeError) ||
          (it->is_regular_file(typeError) &&
           (all || extension.empty() || Lower(path.extension().string()) == extension ||
            (extension == ".json" && Lower(path.extension().string()) == ".zip"))))
        entries.push_back(path);
    }
    std::sort(entries.begin(), entries.end(), [](const fs::path &a, const fs::path &b) {
      return Lower(a.filename().string()) < Lower(b.filename().string());
    });
    const auto pages = std::max<std::size_t>(1, (entries.size()+9)/10);
    if (page >= pages) page = pages-1;
    out << "\nBrowse " << folder.string() << " (page " << page+1 << '/' << pages << ")\n";
    const auto first = page*10;
    for (std::size_t i=first; i<std::min(entries.size(), first+10); ++i)
      out << ' ' << i-first+1 << "  " << entries[i].filename().string()
          << (fs::is_directory(entries[i]) ? "/" : "") << '\n';
    if (entries.empty()) out << "No matching files are visible here.\n";
    out << "N next, P previous, U parent, D another folder, A "
        << (all ? "filter" : "show all") << ", B back, Q quit.\n";
    auto r = Read(in, out, "Choice: ");
    if (r.nav != Nav::Stay) return r;
    const auto v = Lower(r.value);
    if (Help(r.value)) out << "Choose a listed file, open a folder, or change folders.\n";
    else if (v == "n" || v == "next") { if (page+1 < pages) ++page; }
    else if (v == "p" || v == "previous") { if (page) --page; }
    else if (v == "u" || v == "up") { folder = folder.parent_path(); page=0; }
    else if (v == "a" || v == "all") { all=!all; page=0; }
    else if (v == "d" || v == "folder") {
      auto entered = Read(in, out, "Folder path: ");
      if (entered.nav != Nav::Stay) return entered;
      try { folder=Normalize(entered.value); page=0; }
      catch (const std::exception &e) { out << e.what() << '\n'; }
    } else {
      if (const auto selected=Number(v); selected && *selected>=1 && *selected<=10 &&
          first+*selected<=entries.size()) {
        const auto index=first+*selected-1;
        if (fs::is_directory(entries[index])) { folder=entries[index]; page=0; }
        else return {Nav::Stay, entries[index].lexically_normal().string()};
      } else out << "Choose a listed number or control.\n";
    }
  }
}
Reply Missing(std::istream &in, std::ostream &out, const fs::path &path, const Field &f) {
  out << "I could not find that file: " << path.string() << '\n';
  if (fs::is_directory(path.parent_path())) {
    std::error_code ec;
    std::size_t shown=0;
    for (fs::directory_iterator it(path.parent_path(), ec), end;
         !ec && it!=end && shown<3; it.increment(ec)) {
      if (it->is_regular_file() &&
          Lower(it->path().filename().string()).find(Lower(path.stem().string())) != std::string::npos) {
        out << "Possible match: " << it->path().filename().string() << '\n';
        ++shown;
      }
    }
  }
  for (;;) {
    out << "1 Try again  2 Browse nearby  3 Browse another folder  B Back  Q Quit\n";
    auto r=Read(in,out,"Choice: ");
    if (r.nav!=Nav::Stay) return r;
    if (Help(r.value)) out << "You can retry the path, choose a file, or go back.\n";
    else if (r.value=="1" || Lower(r.value)=="retry") return {Nav::Stay,{}};
    else if (r.value=="2") return Browse(in,out,fs::is_directory(path.parent_path()) ?
                                           path.parent_path() : fs::current_path(),f);
    else if (r.value=="3") return Browse(in,out,fs::current_path(),f);
    else out << "Choose 1, 2, 3, B, or Q.\n";
  }
}
Reply FileList(std::istream &in, std::ostream &out, const Field &f, const Answers &a) {
  auto files=Lines(Get(a,f.key));
  out << f.question << " Add one existing file per line, then type Done.\n";
  if (!files.empty()) out << files.size() << " file(s) already selected.\n";
  for (;;) {
    auto r=Read(in,out,"File path (Done / Browse / B / Q): ");
    if (r.nav==Nav::Quit) return r;
    if (r.nav==Nav::Back) {
      if (files.empty()) return r;
      files.pop_back(); out << "Removed the most recent file.\n"; continue;
    }
    if (Help(r.value)) { out << "Enter a complete path per line. Done finishes; B removes the most recent file.\n"; continue; }
    if (Lower(r.value)=="done") {
      if (files.empty()) { out << "Add at least one file.\n"; continue; }
      std::string joined;
      for (const auto &file:files) { if (!joined.empty()) joined+='\n'; joined+=file; }
      return {Nav::Stay,joined};
    }
    if (Lower(r.value)=="browse") {
      r=Browse(in,out,fs::current_path(),f);
      if (r.nav!=Nav::Stay) return r;
    }
    try {
      auto path=Normalize(r.value);
      if (!fs::is_regular_file(path)) {
        r=Missing(in,out,path,f);
        if (r.nav!=Nav::Stay) return r;
        if (r.value.empty()) continue;
        path=Normalize(r.value);
      }
      if (std::find(files.begin(),files.end(),path.string())!=files.end())
        out << "That file is already selected.\n";
      else { files.push_back(path.string()); out << "Added: " << path.string() << '\n'; }
    } catch (const std::exception &e) { out << e.what() << '\n'; }
  }
}
Reply FieldPrompt(std::istream &in, std::ostream &out, const Workflow &w,
                  const Field &f, const Answers &a) {
  if (f.kind==InputKind::FileList) return FileList(in,out,f,a);
  for (;;) {
    out << '\n' << f.question << '\n';
    if (f.kind==InputKind::Choice)
      for (std::size_t i=0;i<f.choices.size();++i)
        out << ' ' << i+1 << "  " << f.choices[i] << '\n';
    const auto previous=Get(a,f.key);
    if (!previous.empty()) out << "Current: " << previous << " (Enter keeps it)\n";
    else if (!f.defaultValue.empty()) out << "Default: " << f.defaultValue << '\n';
    if (f.optional) out << "Enter skips this option; Clear removes a previous answer.\n";
    auto r=Read(in,out,"Answer (? help / B back / Q quit): ");
    if (r.nav!=Nav::Stay) return r;
    if (Help(r.value)) { out << (f.help.empty() ? "Choose a listed option or enter the requested value." : f.help) << '\n'; continue; }
    if (Lower(r.value)=="clear" && f.optional) return {Nav::Stay,{}};
    if (r.value.empty()) {
      if (!previous.empty()) return {Nav::Stay,previous};
      if (!f.defaultValue.empty()) return {Nav::Stay,f.defaultValue};
      if (f.optional) return {Nav::Stay,{}};
      out << "Please enter an answer; an empty line is never approval.\n"; continue;
    }
    if (f.kind==InputKind::Choice) {
      std::optional<std::size_t> match;
      if (const auto n=Number(r.value); n && *n>=1 && *n<=f.choices.size())
        match=*n-1;
      if (!match) {
        auto needle=Lower(r.value);
        for (std::size_t i=0;i<f.choices.size();++i) {
          auto label=Lower(f.choices[i]);
          auto value=i<f.values.size() ? Lower(f.values[i]) : label;
          if (needle==label || needle==value) { match=i; break; }
        }
      }
      if (!match) {
        auto needle=Lower(r.value);
        for (std::size_t i=0;i<f.choices.size();++i) if (Starts(Lower(f.choices[i]),needle)) {
          if (match) { match.reset(); break; }
          match=i;
        }
      }
      if (!match) { out << "Choose a listed number or unambiguous name.\n"; continue; }
      return {Nav::Stay,*match<f.values.size() ? f.values[*match] : f.choices[*match]};
    }
    if (f.kind==InputKind::Integer) {
      try {
        std::size_t used=0;
        auto n=std::stoll(r.value,&used);
        if (used!=r.value.size() ||
            ((f.minimum!=0 || f.maximum!=0) && (n<f.minimum || n>f.maximum)))
          throw std::invalid_argument("range");
        return {Nav::Stay,std::to_string(n)};
      } catch (...) {
        out << "Enter a whole number";
        if (f.minimum!=0 || f.maximum!=0) out << " from " << f.minimum << " to " << f.maximum;
        out << ".\n"; continue;
      }
    }
    const bool proofPackage=w.path=="proof verify" && f.key=="package";
    const bool proofFolder=w.path=="proof post-emulator" && f.key=="directory";
    if (f.kind==InputKind::ExistingFile || f.kind==InputKind::ExistingDirectory ||
        f.kind==InputKind::OutputFile || f.kind==InputKind::OutputDirectory ||
        proofPackage || proofFolder) {
      if (r.value=="-" && DashAllowed(w,f)) return r;
      if (Lower(r.value)=="browse" && (f.kind==InputKind::ExistingFile || proofPackage)) {
        r=Browse(in,out,fs::current_path(),f);
        if (r.nav!=Nav::Stay) return r;
      }
      try {
        auto path=Normalize(r.value);
        if (f.kind==InputKind::ExistingFile && !fs::is_regular_file(path)) {
          r=Missing(in,out,path,f);
          if (r.nav!=Nav::Stay) return r;
          if (r.value.empty()) continue;
          path=Normalize(r.value);
        }
        if (f.kind==InputKind::ExistingDirectory && !fs::is_directory(path)) {
          out << "That folder does not exist.\n"; continue;
        }
        if (proofPackage && !fs::is_directory(path) && !fs::is_regular_file(path)) {
          out << "Choose an existing proof folder or ZIP.\n"; continue;
        }
        if (proofFolder && Get(a,"mode")=="existing" && !fs::is_directory(path)) {
          out << "Choose an existing proof folder.\n"; continue;
        }
        if ((f.kind==InputKind::OutputFile || f.kind==InputKind::OutputDirectory ||
             (proofFolder && Get(a,"mode")=="new")) &&
            !fs::is_directory(path.parent_path())) {
          out << "The parent folder does not exist.\n"; continue;
        }
        out << "Resolved path: " << path.string() << '\n';
        return {Nav::Stay,path.string()};
      } catch (const std::exception &e) { out << e.what() << '\n'; continue; }
    }
    return r;
  }
}

struct Destination { fs::path path; bool directory=false; bool updated=false; };
std::vector<fs::path> Sources(const Workflow &w, const Answers &a) {
  std::vector<fs::path> result;
  for (const auto &f:w.fields) {
    if (!Visible(f,a)) continue;
    const auto v=Get(a,f.key);
    if (v.empty() || v=="-") continue;
    if (f.kind==InputKind::ExistingFile || f.kind==InputKind::ExistingDirectory)
      result.emplace_back(v);
    if (f.kind==InputKind::FileList)
      for (const auto &line:Lines(v)) result.emplace_back(line);
  }
  return result;
}
bool Conversion(const std::string &path) {
  return path=="red convert" || path=="blue convert" ||
         path=="rjson convert" || path=="bjson convert" ||
         Starts(path,"convert red-") || Starts(path,"convert blue-") ||
         path.find("convert_to_")!=std::string::npos ||
         path=="red-jp convert" || path=="green-jp convert";
}
std::string ConversionDefault(const Workflow &w, const Answers &a) {
  auto source=Get(a,"source");
  if (source.empty()) source=Get(a,"json");
  if (source.empty()) return "";
  fs::path input(source);
  auto name=input.filename().string();
  bool stripped=false;
  for (const std::string suffix:{".red.json",".blue.json"}) {
    if (name.ends_with(suffix)) { name.resize(name.size()-suffix.size()); stripped=true; break; }
  }
  if (!stripped) name=input.stem().string();
  const bool leaf=w.path.find("leafgreen")!=std::string::npos ||
                  w.path.find("lgjson")!=std::string::npos ||
                  Get(a,"target")=="leafgreen";
  const bool plan=w.path.find("convert_to_")!=std::string::npos ||
                  Get(a,"planOnly")=="yes";
  return (input.parent_path() /
          (name+(plan ? (leaf ? ".lg.json" : ".fred.json") :
                        (leaf ? "_lg.sav" : "_fr.sav")))).string();
}
fs::path Numbered(const fs::path &path, std::size_t number) {
  return path.parent_path() / (path.stem().string()+"_"+std::to_string(number)+
                               path.extension().string());
}
std::vector<fs::path> OutputFamily(const Workflow &w,const Answers &a,
                                   const fs::path &path) {
  std::vector<fs::path> family{path};
  auto stem=path; stem.replace_extension();
  if (Conversion(w.path)) {
    if (Get(a,"manifest").empty())
      family.emplace_back(stem.string()+".conversion-manifest.json");
    if (Get(a,"report").empty())
      family.emplace_back(stem.string()+".conversion-report.md");
    if (Get(a,"keepIntermediate")=="yes" && path.extension()==".sav")
      family.emplace_back(stem.string() +
          (w.path.find("leafgreen")!=std::string::npos ||
           Get(a,"target")=="leafgreen" || w.path=="bjson convert" ?
             ".lg.json" : ".fred.json"));
  }
  if (w.path=="red repair-checksums")
    family.emplace_back(path.string()+".repair-report.json");
  if (w.path=="rjson generate") {
    family.emplace_back(path.string()+".generation-report.json");
    family.emplace_back(path.string()+".generation-report.md");
  }
  if (w.path=="red end-edit" && Get(a,"dryRun")!="yes") {
    family.emplace_back(path.string()+".edit-report.json");
    family.emplace_back(path.string()+".edit-report.md");
  }
  if (w.path=="frjson generate" || w.path=="lgjson generate")
    family.emplace_back(stem.string()+".generation-report.json");
  if (w.path=="proof red" && Get(a,"zipOutput").empty() && Get(a,"zip")=="yes")
    family.emplace_back(path.string()+".zip");
  return family;
}
bool FamilyExists(const Workflow &w,const Answers &a,const fs::path &path) {
  for (const auto &member:OutputFamily(w,a,path)) if (fs::exists(member)) return true;
  // The direct conversion handler reserves its default audit names even when
  // the user supplies custom manifest or report paths.
  if (Conversion(w.path)) {
    auto stem=path; stem.replace_extension();
    if (fs::exists(fs::path(stem.string()+".conversion-manifest.json")) ||
        fs::exists(fs::path(stem.string()+".conversion-report.md"))) return true;
  }
  return false;
}
bool UpdatesSession(const Workflow &w,const Answers &a) {
  static const std::set<std::string> commands{
    "red edit-session","red pokemon","red bag","red progress",
    "red undo-edit","red annotate-edit","fred edit-session",
    "fred pokemon","fred bag","fred progress","fred undo-edit",
    "fred annotate-edit"};
  return commands.contains(w.path) && Get(a,"dryRun")!="yes";
}
bool WillWrite(const Workflow &w, const Answers &a) {
  if (Get(a,"dryRun")=="yes") return false;
  if (w.path=="get-all-cmds" || w.path=="red summary")
    return !Get(a,"output").empty();
  return w.writes;
}
void ResolveAutoSuffix(const Workflow &w, Answers &a, std::ostream &out) {
  if (w.path=="red end-edit" && Get(a,"dryRun")=="yes") return;
  const bool suffix=Get(a,"autoSuffix")=="yes" || Get(a,"auto_suffix")=="yes";
  if (!suffix) return;
  auto resolve=[&](const std::string &key,bool family) {
    const auto path=fs::path(Get(a,key));
    if (path.empty() || path=="-") return;
    const auto taken=[&](const fs::path &candidate) {
      return family ? FamilyExists(w,a,candidate) : fs::exists(candidate);
    };
    if (!taken(path)) return;
    for (std::size_t number=2;number<100000;++number) {
      const auto candidate=Numbered(path,number);
      if (!taken(candidate)) {
        a[key]=candidate.string();
        out << "The requested output is taken. Selected numbered output: "
            << candidate.string() << '\n';
        return;
      }
    }
    throw std::runtime_error("No available numbered output name was found.");
  };
  if (!Get(a,"output").empty()) resolve("output",true);
  else if (!Get(a,"outputDir").empty()) resolve("outputDir",true);
  if (w.path=="proof red" && !Get(a,"zipOutput").empty())
    resolve("zipOutput",false);
}
std::vector<Destination> Destinations(const Workflow &w, const Answers &a) {
  std::vector<Destination> result;
  if (w.path=="red end-edit" && Get(a,"dryRun")=="yes") return result;
  for (const auto &f:w.fields) {
    if (!Visible(f,a)) continue;
    const auto v=Get(a,f.key);
    if (v.empty() || v=="-") continue;
    if (f.kind==InputKind::OutputFile || f.kind==InputKind::OutputDirectory)
      result.push_back({fs::path(v),f.kind==InputKind::OutputDirectory});
  }
  if (Conversion(w.path)) {
    auto output=Get(a,"output");
    if (output.empty()) output=ConversionDefault(w,a);
    if (!output.empty() && output!="-") {
      const fs::path target(output);
      for (const auto &member:OutputFamily(w,a,target))
        if (std::none_of(result.begin(),result.end(),
            [&](const Destination &d){return d.path==member;}))
          result.push_back({member});
    }
  } else if (const auto output=Get(a,"output"); !output.empty() && output!="-") {
    for (const auto &member:OutputFamily(w,a,fs::path(output)))
      if (std::none_of(result.begin(),result.end(),
          [&](const Destination &d){return d.path==member;}))
        result.push_back({member});
  }
  if (w.path=="proof red" && !Get(a,"outputDir").empty()) {
    for (const auto &member:OutputFamily(w,a,fs::path(Get(a,"outputDir"))))
      if (std::none_of(result.begin(),result.end(),
          [&](const Destination &d){return d.path==member;}))
        result.push_back({member});
  }
  if (w.path=="red-jp convert" || w.path=="green-jp convert") {
    auto source=Get(a,"source");
    if (!source.empty()) {
      auto stem=fs::path(source); stem.replace_extension();
      result.push_back({fs::path(stem.string()+(w.path=="green-jp convert" ? ".green.jp.json" : ".red.jp.json"))});
      result.push_back({fs::path(stem.string()+".red.json")});
    }
  }
  if (w.path=="proof post-emulator" && !Get(a,"directory").empty()) {
    const fs::path directory(Get(a,"directory"));
    if (Get(a,"mode")=="new") result.push_back({directory,true});
    result.push_back({directory/"post-emulator-validation.json"});
    result.push_back({directory/"post-emulator-validation.md"});
    if (Get(a,"mode")=="existing")
      result.push_back({directory/"proof-manifest.json",false,true});
  }
  if (UpdatesSession(w,a) && !Get(a,"session").empty())
    result.push_back({fs::path(Get(a,"session")),false,true});
  return result;
}
std::vector<std::string> Preflight(const Workflow &w,const Answers &a,
                                   const std::vector<Destination> &dest) {
  std::vector<std::string> errors;
  const auto sources=Sources(w,a);
  std::set<std::string> seen;
  for (const auto &d:dest) {
    if (d.path.empty()) continue;
    const auto p=d.path.lexically_normal();
    if (!seen.insert(p.string()).second)
      errors.push_back("Two outputs have the same path: "+p.string());
    for (const auto &source:sources)
      if (!d.updated && source.lexically_normal()==p)
        errors.push_back("An output would replace its input: "+p.string());
    if (fs::exists(p) && !d.updated)
      errors.push_back("An output already exists: "+p.string());
  }
  const auto source=Get(a,"source");
  const bool sourceJson=fs::path(source).extension()==".json";
  if (Conversion(w.path) && sourceJson &&
      (Get(a,"repair")=="yes" || !Get(a,"repairedSource").empty()))
    errors.push_back("Checksum repair applies to a physical Gen I save, not a JSON source.");
  if (w.path=="proof post-emulator" && Get(a,"mode")=="existing") {
    const auto directory=fs::path(Get(a,"directory"));
    if (!fs::is_directory(directory))
      errors.push_back("Choose an existing proof folder.");
    else if (!fs::is_regular_file(directory/"proof-manifest.json"))
      errors.push_back("That proof folder has no proof-manifest.json file.");
  }
  return errors;
}
void Review(std::ostream &out,const Workflow &w,const Answers &a,
            const std::vector<std::string> &argv,const std::vector<Destination> &dest) {
  out << "\nReview: " << w.path << '\n';
  if (!w.note.empty()) out << w.note << '\n';
  for (const auto &f:w.fields) {
    if (!Visible(f,a)) continue;
    const auto value=Get(a,f.key);
    if (!value.empty()) out << "- " << f.question << " " << value << '\n';
  }
  out << "Exact direct command: " << Command(argv) << '\n';
  if (!dest.empty()) {
    out << "Proposed outputs:\n";
    for (const auto &d:dest)
      out << "- " << d.path.string() << (d.updated ? " (updated)" : "") << '\n';
  } else if (WillWrite(w,a)) {
    out << "The command may write to a selected session or a documented default output.\n";
  } else out << "This task reads data and displays its result.\n";
}
class CinBridge {
 public:
  explicit CinBridge(std::istream &in): prior_(std::cin.rdbuf(in.rdbuf())) {}
  ~CinBridge() {std::cin.rdbuf(prior_);}
 private:
  std::streambuf *prior_;
};
Nav WorkflowRun(const Workflow &w,std::istream &in,std::ostream &out,std::ostream &err) {
  if (w.path=="interactive") return Nav::Main;
  Answers answers;
  std::vector<const Field*> basic,advanced;
  for (const auto &f:w.fields) (f.advanced ? advanced : basic).push_back(&f);
  std::size_t step=0;
  const auto gate=basic.size();
  const auto review=gate+(advanced.empty() ? 0 : 1+advanced.size());
  for (;;) {
    if (step<gate) {
      const auto &f=*basic[step];
      if (!Visible(f,answers)) {answers.erase(f.key);++step;continue;}
      auto r=FieldPrompt(in,out,w,f,answers);
      if (r.nav==Nav::Quit) return Nav::Quit;
      if (r.nav==Nav::Back) {if (!step) return Nav::Back;--step;continue;}
      answers[f.key]=r.value; ClearHidden(w,answers);++step;continue;
    }
    if (!advanced.empty() && step==gate) {
      out << "\nAdvanced options are available for this command.\n";
      auto r=Read(in,out,"Review advanced options? [Y/N/?/B/Q]: ");
      if (r.nav==Nav::Quit) return Nav::Quit;
      if (r.nav==Nav::Back) {if (!step)return Nav::Back;--step;continue;}
      if (Help(r.value)) {out << "Advanced choices include specialist flags and custom paths.\n";continue;}
      if (Yes(r.value)) {++step;continue;}
      if (No(r.value)) {
        for (const auto *f:advanced) answers.erase(f->key);
        step=review;continue;
      }
      out << "Choose Yes or No.\n";continue;
    }
    if (!advanced.empty() && step>gate && step<review) {
      const auto &f=*advanced[step-gate-1];
      if (!Visible(f,answers)) {answers.erase(f.key);++step;continue;}
      auto r=FieldPrompt(in,out,w,f,answers);
      if (r.nav==Nav::Quit) return Nav::Quit;
      if (r.nav==Nav::Back) {--step;continue;}
      answers[f.key]=r.value;ClearHidden(w,answers);++step;continue;
    }
    if (step<review) {step=review;continue;}
    ClearHidden(w,answers);
    std::vector<std::string> argv;
    try {
      ResolveAutoSuffix(w,answers,out);
      argv=Arguments(w,answers);
    } catch (const std::exception &e) {
      out << "These choices need correction: " << e.what() << '\n';
      step=review ? review-1 : 0;continue;
    }
    const auto dest=Destinations(w,answers);
    Review(out,w,answers,argv,dest);
    const auto problems=Preflight(w,answers,dest);
    if (!problems.empty()) {
      for (const auto &problem:problems) out << "Please correct this: " << problem << '\n';
      auto r=Read(in,out,"E edit from the first answer, B previous answer, Q quit: ");
      if (r.nav==Nav::Quit) return Nav::Quit;
      step=Lower(r.value)=="e" ? 0 : (review ? review-1 : 0);
      continue;
    }
    const bool willWrite=WillWrite(w,answers);
    auto r=Read(in,out,willWrite ?
      "Run and write these outputs? Type YES, E edit, B back, Q quit: " :
      "Run this read-only task? Type YES, E edit, B back, Q quit: ");
    if (r.nav==Nav::Quit) return Nav::Quit;
    if (r.nav==Nav::Back || No(r.value)) {step=review ? review-1 : 0;continue;}
    if (Help(r.value)) {out << "Review the exact command and outputs above. Only YES runs it.\n";continue;}
    if (Lower(r.value)=="e" || Lower(r.value)=="edit") {step=0;continue;}
    if (Lower(r.value)!="yes") {out << "Type YES to run, E to edit, B to return, or Q to quit.\n";continue;}
    int code=1;
    try {
      CinBridge bridge(in);
      code=CommandRouter{}.Run(argv,out,err);
    } catch (const std::exception &e) {err << "pkmn interactive: " << e.what() << '\n';}
    if (code==0) {
      out << "\nTask complete.\n";
      if (!dest.empty()) {
        out << "Output locations:\n";
        for (const auto &d:dest) if (fs::exists(d.path))
          out << "- " << d.path.string() << '\n';
      }
    } else if (code==ToInt(ExitCode::SemanticMismatch) &&
               Starts(w.path,"compare "))
      out << "\nComparison complete: differences were found. Review the report above.\n";
    else out << "\nThe command reported an error (code " << code <<
                "). You can edit answers or retry without leaving guided mode.\n";
    for (;;) {
      out << "Next: 1 Main menu  2 Browse commands  3 Repeat this task  "
          << "4 Inspect or validate  5 Compare  B Edit answers  Q Quit\n";
      auto next=Read(in,out,"Choice: ");
      if (next.nav==Nav::Quit) return Nav::Quit;
      if (next.nav==Nav::Back) {step=review ? review-1 : 0;break;}
      if (Help(next.value)) {out << "Choose a next task or go back to correct an answer.\n";continue;}
      if (next.value=="1" || Lower(next.value)=="menu") return Nav::Main;
      if (next.value=="2" || Lower(next.value)=="browse") return Nav::Browser;
      if (next.value=="3" || Lower(next.value)=="repeat") {answers.clear();step=0;break;}
      if (next.value=="4") return Nav::Inspect;
      if (next.value=="5") return Nav::Compare;
      out << "Choose 1 through 5, B, or Q.\n";
    }
  }
}
std::vector<const CommandSpec*> Group(int group) {
  std::vector<const CommandSpec*> result;
  for (const auto &command:AvailableCommands()) {
    const std::string p(command.path);
    bool include=false;
    if (group==1) include=Conversion(p) || p=="convert routes" || p=="convert batch" ||
                                 Starts(p,"red-jp ") || Starts(p,"rjpjson ") || Starts(p,"green-jp ") || Starts(p,"gjpjson ");
    else if (group==2) include=Starts(p,"red-jp ") || Starts(p,"rjpjson ");
    else if (group==3) include=Starts(p,"red ") || Starts(p,"fred ") ||
                                Starts(p,"blue ") || Starts(p,"leafgreen ");
    else if (group==4) include=Starts(p,"rjson ") || Starts(p,"frjson ") ||
                                Starts(p,"bjson ") || Starts(p,"lgjson ") ||
                                Starts(p,"rjpjson ") || p.ends_with(" decode") ||
                                p.ends_with(" decode-batch");
    else if (group==5) include=p.find("edit")!=std::string::npos ||
                                p.ends_with(" pokemon") || p.ends_with(" bag") ||
                                p.ends_with(" progress");
    else if (group==6) include=Starts(p,"compare ");
    else if (group==7) include=Starts(p,"proof ") ||
                                p.find("post-emulator")!=std::string::npos;
    else if (group==9) include=command.category=="General" ||
                                Starts(p,"convert routes") || Starts(p,"convert inspect") ||
                                Starts(p,"convert explain") ||
                                Starts(p,"convert validate-manifest") ||
                                p.find("events ")!=std::string::npos;
    if (include) result.push_back(&command);
  }
  return result;
}
void Planned(std::ostream &out) {
  out << "\nPlanned commands (not executable in this build):\n";
  for (const auto &name:{
    "red-jp convert-gen1","green-jp convert-gen1",
    "convert plan","verify-conversion",
    "identify","profiles","json names","json provenance","json export-report",
    "validate-batch","batch report","json diff","save info",
    "outputs list","self-test","interactive --search"})
    out << "- " << name << " [Planned]\n";
  out << "Conversion --dry-run and expanded explain are also planned.\n";
}
Nav Browser(std::istream &in,std::ostream &out,std::ostream &err,int group=8) {
  std::string search;
  std::size_t page=0;
  for (;;) {
    std::vector<const CommandSpec*> records;
    if (group==8) for (const auto &c:AvailableCommands()) records.push_back(&c);
    else records=Group(group);
    if (!search.empty()) {
      const auto needle=Lower(search);
      records.erase(std::remove_if(records.begin(),records.end(),
        [&](const CommandSpec *c) {
          return Lower(std::string(c->path)).find(needle)==std::string::npos &&
                 Lower(std::string(c->description)).find(needle)==std::string::npos &&
                 Lower(std::string(c->category)).find(needle)==std::string::npos;
        }),records.end());
    }
    const auto pages=std::max<std::size_t>(1,(records.size()+9)/10);
    if (page>=pages) page=pages-1;
    out << "\n" << (group==8 ? "All current commands" : "Related commands")
        << " (" << records.size() << ", page " << page+1 << '/' << pages << ")\n";
    const auto first=page*10;
    for (std::size_t i=first;i<std::min(records.size(),first+10);++i) {
      const auto *workflow=Find(std::string(records[i]->path));
      out << ' ' << i-first+1 << "  " << records[i]->path
          << " [Available, guided]";
      if (workflow) {
        const auto &note=workflow->note;
        if (note.find("EXPERIMENTAL")!=std::string::npos)
          out << " [EXPERIMENTAL]";
        else if (note.find("STATICALLY_VALIDATED_COMMUNITY_TESTING")!=std::string::npos &&
                 note.find("EMULATOR_VERIFIED")!=std::string::npos)
          out << " [route-dependent evidence]";
        else if (note.find("STATICALLY_VALIDATED_COMMUNITY_TESTING")!=std::string::npos)
          out << " [STATICALLY_VALIDATED_COMMUNITY_TESTING]";
        else if (note.find("EMULATOR_VERIFIED")!=std::string::npos)
          out << " [EMULATOR_VERIFIED]";
      }
      out << " - " << records[i]->description << '\n';
    }
    if (records.empty()) out << "No current command matches that search.\n";
    out << "S search, N next page, P previous, C clear search, "
        << "R planned roadmap, B back, Q quit.\n";
    auto r=Read(in,out,"Choose a command: ");
    if (r.nav==Nav::Quit) return Nav::Quit;
    if (r.nav==Nav::Back) return Nav::Main;
    auto v=Lower(r.value);
    if (Help(r.value)) {out << "Search by game, task, or command, then choose a listed number.\n";continue;}
    if (v=="r" || v=="roadmap") {Planned(out);continue;}
    if (v=="s" || v=="search" || Starts(v,"/")) {
      if (Starts(v,"/") && v.size()>1) search=r.value.substr(1);
      else {
        auto query=Read(in,out,"Search command or task: ");
        if (query.nav==Nav::Quit) return Nav::Quit;
        if (query.nav==Nav::Back) continue;
        search=query.value;
      }
      page=0;continue;
    }
    if (v=="n" || v=="next") {if (page+1<pages)++page;continue;}
    if (v=="p" || v=="previous") {if (page)--page;continue;}
    if (v=="c" || v=="clear") {search.clear();page=0;continue;}
    const CommandSpec *selected=nullptr;
    if (const auto chosen=Number(r.value)) {
      const auto n=*chosen;
      if (n>=1 && n<=10 && first+n<=records.size())
        selected=records[first+n-1];
    }
    if (!selected) for (const auto *c:records)
      if (Lower(std::string(c->path))==v) {selected=c;break;}
    if (!selected) {out << "Choose a listed number or use S to search.\n";continue;}
    const auto *workflow=Find(std::string(selected->path));
    if (!workflow) {out << "This command is not available in this build.\n";continue;}
    out << "\n" << selected->path << " [Available, guided]\n"
        << selected->description << '\n'
        << "Usage: " << selected->usage << '\n';
    if (!workflow->note.empty()) out << workflow->note << '\n';
    if (workflow->path=="interactive") {out << "You are already in guided mode.\n";continue;}
    for (;;) {
      auto choice=Read(in,out,"1 Walk me through it  2 Show direct syntax  B Back  Q Quit: ");
      if (choice.nav==Nav::Quit) return Nav::Quit;
      if (choice.nav==Nav::Back) break;
      if (Help(choice.value)) {out << "The guided path asks one question at a time.\n";continue;}
      if (choice.value=="2") {out << selected->usage << '\n';continue;}
      if (choice.value=="1" || Lower(choice.value)=="walk") {
        const auto done=WorkflowRun(*workflow,in,out,err);
        if (done==Nav::Back || done==Nav::Browser) break;
        return done;
      }
      out << "Choose 1, 2, B, or Q.\n";
    }
  }
}
Nav ConversionJourney(std::istream &in,std::ostream &out,std::ostream &err) {
  Answers answers;
  int step=0;
  for (;;) {
    const bool green=Get(answers,"game")=="greenjp";
    const bool japanese=Get(answers,"game")=="jp" || green;
    const auto *workflow=Find(japanese ? (green ? "green-jp convert" : "red-jp convert") :
        "convert " + Get(answers,"game") + "-" + Get(answers,"target"));
    if (step<3) {
      Field field;
      if (step==0) {
        field.key="game"; field.question="Which game is your save from?";
        field.kind=InputKind::Choice;
        field.choices={"Red (English)","Blue (English)","Red (Japanese, experimental)","Green (Japanese 1.0, experimental)"};
        field.values={"red","blue","jp","greenjp"};
        field.help="Japanese games currently convert to FireRed only. Green 1.1 is not supported yet.";
      } else if (step==1) {
        field.key="source"; field.question="Choose your save file. Paste its path or type Browse.";
        field.kind=InputKind::ExistingFile;
        field.help="Use your game's .sav file. Spaces and quotes are accepted. JSON conversion is under Browse every current command.";
      } else {
        field.kind=InputKind::Choice;
        if (japanese) {
          field.key="profile";
          field.question="Japanese Red currently converts to FireRed only. Which game version made this save?";
          field.choices={"Original release (1.0)","Revised release (1.1)","I don't know"};
          field.values={"JP_RED_REV0","JP_RED_REV1","unknown"};
          if (green) {
            field.question="Japanese Green converts to FireRed. Confirm your game version:";
            field.choices={"Original release (1.0)","Revised release (1.1) or I don't know"};
            field.values={"JP_GREEN_REV0","unknown"};
          }
          field.help="Check the version of your Japanese Red game. The save alone cannot identify it reliably.";
        } else {
          field.key="target"; field.question="Which game would you like to play it in?";
          field.choices={"FireRed","LeafGreen"}; field.values={"firered","leafgreen"};
        }
      }
      Workflow promptWorkflow; // Simple mode accepts physical saves only.
      auto reply=FieldPrompt(in,out,promptWorkflow,field,answers);
      if (reply.nav==Nav::Quit) return Nav::Quit;
      if (reply.nav==Nav::Back) {if (!step) return Nav::Main; --step; continue;}
      if (step==1 && Lower(fs::path(reply.value).extension().string())==".json") {
        out << "Choose the original .sav file here. For JSON files, use the command browser (menu 8).\n";
        continue;
      }
      if (step==2 && reply.value=="unknown") {
        out << "Please check your game version before converting. We cannot safely choose it for you.\n";
        continue;
      }
      answers[field.key]=reply.value;
      if (step==0) {answers.erase("profile"); answers.erase("target");}
      if (japanese) answers["target"]="firered";
      ++step; continue;
    }
    if (!workflow) return Nav::Main;
    const fs::path source(Get(answers,"source"));
    const auto preferred=source.parent_path() /
        (source.stem().string()+(Get(answers,"target")=="leafgreen" ? "_lg.sav" : "_fr.sav"));
    auto destination=preferred;
    for (std::size_t n=2; FamilyExists(*workflow,answers,destination); ++n)
      destination=Numbered(preferred,n);
    answers["output"]=destination.string();
    const auto problems=Preflight(*workflow,answers,Destinations(*workflow,answers));
    if (!problems.empty()) {
      for (const auto &problem:problems) out << problem << '\n';
      out << "Choose another source location or use the advanced conversion tools.\n";
      step=1; continue;
    }
    out << "\nConvert " << source.filename().string() << " to "
        << (Get(answers,"target")=="leafgreen" ? "LeafGreen" : "FireRed") << "?\n"
        << "New save: " << destination.string() << '\n'
        << "Your original save stays unchanged. Conversion reports are saved alongside the result.\n";
    if (japanese)
      out << "Experimental: real-save and emulator testing is still pending. Japanese names are preserved in an archive; the generated save uses English-compatible names.\n"
          << "Japanese archive and conversion JSON will also be saved beside your original.\n";
    else {
      out << "Save checksums will be repaired for conversion if needed.\n";
      if (Get(answers,"game")!="red" || Get(answers,"target")!="firered")
        out << "This route still needs community testing in an emulator.\n";
    }
    auto confirm=Read(in,out,"Type YES to convert, B to go back, or Q to quit: ");
    if (confirm.nav==Nav::Quit) return Nav::Quit;
    if (confirm.nav==Nav::Back || No(confirm.value)) {step=2; continue;}
    if (Help(confirm.value)) {out << "A new save is created next to your original. Existing results receive a numbered filename.\n";continue;}
    if (Lower(confirm.value)!="yes") continue;
    std::vector<std::string> argv=japanese ?
      std::vector<std::string>{green ? "green-jp" : "red-jp","convert",source.string(),"--profile",Get(answers,"profile"),"--output",destination.string()} :
      std::vector<std::string>{"convert",Get(answers,"game")+"-"+Get(answers,"target"),source.string(),destination.string(),"--auto-repair-checksum"};
    std::ostringstream details,errors;
    const int code=CommandRouter{}.Run(argv,details,errors);
    if (code==0) out << "\nConverted save ready: " << destination.string()
                    << "\nLoad this save with the target game in your emulator.\n";
    else {out << "Conversion could not finish.\n"; err << errors.str();}
    return Nav::Main;
  }
}
} // namespace

int Run(const std::vector<std::string> &arguments,std::istream &input,
        std::ostream &output,std::ostream &error) {
  if (!arguments.empty()) {
    if (arguments.size()==1 && (arguments[0]=="--help" || arguments[0]=="help")) {
      output << "pkmn interactive: guided tasks and searchable browser for all "
             << AvailableCommands().size() << " current commands.\n"
             << "Every screen accepts ? for help, B for back, and Q for quit.\n";
      return 0;
    }
    error << "pkmn interactive: unsupported option; launch without arguments\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  try {static_cast<void>(options::AllWorkflows());}
  catch (const std::exception &e) {
    error << "pkmn interactive: command coverage error: " << e.what() << '\n';
    return ToInt(ExitCode::GeneralFailure);
  }
  int next=0;
  for (;;) {
    if (next==0) {
      output << "\npkmn 3.1 interactive - guided mode\n"
             << "1 Convert a save (simple, including Japanese Red/Green)\n"
             << "2 Advanced conversion and Japanese tools\n"
             << "3 Inspect, validate, summarize, or repair a save\n"
             << "4 Decode, generate, reconstruct, or migrate save data\n"
             << "5 Edit a save safely\n"
             << "6 Compare saves or playthrough progress\n"
             << "7 Run proof or emulator checks\n"
             << "8 Browse every current command and option\n"
             << "9 Doctor, settings, and beginner help\n"
             << "0 Exit\n? Help  B Back  Q Quit\n";
      auto r=Read(input,output,"Choose a task: ");
      if (r.nav==Nav::Quit || r.value=="0") return 0;
      if (r.nav==Nav::Back) continue;
      if (Help(r.value)) {
        output << "Choose a numbered task. The browser searches all current endpoints. "
               << "Inputs are never overwritten; writes require a final YES.\n";
        continue;
      }
      if (const auto chosen=Number(r.value); chosen && *chosen<=9)
        next=static_cast<int>(*chosen);
      else next=0;
      if (next<1 || next>9) {next=0;output << "Choose a number from 0 to 9.\n";continue;}
    }
    const auto group=next;next=0;
    const auto result=group==1 ? ConversionJourney(input,output,error) :
                      group==2 ? Browser(input,output,error,1) :
                                 Browser(input,output,error,group);
    if (result==Nav::Quit) return 0;
    if (result==Nav::Browser) next=8;
    else if (result==Nav::Inspect) next=3;
    else if (result==Nav::Compare) next=6;
  }
}
} // namespace pkmn::cli::commands::interactive
