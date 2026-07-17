#include "commands/proof/ProofCommand.hpp"

#include <filesystem>
#include <fstream>
#include <map>
#include <ostream>
#include <stdexcept>

#include "app/ExitCode.hpp"
#include "app/Version.hpp"
#include "red/comparison/Comparison.hpp"
#include "red/generation/SemanticGenerator.hpp"
#include "red/json/RedDecoder.hpp"
#include "red/json/RedJsonDocument.hpp"
#include "red/proof/PostEmulator.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/Sha256.hpp"
#include "util/OutputPath.hpp"
#include "util/ZipWriter.hpp"
#include "util/AtomicOutput.hpp"

namespace pkmn::cli::commands::proof {
namespace {
using Json = red::json::OrderedJson;
void Text(const std::filesystem::path &path, const std::string &value) {
  std::ofstream f(path, std::ios::binary);
  f << value;
  if (!f)
    throw std::runtime_error("could not write proof report");
}
void Binary(const std::filesystem::path &path,
            const red::save::RedSave::Bytes &value) {
  std::ofstream f(path, std::ios::binary);
  f.write(reinterpret_cast<const char *>(value.data()),
          static_cast<std::streamsize>(value.size()));
  if (!f)
    throw std::runtime_error("could not write proof save");
}
std::filesystem::path DefaultDirectory(const std::filesystem::path &source) {
  return (source.has_parent_path() ? source.parent_path()
                                   : std::filesystem::path(".")) /
         (source.stem().string() + ".pkmn-proof");
}

std::filesystem::path
DefaultPostEmulatorDirectory(const std::filesystem::path &before) {
  return (before.has_parent_path() ? before.parent_path()
                                   : std::filesystem::path(".")) /
         (before.stem().string() + ".post-emulator-validation");
}

std::string BuildPlatform() {
#if defined(__APPLE__)
  return "macos";
#elif defined(_WIN32)
  return "windows";
#elif defined(__linux__)
  return "linux";
#else
  return "unknown";
#endif
}

int VerifyPackage(const std::vector<std::string> &args, std::ostream &output,
                  std::ostream &error) {
  if (args.empty() || args.size() > 3 ||
      (args.size() == 3 &&
       (args[1] != "--format" || args[2] != "json"))) {
    error << "pkmn proof verify: expected <proof-directory|proof.zip> "
             "[--format json]\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  const std::filesystem::path package = args[0];
  const bool jsonOutput = args.size() == 3;
  try {
    const bool zip = std::filesystem::is_regular_file(package) &&
                     package.extension() == ".zip";
    const auto entries = zip ? util::ReadDeterministicZip(package)
                             : util::ReadProofDirectoryEntries(package);
    std::map<std::string, std::vector<std::uint8_t>> files;
    for (const auto &[name, bytes] : entries) {
      if (!files.emplace(name, bytes).second)
        throw std::runtime_error("duplicate proof artifact: " + name);
    }
    if (!files.contains("proof-manifest.json"))
      throw std::runtime_error("proof-manifest.json is missing");
    const auto &manifestBytes = files.at("proof-manifest.json");
    const Json manifest = Json::parse(manifestBytes.begin(), manifestBytes.end());
    if (manifest.value("tool", "") != "pkmn" ||
        !manifest.contains("artifactSha256") ||
        !manifest.at("artifactSha256").is_object())
      throw std::runtime_error("unsupported or incomplete proof manifest");
    std::size_t verified = 0;
    for (const auto &[name, expectedValue] :
         manifest.at("artifactSha256").items()) {
      if (!files.contains(name))
        throw std::runtime_error("manifest artifact is missing: " + name);
      const auto expected = expectedValue.get<std::string>();
      if (util::Sha256Hex(files.at(name)) != expected)
        throw std::runtime_error("artifact SHA-256 mismatch: " + name);
      ++verified;
    }
    for (const auto &[name, bytes] : files) {
      (void)bytes;
      if (name != "proof-manifest.json" &&
          !manifest.at("artifactSha256").contains(name))
        throw std::runtime_error("unhashed artifact is present: " + name);
    }
    if (!files.contains("generated.sav"))
      throw std::runtime_error("generated.sav is missing");
    const red::save::RedSave generated(files.at("generated.sav"));
    if (!red::validation::SaveValidator::Validate(generated).Valid())
      throw std::runtime_error("generated.sav checksum validation failed");
    if (manifest.contains("generatedSha256") &&
        util::Sha256Hex(generated.BytesView()) !=
            manifest.at("generatedSha256").get<std::string>())
      throw std::runtime_error("generated save does not match manifest hash");
    for (const auto *name : {"original.red.json", "generated.red.json"}) {
      if (!files.contains(name))
        throw std::runtime_error(std::string(name) + " is missing");
      const auto document = Json::parse(files.at(name).begin(), files.at(name).end());
      if (!red::json::ValidateDocument(document).Valid())
        throw std::runtime_error(std::string(name) + " failed schema validation");
    }
    const Json report = {{"valid", true},
                         {"packageType", zip ? "deterministic-zip" : "directory"},
                         {"artifactCount", files.size()},
                         {"artifactHashesVerified", verified},
                         {"generatedChecksumsValid", true},
                         {"manifestToolVersion", manifest.value("toolVersion", "unknown")}};
    if (jsonOutput)
      output << report.dump(2) << '\n';
    else
      output << "Proof package verification: passed\nPackage: "
             << package.filename().string() << "\nArtifacts hashed: "
             << verified << "\nGenerated save checksums: valid\n";
    return 0;
  } catch (const std::exception &exception) {
    error << "pkmn proof verify: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}
} // namespace

int RunPostEmulator(const std::vector<std::string> &args,
                    std::ostream &output, std::ostream &error) {
  std::filesystem::path before;
  std::filesystem::path after;
  std::filesystem::path directory;
  bool updateProof = false;
  for (std::size_t index = 0; index < args.size(); ++index) {
    if (args[index] == "--before" && index + 1 < args.size())
      before = args[++index];
    else if (args[index] == "--after" && index + 1 < args.size())
      after = args[++index];
    else if (args[index] == "--output-dir" && index + 1 < args.size())
      directory = args[++index];
    else if (args[index] == "--proof-dir" && index + 1 < args.size()) {
      directory = args[++index];
      updateProof = true;
    } else {
      error << "pkmn proof post-emulator: invalid arguments\n";
      return ToInt(ExitCode::InvalidArguments);
    }
  }
  if (before.empty() || after.empty()) {
    error << "pkmn proof post-emulator: --before and --after are required\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  if (directory.empty())
    directory = DefaultPostEmulatorDirectory(before);
  const auto jsonPath = directory / "post-emulator-validation.json";
  const auto markdownPath = directory / "post-emulator-validation.md";
  if ((!updateProof && std::filesystem::exists(directory)) ||
      std::filesystem::exists(jsonPath) ||
      std::filesystem::exists(markdownPath)) {
    error << "pkmn proof post-emulator: refusing existing output\n";
    return ToInt(ExitCode::OutputFailure);
  }
  if (updateProof &&
      !std::filesystem::is_regular_file(directory / "proof-manifest.json")) {
    error << "pkmn proof post-emulator: --proof-dir must identify an existing "
             "proof package\n";
    return ToInt(ExitCode::InvalidInput);
  }
  try {
    const auto result = red::proof::ValidatePostEmulator(before, after);
    const auto jsonText = result.report.dump(2) + "\n";
    const auto markdownText = result.markdown;
    if (!updateProof) {
      util::DirectoryTransaction transaction(directory);
      Text(transaction.Path() / jsonPath.filename(), jsonText);
      Text(transaction.Path() / markdownPath.filename(), markdownText);
      transaction.Commit();
    } else {
      util::OutputTransaction transaction;
      transaction.StageText(jsonPath, jsonText);
      transaction.StageText(markdownPath, markdownText);
      transaction.Commit();
    }
    if (updateProof) {
      Json manifest;
      {
        std::ifstream input(directory / "proof-manifest.json");
        input >> manifest;
      }
      manifest["emulatorValidation"] = result.passed ? "passed" : "failed";
      manifest["postEmulatorValidation"] = {
          {"report", "post-emulator-validation.json"},
          {"afterSha256", result.report.at("after").at("sha256")},
          {"passed", result.passed}};
      manifest["artifactSha256"]["post-emulator-validation.json"] =
          util::Sha256Hex(std::vector<std::uint8_t>(jsonText.begin(), jsonText.end()));
      manifest["artifactSha256"]["post-emulator-validation.md"] =
          util::Sha256Hex(std::vector<std::uint8_t>(markdownText.begin(), markdownText.end()));
      for (const auto *name : {"post-emulator-validation.json",
                               "post-emulator-validation.md"}) {
        bool found = false;
        for (const auto &artifact : manifest.at("artifacts"))
          found = found || artifact == name;
        if (!found)
          manifest["artifacts"].push_back(name);
      }
      util::ReplaceTextAtomic(directory / "proof-manifest.json",
                              manifest.dump(2) + "\n");
    }
    output << "Post-emulator validation: "
           << (result.passed ? "passed" : "failed")
           << "\nOutput: " << directory.filename().string()
           << "\nSource files modified: no\n";
    return result.passed ? 0
                         : ToInt(ExitCode::PostEmulatorValidationFailure);
  } catch (const std::exception &exception) {
    error << "pkmn proof post-emulator: " << exception.what() << '\n';
    return ToInt(ExitCode::PostEmulatorValidationFailure);
  }
}

int Run(const std::vector<std::string> &args, std::ostream &output,
        std::ostream &error) {
  if (args.empty() || args[0] == "help" || args[0] == "--help") {
    output << "Usage:\n"
              "  pkmn proof red <source.sav> [--output-dir <directory>] "
              "[--zip|--zip-output <archive.zip>] [--auto-suffix]\n"
              "  pkmn proof post-emulator --before <save.sav> --after "
              "<save.sav> [--output-dir <directory>|--proof-dir <directory>]\n"
              "  pkmn proof verify <proof-directory|proof.zip> [--format json]\n";
    return 0;
  }
  if (args[0] == "post-emulator")
    return RunPostEmulator({args.begin() + 1, args.end()}, output, error);
  if (args[0] == "verify")
    return VerifyPackage({args.begin() + 1, args.end()}, output, error);
  if (args.size() < 2 || args[0] != "red") {
    error << "pkmn proof: expected 'red <source.sav>'\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  std::filesystem::path source = args[1];
  auto directory = DefaultDirectory(source);
  std::filesystem::path zipPath;
  bool createZip = false;
  bool autoSuffix = false;
  bool explicitZipPath = false;
  for (std::size_t index = 2; index < args.size(); ++index) {
    if (args[index] == "--output-dir" && index + 1 < args.size())
      directory = args[++index];
    else if (args[index] == "--zip")
      createZip = true;
    else if (args[index] == "--zip-output" && index + 1 < args.size()) {
      createZip = true;
      explicitZipPath = true;
      zipPath = args[++index];
    } else if (args[index] == "--auto-suffix") {
      autoSuffix = true;
    } else {
      error << "pkmn proof red: invalid arguments\n";
      return ToInt(ExitCode::InvalidArguments);
    }
  }
  const auto preferredDirectory = directory;
  if (autoSuffix && std::filesystem::exists(directory)) {
    for (std::size_t number = 2;; ++number) {
      directory = util::NumberedOutputPath(preferredDirectory, number);
      if (!std::filesystem::exists(directory))
        break;
    }
  }
  if (zipPath.empty()) {
    zipPath = directory;
    zipPath += ".zip";
  }
  if (autoSuffix && createZip && !explicitZipPath &&
      std::filesystem::exists(zipPath)) {
    for (std::size_t number = 2;; ++number) {
      const auto candidate = util::NumberedOutputPath(preferredDirectory, number);
      auto candidateZip = candidate;
      candidateZip += ".zip";
      if (!std::filesystem::exists(candidate) &&
          !std::filesystem::exists(candidateZip)) {
        directory = candidate;
        zipPath = candidateZip;
        break;
      }
    }
  }
  if (autoSuffix && createZip && explicitZipPath &&
      std::filesystem::exists(zipPath)) {
    const auto preferredZipPath = zipPath;
    for (std::size_t number = 2;; ++number) {
      zipPath = util::NumberedOutputPath(preferredZipPath, number);
      if (!std::filesystem::exists(zipPath))
        break;
    }
  }
  if (std::filesystem::exists(directory)) {
    error << "pkmn proof red: refusing existing proof directory\n";
    return ToInt(ExitCode::OutputFailure);
  }
  if (createZip && std::filesystem::exists(zipPath)) {
    error << "pkmn proof red: refusing existing proof ZIP\n";
    return ToInt(ExitCode::OutputFailure);
  }
  try {
    const auto finalDirectory = directory;
    util::DirectoryTransaction directoryTransaction(finalDirectory);
    directory = directoryTransaction.Path();
    const auto original = red::save::RedSave::Read(source);
    const auto sourceIntegrity =
        red::validation::SaveValidator::Validate(original);
    if (!sourceIntegrity.Valid())
      throw std::runtime_error("source save failed integrity validation");
    const auto decoded = red::json::Decode(original, source.filename().string(),
                                           sourceIntegrity, {true});
    const auto generated = red::generation::Generate(decoded);
    const auto generatedAgain = red::generation::Generate(decoded);
    auto changedPhysical = decoded;
    changedPhysical["physicalImage"] = "isolation mutation";
    const auto isolatedGeneration = red::generation::Generate(changedPhysical);
    const bool deterministic = generated.bytes == generatedAgain.bytes;
    const bool isolated = generated.bytes == isolatedGeneration.bytes;
    if (!deterministic || !isolated)
      throw std::runtime_error(
          "determinism or physical-image isolation proof failed");
    const red::save::RedSave generatedSave(generated.bytes);
    const auto generatedIntegrity =
        red::validation::SaveValidator::Validate(generatedSave);
    const auto redecoded = red::json::Decode(generatedSave, "generated.sav",
                                             generatedIntegrity, {true});
    const auto physical =
        red::comparison::ComparePhysical(original.BytesView(), generated.bytes);
    const auto semantic = red::comparison::CompareSemantic(decoded, redecoded);
    Text(directory / "original.red.json", red::json::Serialize(decoded));
    Binary(directory / "generated.sav", generated.bytes);
    Text(directory / "generated.red.json", red::json::Serialize(redecoded));
    Text(directory / "physical-comparison.json", physical.dump(2) + "\n");
    Text(directory / "physical-comparison.md",
         red::comparison::PhysicalMarkdown(physical));
    Text(directory / "semantic-comparison.json", semantic.dump(2) + "\n");
    Text(directory / "semantic-comparison.md",
         red::comparison::SemanticMarkdown(semantic));
    Json combined = {{"physical", physical}, {"semantic", semantic}};
    Text(directory / "comparison.json", combined.dump(2) + "\n");
    Text(directory / "comparison.md",
         "# Pokemon Red Proof Comparison\n\n" +
             red::comparison::PhysicalMarkdown(physical) + "\n" +
             red::comparison::SemanticMarkdown(semantic));
    Json summary = {
        {"sourceLogicalName", source.filename().string()},
        {"physicalIdentical", physical.at("identical")},
        {"physicalEqualPercent", physical.at("equalPercent")},
        {"semanticEquivalentUnderPolicy", semantic.at("equivalent")},
        {"unexpectedSemanticMismatches",
         semantic.at("unexpectedMismatchCount")},
        {"generatedIntegrityValid", generatedIntegrity.Valid()},
        {"deterministic", deterministic},
        {"physicalImageIsolation", isolated},
        {"emulatorValidation", "required-manual-gate"}};
    Text(directory / "summary-comparison.json", summary.dump(2) + "\n");
    Text(directory / "summary-comparison.md",
         "# Proof Summary\n\n- Physical identity: " +
             std::string(physical.at("identical").get<bool>() ? "yes" : "no") +
             "\n- Physical equal percent: " +
             std::to_string(physical.at("equalPercent").get<double>()) +
             "\n- Semantic equivalence under policy: " +
             (semantic.at("equivalent").get<bool>() ? "yes" : "no") +
             "\n- Generated integrity: valid\n- Determinism: passed\n- "
             "Physical-image isolation: passed\n- Emulator validation: "
             "required manual gate\n");
    Text(directory / "semantic-json-comparison.json",
         semantic.dump(2) + "\n");
    Text(directory / "semantic-json-comparison.md",
         red::comparison::SemanticMarkdown(semantic));
    Json archival = {
        {"policy", "archival physical images are compared but never used for semantic generation"},
        {"originalPhysicalImagePresent", decoded.contains("physicalImage")},
        {"generatedPhysicalImagePresent", redecoded.contains("physicalImage")},
        {"originalSha256", util::Sha256Hex(original.BytesView())},
        {"generatedSha256", util::Sha256Hex(generated.bytes)},
        {"physicalImagesIdentical", physical.at("identical")},
        {"semanticGenerationUsedPhysicalImage", false}};
    Text(directory / "archival-json-comparison.json",
         archival.dump(2) + "\n");
    Text(directory / "archival-json-comparison.md",
         "# Archival JSON Comparison\n\n- Original physical image: present\n"
         "- Generated physical image: present\n- Physical images identical: " +
             std::string(physical.at("identical").get<bool>() ? "yes" : "no") +
             "\n- Semantic generation used physicalImage: no\n");
    Text(directory / "generation-report.json", generated.report.dump(2) + "\n");
    Text(directory / "generation-report.md", generated.markdownReport);
    Text(directory / "emulator-checklist.md",
         "# Emulator Checklist\n\n- [ ] Continue screen renders normally\n- [ "
         "] Continue loads without corruption\n- [ ] Player appears in Red's "
         "house second floor\n- [ ] Movement, menus, party, and PC work\n- [ ] "
         "Save normally, fully shut down, and reload\n- [ ] Return the "
         "post-emulator save for internal validation\n\nAutomated proof does "
         "not replace this manual gate.\n");
    Json artifactHashes = Json::object();
    for (const auto &[name, bytes] : util::ReadProofDirectoryEntries(directory))
      artifactHashes[name] = util::Sha256Hex(bytes);
    Json manifest = {
        {"tool", "pkmn"},
        {"toolVersion", std::string(kVersion)},
        {"buildPlatform", BuildPlatform()},
        {"sourceLogicalName", source.filename().string()},
        {"sourceSha256", util::Sha256Hex(original.BytesView())},
        {"generatedSha256", util::Sha256Hex(generated.bytes)},
        {"deterministic", deterministic},
        {"physicalImageIsolation", isolated},
        {"generatedIntegrityValid", generatedIntegrity.Valid()},
        {"semanticEquivalentUnderPolicy", semantic.at("equivalent")},
        {"emulatorValidation", "required-manual-gate"},
        {"artifacts",
         Json::array({"original.red.json", "generated.sav",
                      "generated.red.json", "comparison.md",
                      "comparison.json", "physical-comparison.md",
                      "physical-comparison.json", "semantic-comparison.md",
                      "semantic-comparison.json", "generation-report.md",
                      "generation-report.json", "summary-comparison.md",
                      "summary-comparison.json",
                      "semantic-json-comparison.md",
                      "semantic-json-comparison.json",
                      "archival-json-comparison.md",
                      "archival-json-comparison.json",
                      "emulator-checklist.md"})},
        {"artifactSha256", artifactHashes},
        {"supportedSemanticAreas",
         Json::array({"trainer-core", "pokedex", "inventory", "party",
                      "pc-storage", "current-box-cache", "daycare",
                      "hall-of-fame", "named-events", "raw-world-state"})},
        {"deferredSemanticAreas",
         Json::array({"arbitrary-runtime-location", "unverified-scripts",
                      "firered", "red-to-firered-conversion"})},
        {"privacy",
         {{"containsSaveData", true},
          {"containsRom", false},
          {"containsAbsolutePaths", false},
          {"publicationReviewRequired", true}}}};
    Text(directory / "proof-manifest.json", manifest.dump(2) + "\n");
    directoryTransaction.Commit();
    directory = finalDirectory;
    if (createZip)
      util::WriteDeterministicZip(
          zipPath, util::ReadProofDirectoryEntries(directory));
    output << "Pokemon Red proof package created\nOutput: "
           << directory.filename().string()
           << "\nDeterminism: passed\nPhysical-image isolation: "
              "passed\nEmulator validation: required\n";
    if (createZip)
      output << "ZIP: " << zipPath.filename().string()
             << " (contains save data; publication review required)\n";
    return semantic.at("equivalent").get<bool>()
               ? 0
               : ToInt(ExitCode::SemanticMismatch);
  } catch (const std::exception &e) {
    error << "pkmn proof red: " << e.what() << '\n';
    return ToInt(ExitCode::GeneralFailure);
  }
}
} // namespace pkmn::cli::commands::proof
