#include "commands/red/EditCommand.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "app/ExitCode.hpp"
#include "red/editing/EditSession.hpp"

namespace pkmn::cli::commands::red::edit {
namespace {
using Json = pkmn::cli::red::editing::Json;

void Help(std::ostream &out) {
  out << "Usage:\n"
      << "  pkmn red edit <save.sav>\n"
      << "  pkmn red begin-edit <save.sav> [--output <session.json>]\n"
      << "  pkmn red edit-session <session.json> [edits...]\n"
      << "  pkmn red pending-edits <session.json>\n"
      << "  pkmn red validate-edit <session.json>\n"
      << "  pkmn red end-edit <session.json> [--output <output.sav>]\n\n"
      << "Named edits: --trainer-name <text>, --rival-name <text>, "
         "--trainer-id <n>, --money <n>, --coins <n>, --badges <0..255>\n"
      << "Advanced edit: --set </decoded/json/pointer> <JSON-value>\n"
      << "Arbitrary location editing is disabled. Source saves are never "
         "overwritten.\n";
}

Json ParseUnsigned(const std::string &value, std::uint64_t maximum,
                   const std::string &label) {
  std::size_t used = 0;
  const auto parsed = std::stoull(value, &used);
  if (used != value.size() || parsed > maximum)
    throw std::runtime_error(label + " is outside its supported range");
  return parsed;
}

void ApplyArguments(Json &session, const std::vector<std::string> &args,
                    std::size_t begin) {
  for (std::size_t i = begin; i < args.size(); ++i) {
    if (i + 1 >= args.size())
      throw std::runtime_error("edit option requires a value");
    const auto &option = args[i];
    if (option == "--set") {
      if (i + 2 >= args.size())
        throw std::runtime_error("--set requires a JSON pointer and value");
      Json value;
      try {
        value = Json::parse(args[i + 2]);
      } catch (const std::exception &) {
        throw std::runtime_error(
            "--set value must be valid JSON (quote JSON strings)");
      }
      pkmn::cli::red::editing::AddEdit(session, args[i + 1], value);
      i += 2;
      continue;
    }
    const auto &value = args[++i];
    if (option == "--trainer-name")
      pkmn::cli::red::editing::AddEdit(
          session, "/decoded/trainer/name/value", value);
    else if (option == "--rival-name")
      pkmn::cli::red::editing::AddEdit(
          session, "/decoded/rival/name/value", value);
    else if (option == "--trainer-id")
      pkmn::cli::red::editing::AddEdit(
          session, "/decoded/trainer/trainerId",
          ParseUnsigned(value, 65535, "trainer ID"));
    else if (option == "--money")
      pkmn::cli::red::editing::AddEdit(
          session, "/decoded/moneyAndCoins/money",
          ParseUnsigned(value, 999999, "money"));
    else if (option == "--coins")
      pkmn::cli::red::editing::AddEdit(
          session, "/decoded/moneyAndCoins/coins",
          ParseUnsigned(value, 9999, "coins"));
    else if (option == "--badges")
      pkmn::cli::red::editing::AddEdit(
          session, "/decoded/badges/raw",
          ParseUnsigned(value, 255, "badges"));
    else
      throw std::runtime_error("unsupported edit option: " + option);
  }
}

void WriteResult(const std::filesystem::path &outputPath,
                 const pkmn::cli::red::generation::Result &result,
                 const Json &session) {
  auto jsonReport = outputPath;
  jsonReport += ".edit-report.json";
  auto markdownReport = outputPath;
  markdownReport += ".edit-report.md";
  for (const auto &path : {outputPath, jsonReport, markdownReport})
    if (std::filesystem::exists(path))
      throw std::runtime_error("refusing to overwrite output or edit report");
  std::ofstream save(outputPath, std::ios::binary);
  save.write(reinterpret_cast<const char *>(result.bytes.data()),
             static_cast<std::streamsize>(result.bytes.size()));
  if (!save)
    throw std::runtime_error("could not write complete edited save");
  Json report = result.report;
  report["workflow"] = "validated-copy-edit";
  report["pendingEdits"] = session.at("pendingEdits");
  report["sourceOverwritten"] = false;
  std::ofstream(jsonReport) << report.dump(2) << '\n';
  std::ofstream(markdownReport)
      << "# Pokemon Red Edit Report\n\n- Source overwritten: no\n- "
         "Pending edits applied: "
      << session.at("pendingEdits").size()
      << "\n- Checksums regenerated and validated: yes\n\n"
      << result.markdownReport;
}

int End(Json &session, const std::filesystem::path &outputPath,
        std::ostream &out) {
  if (session.at("pendingEdits").empty())
    throw std::runtime_error("edit session has no pending edits");
  const auto result = pkmn::cli::red::editing::Validate(session);
  WriteResult(outputPath, result, session);
  out << "Validated Pokemon Red edit complete\nOutput: "
      << outputPath.filename().string()
      << "\nSource overwritten: no\nChecksum validation: passed\n";
  return ToInt(ExitCode::Success);
}

} // namespace

int Run(const std::vector<std::string> &arguments, std::ostream &output,
        std::ostream &error) {
  if (arguments.empty() || arguments.front() == "help" ||
      arguments.front() == "--help") {
    Help(output);
    return 0;
  }
  const auto &command = arguments.front();
  try {
    if (command == "begin-edit" &&
        (arguments.size() == 2 ||
         (arguments.size() == 4 && arguments[2] == "--output"))) {
      const std::filesystem::path source = arguments[1];
      const auto path = arguments.size() == 4
                            ? std::filesystem::path(arguments[3])
                            : pkmn::cli::red::editing::DefaultSessionPath(source);
      const auto session = pkmn::cli::red::editing::Begin(source);
      pkmn::cli::red::editing::Save(path, session, true);
      output << "Pokemon Red edit session created\nSession: "
             << path.filename().string()
             << "\nSource is read-only; physicalImage is excluded.\n";
      return 0;
    }
    if (command == "edit-session" && arguments.size() >= 4) {
      auto session = pkmn::cli::red::editing::Load(arguments[1]);
      ApplyArguments(session, arguments, 2);
      pkmn::cli::red::editing::Validate(session);
      pkmn::cli::red::editing::Save(arguments[1], session, false);
      output << "Pending edits updated and validated: "
             << session.at("pendingEdits").size() << '\n';
      return 0;
    }
    if (command == "pending-edits" && arguments.size() == 2) {
      const auto session = pkmn::cli::red::editing::Load(arguments[1]);
      output << "Pending edits: " << session.at("pendingEdits").size() << '\n';
      for (const auto &edit : session.at("pendingEdits"))
        output << "- " << edit.at("path").get<std::string>() << ": "
               << edit.at("previous").dump() << " -> "
               << edit.at("value").dump() << '\n';
      return 0;
    }
    if (command == "validate-edit" && arguments.size() == 2) {
      const auto session = pkmn::cli::red::editing::Load(arguments[1]);
      pkmn::cli::red::editing::Validate(session);
      output << "Edit validation: passed\nPending edits: "
             << session.at("pendingEdits").size()
             << "\nSource unchanged: yes\nGenerated checksums: valid\n";
      return 0;
    }
    if (command == "end-edit" &&
        (arguments.size() == 2 ||
         (arguments.size() == 4 && arguments[2] == "--output"))) {
      auto session = pkmn::cli::red::editing::Load(arguments[1]);
      const auto path = arguments.size() == 4
                            ? std::filesystem::path(arguments[3])
                            : pkmn::cli::red::editing::DefaultOutputPath(session);
      return End(session, path, output);
    }
    if (command == "edit" && arguments.size() == 2) {
      auto session = pkmn::cli::red::editing::Begin(arguments[1]);
      output << "Interactive Red edit (leave a value blank to keep it)\n";
      for (const auto &[label, option] :
           std::vector<std::pair<std::string, std::string>>{
               {"Trainer name", "--trainer-name"}, {"Rival name", "--rival-name"},
               {"Money", "--money"}, {"Coins", "--coins"}}) {
        output << label << ": " << std::flush;
        std::string value;
        std::getline(std::cin, value);
        if (!value.empty())
          ApplyArguments(session, {command, option, value}, 1);
      }
      return End(session, pkmn::cli::red::editing::DefaultOutputPath(session),
                 output);
    }
  } catch (const std::exception &exception) {
    error << "pkmn red " << command << ": " << exception.what() << '\n';
    return command == "end-edit" ? ToInt(ExitCode::OutputFailure)
                                  : ToInt(ExitCode::EditValidationFailure);
  }
  error << "pkmn red: invalid edit command arguments\n";
  Help(error);
  return ToInt(ExitCode::InvalidArguments);
}

} // namespace pkmn::cli::commands::red::edit
