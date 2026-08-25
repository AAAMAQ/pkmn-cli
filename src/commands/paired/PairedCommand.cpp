#include "commands/paired/PairedCommand.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

#include "app/ExitCode.hpp"
#include "commands/conversion/ConversionCommand.hpp"
#include "commands/fred/FredCommand.hpp"
#include "commands/red/RedCommand.hpp"
#include "commands/rjson/RjsonCommand.hpp"
#include "red/json/RedDecoder.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/AtomicOutput.hpp"
#include "FileManipulation.hpp"
#include "FireRedLogicalSave.hpp"
#include "FireRedMasterJson.hpp"

namespace pkmn::cli::commands::paired {
namespace {

std::filesystem::path WithSuffix(const std::filesystem::path& input,
                                 std::string_view suffix) {
  return input.parent_path() / (input.stem().string() + std::string(suffix));
}

std::string CanonicalBase(const std::filesystem::path& input,
                          std::string_view suffix) {
  auto name = input.filename().string();
  if (name.ends_with(suffix)) name.resize(name.size() - suffix.size());
  else name = input.stem().string();
  return name;
}

bool JsonProfileConflicts(const std::filesystem::path& input,
                          std::string_view expected) {
  std::ifstream stream(input);
  if (!stream) return false;
  nlohmann::ordered_json document;
  try { stream >> document; } catch (...) { return false; }
  std::string declared;
  if (document.contains("schema") && document["schema"].is_object())
    declared = document["schema"].value("gameProfile", "");
  if (declared.empty()) declared = document.value("gameProfile", "");
  return !declared.empty() && declared != expected;
}

int BlueDecode(const std::vector<std::string>& arguments, std::ostream& output,
               std::ostream& error) {
  if (arguments.size() != 2 &&
      !(arguments.size() == 4 && arguments[2] == "--output"))
    return ToInt(ExitCode::InvalidArguments);
  try {
    const std::filesystem::path input = arguments[1];
    const auto destination = arguments.size() == 4
                                 ? std::filesystem::path(arguments[3])
                                 : WithSuffix(input, ".blue.json");
    const auto save = pkmn::cli::red::save::RedSave::Read(input);
    const auto integrity = pkmn::cli::red::validation::SaveValidator::Validate(save);
    if (!integrity.Valid()) return ToInt(ExitCode::ChecksumFailure);
    auto document = pkmn::cli::red::json::Decode(save, input.filename().string(), integrity,
                                      {.includePhysicalImage = false});
    document["schema"]["gameProfile"] = "GEN1_BLUE";
    document["sourceDeclaration"] = {
        {"game", "Pokemon Blue"}, {"profile", "GEN1_BLUE"},
        {"basis", "explicit-blue-command"},
        {"binaryLayout", "shared-generation-I-save-layout"}};
    util::WriteTextAtomic(destination, pkmn::cli::red::json::Serialize(document));
    output << "Decoded Pokemon Blue save\nOutput: " << destination.string()
           << "\nProfile declaration: GEN1_BLUE\n";
    return 0;
  } catch (const std::exception& exception) {
    error << "pkmn blue decode: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}

int LeafGreenDecode(const std::vector<std::string>& arguments,
                    std::ostream& output, std::ostream& error) {
  if (arguments.size() != 2 &&
      !(arguments.size() == 4 && arguments[2] == "--output"))
    return ToInt(ExitCode::InvalidArguments);
  try {
    const std::filesystem::path input = arguments[1];
    const auto destination = arguments.size() == 4
                                 ? std::filesystem::path(arguments[3])
                                 : WithSuffix(input, ".lg.json");
    const auto bytes = firered::ReadBinaryFile(input);
    const auto analysis = firered::AnalyzeSave(bytes);
    if (!analysis.standardFlashPresent || !analysis.atLeastOneValidSlot ||
        !analysis.activeSlot || analysis.activeSlotAmbiguous)
      return ToInt(ExitCode::ChecksumFailure);
    auto document = nlohmann::ordered_json::parse(
        firered::ExportMasterJson(input, bytes, analysis));
    document["gameProfile"] = "GEN3_LEAFGREEN";
    document["sourceDeclaration"] = {
        {"game", "Pokemon LeafGreen"}, {"profile", "GEN3_LEAFGREEN"},
        {"basis", "explicit-leafgreen-command"},
        {"binaryLayout", "shared-generation-III-kanto-remake-layout"}};
    util::WriteTextAtomic(destination, document.dump(2) + "\n");
    output << "Decoded Pokemon LeafGreen save\nOutput: " << destination.string()
           << "\nProfile declaration: GEN3_LEAFGREEN\n";
    return 0;
  } catch (const std::exception& exception) {
    error << "pkmn leafgreen decode: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}

std::vector<std::string> RouteArguments(std::string route,
                                        const std::vector<std::string>& arguments) {
  std::vector<std::string> forwarded{std::move(route)};
  forwarded.insert(forwarded.end(), arguments.begin() + 1, arguments.end());
  return forwarded;
}

void ReplaceAll(std::string& value, std::string_view from,
                std::string_view to) {
  std::size_t position = 0;
  while ((position = value.find(from, position)) != std::string::npos) {
    value.replace(position, from.size(), to);
    position += to.size();
  }
}

template <typename Function>
int ProfiledDelegate(Function&& function, std::ostream& output,
                     std::ostream& error, bool blue) {
  std::ostringstream capturedOutput, capturedError;
  const int result = function(capturedOutput, capturedError);
  auto stdoutText = capturedOutput.str();
  auto stderrText = capturedError.str();
  if (blue) {
    ReplaceAll(stdoutText, "Pokemon Red", "Pokemon Blue");
    ReplaceAll(stderrText, "Pokemon Red", "Pokemon Blue");
    ReplaceAll(stdoutText, "Red JSON", "Blue JSON");
    ReplaceAll(stdoutText, ".red.json", ".blue.json");
    ReplaceAll(stderrText, "pkmn red", "pkmn blue");
  } else {
    ReplaceAll(stdoutText, "Pokemon FireRed", "Pokemon LeafGreen");
    ReplaceAll(stderrText, "Pokemon FireRed", "Pokemon LeafGreen");
    ReplaceAll(stdoutText, "FireRed", "LeafGreen");
    ReplaceAll(stderrText, "FireRed", "LeafGreen");
    ReplaceAll(stdoutText, "firered", "leafgreen");
    ReplaceAll(stderrText, "firered", "leafgreen");
    ReplaceAll(stdoutText, ".fred.json", ".lg.json");
    ReplaceAll(stderrText, ".fred.json", ".lg.json");
  }
  output << stdoutText;
  error << stderrText;
  return result;
}

}  // namespace

int RunBlue(const std::vector<std::string>& arguments, std::ostream& output,
            std::ostream& error) {
  if (arguments.empty() || arguments.front() == "help" ||
      arguments.front() == "--help") {
    output << "Pokemon Blue uses the verified shared Generation I engine.\n"
              "Key commands: blue summary|inspect|validate|decode|repair-checksums|events|convert\n"
              "Default conversion: pkmn blue convert <save.sav> -> LeafGreen\n"
              "Use pkmn convert blue-firered for the alternate remake target.\n";
    return 0;
  }
  if (arguments.front() == "decode") return BlueDecode(arguments, output, error);
  if (arguments.front() == "convert") {
    std::string route = "blue-leafgreen";
    std::vector<std::string> forwarded{route};
    for (std::size_t index = 1; index < arguments.size(); ++index) {
      if (arguments[index] == "--target" && index + 1 < arguments.size()) {
        const auto target = arguments[++index];
        if (target == "firered" || target == "fr") route = "blue-firered";
        else if (target != "leafgreen" && target != "lg")
          return ToInt(ExitCode::InvalidArguments);
        continue;
      }
      forwarded.push_back(arguments[index]);
    }
    forwarded[0] = route;
    return commands::conversion::RunConvert(forwarded, output, error);
  }
  return ProfiledDelegate(
      [&](std::ostream& delegatedOutput, std::ostream& delegatedError) {
        return commands::red::Run(arguments, delegatedOutput, delegatedError);
      }, output, error, true);
}

int RunBlueJson(const std::vector<std::string>& arguments,
                std::ostream& output, std::ostream& error) {
  if (arguments.empty() || arguments.front() == "help" ||
      arguments.front() == "--help") {
    output << "Canonical Pokemon Blue JSON profile: GEN1_BLUE (.blue.json).\n"
              "Conversion: bjson convert, convert_to_lgjson, convert_to_frjson.\n"
              "Other commands share the canonical Gen I JSON engine.\n";
    return 0;
  }
  if (arguments.size() >= 2 && arguments.front() != "schema" &&
      JsonProfileConflicts(arguments[1], "GEN1_BLUE")) {
    error << "pkmn bjson: JSON gameProfile conflicts with GEN1_BLUE\n";
    return ToInt(ExitCode::InvalidInput);
  }
  if (arguments.front() == "schema") {
    const bool json = arguments.size() == 3 && arguments[1] == "--format" &&
                      arguments[2] == "json";
    if (arguments.size() != 1 && !json)
      return ToInt(ExitCode::InvalidArguments);
    if (json)
      output << nlohmann::ordered_json({
          {"format", "pkmn-blue-json-schema-capabilities"},
          {"schemaFormat", "pkmn-red-master-save"},
          {"schemaVersion", "0.1.0"}, {"gameProfile", "GEN1_BLUE"},
          {"extension", ".blue.json"},
          {"binaryEngine", "GEN1_KANTO_SHARED"}}).dump(2) << '\n';
    else
      output << "Canonical Pokemon Blue JSON\nBase format: pkmn-red-master-save 0.1.0\n"
                "Required profile: GEN1_BLUE\nExtension: .blue.json\n";
    return 0;
  }
  if (arguments.front() == "convert" || arguments.front() == "convert_to_lgjson" ||
      arguments.front() == "convert_to_frjson") {
    const bool plan = arguments.front() != "convert";
    const auto route = arguments.front() == "convert_to_frjson"
                           ? "blue-firered" : "blue-leafgreen";
    auto forwarded = RouteArguments(route, arguments);
    if (plan) forwarded.insert(forwarded.begin() + 2, "--plan-only");
    return commands::conversion::RunConvert(forwarded, output, error);
  }
  if ((arguments.front() == "migrate" || arguments.front() == "update_schema") &&
      arguments.size() == 2) {
    auto forwarded = arguments;
    const std::filesystem::path input = arguments[1];
    const auto tag = arguments.front() == "migrate" ? ".migrated" : ".updated";
    forwarded.push_back("--output");
    forwarded.push_back((input.parent_path() /
        (CanonicalBase(input, ".blue.json") + tag + ".blue.json")).string());
    return ProfiledDelegate(
        [&](std::ostream& delegatedOutput, std::ostream& delegatedError) {
          return commands::rjson::Run(forwarded, delegatedOutput, delegatedError);
        }, output, error, true);
  }
  return ProfiledDelegate(
      [&](std::ostream& delegatedOutput, std::ostream& delegatedError) {
        return commands::rjson::Run(arguments, delegatedOutput, delegatedError);
      }, output, error, true);
}

int RunLeafGreen(const std::vector<std::string>& arguments,
                 std::ostream& output, std::ostream& error) {
  if (arguments.empty() || arguments.front() == "help" ||
      arguments.front() == "--help") {
    output << "Pokemon LeafGreen uses the shared Generation III Kanto-remake engine.\n"
              "Commands: leafgreen inspect|validate|summary|decode|repair-checksums|events|edit.\n";
    return 0;
  }
  if (arguments.front() == "decode")
    return LeafGreenDecode(arguments, output, error);
  return ProfiledDelegate(
      [&](std::ostream& delegatedOutput, std::ostream& delegatedError) {
        return commands::fred::Run(arguments, delegatedOutput, delegatedError);
      }, output, error, false);
}

int RunLeafGreenJson(const std::vector<std::string>& arguments,
                     std::ostream& output, std::ostream& error) {
  if (arguments.empty() || arguments.front() == "help" ||
      arguments.front() == "--help") {
    output << "Canonical Pokemon LeafGreen JSON profile: GEN3_LEAFGREEN (.lg.json).\n"
              "Generation and validation share the Kanto-remake JSON engine.\n";
    return 0;
  }
  if (arguments.size() >= 2 && arguments.front() != "schema" &&
      JsonProfileConflicts(arguments[1], "GEN3_LEAFGREEN")) {
    error << "pkmn lgjson: JSON gameProfile conflicts with GEN3_LEAFGREEN\n";
    return ToInt(ExitCode::InvalidInput);
  }
  auto forwarded = arguments;
  if ((arguments.front() == "update_schema" || arguments.front() == "migrate") &&
      arguments.size() == 2) {
    const std::filesystem::path input = arguments[1];
    const auto tag = arguments.front() == "migrate" ? ".migrated" : ".updated";
    forwarded.push_back("--output");
    forwarded.push_back((input.parent_path() /
        (CanonicalBase(input, ".lg.json") + tag + ".lg.json")).string());
  } else if (arguments.front() == "generate" && arguments.size() == 2) {
    const std::filesystem::path input = arguments[1];
    forwarded.push_back((input.parent_path() /
        (CanonicalBase(input, ".lg.json") + "_generated.sav")).string());
  } else if (arguments.front() == "reconstruct" && arguments.size() == 2) {
    const std::filesystem::path input = arguments[1];
    forwarded.push_back("--output");
    forwarded.push_back((input.parent_path() /
        (CanonicalBase(input, ".lg.json") + "_reconstructed.sav")).string());
  }
  return ProfiledDelegate(
      [&](std::ostream& delegatedOutput, std::ostream& delegatedError) {
        return commands::conversion::RunFrjson(forwarded, delegatedOutput,
                                                delegatedError);
      }, output, error, false);
}

}  // namespace pkmn::cli::commands::paired
