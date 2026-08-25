#include "commands/conversion/ConversionCommand.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "app/ExitCode.hpp"
#include "conversion/RouteRegistry.hpp"
#include "red/generation/SemanticGenerator.hpp"
#include "red/json/RedDecoder.hpp"
#include "red/json/RedJsonDocument.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/AtomicOutput.hpp"
#include "util/OutputPath.hpp"
#include "util/ResourceLocator.hpp"
#include "util/Sha256.hpp"
#include "FileManipulation.hpp"
#include "FireRedMasterJson.hpp"

namespace pkmn::cli::commands::conversion {
namespace {

std::string ShellQuote(const std::string &value) {
#if defined(_WIN32)
  std::string result = "\"";
  for (const char character : value) {
    if (character == '"') result += "\"\"";
    else result += character;
  }
  return result + "\"";
#else
  std::string result = "'";
  for (const char character : value) {
    if (character == '\'') result += "'\\''";
    else result += character;
  }
  return result + "'";
#endif
}

std::filesystem::path UniqueTemporary(const std::string &suffix) {
  const auto tick = std::chrono::high_resolution_clock::now()
                        .time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("pkmn-v2-" + std::to_string(tick) + suffix);
}

std::string ReadText(const std::filesystem::path &path) {
  std::ifstream input(path);
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

int RunRuntime(const std::vector<std::string> &arguments,
               std::ostream &output, std::ostream &error,
               ExitCode failure = ExitCode::GenerationFailure) {
  const auto stdoutPath = UniqueTemporary(".stdout");
  const auto stderrPath = UniqueTemporary(".stderr");
  try {
    std::ostringstream command;
    const auto bundledRuntime = util::BundledRuntimeExecutablePath();
    if (!bundledRuntime.empty()) {
      command << ShellQuote(bundledRuntime.string());
    } else {
#if defined(_WIN32)
      command << "python ";
#else
      command << "python3 ";
#endif
      command << ShellQuote(util::FireRedRuntimeScriptPath().string());
    }
    for (const auto &argument : arguments)
      command << ' ' << ShellQuote(argument);
    command << " >" << ShellQuote(stdoutPath.string())
            << " 2>" << ShellQuote(stderrPath.string());
    const int status = std::system(command.str().c_str());
    const auto runtimeOutput = ReadText(stdoutPath);
    output << runtimeOutput;
    error << ReadText(stderrPath);
    std::error_code ignored;
    std::filesystem::remove(stdoutPath, ignored);
    std::filesystem::remove(stderrPath, ignored);
    if (status == 0 &&
        runtimeOutput.find("WITH_WARNINGS") != std::string::npos)
      error << "Report reproducible pkmn problems: "
               "https://github.com/AAAMAQ/pkmn-cli/issues\n";
    return status == 0 ? ToInt(ExitCode::Success) : ToInt(failure);
  } catch (const std::exception &exception) {
    std::error_code ignored;
    std::filesystem::remove(stdoutPath, ignored);
    std::filesystem::remove(stderrPath, ignored);
    error << "pkmn FireRed runtime: " << exception.what() << '\n';
    return ToInt(failure);
  }
}

std::string BaseName(const std::filesystem::path &input,
                     std::string_view suffix) {
  auto name = input.filename().string();
  if (name.ends_with(suffix)) name.resize(name.size() - suffix.size());
  else name = input.stem().string();
  return name;
}

std::filesystem::path DefaultSaveOutput(const std::filesystem::path &input,
                                        pkmn::cli::conversion::GameId source,
                                        pkmn::cli::conversion::GameId target) {
  const auto sourceSuffix = source == pkmn::cli::conversion::GameId::Blue
                                ? ".blue.json" : ".red.json";
  const auto targetSuffix = target == pkmn::cli::conversion::GameId::LeafGreen
                                ? "_lg.sav" : "_fr.sav";
  return input.parent_path() / (BaseName(input, sourceSuffix) + targetSuffix);
}

std::filesystem::path DefaultTargetJsonOutput(
    const std::filesystem::path &input,
    pkmn::cli::conversion::GameId source,
    pkmn::cli::conversion::GameId target) {
  const auto sourceSuffix = source == pkmn::cli::conversion::GameId::Blue
                                ? ".blue.json" : ".red.json";
  const auto targetSuffix = target == pkmn::cli::conversion::GameId::LeafGreen
                                ? ".lg.json" : ".fred.json";
  return input.parent_path() / (BaseName(input, sourceSuffix) + targetSuffix);
}

std::filesystem::path DefaultUpdated(const std::filesystem::path &input,
                                     std::string_view suffix) {
  return input.parent_path() /
         (BaseName(input, suffix) + ".updated" + std::string(suffix));
}

std::filesystem::path ResolveTemplate(const std::filesystem::path &explicitPath) {
  if (!explicitPath.empty()) return explicitPath;
  if (const char *configured = std::getenv("PKMN_FIRERED_TEMPLATE"))
    return configured;
  return util::FireRedTemplatePath();
}

bool OutputFamilyExists(const std::filesystem::path &output) {
  auto stem = output;
  stem.replace_extension();
  return std::filesystem::exists(output) ||
         std::filesystem::exists(stem.string() + ".conversion-manifest.json") ||
         std::filesystem::exists(stem.string() + ".conversion-report.md");
}

std::filesystem::path SelectOutput(std::filesystem::path output,
                                   bool autoSuffix) {
  if (!OutputFamilyExists(output)) return output;
  if (!autoSuffix)
    throw std::runtime_error("refusing to overwrite an existing output family");
  const auto preferred = output;
  for (std::size_t number = 2;; ++number) {
    output = util::NumberedOutputPath(preferred, number);
    if (!OutputFamilyExists(output)) return output;
  }
}

struct ConversionOptions {
  std::filesystem::path input;
  std::filesystem::path output;
  std::filesystem::path templatePath;
  std::string salt;
  std::string sourceName;
  std::string sourceSha256;
  std::filesystem::path manifestPath;
  std::filesystem::path reportPath;
  bool autoSuffix = false;
  bool keepIntermediate = false;
  bool autoRepairChecksum = false;
  std::filesystem::path repairedSourcePath;
  bool sourceRepairApplied = false;
  std::string sourceOriginalSha256;
  std::string sourceRepairedSha256;
  pkmn::cli::conversion::GameId sourceGame = pkmn::cli::conversion::GameId::Red;
  pkmn::cli::conversion::GameId targetGame = pkmn::cli::conversion::GameId::FireRed;
};

ConversionOptions ParseConversion(const std::vector<std::string> &arguments,
                                  bool toFrjson,
                                  pkmn::cli::conversion::GameId sourceGame = pkmn::cli::conversion::GameId::Red,
                                  pkmn::cli::conversion::GameId targetGame = pkmn::cli::conversion::GameId::FireRed) {
  if (arguments.size() < 2) throw std::runtime_error("input is required");
  ConversionOptions options;
  options.sourceGame = sourceGame;
  options.targetGame = targetGame;
  options.input = arguments[1];
  options.output = toFrjson ? DefaultTargetJsonOutput(options.input, sourceGame, targetGame)
                            : DefaultSaveOutput(options.input, sourceGame, targetGame);
  std::size_t index = 2;
  if (index < arguments.size() && !arguments[index].starts_with("--"))
    options.output = arguments[index++];
  for (; index < arguments.size(); ++index) {
    if (arguments[index] == "--output" && index + 1 < arguments.size())
      options.output = arguments[++index];
    else if (!toFrjson && arguments[index] == "--template" &&
             index + 1 < arguments.size())
      options.templatePath = arguments[++index];
    else if (arguments[index] == "--salt" && index + 1 < arguments.size())
      options.salt = arguments[++index];
    else if (arguments[index] == "--manifest" && index + 1 < arguments.size())
      options.manifestPath = arguments[++index];
    else if (arguments[index] == "--report" && index + 1 < arguments.size())
      options.reportPath = arguments[++index];
    else if (arguments[index] == "--policy" && index + 1 < arguments.size()) {
      if (arguments[++index] != "original")
        throw std::runtime_error("only the pinned 'original' conversion policy is supported");
    }
    else if (arguments[index] == "--auto-suffix")
      options.autoSuffix = true;
    else if (!toFrjson && arguments[index] == "--keep-intermediate")
      options.keepIntermediate = true;
    else if (!toFrjson &&
             (arguments[index] == "--auto-repair-checksum" ||
              arguments[index] == "--auto_repair_checksum"))
      options.autoRepairChecksum = true;
    else if (!toFrjson && arguments[index] == "--write-repaired-source" &&
             index + 1 < arguments.size())
      options.repairedSourcePath = arguments[++index];
    else
      throw std::runtime_error("invalid conversion option");
  }
  if (!options.repairedSourcePath.empty())
    options.autoRepairChecksum = true;
  options.output = SelectOutput(options.output, options.autoSuffix);
  return options;
}

int ConvertJson(const ConversionOptions &options, bool toFrjson,
                std::ostream &output, std::ostream &error) {
  if (options.sourceName.empty() &&
      (options.autoRepairChecksum || !options.repairedSourcePath.empty()))
    throw std::runtime_error(
        "checksum repair applies to physical Pokemon Red saves, not JSON");
  const auto loaded = red::json::LoadAndValidate(options.input);
  if (!loaded.validation.Valid()) {
    error << "pkmn conversion: Red JSON validation failed\n";
    return ToInt(ExitCode::InvalidInput);
  }
  const auto expectedProfile = options.sourceGame == pkmn::cli::conversion::GameId::Blue
                                   ? "GEN1_BLUE" : "GEN1_RED";
  if (loaded.root.contains("schema") &&
      loaded.root["schema"].contains("gameProfile") &&
      loaded.root["schema"]["gameProfile"].get<std::string>() != expectedProfile) {
    error << "pkmn conversion: JSON gameProfile conflicts with the explicit route; expected "
          << expectedProfile << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
  std::vector<std::string> runtime = {
      toFrjson ? "convert-to-frjson" : "convert-to-save",
      options.input.string(), "--output", options.output.string()};
  runtime.push_back("--source-game");
  runtime.push_back(options.sourceGame == pkmn::cli::conversion::GameId::Blue ? "blue" : "red");
  runtime.push_back("--target-game");
  runtime.push_back(options.targetGame == pkmn::cli::conversion::GameId::LeafGreen ? "leafgreen" : "firered");
  if (!options.salt.empty()) {
    runtime.push_back("--salt");
    runtime.push_back(options.salt);
  }
  if (!options.sourceName.empty()) {
    runtime.push_back("--source-name");
    runtime.push_back(options.sourceName);
  }
  if (!options.sourceSha256.empty()) {
    runtime.push_back("--source-sha256");
    runtime.push_back(options.sourceSha256);
  }
  runtime.push_back("--source-checksum-status");
  runtime.push_back(options.sourceName.empty()
                        ? "JSON_VALIDATED"
                        : (options.sourceRepairApplied ? "REPAIRED_IN_MEMORY"
                                                       : "VALID"));
  runtime.push_back("--source-repair-requested");
  runtime.push_back(options.autoRepairChecksum ? "true" : "false");
  runtime.push_back("--source-repair-applied");
  runtime.push_back(options.sourceRepairApplied ? "true" : "false");
  runtime.push_back("--repaired-source-written");
  runtime.push_back(options.repairedSourcePath.empty() ? "false" : "true");
  if (!options.sourceOriginalSha256.empty()) {
    runtime.push_back("--source-original-sha256");
    runtime.push_back(options.sourceOriginalSha256);
  }
  if (!options.sourceRepairedSha256.empty()) {
    runtime.push_back("--source-repaired-sha256");
    runtime.push_back(options.sourceRepairedSha256);
  }
  if (!options.manifestPath.empty()) {
    runtime.push_back("--manifest"); runtime.push_back(options.manifestPath.string());
  }
  if (!options.reportPath.empty()) {
    runtime.push_back("--report"); runtime.push_back(options.reportPath.string());
  }
  if (!toFrjson) {
    runtime.push_back("--template");
    runtime.push_back(ResolveTemplate(options.templatePath).string());
    if (options.keepIntermediate) runtime.push_back("--keep-intermediate");
  }
  return RunRuntime(runtime, output, error);
}

int RunUpdate(const std::vector<std::string> &arguments, std::string kind,
              std::ostream &output, std::ostream &error) {
  if (arguments.size() < 2)
    return ToInt(ExitCode::InvalidArguments);
  const std::filesystem::path input = arguments[1];
  if (kind == "rjson") {
    try {
      const auto loaded = red::json::LoadAndValidate(input);
      if (!loaded.validation.Valid()) {
        error << "pkmn rjson update_schema: input validation failed\n";
        return ToInt(ExitCode::InvalidInput);
      }
    } catch (const std::exception &exception) {
      error << "pkmn rjson update_schema: " << exception.what() << '\n';
      return ToInt(ExitCode::InvalidInput);
    }
  }
  const auto suffix = kind == "rjson" ? ".red.json" : ".fred.json";
  auto destination = DefaultUpdated(input, suffix);
  bool autoSuffix = false;
  for (std::size_t index = 2; index < arguments.size(); ++index) {
    if (arguments[index] == "--output" && index + 1 < arguments.size())
      destination = arguments[++index];
    else if (arguments[index] == "--auto-suffix") autoSuffix = true;
    else return ToInt(ExitCode::InvalidArguments);
  }
  if (std::filesystem::exists(destination)) {
    if (!autoSuffix) {
      error << "pkmn " << kind << " update_schema: refusing existing output\n";
      return ToInt(ExitCode::OutputFailure);
    }
    const auto preferred = destination;
    for (std::size_t number = 2; std::filesystem::exists(destination); ++number)
      destination = util::NumberedOutputPath(preferred, number, suffix);
  }
  return RunRuntime({"update-schema", kind, input.string(), "--output",
                     destination.string()}, output, error,
                    ExitCode::InvalidInput);
}

} // namespace

int RunGen1Convert(const std::vector<std::string> &arguments,
                   pkmn::cli::conversion::GameId sourceGame,
                   pkmn::cli::conversion::GameId targetGame,
                   std::ostream &output, std::ostream &error) {
  try {
    auto options = ParseConversion(arguments, false, sourceGame, targetGame);
    const auto sourceLabel = sourceGame == pkmn::cli::conversion::GameId::Blue ? "blue" : "red";
    const auto sourceDisplay = sourceGame == pkmn::cli::conversion::GameId::Blue ? "Blue" : "Red";
    const auto originalSave = red::save::RedSave::Read(options.input);
    const auto originalIntegrity =
        red::validation::SaveValidator::Validate(originalSave);
    if (!originalIntegrity.expectedSize) {
      error << "pkmn " << sourceLabel << " convert: input is not a standard Pokemon "
            << sourceDisplay << " save\n";
      return ToInt(ExitCode::InvalidInput);
    }
    auto save = originalSave;
    auto integrity = originalIntegrity;
    options.sourceOriginalSha256 = util::Sha256Hex(originalSave.BytesView());
    if (!integrity.Valid() && !options.autoRepairChecksum) {
      error << "pkmn " << sourceLabel << " convert: Gen I checksum validation failed\n";
      return ToInt(ExitCode::ChecksumFailure);
    }
    if (!integrity.Valid()) {
      auto repaired = originalSave.BytesView();
      red::generation::RepairChecksums(repaired);
      save = red::save::RedSave(std::move(repaired));
      integrity = red::validation::SaveValidator::Validate(save);
      if (!integrity.Valid()) {
        error << "pkmn " << sourceLabel << " convert: checksum repair did not produce a valid "
                 "source\n";
        return ToInt(ExitCode::ChecksumFailure);
      }
      options.sourceRepairApplied = true;
      options.sourceRepairedSha256 = util::Sha256Hex(save.BytesView());
    }
    const auto temporary = UniqueTemporary(".red.json");
    const auto document = red::json::Decode(
        save, options.input.filename().string(), integrity,
        {.includePhysicalImage = false});
    const auto semanticValidation = red::json::ValidateDocument(document);
    if (!semanticValidation.Valid()) {
      error << "pkmn red convert: source contains invalid semantic save data; "
               "checksum repair cannot make it convertible\n";
      for (const auto &message : semanticValidation.errors)
        error << "  - " << message << '\n';
      return ToInt(ExitCode::InvalidInput);
    }
    if (!options.repairedSourcePath.empty()) {
      util::WriteBinaryAtomic(options.repairedSourcePath, save.BytesView());
    }
    auto profiledDocument = document;
    profiledDocument["schema"]["gameProfile"] =
        sourceGame == pkmn::cli::conversion::GameId::Blue ? "GEN1_BLUE" : "GEN1_RED";
    profiledDocument["sourceDeclaration"] = {
        {"game", sourceGame == pkmn::cli::conversion::GameId::Blue ? "Pokemon Blue" : "Pokemon Red"},
        {"profile", profiledDocument["schema"]["gameProfile"]},
        {"basis", "explicit-conversion-route"},
        {"binaryLayout", "shared-generation-I-save-layout"},
    };
    util::WriteTextAtomic(temporary, red::json::Serialize(profiledDocument));
    options.sourceName = options.input.filename().string();
    // The source identity remains the original user's file. The repaired hash
    // is recorded separately in Manifest 3.0.
    options.sourceSha256 = options.sourceOriginalSha256;
    options.input = temporary;
    int result = 0;
    try {
      result = ConvertJson(options, false, output, error);
    } catch (...) {
      std::error_code ignored;
      std::filesystem::remove(temporary, ignored);
      throw;
    }
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    return result;
  } catch (const std::exception &exception) {
    error << "pkmn Gen I convert: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}

int RunRedConvert(const std::vector<std::string> &arguments,
                  std::ostream &output, std::ostream &error) {
  return RunGen1Convert(arguments, pkmn::cli::conversion::GameId::Red,
                        pkmn::cli::conversion::GameId::FireRed, output, error);
}

int RunRjsonExtension(const std::vector<std::string> &arguments,
                      std::ostream &output, std::ostream &error) {
  try {
    if (arguments.empty()) return ToInt(ExitCode::InvalidArguments);
    if (arguments.front() == "update_schema")
      return RunUpdate(arguments, "rjson", output, error);
    if (arguments.front() == "convert_to_frjson")
      return ConvertJson(ParseConversion(arguments, true), true, output, error);
    if (arguments.front() == "convert")
      return ConvertJson(ParseConversion(arguments, false), false, output, error);
  } catch (const std::exception &exception) {
    error << "pkmn rjson " << arguments.front() << ": " << exception.what()
          << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
  return ToInt(ExitCode::InvalidArguments);
}

int RunFrjson(const std::vector<std::string> &arguments,
              std::ostream &output, std::ostream &error) {
  if (arguments.empty() || arguments.front() == "help" ||
      arguments.front() == "--help") {
    output << "Usage:\n"
           << "  pkmn frjson inspect <save.fred.json>\n"
           << "  pkmn frjson validate <save.fred.json>\n"
           << "  pkmn frjson schema [--format json]\n"
           << "  pkmn frjson update_schema <save.fred.json> [--output <file>]\n"
           << "  pkmn frjson migrate <save.fred.json> [--output <file>]\n"
           << "  pkmn frjson reconstruct <save.fred.json> [--output <save.sav>]\n"
           << "  pkmn frjson generate <save.fred.json> [output.sav] [--template <clean.sav>]\n"
           << "  pkmn frjson generate-batch <save.fred.json>... --output-dir <directory> [--template <clean.sav>]\n";
    return 0;
  }
  if (arguments.front() == "schema") {
    const bool json = arguments.size() == 3 && arguments[1] == "--format" &&
                      arguments[2] == "json";
    if (arguments.size() != 1 && !json)
      return ToInt(ExitCode::InvalidArguments);
    if (json)
      output << "{\"nativeSchemaVersion\":\"0.4.0\","
                "\"plannedFormat\":\"pkmn-firered-planned-save\","
                "\"plannedFormatVersion\":\"1.0.0\","
                "\"nativeSemanticGenerationGate\":\"phase-5-accepted\"}\n";
    else
      output << "FireRed JSON schemas\n"
             << "  native: 0.4.0\n"
             << "  planned conversion: pkmn-firered-planned-save 1.0.0\n"
             << "  native semantic generation: Phase 5 accepted\n";
    return 0;
  }
  if (arguments.front() == "update_schema")
    return RunUpdate(arguments, "frjson", output, error);
  if (arguments.front() == "migrate") {
    auto forwarded = arguments; forwarded[0] = "update_schema";
    return RunUpdate(forwarded, "frjson", output, error);
  }
  if (arguments.front() == "reconstruct" && arguments.size() >= 2) {
    std::filesystem::path input = arguments[1];
    std::filesystem::path destination = input.parent_path() /
        (BaseName(input, ".fred.json") + "_reconstructed.sav");
    if (arguments.size() == 4 && arguments[2] == "--output")
      destination = arguments[3];
    else if (arguments.size() != 2)
      return ToInt(ExitCode::InvalidArguments);
    if (std::filesystem::exists(destination)) {
      error << "pkmn frjson reconstruct: refusing existing output\n";
      return ToInt(ExitCode::OutputFailure);
    }
    try {
      std::string sourceName;
      const auto bytes = firered::ImportPhysicalImage(
          firered::ReadTextFile(input), sourceName);
      firered::WriteNewBinaryFile(destination, bytes);
      output << "FireRed archival reconstruction complete\nOutput: "
             << destination.string() << '\n';
      return 0;
    } catch (const std::exception &exception) {
      error << "pkmn frjson reconstruct: " << exception.what() << '\n';
      return ToInt(ExitCode::InvalidInput);
    }
  }
  if ((arguments.front() == "inspect" || arguments.front() == "validate") &&
      arguments.size() == 2)
    return RunRuntime({"validate-frjson", arguments[1]}, output, error,
                      ExitCode::InvalidInput);
  if (arguments.front() == "generate" && arguments.size() >= 2) {
    std::filesystem::path input = arguments[1];
    std::filesystem::path destination = input.parent_path() /
        (BaseName(input, ".fred.json") + "_generated.sav");
    std::filesystem::path templatePath;
    std::size_t index = 2;
    if (index < arguments.size() && !arguments[index].starts_with("--"))
      destination = arguments[index++];
    for (; index < arguments.size(); ++index) {
      if (arguments[index] == "--template" && index + 1 < arguments.size())
        templatePath = arguments[++index];
      else return ToInt(ExitCode::InvalidArguments);
    }
    if (std::filesystem::exists(destination)) {
      error << "pkmn frjson generate: refusing existing output\n";
      return ToInt(ExitCode::OutputFailure);
    }
    try {
      return RunRuntime({"generate-frjson", input.string(), "--template",
                         ResolveTemplate(templatePath).string(), "--output",
                         destination.string()}, output, error);
    } catch (const std::exception &exception) {
      error << "pkmn frjson generate: " << exception.what() << '\n';
      return ToInt(ExitCode::InvalidInput);
    }
  }
  if (arguments.front() == "generate-batch") {
    std::filesystem::path directory, templatePath;
    std::vector<std::filesystem::path> inputs;
    for (std::size_t index = 1; index < arguments.size(); ++index) {
      if (arguments[index] == "--output-dir" && index + 1 < arguments.size()) directory = arguments[++index];
      else if (arguments[index] == "--template" && index + 1 < arguments.size()) templatePath = arguments[++index];
      else if (arguments[index].starts_with("--")) return ToInt(ExitCode::InvalidArguments);
      else inputs.emplace_back(arguments[index]);
    }
    if (directory.empty() || inputs.empty()) return ToInt(ExitCode::InvalidArguments);
    if (std::filesystem::exists(directory)) return ToInt(ExitCode::OutputFailure);
    const auto temporaryDirectory = std::filesystem::path(directory.string() + ".tmp");
    if (std::filesystem::exists(temporaryDirectory)) return ToInt(ExitCode::OutputFailure);
    try {
      std::filesystem::create_directories(temporaryDirectory);
      for (const auto &input : inputs) {
        const auto destination = temporaryDirectory / (BaseName(input, ".fred.json") + "_generated.sav");
        const auto result = RunRuntime({"generate-frjson", input.string(), "--template",
          ResolveTemplate(templatePath).string(), "--output", destination.string()}, output, error);
        if (result != 0) throw std::runtime_error("generation failed for " + input.filename().string());
      }
      std::filesystem::rename(temporaryDirectory, directory);
      output << "FireRed JSON generation batch complete\nOutput: " << directory << '\n';
      return 0;
    } catch (const std::exception &exception) {
      std::error_code ignored; std::filesystem::remove_all(temporaryDirectory, ignored);
      error << "pkmn frjson generate-batch: " << exception.what() << '\n';
      return ToInt(ExitCode::GenerationFailure);
    }
  }
  error << "pkmn frjson: invalid or unsupported arguments\n";
  return ToInt(ExitCode::InvalidArguments);
}

int RunRuntimeUtility(const std::vector<std::string> &arguments,
                      std::ostream &output, std::ostream &error) {
  return RunRuntime(arguments, output, error, ExitCode::InvalidInput);
}

int RunConvert(const std::vector<std::string> &arguments,
               std::ostream &output, std::ostream &error) {
  if (arguments.empty() || arguments.front() == "help" ||
      arguments.front() == "--help") {
    output << "Usage:\n"
           << "  pkmn convert red-to-firered <red.sav|red.json> [output.sav] [conversion options]\n"
           << "  pkmn convert red-firered <red.sav|red.json> [output.sav] [conversion options]\n"
           << "  pkmn convert red-leafgreen <red.sav|red.json> [output.sav] [conversion options]\n"
           << "  pkmn convert blue-firered <blue.sav|blue.json> [output.sav] [conversion options]\n"
           << "  pkmn convert blue-leafgreen <blue.sav|blue.json> [output.sav] [conversion options]\n"
           << "  pkmn convert routes [--format json]\n"
           << "  pkmn convert inspect <event|trainer|item> [query]\n"
           << "  pkmn convert explain <event|trainer|item> <query>\n"
           << "  pkmn convert validate-manifest <conversion-manifest.json>\n"
           << "  pkmn convert batch <source.sav|source.json>... --route <route> --output-dir <directory> [--template <clean.sav>]\n";
    return 0;
  }
  if (arguments.front() == "routes") {
    const bool json = arguments.size() == 3 && arguments[1] == "--format" &&
                      arguments[2] == "json";
    if (arguments.size() != 1 && !json)
      return ToInt(ExitCode::InvalidArguments);
    if (json) {
      nlohmann::ordered_json records = nlohmann::ordered_json::array();
      for (const auto &route : pkmn::cli::conversion::Routes()) {
        records.push_back({
            {"route", route.key},
            {"sourceProfile", pkmn::cli::conversion::Profile(route.source).key},
            {"targetProfile", pkmn::cli::conversion::Profile(route.target).key},
            {"capability", pkmn::cli::conversion::CapabilityName(route.capability)},
            {"evidence", route.evidence},
            {"policy", route.conversionPolicy},
        });
      }
      output << nlohmann::ordered_json({{"format", "pkmn-route-registry"},
                                        {"version", "3.0.0"},
                                        {"routes", records}})
                    .dump(2)
             << '\n';
    } else {
      output << "pkmn conversion routes\n";
      for (const auto &route : pkmn::cli::conversion::Routes())
        output << "  " << route.key << "  "
               << pkmn::cli::conversion::CapabilityName(route.capability) << "  "
               << route.evidence << '\n';
    }
    return 0;
  }
  if ((arguments.front() == "inspect" || arguments.front() == "explain") &&
      arguments.size() >= 2 && arguments.size() <= 3) {
    std::vector<std::string> runtime{"bridge-inspect", arguments[1]};
    if (arguments.size() == 3) runtime.push_back(arguments[2]);
    return RunRuntime(runtime, output, error, ExitCode::InvalidInput);
  }
  if (arguments.front() == "validate-manifest" && arguments.size() == 2)
    return RunRuntime({"validate-manifest", arguments[1]}, output, error,
                      ExitCode::InvalidInput);
  const auto selectedRoute = pkmn::cli::conversion::FindRoute(arguments.front());
  if (selectedRoute && selectedRoute->capability ==
                           pkmn::cli::conversion::Capability::Planned) {
    error << "pkmn convert: route '" << selectedRoute->key
          << "' is declared but not available in this build\n";
    return ToInt(ExitCode::UnsupportedOperation);
  }
  if (selectedRoute && arguments.size() >= 2) {
    const auto sourceGame = selectedRoute->source;
    const auto targetGame = selectedRoute->target;
    const std::filesystem::path input = arguments[1];
    bool planOnly = false;
    std::filesystem::path explicitOutput;
    std::vector<std::string> options;
    for (std::size_t index = 2; index < arguments.size(); ++index) {
      if (arguments[index] == "--preview" || arguments[index] == "--plan-only")
        planOnly = true;
      else if ((arguments[index] == "--output-json" || arguments[index] == "--output-save" ||
                arguments[index] == "--output") &&
               index + 1 < arguments.size()) {
        planOnly = planOnly || arguments[index] == "--output-json";
        explicitOutput = arguments[++index];
      } else if (arguments[index] == "--format" && index + 1 < arguments.size()) {
        const auto format = arguments[++index];
        if (format != "text" && format != "json" && format != "markdown")
          return ToInt(ExitCode::InvalidArguments);
      } else if (!arguments[index].starts_with("--") && explicitOutput.empty())
        explicitOutput = arguments[index];
      else {
        const auto optionName = arguments[index];
        const bool omitForPlan = planOnly &&
            (optionName == "--template" || optionName == "--keep-intermediate");
        if (!omitForPlan) options.push_back(optionName);
        if ((arguments[index] == "--template" || arguments[index] == "--salt" ||
             arguments[index] == "--manifest" || arguments[index] == "--report" ||
             arguments[index] == "--policy" ||
             arguments[index] == "--write-repaired-source") &&
            index + 1 < arguments.size()) {
          const auto value = arguments[++index];
          if (!omitForPlan) options.push_back(value);
        }
      }
    }
    const bool inputJson = input.filename().string().ends_with(".json");
    std::filesystem::path jsonInput = input;
    std::filesystem::path temporary;
    try {
      if (!inputJson && planOnly) {
        const auto save = red::save::RedSave::Read(input);
        const auto integrity = red::validation::SaveValidator::Validate(save);
        if (!integrity.Valid()) throw std::runtime_error("Gen I save validation failed");
        temporary = UniqueTemporary(".red.json");
        auto document = red::json::Decode(save, input.filename().string(), integrity,
                                          {.includePhysicalImage = false});
        document["schema"]["gameProfile"] =
            sourceGame == pkmn::cli::conversion::GameId::Blue ? "GEN1_BLUE" : "GEN1_RED";
        util::WriteTextAtomic(temporary, red::json::Serialize(document));
        jsonInput = temporary;
      }
      std::vector<std::string> forwarded{planOnly ? "convert_to_frjson" : "convert",
                                         (planOnly ? jsonInput : input).string()};
      if (planOnly && explicitOutput.empty())
        explicitOutput = DefaultTargetJsonOutput(input, sourceGame, targetGame);
      if (!explicitOutput.empty()) forwarded.push_back(explicitOutput.string());
      forwarded.insert(forwarded.end(), options.begin(), options.end());
      int result = 0;
      if (planOnly || inputJson) {
        const auto parsed = ParseConversion(forwarded, planOnly, sourceGame, targetGame);
        result = ConvertJson(parsed, planOnly, output, error);
      } else {
        result = RunGen1Convert(forwarded, sourceGame, targetGame, output, error);
      }
      if (!temporary.empty()) { std::error_code ignored; std::filesystem::remove(temporary, ignored); }
      return result;
    } catch (const std::exception &exception) {
      if (!temporary.empty()) { std::error_code ignored; std::filesystem::remove(temporary, ignored); }
      error << "pkmn convert " << selectedRoute->key << ": " << exception.what() << '\n';
      return ToInt(ExitCode::InvalidInput);
    }
  }
  if (arguments.front() == "batch") {
    std::filesystem::path directory, templatePath;
    std::string salt;
    auto batchRoute = pkmn::cli::conversion::FindRoute("red-firered");
    std::vector<std::filesystem::path> inputs;
    for (std::size_t index = 1; index < arguments.size(); ++index) {
      if (arguments[index] == "--output-dir" && index + 1 < arguments.size())
        directory = arguments[++index];
      else if (arguments[index] == "--template" && index + 1 < arguments.size())
        templatePath = arguments[++index];
      else if (arguments[index] == "--salt" && index + 1 < arguments.size())
        salt = arguments[++index];
      else if (arguments[index] == "--route" && index + 1 < arguments.size()) {
        batchRoute = pkmn::cli::conversion::FindRoute(arguments[++index]);
        if (!batchRoute || batchRoute->capability !=
                               pkmn::cli::conversion::Capability::Available)
          return ToInt(ExitCode::UnsupportedOperation);
      }
      else if (arguments[index].starts_with("--"))
        return ToInt(ExitCode::InvalidArguments);
      else inputs.emplace_back(arguments[index]);
    }
    if (directory.empty() || inputs.empty()) return ToInt(ExitCode::InvalidArguments);
    if (std::filesystem::exists(directory)) return ToInt(ExitCode::OutputFailure);
    const auto temporaryDirectory = std::filesystem::path(directory.string() + ".tmp");
    if (std::filesystem::exists(temporaryDirectory)) return ToInt(ExitCode::OutputFailure);
    try {
      std::filesystem::create_directories(temporaryDirectory);
      for (const auto &input : inputs) {
        const auto destination = temporaryDirectory /
            DefaultSaveOutput(input, batchRoute->source, batchRoute->target).filename();
        std::vector<std::string> forwarded{"convert", input.string(), destination.string(),
                                           "--template", ResolveTemplate(templatePath).string()};
        if (!salt.empty()) { forwarded.push_back("--salt"); forwarded.push_back(salt); }
        const auto result = input.filename().string().ends_with(".json")
            ? ConvertJson(ParseConversion(forwarded, false, batchRoute->source,
                                          batchRoute->target),
                          false, output, error)
            : RunGen1Convert(forwarded, batchRoute->source, batchRoute->target,
                             output, error);
        if (result != 0) throw std::runtime_error("batch conversion failed for " + input.filename().string());
      }
      std::filesystem::rename(temporaryDirectory, directory);
      output << "Conversion batch complete\nOutput: " << directory << '\n';
      return 0;
    } catch (const std::exception &exception) {
      std::error_code ignored; std::filesystem::remove_all(temporaryDirectory, ignored);
      error << "pkmn convert batch: " << exception.what() << '\n';
      return ToInt(ExitCode::GenerationFailure);
    }
  }
  error << "pkmn convert: invalid arguments\n";
  return ToInt(ExitCode::InvalidArguments);
}

} // namespace pkmn::cli::commands::conversion
