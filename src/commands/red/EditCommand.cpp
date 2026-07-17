#include "commands/red/EditCommand.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

#include "app/ExitCode.hpp"
#include "red/editing/EditSession.hpp"
#include "util/OutputPath.hpp"
#include "util/AtomicOutput.hpp"

namespace pkmn::cli::commands::red::edit {
namespace {
using Json = pkmn::cli::red::editing::Json;

void Help(std::ostream &out) {
  out << "Usage:\n"
      << "  pkmn red edit <save.sav>\n"
      << "  pkmn red begin-edit <save.sav> [--output <session.json>]\n"
      << "  pkmn red edit-session <session.json> [edits...]\n"
      << "  pkmn red pending-edits <session.json>\n"
      << "  pkmn red undo-edit <session.json> [--count <n>]\n"
      << "  pkmn red edit-history <session.json> [--format json]\n"
      << "  pkmn red annotate-edit <session.json> <note>\n"
      << "  pkmn red validate-edit <session.json>\n"
      << "  pkmn red end-edit <session.json> [--output <output.sav>] "
         "[--auto-suffix] [--dry-run] [--format json]\n\n"
      << "Named edits: --trainer-name <text>, --rival-name <text>, "
         "--trainer-id <n>, --money <n>, --coins <n>, --badges <0..255|all>, "
         "--selected-box <1..12>, --event <EVENT_NAME> <on|off>\n"
      << "Safe location: --location-preset reds-house-2f\n"
      << "Complex value files: --bag-file, --pc-items-file, --party-file, "
         "--boxes-file, --current-box-file, --daycare-file, "
         "--hall-of-fame-file, --pokedex-file, --options-file, "
         "--playtime-file, --world-state-file\n"
      << "Advanced edits: --set </decoded/json/pointer> <JSON-value> or "
         "--set-file </decoded/json/pointer> <value.json>\n"
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

Json LoadJsonValue(const std::filesystem::path &path) {
  std::ifstream input(path);
  if (!input)
    throw std::runtime_error("could not open edit JSON value file");
  Json value;
  input >> value;
  return value;
}

void ApplyArguments(Json &session, const std::vector<std::string> &args,
                    std::size_t begin) {
  for (std::size_t i = begin; i < args.size(); ++i) {
    if (i + 1 >= args.size())
      throw std::runtime_error("edit option requires a value");
    const auto &option = args[i];
    if (option == "--event" || option == "--story-flag" ||
        option == "--trainer-battle" || option == "--static-encounter") {
      if (i + 2 >= args.size())
        throw std::runtime_error(option + " requires an event name and on/off");
      const auto &state = args[i + 2];
      if (state != "on" && state != "off" && state != "true" &&
          state != "false" && state != "1" && state != "0")
        throw std::runtime_error("named event value must be on or off");
      pkmn::cli::red::editing::AddNamedEventEdit(
          session, args[i + 1],
          state == "on" || state == "true" || state == "1");
      i += 2;
      continue;
    }
    if (option == "--location-preset") {
      pkmn::cli::red::editing::ApplySafeLocationPreset(session, args[++i]);
      continue;
    }
    if (option == "--set" || option == "--set-file") {
      if (i + 2 >= args.size())
        throw std::runtime_error(option +
                                 " requires a JSON pointer and value");
      Json value;
      if (option == "--set-file") {
        value = LoadJsonValue(args[i + 2]);
      } else {
        try {
          value = Json::parse(args[i + 2]);
        } catch (const std::exception &) {
          throw std::runtime_error(
              "--set value must be valid JSON (quote JSON strings)");
        }
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
          value == "all" ? Json(255)
                         : ParseUnsigned(value, 255, "badges"));
    else if (option == "--selected-box")
      pkmn::cli::red::editing::AddEdit(
          session, "/decoded/currentBoxCache/selectedBoxNumber",
          ParseUnsigned(value, 12, "selected box"));
    else {
      const std::map<std::string, std::string> fileOptions = {
          {"--bag-file", "/decoded/inventory/bag"},
          {"--pc-items-file", "/decoded/inventory/pcItems"},
          {"--party-file", "/decoded/party"},
          {"--boxes-file", "/decoded/pcStorage/boxes"},
          {"--current-box-file", "/decoded/currentBoxCache"},
          {"--daycare-file", "/decoded/daycare"},
          {"--hall-of-fame-file", "/decoded/hallOfFame"},
          {"--pokedex-file", "/decoded/pokedex"},
          {"--options-file", "/decoded/options"},
          {"--playtime-file", "/decoded/playtime"},
          {"--world-state-file", "/decoded/worldStateRaw"}};
      const auto found = fileOptions.find(option);
      if (found == fileOptions.end())
        throw std::runtime_error("unsupported edit option: " + option);
      pkmn::cli::red::editing::AddEdit(session, found->second,
                                       LoadJsonValue(value));
    }
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
  Json report = result.report;
  report["workflow"] = "validated-copy-edit";
  report["pendingEdits"] = session.at("pendingEdits");
  report["sourceOverwritten"] = false;
  const auto markdown =
      "# Pokemon Red Edit Report\n\n- Source overwritten: no\n- Pending edits "
      "applied: " + std::to_string(session.at("pendingEdits").size()) +
      "\n- Checksums regenerated and validated: yes\n\n" +
      result.markdownReport;
  util::OutputTransaction transaction;
  transaction.StageBinary(outputPath, result.bytes);
  transaction.StageText(jsonReport, report.dump(2) + '\n');
  transaction.StageText(markdownReport, markdown);
  transaction.Commit();
}

bool EditOutputAvailable(const std::filesystem::path &outputPath) {
  return !std::filesystem::exists(outputPath) &&
         !std::filesystem::exists(outputPath.string() + ".edit-report.json") &&
         !std::filesystem::exists(outputPath.string() + ".edit-report.md");
}

std::filesystem::path AutoSuffixEditOutput(
    const std::filesystem::path &preferred) {
  if (EditOutputAvailable(preferred))
    return preferred;
  for (std::size_t number = 2;; ++number) {
    const auto candidate = util::NumberedOutputPath(preferred, number);
    if (EditOutputAvailable(candidate))
      return candidate;
  }
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

void PrintPending(const Json &session, std::ostream &output) {
  output << "Pending edits: " << session.at("pendingEdits").size() << '\n';
  for (const auto &edit : session.at("pendingEdits"))
    output << "- " << edit.at("path").get<std::string>() << ": "
           << edit.at("previous").dump() << " -> "
           << edit.at("value").dump() << '\n';
}

int Interactive(const std::filesystem::path &source, std::ostream &output) {
  auto session = pkmn::cli::red::editing::Begin(source);
  const std::vector<std::pair<std::string, std::string>> actions = {
      {"Trainer name", "--trainer-name"},
      {"Rival name", "--rival-name"},
      {"Trainer ID", "--trainer-id"},
      {"Money", "--money"},
      {"Coins", "--coins"},
      {"Badges (0..255 or all)", "--badges"},
      {"Bag object from JSON file", "--bag-file"},
      {"PC-item object from JSON file", "--pc-items-file"},
      {"Party object from JSON file", "--party-file"},
      {"All 12 boxes array from JSON file", "--boxes-file"},
      {"Current-box/cache object from JSON file", "--current-box-file"},
      {"Daycare object from JSON file", "--daycare-file"},
      {"Hall of Fame object from JSON file", "--hall-of-fame-file"},
      {"Pokedex object from JSON file", "--pokedex-file"},
      {"Options object from JSON file", "--options-file"},
      {"Playtime object from JSON file", "--playtime-file"},
      {"Supported world-state object from JSON file", "--world-state-file"},
      {"Selected box (1..12)", "--selected-box"},
      {"Named event/story/trainer/static flag", "--event"}};
  while (true) {
    output << "\nPokemon Red copy-first editor\nSource: "
           << source.filename().string() << "\nPending edits: "
           << session.at("pendingEdits").size() << "\n";
    for (std::size_t index = 0; index < actions.size(); ++index)
      output << index + 1 << ". " << actions[index].first << '\n';
    output << "20. Advanced JSON-pointer edit\n"
              "21. View pending edits\n"
              "22. Validate pending edits\n"
              "23. Save validated edited copy\n"
              "24. Discard edits and exit\nChoice: "
           << std::flush;
    std::string choiceText;
    if (!std::getline(std::cin, choiceText)) {
      output << "\nInput closed; edits discarded.\n";
      return 0;
    }
    std::size_t choice = 0;
    try {
      choice = static_cast<std::size_t>(std::stoul(choiceText));
    } catch (const std::exception &) {
      output << "Invalid menu choice.\n";
      continue;
    }
    if (choice >= 1 && choice <= actions.size()) {
      if (choice == 19) {
        output << "Verified EVENT_NAME: " << std::flush;
        std::string name;
        std::getline(std::cin, name);
        output << "State (on/off): " << std::flush;
        std::string state;
        std::getline(std::cin, state);
        auto candidate = session;
        try {
          ApplyArguments(candidate, {"edit", "--event", name, state}, 1);
          pkmn::cli::red::editing::Validate(candidate);
          session = std::move(candidate);
          output << "Named event edit staged and validated.\n";
        } catch (const std::exception &exception) {
          output << "Edit rejected: " << exception.what() << '\n';
        }
        continue;
      }
      output << (choice <= 6 || choice == 18 ? "Value: " : "JSON file: ")
             << std::flush;
      std::string value;
      if (!std::getline(std::cin, value) || value.empty()) {
        output << "No edit applied.\n";
        continue;
      }
      auto candidate = session;
      try {
        ApplyArguments(candidate,
                       {"edit", actions[choice - 1].second, value}, 1);
        pkmn::cli::red::editing::Validate(candidate);
        session = std::move(candidate);
        output << "Edit staged and validated.\n";
      } catch (const std::exception &exception) {
        output << "Edit rejected: " << exception.what() << '\n';
      }
      continue;
    }
    if (choice == 20) {
      output << "JSON pointer: " << std::flush;
      std::string pointer;
      std::getline(std::cin, pointer);
      output << "JSON value: " << std::flush;
      std::string value;
      std::getline(std::cin, value);
      auto candidate = session;
      try {
        ApplyArguments(candidate, {"edit", "--set", pointer, value}, 1);
        pkmn::cli::red::editing::Validate(candidate);
        session = std::move(candidate);
        output << "Edit staged and validated.\n";
      } catch (const std::exception &exception) {
        output << "Edit rejected: " << exception.what() << '\n';
      }
    } else if (choice == 21) {
      PrintPending(session, output);
    } else if (choice == 22) {
      try {
        pkmn::cli::red::editing::Validate(session);
        output << "Validation passed.\n";
      } catch (const std::exception &exception) {
        output << "Validation failed: " << exception.what() << '\n';
      }
    } else if (choice == 23) {
      if (session.at("pendingEdits").empty()) {
        output << "No pending edits to save.\n";
        continue;
      }
      return End(session,
                 pkmn::cli::red::editing::DefaultOutputPath(session), output);
    } else if (choice == 24) {
      output << "Edits discarded; source remains unchanged.\n";
      return 0;
    } else {
      output << "Invalid menu choice.\n";
    }
  }
}

} // namespace

int Run(const std::vector<std::string> &arguments, std::ostream &output,
        std::ostream &error) {
  if (arguments.empty() || arguments.front() == "help" ||
      arguments.front() == "--help" ||
      (arguments.size() == 2 &&
       (arguments[1] == "help" || arguments[1] == "--help" ||
        arguments[1] == "-h"))) {
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
    if (command == "edit-session" && arguments.size() >= 3) {
      auto session = pkmn::cli::red::editing::Load(arguments[1]);
      bool dryRun = false;
      bool json = false;
      std::vector<std::string> filtered(arguments.begin(), arguments.begin() + 2);
      for (std::size_t index = 2; index < arguments.size(); ++index) {
        if (arguments[index] == "--dry-run")
          dryRun = true;
        else if (arguments[index] == "--format" &&
                 index + 1 < arguments.size() &&
                 arguments[index + 1] == "json") {
          json = true;
          ++index;
        } else
          filtered.push_back(arguments[index]);
      }
      if (filtered.size() < 4)
        throw std::runtime_error("edit-session requires at least one edit");
      ApplyArguments(session, filtered, 2);
      const auto result = pkmn::cli::red::editing::Validate(session);
      if (!dryRun)
        pkmn::cli::red::editing::Save(arguments[1], session, false);
      if (json)
        output << Json({{"valid", true}, {"dryRun", dryRun},
                        {"sessionPersisted", !dryRun},
                        {"pendingEditCount", session.at("pendingEdits").size()},
                        {"generatedSha256", result.report.at("outputSha256")}}).dump(2)
               << '\n';
      else
        output << (dryRun ? "Dry-run edits validated; session unchanged: "
                          : "Pending edits updated and validated: ")
               << session.at("pendingEdits").size() << '\n';
      return 0;
    }
    if (command == "pending-edits" &&
        (arguments.size() == 2 ||
         (arguments.size() == 4 && arguments[2] == "--format" &&
          arguments[3] == "json"))) {
      const auto session = pkmn::cli::red::editing::Load(arguments[1]);
      if (arguments.size() == 4)
        output << Json({{"pendingEditCount", session.at("pendingEdits").size()},
                        {"pendingEdits", session.at("pendingEdits")}}).dump(2)
               << '\n';
      else
        PrintPending(session, output);
      return 0;
    }
    if (command == "undo-edit" && arguments.size() >= 2) {
      auto session = pkmn::cli::red::editing::Load(arguments[1]);
      std::size_t count = 1;
      if (arguments.size() == 4 && arguments[2] == "--count")
        count = ParseUnsigned(arguments[3], session.at("pendingEdits").size(),
                              "undo count").get<std::size_t>();
      else if (arguments.size() != 2)
        throw std::runtime_error("invalid undo-edit arguments");
      pkmn::cli::red::editing::UndoLast(session, count);
      pkmn::cli::red::editing::Validate(session);
      pkmn::cli::red::editing::Save(arguments[1], session, false);
      output << "Pending edits undone: " << count << "\nRemaining: "
             << session.at("pendingEdits").size() << '\n';
      return 0;
    }
    if (command == "annotate-edit" && arguments.size() == 3) {
      auto session = pkmn::cli::red::editing::Load(arguments[1]);
      pkmn::cli::red::editing::AddAnnotation(session, arguments[2]);
      pkmn::cli::red::editing::Save(arguments[1], session, false);
      output << "Edit-session annotation recorded\n";
      return 0;
    }
    if (command == "edit-history" &&
        (arguments.size() == 2 ||
         (arguments.size() == 4 && arguments[2] == "--format" &&
          arguments[3] == "json"))) {
      const auto session = pkmn::cli::red::editing::Load(arguments[1]);
      const Json history = {{"commandHistory", session.value(
                                 "commandHistory", Json::array())},
                            {"annotations", session.value(
                                 "annotations", Json::array())}};
      if (arguments.size() == 4)
        output << history.dump(2) << '\n';
      else
        output << "Command history: " << history.at("commandHistory").size()
               << "\nAnnotations: " << history.at("annotations").size()
               << '\n';
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
    if (command == "end-edit" && arguments.size() >= 2) {
      auto session = pkmn::cli::red::editing::Load(arguments[1]);
      auto path = pkmn::cli::red::editing::DefaultOutputPath(session);
      bool autoSuffix = false;
      bool dryRun = false;
      bool json = false;
      for (std::size_t index = 2; index < arguments.size(); ++index) {
        if (arguments[index] == "--output" && index + 1 < arguments.size())
          path = arguments[++index];
        else if (arguments[index] == "--auto-suffix")
          autoSuffix = true;
        else if (arguments[index] == "--dry-run")
          dryRun = true;
        else if (arguments[index] == "--format" &&
                 index + 1 < arguments.size() &&
                 arguments[index + 1] == "json") {
          json = true;
          ++index;
        }
        else
          throw std::runtime_error("invalid end-edit option");
      }
      if (dryRun) {
        if (session.at("pendingEdits").empty())
          throw std::runtime_error("edit session has no pending edits");
        const auto result = pkmn::cli::red::editing::Validate(session);
        if (json)
          output << Json({{"valid", true}, {"dryRun", true},
                          {"outputWritten", false},
                          {"plannedOutput", path.filename().string()},
                          {"generatedSha256", result.report.at("outputSha256")}})
                        .dump(2)
                 << '\n';
        else
          output << "Edit dry run: passed\nOutput written: no\nPlanned output: "
                 << path.filename().string() << '\n';
        return 0;
      }
      if (autoSuffix)
        path = AutoSuffixEditOutput(path);
      return End(session, path, output);
    }
    if (command == "edit" && arguments.size() == 2) {
      return Interactive(arguments[1], output);
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
