#include "commands/rjson/RjsonCommand.hpp"

#include <filesystem>
#include <fstream>
#include <ostream>
#include <iostream>
#include <set>

#include "app/ExitCode.hpp"
#include "red/generation/SemanticGenerator.hpp"
#include "red/json/RedJsonDocument.hpp"
#include "red/events/EventCatalog.hpp"
#include "red/codec/Gen1Codec.hpp"
#include "util/Sha256.hpp"
#include "util/OutputPath.hpp"
#include "util/AtomicOutput.hpp"
#include "app/Version.hpp"

namespace pkmn::cli::commands::rjson {
namespace {

void Help(std::ostream &output) {
  output
      << "Usage:\n"
      << "  pkmn rjson inspect <file.red.json> [--format json]\n"
      << "  pkmn rjson validate <file.red.json> [--format json] "
         "[--profile standard|strict|generation|archival]\n"
      << "  pkmn rjson generate <file.red.json> [output.sav] [--auto-suffix]\n"
      << "  pkmn rjson reconstruct <file.red.json> [--output <output.sav>] "
         "[--auto-suffix]\n"
      << "  pkmn rjson migrate <file.red.json> [--output <migrated.red.json>] "
         "[--auto-suffix]\n"
      << "  pkmn rjson schema [--format json]\n"
      << "  pkmn rjson generate-batch <file.red.json>... --output-dir "
         "<directory>\n";
}

int RunSchema(const std::vector<std::string> &arguments,
              std::ostream &output, std::ostream &error) {
  bool json = false;
  if (arguments.size() == 3 && arguments[1] == "--format" &&
      arguments[2] == "json")
    json = true;
  else if (arguments.size() != 1) {
    error << "pkmn rjson schema: invalid arguments\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  const red::json::OrderedJson report = {
      {"format", "pkmn-red-json-schema-capabilities"},
      {"reportVersion", "1.0.0"},
      {"schemaFormat", "pkmn-red-master-save"},
      {"schemaVersion", "0.1.0"},
      {"extension", ".red.json"},
      {"requiredTopLevel", {"schema", "source", "integrity", "decoded"}},
      {"optionalTopLevel", {"tool", "reconstruction", "diagnostics", "physicalImage"}},
      {"validationProfiles", {"standard", "strict", "generation", "archival"}},
      {"semanticAuthority", "decoded"},
      {"archivalAuthority", "physicalImage"},
      {"physicalImageUsedForGeneration", false},
      {"namedEventCatalogSize", red::events::Catalog().size()},
      {"documentation", "docs/RED_JSON_SCHEMA.md"}};
  if (json)
    output << report.dump(2) << '\n';
  else
    output << "Canonical Pokemon Red JSON\n"
           << "Format: pkmn-red-master-save\nSchema version: 0.1.0\n"
           << "Extension: .red.json\nValidation profiles: standard, strict, "
              "generation, archival\n"
           << "Semantic authority: decoded (physicalImage is archival only)\n"
           << "Verified named events: " << red::events::Catalog().size() << '\n'
           << "Reference: docs/RED_JSON_SCHEMA.md\n";
  return ToInt(ExitCode::Success);
}

std::filesystem::path DefaultGeneration(const std::filesystem::path &input) {
  std::string name = input.filename().string();
  constexpr std::string_view suffix = ".red.json";
  if (name.ends_with(suffix))
    name.resize(name.size() - suffix.size());
  else
    name = input.stem().string();
  return (input.has_parent_path() ? input.parent_path()
                                  : std::filesystem::path(".")) /
         (name + "_generated_semantic.sav");
}

std::filesystem::path
DefaultReconstruction(const std::filesystem::path &input) {
  std::string name = input.filename().string();
  constexpr std::string_view suffix = ".red.json";
  if (name.ends_with(suffix))
    name.resize(name.size() - suffix.size());
  else
    name = input.stem().string();
  return (input.has_parent_path() ? input.parent_path()
                                  : std::filesystem::path(".")) /
         (name + "_reconstructed_from-physical-image.sav");
}

std::filesystem::path DefaultMigration(const std::filesystem::path &input) {
  std::string name = input.filename().string();
  constexpr std::string_view suffix = ".red.json";
  if (name.ends_with(suffix)) name.resize(name.size() - suffix.size());
  return input.parent_path() / (name + ".migrated.red.json");
}

int RunMigration(const std::vector<std::string> &arguments,
                 std::ostream &output, std::ostream &error) {
  if (arguments.size() < 2)
    return ToInt(ExitCode::InvalidArguments);
  const std::filesystem::path inputPath = arguments[1];
  auto outputPath = DefaultMigration(inputPath);
  bool autoSuffix = false;
  for (std::size_t index = 2; index < arguments.size(); ++index) {
    if (arguments[index] == "--output" && index + 1 < arguments.size())
      outputPath = arguments[++index];
    else if (arguments[index] == "--auto-suffix")
      autoSuffix = true;
    else {
      error << "pkmn rjson migrate: invalid arguments\n";
      return ToInt(ExitCode::InvalidArguments);
    }
  }
  const auto preferred = outputPath;
  if (autoSuffix && std::filesystem::exists(outputPath))
    for (std::size_t number = 2;; ++number) {
      outputPath = util::NumberedOutputPath(preferred, number, ".red.json");
      if (!std::filesystem::exists(outputPath)) break;
    }
  if (std::filesystem::exists(outputPath)) {
    error << "pkmn rjson migrate: refusing existing output\n";
    return ToInt(ExitCode::OutputFailure);
  }
  try {
    const auto loaded = red::json::LoadAndValidate(inputPath);
    if (!loaded.validation.Valid())
      throw std::runtime_error("input document does not validate");
    auto document = loaded.root;
    auto &decoded = document.at("decoded");
    bool enrichedEvents = false;
    if (!decoded.contains("events")) {
      const auto bytes = red::codec::DecodeHex(
          decoded.at("worldStateRaw").at("eventFlagsHex").get<std::string>());
      const auto named = red::events::DecodeNamedState(bytes);
      for (const auto &[key, value] : named.items())
        decoded[key] = value;
      enrichedEvents = true;
    }
    document["tool"] = {{"name", "pkmn"}, {"version", std::string(kVersion)}};
    document["migration"] = {
        {"targetSchemaVersion", "0.1.0"},
        {"profile", "canonical-enrichment"},
        {"namedEventCatalogAdded", enrichedEvents},
        {"semanticValuesChanged", false}};
    const auto validation = red::json::ValidateDocument(document);
    if (!validation.Valid())
      throw std::runtime_error("migrated document failed validation");
    util::WriteTextAtomic(outputPath, red::json::Serialize(document));
    output << "Red JSON migration complete\nOutput: "
           << outputPath.filename().string()
           << "\nSchema: 0.1.0\nNamed event catalog added: "
           << (enrichedEvents ? "yes" : "already present") << '\n';
    return 0;
  } catch (const std::exception &exception) {
    error << "pkmn rjson migrate: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}

int RunGenerateBatch(const std::vector<std::string> &arguments,
                     std::ostream &output, std::ostream &error) {
  std::vector<std::filesystem::path> inputs;
  std::filesystem::path directory;
  for (std::size_t index = 1; index < arguments.size(); ++index) {
    if (arguments[index] == "--output-dir" && index + 1 < arguments.size())
      directory = arguments[++index];
    else if (arguments[index].starts_with("--"))
      return ToInt(ExitCode::InvalidArguments);
    else
      inputs.emplace_back(arguments[index]);
  }
  if (inputs.empty() || directory.empty())
    return ToInt(ExitCode::InvalidArguments);
  if (std::filesystem::exists(directory)) {
    error << "pkmn rjson generate-batch: refusing existing output directory\n";
    return ToInt(ExitCode::OutputFailure);
  }
  try {
    util::DirectoryTransaction transaction(directory);
    std::set<std::string> names;
    auto rows = red::json::OrderedJson::array();
    for (const auto &input : inputs) {
      std::string stem = input.filename().string();
      if (stem.ends_with(".red.json")) stem.resize(stem.size() - 9);
      const auto name = stem + "_generated_semantic.sav";
      if (!names.insert(name).second)
        throw std::runtime_error("batch inputs produce duplicate output names");
      const auto loaded = red::json::LoadAndValidate(input);
      if (!loaded.validation.Valid())
        throw std::runtime_error(input.filename().string() +
                                 " failed semantic validation");
      const auto generated = red::generation::Generate(loaded.root);
      util::WriteBinaryAtomic(transaction.Path() / name, generated.bytes);
      util::WriteTextAtomic(transaction.Path() / (name + ".generation-report.json"),
                            generated.report.dump(2) + '\n');
      util::WriteTextAtomic(transaction.Path() / (name + ".generation-report.md"),
                            generated.markdownReport);
      rows.push_back({{"input", input.filename().string()}, {"output", name},
                      {"sha256", util::Sha256Hex(generated.bytes)}});
    }
    util::WriteTextAtomic(
        transaction.Path() / "batch-manifest.json",
        red::json::OrderedJson({{"command", "rjson generate-batch"},
                                {"count", inputs.size()},
                                {"physicalImageAuthority", false},
                                {"results", rows}}).dump(2) + '\n');
    transaction.Commit();
    output << "Batch semantic generation complete\nOutput directory: "
           << directory.filename().string() << "\nFiles: " << inputs.size()
           << '\n';
    return 0;
  } catch (const std::exception &exception) {
    error << "pkmn rjson generate-batch: " << exception.what() << '\n';
    return ToInt(ExitCode::GenerationFailure);
  }
}

void PrintValidation(const red::json::LoadedDocument &document,
                     std::ostream &output) {
  const auto &status = document.validation;
  output
      << "Schema: " << (status.schemaValid ? "valid 0.1.0" : "invalid") << '\n'
      << "Semantics: " << (status.semanticsValid ? "valid" : "invalid") << '\n'
      << "Physical image: "
      << (status.hasPhysicalImage
              ? (status.physicalImageValid ? "present and valid"
                                           : "present but invalid")
              : "absent")
      << '\n'
      << "Semantic generation policy: physicalImage is never generation "
         "authority\n"
      << "Named event support: verified catalog and synchronized views\n"
      << "Deferred support: arbitrary runtime scripts and unverified map-state "
         "editing\n";
  for (const auto &warning : status.warnings)
    output << "Warning: " << warning << '\n';
  for (const auto &error : status.errors)
    output << "Error: " << error << '\n';
}

red::json::OrderedJson
ValidationJson(const red::json::LoadedDocument &document) {
  const auto &status = document.validation;
  red::json::OrderedJson result = {
      {"format", "pkmn-red-json-validation"},
      {"reportVersion", "1.0.0"},
      {"command", "rjson validate"},
      {"valid", status.Valid()},
      {"syntaxValid", status.syntaxValid},
      {"schemaValid", status.schemaValid},
      {"semanticsValid", status.semanticsValid},
      {"physicalImagePresent", status.hasPhysicalImage},
      {"physicalImageValid",
       status.hasPhysicalImage ? red::json::OrderedJson(status.physicalImageValid)
                               : red::json::OrderedJson(nullptr)},
      {"semanticGenerationUsesPhysicalImage", false},
      {"warnings", status.warnings},
      {"errors", status.errors}};
  if (document.root.contains("schema"))
    result["schemaVersion"] =
        document.root.at("schema").value("schemaVersion", "");
  if (document.root.contains("decoded") && status.semanticsValid) {
    const auto &decoded = document.root.at("decoded");
    result["summary"] = {
        {"trainer", decoded.at("trainer").at("name").at("value")},
        {"partyCount", decoded.at("party").at("count")},
        {"pcBoxCount", decoded.at("pcStorage").at("boxes").size()}};
  }
  return result;
}

} // namespace

int Run(const std::vector<std::string> &arguments, std::ostream &output,
        std::ostream &error) {
  if (arguments.empty() || arguments.front() == "help" ||
      arguments.front() == "--help" || arguments.front() == "-h") {
    Help(output);
    return ToInt(ExitCode::Success);
  }
  if (arguments.front() == "migrate")
    return RunMigration(arguments, output, error);
  if (arguments.front() == "schema")
    return RunSchema(arguments, output, error);
  if (arguments.front() == "generate-batch")
    return RunGenerateBatch(arguments, output, error);
  if ((arguments.front() == "inspect" || arguments.front() == "validate") &&
      arguments.size() >= 2) {
    bool json = false;
    std::string profile = "standard";
    for (std::size_t index = 2; index < arguments.size(); ++index) {
      if (arguments[index] == "--format" && index + 1 < arguments.size() &&
          arguments[index + 1] == "json") {
        json = true;
        ++index;
      } else if (arguments[index] == "--profile" &&
                 index + 1 < arguments.size()) {
        profile = arguments[++index];
      } else {
        error << "pkmn rjson " << arguments.front() << ": invalid option\n";
        return ToInt(ExitCode::InvalidArguments);
      }
    }
    if (profile != "standard" && profile != "strict" &&
        profile != "generation" && profile != "archival") {
      error << "pkmn rjson: unknown validation profile\n";
      return ToInt(ExitCode::InvalidArguments);
    }
    const auto document = arguments[1] == "-"
                              ? red::json::LoadAndValidate(std::cin)
                              : red::json::LoadAndValidate(arguments[1]);
    bool profileValid = document.validation.Valid();
    std::string profileError;
    if (profile == "archival" && !document.validation.hasPhysicalImage) {
      profileValid = false;
      profileError = "archival profile requires physicalImage";
    }
    if (profile == "strict" &&
        (!document.root.contains("decoded") ||
         !document.root.at("decoded").contains("events"))) {
      profileValid = false;
      profileError = "strict profile requires the verified named-event catalog";
    }
    if (profile == "generation" && profileValid) {
      try {
        (void)red::generation::Generate(document.root);
      } catch (const std::exception &exception) {
        profileValid = false;
        profileError = std::string("generation profile rejected the document: ") +
                       exception.what();
      }
    }
    if (json) {
      auto report = ValidationJson(document);
      report["profile"] = profile;
      report["profileValid"] = profileValid;
      if (!profileError.empty()) report["profileError"] = profileError;
      output << report.dump(2) << '\n';
    }
    else
      PrintValidation(document, output);
    if (!json)
      output << "Validation profile: " << profile << " ("
             << (profileValid ? "passed" : "failed") << ")\n";
    if (!json && arguments.front() == "inspect" &&
        document.validation.semanticsValid) {
      if (document.root.contains("decoded")) {
        const auto &decoded = document.root.at("decoded");
        output
            << "Trainer: "
            << decoded.at("trainer").at("name").at("value").get<std::string>()
            << '\n'
            << "Party: " << decoded.at("party").at("count") << '\n'
            << "PC boxes: " << decoded.at("pcStorage").at("boxes").size()
            << '\n';
      }
    }
    return profileValid ? ToInt(ExitCode::Success)
                        : ToInt(ExitCode::InvalidInput);
  }
  if (arguments.front() == "generate" && arguments.size() >= 2) {
    auto outputPath = DefaultGeneration(arguments[1]);
    bool outputSeen = false;
    bool autoSuffix = false;
    for (std::size_t index = 2; index < arguments.size(); ++index) {
      if (arguments[index] == "--auto-suffix")
        autoSuffix = true;
      else if (!outputSeen && !arguments[index].starts_with("--")) {
        outputPath = arguments[index];
        outputSeen = true;
      } else {
        error << "pkmn rjson generate: invalid arguments\n";
        return ToInt(ExitCode::InvalidArguments);
      }
    }
    const bool stdoutOutput = outputPath == std::filesystem::path("-");
    const auto preferredOutputPath = outputPath;
    auto available = [](const std::filesystem::path &path) {
      return !std::filesystem::exists(path) &&
             !std::filesystem::exists(path.string() +
                                      ".generation-report.json") &&
             !std::filesystem::exists(path.string() +
                                      ".generation-report.md");
    };
    if (!stdoutOutput && autoSuffix && !available(outputPath)) {
      for (std::size_t number = 2;; ++number) {
        outputPath = util::NumberedOutputPath(preferredOutputPath, number);
        if (available(outputPath))
          break;
      }
    }
    auto jsonReport = outputPath;
    jsonReport += ".generation-report.json";
    auto markdownReport = outputPath;
    markdownReport += ".generation-report.md";
    for (const auto &path : {outputPath, jsonReport, markdownReport}) {
      if (stdoutOutput) break;
      if (std::filesystem::exists(path)) {
        error << "pkmn rjson generate: refusing to overwrite output/report: "
              << path.filename().string() << '\n';
        return ToInt(ExitCode::OutputFailure);
      }
    }
    try {
      red::json::OrderedJson document;
      if (arguments[1] == "-")
        std::cin >> document;
      else {
        std::ifstream input(arguments[1]);
        if (!input)
          throw std::runtime_error("could not open Red JSON input");
        input >> document;
      }
      const auto result = red::generation::Generate(document);
      if (stdoutOutput) {
        output.write(reinterpret_cast<const char *>(result.bytes.data()),
                     static_cast<std::streamsize>(result.bytes.size()));
        return output ? ToInt(ExitCode::Success)
                      : ToInt(ExitCode::OutputFailure);
      }
      util::OutputTransaction transaction;
      transaction.StageBinary(outputPath, result.bytes);
      transaction.StageText(jsonReport, result.report.dump(2) + '\n');
      transaction.StageText(markdownReport, result.markdownReport);
      transaction.Commit();
      output << "Semantic generation complete (physicalImage ignored)\n"
             << "Output: " << outputPath.filename().string() << '\n'
             << "Integrity: valid\n";
      return ToInt(ExitCode::Success);
    } catch (const std::exception &exception) {
      error << "pkmn rjson generate: " << exception.what() << '\n';
      return ToInt(ExitCode::GenerationFailure);
    }
  }
  if (arguments.front() == "reconstruct" && arguments.size() >= 2) {
    auto outputPath = DefaultReconstruction(arguments[1]);
    bool autoSuffix = false;
    for (std::size_t index = 2; index < arguments.size(); ++index) {
      if (arguments[index] == "--output" && index + 1 < arguments.size())
        outputPath = arguments[++index];
      else if (arguments[index] == "--auto-suffix")
        autoSuffix = true;
      else {
        error << "pkmn rjson reconstruct: invalid arguments\n";
        return ToInt(ExitCode::InvalidArguments);
      }
    }
    const bool stdoutOutput = outputPath == std::filesystem::path("-");
    const auto preferredOutputPath = outputPath;
    if (!stdoutOutput && autoSuffix && std::filesystem::exists(outputPath)) {
      for (std::size_t number = 2;; ++number) {
        outputPath = util::NumberedOutputPath(preferredOutputPath, number);
        if (!std::filesystem::exists(outputPath))
          break;
      }
    }
    if (!stdoutOutput && std::filesystem::exists(outputPath)) {
      error << "pkmn rjson reconstruct: refusing to overwrite existing output: "
            << outputPath.filename().string() << '\n';
      return ToInt(ExitCode::OutputFailure);
    }
    const auto document = arguments[1] == "-"
                              ? red::json::LoadAndValidate(std::cin)
                              : red::json::LoadAndValidate(arguments[1]);
    if (!document.validation.Valid()) {
      PrintValidation(document, error);
      return ToInt(ExitCode::InvalidInput);
    }
    if (!document.validation.hasPhysicalImage) {
      error << "pkmn rjson reconstruct: physicalImage is required for archival "
               "reconstruction\n";
      return ToInt(ExitCode::InvalidInput);
    }
    try {
      const auto bytes = red::json::PhysicalBytes(document.root);
      const auto expected = document.root.at("source")
                                .at("hashes")
                                .at("wholeFileSha256")
                                .get<std::string>();
      if (util::Sha256Hex(bytes) != expected)
        throw std::runtime_error("reconstructed SHA-256 mismatch");
      if (stdoutOutput) {
        output.write(reinterpret_cast<const char *>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        return output ? ToInt(ExitCode::Success)
                      : ToInt(ExitCode::OutputFailure);
      }
      util::WriteBinaryAtomic(outputPath, bytes);
      output << "Archival reconstruction complete (physicalImage authority)\n"
             << "Output: " << outputPath.filename().string() << '\n';
      return ToInt(ExitCode::Success);
    } catch (const std::exception &exception) {
      error << "pkmn rjson reconstruct: " << exception.what() << '\n';
      return ToInt(ExitCode::OutputFailure);
    }
  }
  error << "pkmn rjson: invalid or unsupported arguments\n";
  Help(error);
  return ToInt(ExitCode::InvalidArguments);
}

} // namespace pkmn::cli::commands::rjson
