#include "commands/fred/FredCommand.hpp"

#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>

#include <nlohmann/json.hpp>

#include "app/ExitCode.hpp"
#include "FileManipulation.hpp"
#include "FireRedLogicalSave.hpp"
#include "FireRedChecksum.hpp"
#include "FireRedMasterJson.hpp"
#include "FireRedReadOnlyData.hpp"
#include "FireRedSafeEditor.hpp"
#include "FireRedSectionMap.hpp"
#include "GeneratedFireRedTables.hpp"
#include "commands/conversion/ConversionCommand.hpp"

namespace pkmn::cli::commands::fred {
namespace {

void Help(std::ostream &output) {
  output << "Usage:\n"
         << "  pkmn fred inspect <save.sav> [--format json]\n"
         << "  pkmn fred validate <save.sav> [--format json]\n"
         << "  pkmn fred summary <save.sav> [--detailed]\n"
         << "  pkmn fred decode <save.sav> [--output <save.fred.json>]\n"
         << "  pkmn fred repair-checksums <save.sav> [--output <copy.sav>]\n"
         << "  pkmn fred validate-batch <save.sav>... [--format json]\n"
         << "  pkmn fred decode-batch <save.sav>... --output-dir <directory>\n"
         << "  pkmn fred validate-post-emulator <before.sav> <after.sav> [--output-dir <directory>]\n"
         << "  pkmn fred events list [--kind flag|variable|trainer|semantic] [--format json]\n"
         << "  pkmn fred events search <query> [--kind flag|variable|trainer|semantic] [--format json]\n"
         << "  pkmn fred events show <name|id> [--kind flag|variable|trainer|semantic] [--format json]\n"
         << "  pkmn fred edit <save.sav> [--output <copy.sav>] "
            "[--player-name <name>] [--rival-name <name>] "
            "[--money <value>] [--coins <value>] [--badge <1-8>:<on|off>]\n"
         << "  pkmn fred begin-edit <save.sav> [--output <session.json>]\n"
         << "  pkmn fred edit-session <session.json> [safe edit options]\n"
         << "  pkmn fred pokemon <session.json> party <slot> rename <name>\n"
         << "  pkmn fred bag <session.json> quantity <pocket> <slot> <item-id> <quantity>\n"
         << "  pkmn fred progress <session.json> badge <1-8> <on|off>\n"
         << "  pkmn fred pending-edits|edit-history|validate-edit <session.json>\n"
         << "  pkmn fred undo-edit <session.json> [--count <number>]\n"
         << "  pkmn fred annotate-edit <session.json> <note>\n"
         << "  pkmn fred end-edit <session.json> [--output <copy.sav>]\n";
}

std::string Lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

int Events(const std::vector<std::string> &arguments, std::ostream &output,
           std::ostream &error) {
  if (arguments.size() < 2) return ToInt(ExitCode::InvalidArguments);
  const auto action = arguments[1];
  if (action != "list" && action != "search" && action != "show")
    return ToInt(ExitCode::InvalidArguments);
  std::string query;
  std::string kind = "flag";
  bool json = false;
  std::size_t index = 2;
  if (action != "list") {
    if (index >= arguments.size()) return ToInt(ExitCode::InvalidArguments);
    query = arguments[index++];
  }
  for (; index < arguments.size(); ++index) {
    if (arguments[index] == "--kind" && index + 1 < arguments.size())
      kind = arguments[++index];
    else if (arguments[index] == "--format" && index + 1 < arguments.size() &&
             arguments[index + 1] == "json") { json = true; ++index; }
    else return ToInt(ExitCode::InvalidArguments);
  }
  if (kind == "trainer" || kind == "semantic") {
    std::vector<std::string> runtime{"bridge-inspect", kind == "trainer" ? "trainer" : "event"};
    if (action != "list") runtime.push_back(query);
    return commands::conversion::RunRuntimeUtility(runtime, output, error);
  }
  if (kind != "flag" && kind != "variable") {
    error << "pkmn fred events: kind must be flag, variable, trainer, or semantic\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  auto records = nlohmann::ordered_json::array();
  auto collect = [&](const auto &table) {
    for (const auto &entry : table) {
      const auto name = std::string(entry.name);
      const auto numeric = std::to_string(entry.id);
      const auto hex = "0x" + [&] { std::ostringstream stream; stream << std::hex
          << std::uppercase << entry.id; return stream.str(); }();
      if (action == "search" && Lower(name).find(Lower(query)) == std::string::npos)
        continue;
      if (action == "show" && query != name && query != numeric && Lower(query) != Lower(hex))
        continue;
      records.push_back({{"id", entry.id}, {"hex", hex}, {"name", name}, {"kind", kind}});
    }
  };
  if (kind == "flag") collect(firered::reference::kFlags);
  else collect(firered::reference::kVariables);
  if (action == "show" && records.empty()) return ToInt(ExitCode::InvalidInput);
  if (json) output << nlohmann::ordered_json({{"pretCommit", firered::reference::kPretCommit},
      {"resultCount", records.size()}, {"records", records}}).dump(2) << '\n';
  else {
    output << "FireRed " << kind << " records: " << records.size() << '\n';
    for (const auto &record : records)
      output << record["hex"].get<std::string>() << "  "
             << record["name"].get<std::string>() << '\n';
  }
  return 0;
}

nlohmann::ordered_json AnalysisJson(const firered::SaveAnalysis &analysis) {
  auto slots = nlohmann::ordered_json::array();
  for (const auto &slot : analysis.slots) {
    slots.push_back({{"index", slot.slotIndex}, {"counter", slot.counter},
                     {"valid", slot.valid},
                     {"allSectionIdsPresent", slot.allSectionIdsPresent},
                     {"countersConsistent", slot.countersConsistent},
                     {"errors", slot.errors}});
  }
  return {{"format", "pkmn-firered-save-validation"},
          {"reportVersion", "2.0.0"},
          {"standardFlashPresent", analysis.standardFlashPresent},
          {"valid", analysis.atLeastOneValidSlot && !analysis.activeSlotAmbiguous},
          {"activeSlot", analysis.activeSlot ? nlohmann::ordered_json(*analysis.activeSlot)
                                              : nlohmann::ordered_json(nullptr)},
          {"activeSlotAmbiguous", analysis.activeSlotAmbiguous},
          {"slots", slots}, {"diagnostics", analysis.diagnostics}};
}

std::filesystem::path DefaultEdit(const std::filesystem::path &input) {
  return input.parent_path() / (input.stem().string() + "_edited.sav");
}

void WriteNewText(const std::filesystem::path &path, const std::string &text) {
  if (std::filesystem::exists(path))
    throw std::runtime_error("refusing to overwrite existing output");
  std::ofstream stream(path, std::ios::binary);
  stream << text;
  if (!stream) throw std::runtime_error("could not write output");
}

void ReplaceText(const std::filesystem::path &path, const std::string &text) {
  const auto temporary = path.string() + ".tmp";
  { std::ofstream stream(temporary, std::ios::binary); stream << text;
    if (!stream) throw std::runtime_error("could not write session"); }
  std::filesystem::rename(temporary, path);
}

nlohmann::ordered_json ReadJson(const std::filesystem::path &path) {
  std::ifstream stream(path);
  if (!stream) throw std::runtime_error("could not open JSON");
  nlohmann::ordered_json value; stream >> value; return value;
}

void ApplyEditOptions(firered::FireRedSafeEditor &editor,
                      const nlohmann::ordered_json &edits) {
  for (const auto &edit : edits) {
    const auto field = edit.at("field").get<std::string>();
    if (field == "playerName") editor.SetPlayerName(edit.at("value"));
    else if (field == "rivalName") editor.SetRivalName(edit.at("value"));
    else if (field == "money") editor.SetMoney(edit.at("value"));
    else if (field == "coins") editor.SetCoins(edit.at("value"));
    else if (field == "badge")
      editor.SetBadge(edit.at("badge"), edit.at("obtained"));
    else if (field == "pokemonNickname")
      editor.SetPartyPokemonNickname(edit.at("slot"), edit.at("value"));
    else if (field == "itemQuantity")
      editor.SetExistingItemQuantity(edit.at("pocket"), edit.at("slot"),
                                     edit.at("itemId"), edit.at("quantity"));
    else throw std::runtime_error("unsupported staged FireRed edit: " + field);
  }
}

int Session(const std::vector<std::string> &arguments, std::ostream &output,
            std::ostream &error) {
  try {
    const auto action = arguments.front();
    if (action == "begin-edit") {
      if (arguments.size() < 2) return ToInt(ExitCode::InvalidArguments);
      const std::filesystem::path source = std::filesystem::absolute(arguments[1]);
      auto destination = source.parent_path() /
          (source.stem().string() + ".fred-edit-session.json");
      if (arguments.size() == 4 && arguments[2] == "--output") destination = arguments[3];
      else if (arguments.size() != 2) return ToInt(ExitCode::InvalidArguments);
      const auto bytes = firered::ReadBinaryFile(source);
      const auto analysis = firered::AnalyzeSave(bytes);
      if (!analysis.activeSlot || analysis.activeSlotAmbiguous ||
          !analysis.slots[*analysis.activeSlot].valid)
        throw std::runtime_error("source is not a valid unambiguous FireRed save");
      nlohmann::ordered_json session{{"format", "pkmn-firered-edit-session"},
        {"version", "1.0.0"}, {"source", source.string()},
        {"edits", nlohmann::ordered_json::array()},
        {"history", nlohmann::ordered_json::array()},
        {"annotations", nlohmann::ordered_json::array()}};
      WriteNewText(destination, session.dump(2) + "\n");
      output << "FireRed edit session created\nOutput: " << destination << '\n';
      return 0;
    }
    if (arguments.size() < 2) return ToInt(ExitCode::InvalidArguments);
    const std::filesystem::path path = arguments[1];
    auto session = ReadJson(path);
    if (session.value("format", "") != "pkmn-firered-edit-session")
      throw std::runtime_error("unsupported FireRed edit session");
    if (action == "pending-edits" || action == "edit-history") {
      const auto &value = action == "pending-edits" ? session["edits"] : session["history"];
      output << value.dump(2) << '\n'; return 0;
    }
    if (action == "annotate-edit") {
      if (arguments.size() != 3) return ToInt(ExitCode::InvalidArguments);
      session["annotations"].push_back(arguments[2]);
      ReplaceText(path, session.dump(2) + "\n");
      output << "Annotation added\n"; return 0;
    }
    if (action == "undo-edit") {
      std::size_t count = 1;
      if (arguments.size() == 4 && arguments[2] == "--count") count = std::stoul(arguments[3]);
      else if (arguments.size() != 2) return ToInt(ExitCode::InvalidArguments);
      while (count-- && !session["edits"].empty()) {
        session["history"].push_back({{"action", "undo"},
                                      {"edit", session["edits"].back()}});
        session["edits"].erase(session["edits"].end() - 1);
      }
      ReplaceText(path, session.dump(2) + "\n");
      output << "Pending edits: " << session["edits"].size() << '\n'; return 0;
    }
    if (action == "edit-session") {
      auto additions = nlohmann::ordered_json::array();
      for (std::size_t index = 2; index < arguments.size(); ++index) {
        if (arguments[index] == "--player-name" && index + 1 < arguments.size())
          additions.push_back({{"field", "playerName"}, {"value", arguments[++index]}});
        else if (arguments[index] == "--rival-name" && index + 1 < arguments.size())
          additions.push_back({{"field", "rivalName"}, {"value", arguments[++index]}});
        else if (arguments[index] == "--money" && index + 1 < arguments.size())
          additions.push_back({{"field", "money"}, {"value", std::stoul(arguments[++index])}});
        else if (arguments[index] == "--coins" && index + 1 < arguments.size())
          additions.push_back({{"field", "coins"}, {"value", std::stoul(arguments[++index])}});
        else if (arguments[index] == "--badge" && index + 1 < arguments.size()) {
          const auto value = arguments[++index]; const auto colon = value.find(':');
          if (colon == std::string::npos) throw std::runtime_error("badge must be N:on or N:off");
          additions.push_back({{"field", "badge"}, {"badge", std::stoul(value.substr(0, colon))},
                               {"obtained", value.substr(colon + 1) == "on"}});
        } else return ToInt(ExitCode::InvalidArguments);
      }
      for (const auto &edit : additions) session["edits"].push_back(edit);
      session["history"].push_back({{"action", "stage"}, {"edits", additions}});
      ReplaceText(path, session.dump(2) + "\n");
      output << "Staged edits: " << additions.size() << '\n'; return 0;
    }
    if (action == "pokemon") {
      if (arguments.size() != 6 || arguments[2] != "party" || arguments[4] != "rename")
        return ToInt(ExitCode::InvalidArguments);
      const nlohmann::ordered_json edit{{"field", "pokemonNickname"},
        {"slot", std::stoul(arguments[3])}, {"value", arguments[5]}};
      session["edits"].push_back(edit);
      session["history"].push_back({{"action", "stage"}, {"edit", edit}});
      ReplaceText(path, session.dump(2) + "\n"); output << "Pokemon nickname edit staged\n"; return 0;
    }
    if (action == "bag") {
      if (arguments.size() != 7 || arguments[2] != "quantity")
        return ToInt(ExitCode::InvalidArguments);
      const nlohmann::ordered_json edit{{"field", "itemQuantity"}, {"pocket", arguments[3]},
        {"slot", std::stoul(arguments[4])}, {"itemId", std::stoul(arguments[5])},
        {"quantity", std::stoul(arguments[6])}};
      session["edits"].push_back(edit);
      session["history"].push_back({{"action", "stage"}, {"edit", edit}});
      ReplaceText(path, session.dump(2) + "\n"); output << "Existing item quantity edit staged\n"; return 0;
    }
    if (action == "progress") {
      if (arguments.size() != 5 || arguments[2] != "badge" ||
          (arguments[4] != "on" && arguments[4] != "off"))
        return ToInt(ExitCode::InvalidArguments);
      const nlohmann::ordered_json edit{{"field", "badge"},
        {"badge", std::stoul(arguments[3])}, {"obtained", arguments[4] == "on"}};
      session["edits"].push_back(edit);
      session["history"].push_back({{"action", "stage"}, {"edit", edit}});
      ReplaceText(path, session.dump(2) + "\n"); output << "Badge progress edit staged\n"; return 0;
    }
    const auto bytes = firered::ReadBinaryFile(session.at("source").get<std::string>());
    const auto analysis = firered::AnalyzeSave(bytes);
    firered::FireRedSafeEditor editor(bytes, analysis);
    ApplyEditOptions(editor, session["edits"]);
    const auto edited = editor.Finish();
    if (action == "validate-edit") {
      const auto validation = firered::AnalyzeSave(edited);
      output << "FireRed edit validation: "
             << (validation.activeSlot && !validation.activeSlotAmbiguous ? "passed" : "failed")
             << "\n" << editor.Report(); return 0;
    }
    if (action == "end-edit") {
      auto destination = DefaultEdit(session.at("source").get<std::string>());
      if (arguments.size() == 4 && arguments[2] == "--output") destination = arguments[3];
      else if (arguments.size() != 2) return ToInt(ExitCode::InvalidArguments);
      firered::WriteNewBinaryFile(destination, edited);
      output << editor.Report() << "\nOutput: " << destination << '\n'; return 0;
    }
    return ToInt(ExitCode::InvalidArguments);
  } catch (const std::exception &exception) {
    error << "pkmn fred session: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}

int RepairChecksums(const std::vector<std::string> &arguments,
                    std::ostream &output, std::ostream &error) {
  if (arguments.size() < 2) return ToInt(ExitCode::InvalidArguments);
  try {
    const std::filesystem::path input = arguments[1];
    auto destination = input.parent_path() / (input.stem().string() + "_checksum_repaired.sav");
    if (arguments.size() == 4 && arguments[2] == "--output") destination = arguments[3];
    else if (arguments.size() != 2) return ToInt(ExitCode::InvalidArguments);
    auto bytes = firered::ReadBinaryFile(input);
    if (bytes.size() < firered::Layout::kStandardFlashSize)
      throw std::runtime_error("input is smaller than a 128 KiB FireRed save");
    std::size_t repaired = 0;
    for (std::size_t sectorNo = 0; sectorNo < 28; ++sectorNo) {
      auto sector = std::span<std::uint8_t>(bytes).subspan(sectorNo * firered::Layout::kSectorSize,
                                                           firered::Layout::kSectorSize);
      const auto id = firered::ReadLe16(sector, firered::Layout::kSectionIdOffset);
      if (firered::ReadLe32(sector, firered::Layout::kSignatureOffset) != firered::Layout::kSectorSignature || id >= 14)
        continue;
      const auto checksum = firered::CalculateSectionChecksum(sector.first(firered::Layout::kSectionDataSizes[id]));
      if (firered::ReadLe16(sector, firered::Layout::kChecksumOffset) != checksum) ++repaired;
      firered::WriteLe16(sector, firered::Layout::kChecksumOffset, checksum);
    }
    firered::WriteNewBinaryFile(destination, bytes);
    const auto after = firered::AnalyzeSave(bytes);
    output << "FireRed checksum repair complete\nRepaired sections: " << repaired
           << "\nValid slot available: " << (after.atLeastOneValidSlot ? "yes" : "no")
           << "\nOutput: " << destination << '\n';
    return after.atLeastOneValidSlot ? 0 : ToInt(ExitCode::ChecksumFailure);
  } catch (const std::exception &exception) {
    error << "pkmn fred repair-checksums: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}

bool ValidAnalysis(const firered::SaveAnalysis &analysis) {
  return analysis.standardFlashPresent && analysis.atLeastOneValidSlot &&
         analysis.activeSlot && !analysis.activeSlotAmbiguous &&
         analysis.slots[*analysis.activeSlot].valid;
}

int ValidateBatch(const std::vector<std::string> &arguments,
                  std::ostream &output, std::ostream &error) {
  bool json = false;
  auto rows = nlohmann::ordered_json::array();
  std::size_t passed = 0;
  try {
    for (std::size_t index = 1; index < arguments.size(); ++index) {
      if (arguments[index] == "--format" && index + 1 < arguments.size() &&
          arguments[index + 1] == "json") { json = true; ++index; continue; }
      if (arguments[index].starts_with("--"))
        return ToInt(ExitCode::InvalidArguments);
      const auto bytes = firered::ReadBinaryFile(arguments[index]);
      const auto analysis = firered::AnalyzeSave(bytes);
      const bool valid = ValidAnalysis(analysis); passed += valid;
      rows.push_back({{"input", std::filesystem::path(arguments[index]).filename().string()},
                      {"valid", valid}, {"analysis", AnalysisJson(analysis)}});
    }
    if (rows.empty()) return ToInt(ExitCode::InvalidArguments);
    if (json) output << nlohmann::ordered_json{{"command", "fred validate-batch"},
      {"inputCount", rows.size()}, {"passed", passed}, {"results", rows}}.dump(2) << '\n';
    else {
      output << "FireRed batch validation: " << passed << '/' << rows.size() << " passed\n";
      for (const auto &row : rows)
        output << (row["valid"].get<bool>() ? "[PASS] " : "[FAIL] ")
               << row["input"].get<std::string>() << '\n';
    }
    return passed == rows.size() ? 0 : ToInt(ExitCode::ChecksumFailure);
  } catch (const std::exception &exception) {
    error << "pkmn fred validate-batch: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}

int DecodeBatch(const std::vector<std::string> &arguments,
                std::ostream &output, std::ostream &error) {
  std::filesystem::path directory;
  std::vector<std::filesystem::path> inputs;
  for (std::size_t index = 1; index < arguments.size(); ++index) {
    if (arguments[index] == "--output-dir" && index + 1 < arguments.size())
      directory = arguments[++index];
    else if (arguments[index].starts_with("--"))
      return ToInt(ExitCode::InvalidArguments);
    else inputs.emplace_back(arguments[index]);
  }
  if (directory.empty() || inputs.empty()) return ToInt(ExitCode::InvalidArguments);
  if (std::filesystem::exists(directory)) {
    error << "pkmn fred decode-batch: refusing existing output directory\n";
    return ToInt(ExitCode::OutputFailure);
  }
  try {
    const auto temporary = directory.string() + ".tmp";
    std::filesystem::create_directories(temporary);
    for (const auto &input : inputs) {
      const auto bytes = firered::ReadBinaryFile(input);
      const auto analysis = firered::AnalyzeSave(bytes);
      if (!ValidAnalysis(analysis)) throw std::runtime_error(input.filename().string() + " is invalid");
      const auto destination = std::filesystem::path(temporary) /
          (input.stem().string() + ".fred.json");
      WriteNewText(destination, firered::ExportMasterJson(input, bytes, analysis));
    }
    std::filesystem::rename(temporary, directory);
    output << "Decoded " << inputs.size() << " FireRed saves\nOutput: " << directory << '\n';
    return 0;
  } catch (const std::exception &exception) {
    std::error_code ignored; std::filesystem::remove_all(directory.string() + ".tmp", ignored);
    error << "pkmn fred decode-batch: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}

int ValidatePostEmulator(const std::vector<std::string> &arguments,
                         std::ostream &output, std::ostream &error) {
  if (arguments.size() < 3) return ToInt(ExitCode::InvalidArguments);
  std::filesystem::path directory = std::filesystem::path(arguments[2]).parent_path() /
      (std::filesystem::path(arguments[2]).stem().string() + ".firered-post-emulator");
  if (arguments.size() == 5 && arguments[3] == "--output-dir") directory = arguments[4];
  else if (arguments.size() != 3) return ToInt(ExitCode::InvalidArguments);
  if (std::filesystem::exists(directory)) return ToInt(ExitCode::OutputFailure);
  try {
    const auto beforeBytes = firered::ReadBinaryFile(arguments[1]);
    const auto afterBytes = firered::ReadBinaryFile(arguments[2]);
    const auto before = firered::AnalyzeSave(beforeBytes);
    const auto after = firered::AnalyzeSave(afterBytes);
    if (!ValidAnalysis(before) || !ValidAnalysis(after))
      throw std::runtime_error("both saves must be valid unambiguous FireRed saves");
    const auto beforeData = firered::DecodeFireRedSave(firered::AssembleLogicalSave(beforeBytes, before));
    const auto afterData = firered::DecodeFireRedSave(firered::AssembleLogicalSave(afterBytes, after));
    const bool identity = beforeData.playerName == afterData.playerName &&
                          beforeData.publicTrainerId == afterData.publicTrainerId;
    const auto report = nlohmann::ordered_json{{"format", "pkmn-firered-post-emulator"},
      {"version", "1.0.0"}, {"passed", identity},
      {"before", {{"valid", true}, {"playerName", beforeData.playerName}, {"trainerId", beforeData.publicTrainerId}, {"saveCounter", before.slots[*before.activeSlot].counter}}},
      {"after", {{"valid", true}, {"playerName", afterData.playerName}, {"trainerId", afterData.publicTrainerId}, {"saveCounter", after.slots[*after.activeSlot].counter}}},
      {"sameTrainerIdentity", identity},
      {"saveCounterAdvanced", after.slots[*after.activeSlot].counter != before.slots[*before.activeSlot].counter}};
    std::filesystem::create_directories(directory);
    WriteNewText(directory / "post-emulator-validation.json", report.dump(2) + "\n");
    WriteNewText(directory / "post-emulator-validation.md",
      "# FireRed Post-Emulator Validation\n\n- Integrity: PASS\n- Trainer identity: " +
      std::string(identity ? "PASS" : "FAIL") + "\n- Save counter changed: " +
      std::string(report["saveCounterAdvanced"].get<bool>() ? "yes" : "no") + "\n");
    output << "FireRed post-emulator validation: " << (identity ? "passed" : "failed")
           << "\nOutput: " << directory << '\n';
    return identity ? 0 : ToInt(ExitCode::SemanticMismatch);
  } catch (const std::exception &exception) {
    error << "pkmn fred validate-post-emulator: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}

int Edit(const std::vector<std::string> &arguments, std::ostream &output,
         std::ostream &error) {
  if (arguments.size() < 2) return ToInt(ExitCode::InvalidArguments);
  try {
    const std::filesystem::path input = arguments[1];
    auto destination = DefaultEdit(input);
    auto bytes = firered::ReadBinaryFile(input);
    auto analysis = firered::AnalyzeSave(bytes);
    if (!analysis.activeSlot || analysis.activeSlotAmbiguous ||
        !analysis.slots[*analysis.activeSlot].valid)
      throw std::runtime_error(
          "input is not an unambiguous valid FireRed save");
    firered::FireRedSafeEditor editor(bytes, analysis);
    for (std::size_t index = 2; index < arguments.size(); ++index) {
      if (arguments[index] == "--output" && index + 1 < arguments.size())
        destination = arguments[++index];
      else if (arguments[index] == "--player-name" && index + 1 < arguments.size())
        editor.SetPlayerName(arguments[++index]);
      else if (arguments[index] == "--rival-name" && index + 1 < arguments.size())
        editor.SetRivalName(arguments[++index]);
      else if (arguments[index] == "--money" && index + 1 < arguments.size())
        editor.SetMoney(static_cast<std::uint32_t>(std::stoul(arguments[++index])));
      else if (arguments[index] == "--coins" && index + 1 < arguments.size())
        editor.SetCoins(static_cast<std::uint16_t>(std::stoul(arguments[++index])));
      else if (arguments[index] == "--badge" && index + 1 < arguments.size()) {
        const auto value = arguments[++index];
        const auto colon = value.find(':');
        if (colon == std::string::npos) throw std::runtime_error("badge must be N:on or N:off");
        const auto setting = value.substr(colon + 1);
        if (setting != "on" && setting != "off")
          throw std::runtime_error("badge must be N:on or N:off");
        editor.SetBadge(std::stoul(value.substr(0, colon)),
                        setting == "on");
      } else throw std::runtime_error("invalid edit option");
    }
    if (editor.Changes().empty()) throw std::runtime_error("no edits requested");
    if (std::filesystem::exists(destination))
      throw std::runtime_error("refusing to overwrite existing output");
    const auto edited = editor.Finish();
    firered::WriteNewBinaryFile(destination, edited);
    output << editor.Report() << "\nOutput: " << destination.string() << '\n';
    return 0;
  } catch (const std::exception &exception) {
    error << "pkmn fred edit: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}

} // namespace

int Run(const std::vector<std::string> &arguments, std::ostream &output,
        std::ostream &error) {
  if (arguments.empty() || arguments.front() == "help" ||
      arguments.front() == "--help") {
    Help(output);
    return 0;
  }
  if (arguments.front() == "edit") return Edit(arguments, output, error);
  if (arguments.front() == "events") return Events(arguments, output, error);
  if (arguments.front() == "repair-checksums")
    return RepairChecksums(arguments, output, error);
  if (arguments.front() == "validate-batch")
    return ValidateBatch(arguments, output, error);
  if (arguments.front() == "decode-batch")
    return DecodeBatch(arguments, output, error);
  if (arguments.front() == "validate-post-emulator")
    return ValidatePostEmulator(arguments, output, error);
  if (arguments.front() == "begin-edit" || arguments.front() == "edit-session" ||
      arguments.front() == "pending-edits" || arguments.front() == "undo-edit" ||
      arguments.front() == "edit-history" || arguments.front() == "annotate-edit" ||
      arguments.front() == "validate-edit" || arguments.front() == "end-edit" ||
      arguments.front() == "pokemon" || arguments.front() == "bag" ||
      arguments.front() == "progress")
    return Session(arguments, output, error);
  if (arguments.size() < 2) return ToInt(ExitCode::InvalidArguments);
  try {
    const std::filesystem::path input = arguments[1];
    const auto bytes = firered::ReadBinaryFile(input);
    const auto analysis = firered::AnalyzeSave(bytes);
    const bool valid = analysis.standardFlashPresent && analysis.atLeastOneValidSlot &&
                       analysis.activeSlot && !analysis.activeSlotAmbiguous &&
                       analysis.slots[*analysis.activeSlot].valid;
    if (arguments.front() == "inspect" || arguments.front() == "validate") {
      const bool json = arguments.size() == 4 && arguments[2] == "--format" &&
                        arguments[3] == "json";
      if (arguments.size() != 2 && !json)
        return ToInt(ExitCode::InvalidArguments);
      if (json) output << AnalysisJson(analysis).dump(2) << '\n';
      else output << "FireRed save: " << input.filename().string()
                  << "\nStandard flash: " << (analysis.standardFlashPresent ? "yes" : "no")
                  << "\nActive slot: "
                  << (analysis.activeSlot ? std::to_string(*analysis.activeSlot) : "none")
                  << "\nValidation: " << (valid ? "passed" : "failed") << '\n';
      return valid ? 0 : ToInt(ExitCode::ChecksumFailure);
    }
    if (!valid) {
      error << "pkmn fred: input is not an unambiguous checksum-valid FireRed save\n";
      return ToInt(ExitCode::ChecksumFailure);
    }
    if (arguments.front() == "summary") {
      const bool detailed = arguments.size() == 3 && arguments[2] == "--detailed";
      if (arguments.size() != 2 && !detailed)
        return ToInt(ExitCode::InvalidArguments);
      const auto decoded = firered::DecodeFireRedSave(
          firered::AssembleLogicalSave(bytes, analysis));
      output << (detailed ? firered::DumpFireRedDetailedSummary(decoded)
                          : firered::DumpFireRedCompactSummary(decoded));
      return 0;
    }
    if (arguments.front() == "decode") {
      auto destination = firered::DefaultJsonPath(input);
      if (arguments.size() == 4 && arguments[2] == "--output")
        destination = arguments[3];
      else if (arguments.size() != 2)
        return ToInt(ExitCode::InvalidArguments);
      firered::WriteNewTextFile(destination,
          firered::ExportMasterJson(input, bytes, analysis));
      output << "Decoded Pokemon FireRed save\nOutput: " << destination.string() << '\n';
      return 0;
    }
  } catch (const std::exception &exception) {
    error << "pkmn fred: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
  Help(error);
  return ToInt(ExitCode::InvalidArguments);
}

} // namespace pkmn::cli::commands::fred
