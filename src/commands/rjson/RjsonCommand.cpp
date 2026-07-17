#include "commands/rjson/RjsonCommand.hpp"

#include <filesystem>
#include <fstream>
#include <ostream>

#include "app/ExitCode.hpp"
#include "red/generation/SemanticGenerator.hpp"
#include "red/json/RedJsonDocument.hpp"
#include "util/Sha256.hpp"
#include "util/OutputPath.hpp"

namespace pkmn::cli::commands::rjson {
namespace {

void Help(std::ostream &output) {
  output
      << "Usage:\n"
      << "  pkmn rjson inspect <file.red.json> [--format json]\n"
      << "  pkmn rjson validate <file.red.json> [--format json]\n"
      << "  pkmn rjson generate <file.red.json> [output.sav] [--auto-suffix]\n"
      << "  pkmn rjson reconstruct <file.red.json> [--output <output.sav>] "
         "[--auto-suffix]\n";
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
      << "Deferred support: named event/story/trainer/static classifications\n";
  for (const auto &warning : status.warnings)
    output << "Warning: " << warning << '\n';
  for (const auto &error : status.errors)
    output << "Error: " << error << '\n';
}

red::json::OrderedJson
ValidationJson(const red::json::LoadedDocument &document) {
  const auto &status = document.validation;
  red::json::OrderedJson result = {
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
  if ((arguments.front() == "inspect" || arguments.front() == "validate") &&
      (arguments.size() == 2 ||
       (arguments.size() == 4 && arguments[2] == "--format" &&
        arguments[3] == "json"))) {
    const auto document = red::json::LoadAndValidate(arguments[1]);
    const bool json = arguments.size() == 4;
    if (json)
      output << ValidationJson(document).dump(2) << '\n';
    else
      PrintValidation(document, output);
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
    return document.validation.Valid() ? ToInt(ExitCode::Success)
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
    const auto preferredOutputPath = outputPath;
    auto available = [](const std::filesystem::path &path) {
      return !std::filesystem::exists(path) &&
             !std::filesystem::exists(path.string() +
                                      ".generation-report.json") &&
             !std::filesystem::exists(path.string() +
                                      ".generation-report.md");
    };
    if (autoSuffix && !available(outputPath)) {
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
      if (std::filesystem::exists(path)) {
        error << "pkmn rjson generate: refusing to overwrite output/report: "
              << path.filename().string() << '\n';
        return ToInt(ExitCode::OutputFailure);
      }
    }
    try {
      std::ifstream input(arguments[1]);
      if (!input)
        throw std::runtime_error("could not open Red JSON input");
      red::json::OrderedJson document;
      input >> document;
      const auto result = red::generation::Generate(document);
      std::ofstream save(outputPath, std::ios::binary);
      save.write(reinterpret_cast<const char *>(result.bytes.data()),
                 static_cast<std::streamsize>(result.bytes.size()));
      std::ofstream(jsonReport) << result.report.dump(2) << '\n';
      std::ofstream(markdownReport) << result.markdownReport;
      if (!save)
        throw std::runtime_error("could not write complete generated save");
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
    const auto preferredOutputPath = outputPath;
    if (autoSuffix && std::filesystem::exists(outputPath)) {
      for (std::size_t number = 2;; ++number) {
        outputPath = util::NumberedOutputPath(preferredOutputPath, number);
        if (!std::filesystem::exists(outputPath))
          break;
      }
    }
    if (std::filesystem::exists(outputPath)) {
      error << "pkmn rjson reconstruct: refusing to overwrite existing output: "
            << outputPath.filename().string() << '\n';
      return ToInt(ExitCode::OutputFailure);
    }
    const auto document = red::json::LoadAndValidate(arguments[1]);
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
      std::ofstream file(outputPath, std::ios::binary | std::ios::trunc);
      file.write(reinterpret_cast<const char *>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
      if (!file)
        throw std::runtime_error("could not write complete reconstructed save");
      const auto expected = document.root.at("source")
                                .at("hashes")
                                .at("wholeFileSha256")
                                .get<std::string>();
      if (util::Sha256Hex(bytes) != expected)
        throw std::runtime_error("reconstructed SHA-256 mismatch");
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
