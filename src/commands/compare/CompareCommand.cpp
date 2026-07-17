#include "commands/compare/CompareCommand.hpp"
#include "app/ExitCode.hpp"
#include "red/comparison/Comparison.hpp"
#include "red/json/RedJsonDocument.hpp"
#include "red/save/RedSave.hpp"
#include "util/AtomicOutput.hpp"
#include <filesystem>
#include <ostream>

namespace pkmn::cli::commands::compare {
namespace {
struct OutputOptions {
  bool jsonToStdout = false;
  std::filesystem::path jsonPath;
  std::filesystem::path markdownPath;
};

OutputOptions ParseOutputOptions(const std::vector<std::string> &arguments) {
  OutputOptions options;
  for (std::size_t index = 3; index < arguments.size(); ++index) {
    if (arguments[index] == "--format" && index + 1 < arguments.size()) {
      const auto &format = arguments[++index];
      if (format == "json")
        options.jsonToStdout = true;
      else if (format != "markdown")
        throw std::runtime_error("--format must be json or markdown");
    } else if (arguments[index] == "--output-json" &&
               index + 1 < arguments.size()) {
      options.jsonPath = arguments[++index];
    } else if (arguments[index] == "--output-markdown" &&
               index + 1 < arguments.size()) {
      options.markdownPath = arguments[++index];
    } else {
      throw std::runtime_error("invalid comparison output option");
    }
  }
  for (const auto &path : {options.jsonPath, options.markdownPath})
    if (!path.empty() && std::filesystem::exists(path))
      throw std::runtime_error("refusing to overwrite comparison report");
  return options;
}

void Emit(const red::json::OrderedJson &report, const std::string &markdown,
          const OutputOptions &options, std::ostream &output) {
  util::OutputTransaction transaction;
  if (!options.jsonPath.empty())
    transaction.StageText(options.jsonPath, report.dump(2) + '\n');
  if (!options.markdownPath.empty())
    transaction.StageText(options.markdownPath, markdown);
  transaction.Commit();
  output << (options.jsonToStdout ? report.dump(2) + "\n" : markdown);
}
} // namespace

int Run(const std::vector<std::string> &a, std::ostream &out,
        std::ostream &err) {
  if (a.empty() || a[0] == "help" || a[0] == "--help") {
    out << "Usage:\n"
           "  pkmn compare physical <a.sav> <b.sav> [report options]\n"
           "  pkmn compare semantic <a.red.json> <b.red.json> [report options]\n"
           "  pkmn compare semantic-batch <baseline.red.json> "
           "<candidate.red.json>... [--format json]\n"
           "Report options:\n"
           "  --format markdown|json\n"
           "  --output-json <report.json>\n"
           "  --output-markdown <report.md>\n";
    return 0;
  }
  if (a[0] == "semantic-batch") {
    bool json = false;
    std::vector<std::filesystem::path> inputs;
    for (std::size_t index = 1; index < a.size(); ++index) {
      if (a[index] == "--format" && index + 1 < a.size() &&
          a[index + 1] == "json") {
        json = true;
        ++index;
      } else if (a[index].starts_with("--")) {
        err << "pkmn compare semantic-batch: invalid option\n";
        return ToInt(ExitCode::InvalidArguments);
      } else
        inputs.emplace_back(a[index]);
    }
    if (inputs.size() < 2) {
      err << "pkmn compare semantic-batch: baseline and candidates required\n";
      return ToInt(ExitCode::InvalidArguments);
    }
    try {
      const auto baseline = red::json::LoadAndValidate(inputs.front());
      if (!baseline.validation.Valid())
        throw std::runtime_error("baseline failed semantic validation");
      auto rows = red::json::OrderedJson::array();
      std::size_t equivalent = 0;
      for (std::size_t index = 1; index < inputs.size(); ++index) {
        const auto candidate = red::json::LoadAndValidate(inputs[index]);
        if (!candidate.validation.Valid())
          throw std::runtime_error(inputs[index].filename().string() +
                                   " failed semantic validation");
        const auto comparison = red::comparison::CompareSemantic(
            baseline.root, candidate.root);
        equivalent += comparison.at("equivalent").get<bool>();
        rows.push_back({{"candidate", inputs[index].filename().string()},
                        {"equivalent", comparison.at("equivalent")},
                        {"unexpectedMismatchCount",
                         comparison.at("unexpectedMismatchCount")},
                        {"comparison", comparison}});
      }
      const auto report = red::json::OrderedJson{
          {"command", "compare semantic-batch"},
          {"baseline", inputs.front().filename().string()},
          {"candidateCount", inputs.size() - 1},
          {"equivalentCount", equivalent}, {"results", rows}};
      if (json)
        out << report.dump(2) << '\n';
      else {
        out << "Semantic batch comparison\nEquivalent: " << equivalent << '/'
            << inputs.size() - 1 << '\n';
        for (const auto &row : rows)
          out << (row.at("equivalent").get<bool>() ? "[ok] " : "[diff] ")
              << row.at("candidate").get<std::string>() << '\n';
      }
      return equivalent == inputs.size() - 1
                 ? 0
                 : ToInt(ExitCode::SemanticMismatch);
    } catch (const std::exception &exception) {
      err << "pkmn compare semantic-batch: " << exception.what() << '\n';
      return ToInt(ExitCode::InvalidInput);
    }
  }
  if (a.size() < 3 || (a[0] != "physical" && a[0] != "semantic")) {
    err << "pkmn compare: invalid arguments\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  try {
    const auto options = ParseOutputOptions(a);
    if (a[0] == "physical") {
      const auto left = red::save::RedSave::Read(a[1]);
      const auto right = red::save::RedSave::Read(a[2]);
      const auto report =
          red::comparison::ComparePhysical(left.BytesView(), right.BytesView());
      Emit(report, red::comparison::PhysicalMarkdown(report), options, out);
      return 0;
    }
    const auto left = red::json::LoadAndValidate(a[1]);
    const auto right = red::json::LoadAndValidate(a[2]);
    if (!left.validation.Valid() || !right.validation.Valid())
      throw std::runtime_error("both semantic inputs must validate");
    const auto report = red::comparison::CompareSemantic(left.root, right.root);
    Emit(report, red::comparison::SemanticMarkdown(report), options, out);
    return report.at("equivalent").get<bool>()
               ? 0
               : ToInt(ExitCode::SemanticMismatch);
  } catch (const std::exception &e) {
    err << "pkmn compare: " << e.what() << '\n';
    return std::string(e.what()).find("overwrite") != std::string::npos ||
                   std::string(e.what()).find("write") != std::string::npos
               ? ToInt(ExitCode::OutputFailure)
               : ToInt(ExitCode::InvalidInput);
  }
}
} // namespace pkmn::cli::commands::compare
