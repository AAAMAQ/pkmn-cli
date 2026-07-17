#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "app/CommandRouter.hpp"
#include "app/ExitCode.hpp"
#include "red/json/RedDecoder.hpp"
#include "red/generation/SemanticGenerator.hpp"
#include "red/data/Gen1Names.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/Sha256.hpp"

namespace {

int failures = 0;

void Expect(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

struct Result {
  int code;
  std::string output;
  std::string error;
};

Result Run(std::vector<std::string> arguments) {
  std::ostringstream output;
  std::ostringstream error;
  const int code = pkmn::cli::CommandRouter{}.Run(arguments, output, error);
  return {code, output.str(), error.str()};
}

std::uint8_t InvertedSum(const std::vector<std::uint8_t> &bytes,
                         std::size_t start, std::size_t endInclusive) {
  std::uint32_t sum = 0;
  for (std::size_t index = start; index <= endInclusive; ++index)
    sum += bytes[index];
  return static_cast<std::uint8_t>(~static_cast<std::uint8_t>(sum & 0xFFU));
}

std::vector<std::uint8_t> ValidSyntheticSave() {
  using Validator = pkmn::cli::red::validation::SaveValidator;
  std::vector<std::uint8_t> bytes(pkmn::cli::red::save::RedSave::ExpectedSize,
                                  0);
  bytes[0x2598] = 0x50;
  bytes[0x25F6] = 0x50;
  bytes[Validator::MainStored] =
      InvertedSum(bytes, Validator::MainStart, Validator::MainEnd);
  for (std::size_t bank = 0; bank < 2; ++bank) {
    bytes[Validator::BankStored[bank]] = InvertedSum(
        bytes, Validator::BankStarts[bank], Validator::BankStored[bank] - 1);
  }
  for (std::size_t box = 0; box < 12; ++box) {
    const std::size_t bank = box / 6;
    const std::size_t withinBank = box % 6;
    const std::size_t start = Validator::BoxOffset(box);
    bytes[Validator::BoxChecksumTables[bank] + withinBank] =
        InvertedSum(bytes, start, start + Validator::BoxBlockSize - 1);
  }
  return bytes;
}

} // namespace

int main() {
  const auto noArguments = Run({});
  Expect(noArguments.code == 0, "no arguments should show help successfully");
  Expect(noArguments.output.find("Usage:") != std::string::npos,
         "help should contain Usage");

  const auto help = Run({"--help"});
  Expect(help.code == 0, "--help should succeed");
  Expect(help.output.find("physicalImage") != std::string::npos,
         "help should state the physicalImage safety boundary");
  Expect(help.output.find("red undo-edit") != std::string::npos &&
             help.output.find("compare progress") != std::string::npos,
         "top-level help should expose every available command endpoint");

  const auto commandCatalog =
      Run({"get-all-cmds", "--format", "json"});
  Expect(commandCatalog.code == 0 &&
             commandCatalog.output ==
                 Run({"get-all-cmds", "--format", "json"}).output &&
             nlohmann::ordered_json::parse(commandCatalog.output)
                     .at("commandCount") == 38 &&
             nlohmann::ordered_json::parse(commandCatalog.output)
                     .at("commands").at(0).contains("usage"),
         "get-all-cmds should expose the complete compiled command catalog");

  const auto version = Run({"--version"});
  Expect(version.code == 0, "--version should succeed");
  Expect(version.output == "pkmn 0.1.0\n", "version output should be stable");
  Expect(Run({"--quiet", "--version"}).output.empty() &&
             Run({"--verbose", "--no-color", "--version"})
                     .output.find("standalone=true") != std::string::npos &&
             Run({"--quiet", "--verbose", "--version"}).code ==
                 pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidArguments),
         "global quiet, verbose, and no-color controls should be predictable");
  Expect(Run({"completion", "bash"}).output.find("complete -F") !=
                 std::string::npos &&
             Run({"completion", "zsh"}).output.find("#compdef pkmn") !=
                 std::string::npos &&
             Run({"completion", "fish"}).output.find("complete -c pkmn") !=
                 std::string::npos,
         "completion should generate bash, zsh, and fish scripts");
  const auto configJson = Run({"config", "show", "--format", "json"});
  Expect(configJson.code == 0 &&
             !nlohmann::ordered_json::parse(configJson.output)
                  .at("externalEngineExecutables")
                  .get<bool>(),
         "config show should expose the compiled standalone safety policy");

  const auto unknown = Run({"wat"});
  Expect(unknown.code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidArguments),
         "unknown command should use invalid-arguments exit code");

  const auto missingDecodeArgument = Run({"red", "decode"});
  Expect(missingDecodeArgument.code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidArguments),
         "red decode should require an input");
  const auto editHelp = Run({"red", "edit", "--help"});
  Expect(editHelp.code == 0 &&
             editHelp.output.find("--party-file") != std::string::npos &&
             editHelp.output.find("--set-file") != std::string::npos,
         "edit subcommand help should describe broad and advanced editing");

  const auto pendingRjson = Run({"rjson", "generate"});
  Expect(pendingRjson.code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidArguments),
         "rjson should reject an unimplemented or incomplete command form");

  const auto future = Run({"fred", "decode", "sample.sav"});
  Expect(future.error.find("emulator-proven") != std::string::npos,
         "FireRed placeholder should state its verification gate");

  const auto doctorHelp = Run({"doctor", "--help"});
  Expect(doctorHelp.code == 0,
         "doctor help should succeed without dependencies");
  Expect(doctorHelp.output.find("No Save Genie or Save Generator") !=
             std::string::npos,
         "doctor help should state that external executables are not required");
  const auto doctor = Run({"doctor"});
  Expect(doctor.code == 0 &&
             doctor.output.find("Standalone readiness: ready") !=
                 std::string::npos,
         "doctor should report standalone readiness without external tools");
  const auto doctorJson = Run({"doctor", "--format", "json"});
  Expect(doctorJson.code == 0 &&
             nlohmann::ordered_json::parse(doctorJson.output)
                     .at("standaloneReadiness") ==
                 "ready",
         "doctor should support machine-readable JSON output");
  const auto deepDoctor = Run({"doctor", "--deep", "--format", "json"});
  Expect(deepDoctor.code == 0 &&
             nlohmann::ordered_json::parse(deepDoctor.output)
                 .at("deepSelfTest").at("deterministic") == true,
         "doctor --deep should run an internal deterministic generation "
         "round trip");

  namespace fs = std::filesystem;
  const fs::path temp =
      fs::temp_directory_path() / "pkmn-cli-red-foundation-tests";
  fs::remove_all(temp);
  fs::create_directories(temp);

  const fs::path validSavePath = temp / "synthetic-valid.sav";
  auto validSave = ValidSyntheticSave();
  {
    std::ofstream saveFile(validSavePath, std::ios::binary);
    saveFile.write(reinterpret_cast<const char *>(validSave.data()),
                   static_cast<std::streamsize>(validSave.size()));
  }
  const pkmn::cli::red::save::RedSave loaded =
      pkmn::cli::red::save::RedSave::Read(validSavePath);
  const auto validation =
      pkmn::cli::red::validation::SaveValidator::Validate(loaded);
  Expect(validation.Valid(),
         "internal Red validator should accept a synthetic valid save");
  Expect(validation.boxes.size() == 12,
         "internal Red validator should check all 12 boxes");
  Expect(pkmn::cli::util::Sha256Hex({}) ==
             "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
         "portable SHA-256 should match the empty-message standard vector");
  Expect(pkmn::cli::red::data::SpeciesName(153) == "BULBASAUR" &&
             pkmn::cli::red::data::MoveName(1) == "POUND" &&
             pkmn::cli::red::data::ItemName(1) == "MASTER BALL" &&
             !pkmn::cli::red::data::MapName(38).empty(),
         "verified Gen I species, move, item, and map names should be bundled");

  const fs::path includedJson = temp / "included.red.json";
  const auto includedDecode =
      Run({"red", "decode", validSavePath.string(), "--output",
           includedJson.string(), "--include-physical-image"});
  Expect(includedDecode.code == 0 && fs::exists(includedJson),
         "red decode should export a requested canonical JSON path");
  nlohmann::ordered_json includedDocument;
  {
    std::ifstream input(includedJson);
    input >> includedDocument;
  }
  Expect(includedDocument.at("schema").at("format") == "pkmn-red-master-save" &&
             includedDocument.at("schema").at("schemaVersion") == "0.1.0",
         "decode should emit canonical schema identity");
  Expect(
      includedDocument.contains("physicalImage") &&
          includedDocument.at("physicalImage")
                  .at("standardSramHex")
                  .get<std::string>()
                  .size() == 0x10000,
      "include-physical-image should preserve the complete 32 KiB SRAM image");
  Expect(
      includedDocument.at("decoded").contains("party") &&
          includedDocument.at("decoded").at("pcStorage").at("boxes").size() ==
              12 &&
          includedDocument.at("decoded").contains("hallOfFame"),
      "decode should contain the required party, storage, and Hall of Fame "
      "sections");
  Expect(includedDocument.at("decoded").at("events").at("flags").size() ==
                 507 &&
             includedDocument.at("decoded")
                 .at("trainerBattles").at("records").is_array() &&
             includedDocument.at("decoded")
                 .at("staticBattles").at("records").is_array() &&
             includedDocument.at("decoded")
                 .at("storyProgress").at("storyFlags").is_array(),
         "decode should expose the complete verified named-event catalog and "
         "classified views");
  const auto readableSummary =
      Run({"red", "summary", validSavePath.string()});
  const auto readableSummaryJson = Run(
      {"red", "summary", validSavePath.string(), "--format", "json"});
  const fs::path summaryPath = temp / "summary.md";
  const auto writtenSummary = Run({"red", "summary", validSavePath.string(),
                                   "--format", "markdown", "--output",
                                   summaryPath.string()});
  const auto collidedSummary = Run({"red", "summary", validSavePath.string(),
                                    "--format", "markdown", "--output",
                                    summaryPath.string()});
  Expect(readableSummary.code == 0 &&
             readableSummary.output.find("Pokemon Red save summary") !=
                 std::string::npos &&
             readableSummary.output.find("Pokedex:") != std::string::npos &&
             readableSummaryJson.code == 0 &&
             writtenSummary.code == 0 && fs::exists(summaryPath) &&
             collidedSummary.code ==
                 pkmn::cli::ToInt(pkmn::cli::ExitCode::OutputFailure) &&
             nlohmann::ordered_json::parse(readableSummaryJson.output)
                     .at("format") == "pkmn-red-readable-summary",
         "red summary should provide readable and machine-readable save stats");

  auto progressedDocument = includedDocument;
  progressedDocument["decoded"]["moneyAndCoins"]["money"] = 1200;
  progressedDocument["decoded"]["moneyAndCoins"]["coins"] = 50;
  progressedDocument["decoded"]["badges"]["raw"] = 1;
  progressedDocument["decoded"]["badges"]["mirrorRaw"] = 1;
  for (auto &record : progressedDocument["decoded"]["events"]["flags"])
    if (record.at("flagIndex") == 119) record["value"] = true;
  for (auto &record :
       progressedDocument["decoded"]["trainerBattles"]["records"])
    if (record.at("flagIndex") == 119) record["completed"] = true;
  const auto progressed =
      pkmn::cli::red::generation::Generate(progressedDocument);
  const fs::path progressedSavePath = temp / "synthetic-progressed.sav";
  {
    std::ofstream progressedFile(progressedSavePath, std::ios::binary);
    progressedFile.write(
        reinterpret_cast<const char *>(progressed.bytes.data()),
        static_cast<std::streamsize>(progressed.bytes.size()));
  }
  const auto progressText = Run({"compare", "progress",
                                 validSavePath.string(),
                                 progressedSavePath.string()});
  const auto progressJson = Run({"compare", "progress",
                                 validSavePath.string(),
                                 progressedSavePath.string(), "--format",
                                 "json"});
  const auto progressReport =
      nlohmann::ordered_json::parse(progressJson.output);
  Expect(progressText.code == 0 &&
             progressText.output.find("Boulder Badge") != std::string::npos &&
             progressText.output.find("Beat Brock") != std::string::npos &&
             progressJson.code == 0 &&
             progressReport.at("samePlaythrough") == true &&
             progressReport.at("currency").at("money").at("delta") == 1200 &&
             progressReport.at("badges").at("gained").at(0) == "Boulder",
         "compare progress should explain gameplay changes between backups");
  auto unrelatedDocument = progressedDocument;
  unrelatedDocument["decoded"]["trainer"]["trainerId"] = 42;
  const auto unrelated =
      pkmn::cli::red::generation::Generate(unrelatedDocument);
  const fs::path unrelatedSavePath = temp / "synthetic-unrelated.sav";
  {
    std::ofstream unrelatedFile(unrelatedSavePath, std::ios::binary);
    unrelatedFile.write(
        reinterpret_cast<const char *>(unrelated.bytes.data()),
        static_cast<std::streamsize>(unrelated.bytes.size()));
  }
  const auto unrelatedProgress = Run({"compare", "progress",
                                      validSavePath.string(),
                                      unrelatedSavePath.string()});
  Expect(unrelatedProgress.code ==
                 pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidInput) &&
             unrelatedProgress.error.find("trainer identity differs") !=
                 std::string::npos,
         "compare progress should reject unrelated trainer identities");
  const auto eventSearch =
      Run({"red", "events", "search", "articuno", "--format", "json"});
  Expect(eventSearch.code == 0 &&
             nlohmann::ordered_json::parse(eventSearch.output)
                     .at("records").at(0).at("name") ==
                 "EVENT_BEAT_ARTICUNO" &&
             Run({"red", "events", "show", "EVENT_GOT_STARTER"}).code == 0,
         "event discovery should search and show verified catalog records");

  const fs::path excludedJson = temp / "excluded.red.json";
  const auto excludedDecode =
      Run({"red", "decode", validSavePath.string(), "--output",
           excludedJson.string(), "--no-physical-image"});
  Expect(excludedDecode.code == 0,
         "red decode should support semantic-only export");
  nlohmann::ordered_json excludedDocument;
  {
    std::ifstream input(excludedJson);
    input >> excludedDocument;
  }
  Expect(!excludedDocument.contains("physicalImage") &&
             !excludedDocument.at("reconstruction").at("available").get<bool>(),
         "no-physical-image should omit archival bytes and mark reconstruction "
         "unavailable");
  const auto pipedDecode = Run({"red", "decode", validSavePath.string(),
                                "--output", "-", "--no-physical-image"});
  Expect(pipedDecode.code == 0 &&
             nlohmann::ordered_json::parse(pipedDecode.output)
                     .at("schema").at("schemaVersion") == "0.1.0",
         "red decode should support canonical JSON on stdout without status "
         "noise");
  {
    std::istringstream input(excludedDocument.dump());
    auto *original = std::cin.rdbuf(input.rdbuf());
    const auto pipedValidation =
        Run({"rjson", "validate", "-", "--format", "json"});
    std::cin.rdbuf(original);
    Expect(pipedValidation.code == 0 &&
               nlohmann::ordered_json::parse(pipedValidation.output)
                   .at("valid") == true,
           "rjson validation should accept canonical JSON from stdin");
  }
  {
    std::istringstream input(excludedDocument.dump());
    auto *original = std::cin.rdbuf(input.rdbuf());
    const auto pipedGeneration = Run({"rjson", "generate", "-", "-"});
    std::cin.rdbuf(original);
    Expect(pipedGeneration.code == 0 &&
               pipedGeneration.output.size() ==
                   pkmn::cli::red::save::RedSave::ExpectedSize,
           "semantic generation should support JSON stdin and binary stdout");
  }
  auto legacySemanticDocument = excludedDocument;
  for (const auto *field : {"events", "trainerBattles", "staticBattles",
                            "storyProgress"})
    legacySemanticDocument["decoded"].erase(field);
  const fs::path legacySemanticJson = temp / "legacy-semantic.red.json";
  const fs::path migratedSemanticJson = temp / "legacy-semantic.migrated.red.json";
  std::ofstream(legacySemanticJson) << legacySemanticDocument.dump(2);
  Expect(Run({"rjson", "migrate", legacySemanticJson.string(), "--output",
              migratedSemanticJson.string()}).code == 0 &&
             fs::exists(migratedSemanticJson),
         "rjson migrate should enrich compatible canonical documents safely");
  {
    nlohmann::ordered_json migrated;
    std::ifstream(migratedSemanticJson) >> migrated;
    Expect(migrated.at("decoded").at("events").at("flags").size() == 507 &&
               migrated.at("migration").at("semanticValuesChanged") == false,
           "migration should add the verified catalog without semantic changes");
  }
  const auto inspectRjson = Run({"rjson", "inspect", includedJson.string()});
  Expect(
      inspectRjson.code == 0 &&
          inspectRjson.output.find("Physical image: present and valid") !=
              std::string::npos,
      "rjson inspect should validate and summarize canonical JSON internally");
  const auto validateRjson = Run({"rjson", "validate", excludedJson.string()});
  Expect(validateRjson.code == 0 &&
             validateRjson.output.find("Physical image: absent") !=
                 std::string::npos,
         "rjson validate should accept semantic-only JSON and report "
         "reconstruction unavailable");
  Expect(Run({"rjson", "validate", excludedJson.string(), "--profile",
              "strict"}).code == 0 &&
             Run({"rjson", "validate", excludedJson.string(), "--profile",
                  "generation"}).code == 0 &&
             Run({"rjson", "validate", excludedJson.string(), "--profile",
                  "archival"}).code ==
                 pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidInput) &&
             Run({"rjson", "validate", includedJson.string(), "--profile",
                  "archival"}).code == 0,
         "validation profiles should distinguish strict semantic and archival "
         "requirements");
  const auto schemaCapabilities =
      Run({"rjson", "schema", "--format", "json"});
  Expect(schemaCapabilities.code == 0 &&
             nlohmann::ordered_json::parse(schemaCapabilities.output)
                     .at("namedEventCatalogSize") == 507 &&
             nlohmann::ordered_json::parse(schemaCapabilities.output)
                     .at("physicalImageUsedForGeneration") == false,
         "schema discovery should expose the supported contract and authority "
         "boundary");
  const fs::path secondValidSavePath = temp / "synthetic-valid-second.sav";
  {
    std::ofstream copy(secondValidSavePath, std::ios::binary);
    copy.write(reinterpret_cast<const char *>(validSave.data()),
               static_cast<std::streamsize>(validSave.size()));
  }
  const fs::path batchDecodeDirectory = temp / "batch-decode";
  Expect(Run({"red", "validate-batch", validSavePath.string(),
              secondValidSavePath.string(), "--format", "json"}).code == 0 &&
             Run({"red", "decode-batch", validSavePath.string(),
                  secondValidSavePath.string(), "--output-dir",
                  batchDecodeDirectory.string(), "--no-physical-image"}).code ==
                 0 &&
             fs::exists(batchDecodeDirectory / "batch-manifest.json"),
         "Red batch validation and decoding should process multiple saves "
         "transactionally");
  const fs::path batchGenerationDirectory = temp / "batch-generation";
  Expect(Run({"rjson", "generate-batch", includedJson.string(),
              excludedJson.string(), "--output-dir",
              batchGenerationDirectory.string()}).code == 0 &&
             fs::exists(batchGenerationDirectory / "batch-manifest.json") &&
             Run({"compare", "semantic-batch", includedJson.string(),
                  excludedJson.string(), "--format", "json"}).code == 0,
         "batch generation and semantic comparison should support automation");
  const auto validateRjsonJson =
      Run({"rjson", "validate", excludedJson.string(), "--format", "json"});
  Expect(validateRjsonJson.code == 0 &&
             nlohmann::ordered_json::parse(validateRjsonJson.output)
                 .at("valid")
                 .get<bool>(),
         "rjson validation should support machine-readable JSON output");

  const fs::path reconstructed = temp / "reconstructed.sav";
  const auto reconstruct = Run({"rjson", "reconstruct", includedJson.string(),
                                "--output", reconstructed.string()});
  Expect(reconstruct.code == 0 && fs::exists(reconstructed),
         "rjson reconstruct should write an archival physical image");
  const auto reconstructedSave =
      pkmn::cli::red::save::RedSave::Read(reconstructed);
  Expect(reconstructedSave.BytesView() == loaded.BytesView(),
         "archival reconstruction should reproduce the source bytes exactly");
  const auto missingPhysicalReconstruct =
      Run({"rjson", "reconstruct", excludedJson.string(), "--output",
           (temp / "missing-physical.sav").string()});
  Expect(missingPhysicalReconstruct.code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidInput),
         "reconstruction should fail safely when physicalImage is absent");
  const auto reconstructionCollision =
      Run({"rjson", "reconstruct", includedJson.string(), "--output",
           reconstructed.string()});
  Expect(reconstructionCollision.code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::OutputFailure),
         "reconstruction should refuse to overwrite an existing output");
  Expect(Run({"rjson", "reconstruct", includedJson.string(), "--output",
              reconstructed.string(), "--auto-suffix"})
                 .code == 0 &&
             fs::exists(temp / "reconstructed_2.sav"),
         "reconstruction should support explicit collision-safe suffixing");

  const fs::path generatedA = temp / "generated-a.sav";
  const auto generationA =
      Run({"rjson", "generate", includedJson.string(), generatedA.string()});
  Expect(generationA.code == 0 && fs::exists(generatedA) &&
             fs::exists(generatedA.string() + ".generation-report.json") &&
             fs::exists(generatedA.string() + ".generation-report.md"),
         "rjson generate should create a save and JSON/Markdown reports "
         "internally");
  const auto generatedValidation =
      pkmn::cli::red::validation::SaveValidator::Validate(
          pkmn::cli::red::save::RedSave::Read(generatedA));
  Expect(generatedValidation.Valid(),
         "semantic generation should regenerate every Red checksum");

  const fs::path generatedB = temp / "generated-b.sav";
  const auto generationB =
      Run({"rjson", "generate", excludedJson.string(), generatedB.string()});
  Expect(generationB.code == 0 &&
             pkmn::cli::red::save::RedSave::Read(generatedA).BytesView() ==
                 pkmn::cli::red::save::RedSave::Read(generatedB).BytesView(),
         "removing physicalImage must not alter semantic generation output");
  auto replacedPhysical = includedDocument;
  replacedPhysical["physicalImage"] = "deliberately malformed and ignored";
  const fs::path replacedJson = temp / "replaced-physical.json";
  std::ofstream(replacedJson) << replacedPhysical.dump(2);
  const fs::path generatedC = temp / "generated-c.sav";
  Expect(Run({"rjson", "generate", replacedJson.string(), generatedC.string()})
                     .code == 0 &&
             pkmn::cli::red::save::RedSave::Read(generatedA).BytesView() ==
                 pkmn::cli::red::save::RedSave::Read(generatedC).BytesView(),
         "replacing physicalImage must not alter or block semantic generation");
  nlohmann::ordered_json generationReport;
  std::ifstream(generatedA.string() + ".generation-report.json") >>
      generationReport;
  Expect(generationReport.at("physicalImageIgnored").get<bool>() &&
             generationReport.at("integrityValid").get<bool>() &&
             generationReport.at("writeOverlapCount") == 0 &&
             generationReport.at("locationCanonicalized").get<bool>(),
         "generation report should prove isolation, integrity, overlap safety, "
         "and location policy");
  Expect(Run({"rjson", "generate", includedJson.string(), generatedA.string()})
                 .code == pkmn::cli::ToInt(pkmn::cli::ExitCode::OutputFailure),
         "semantic generation should refuse output/report collisions");
  Expect(Run({"rjson", "generate", includedJson.string(), generatedA.string(),
              "--auto-suffix"})
                 .code == 0 &&
             fs::exists(temp / "generated-a_2.sav"),
         "semantic generation should suffix all output/report collisions when "
         "explicitly requested");
  auto unsupportedSemantic = excludedDocument;
  unsupportedSemantic["decoded"]["currentBoxCache"]["selectedBoxNumber"] = 99;
  const fs::path unsupportedSemanticJson = temp / "unsupported-semantic.json";
  std::ofstream(unsupportedSemanticJson) << unsupportedSemantic.dump(2);
  Expect(Run({"rjson", "generate", unsupportedSemanticJson.string(),
              (temp / "unsupported.sav").string()})
                 .code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::GenerationFailure),
         "semantic generation should fail closed on unsupported state");

  const auto physicalSame =
      Run({"compare", "physical", generatedA.string(), generatedA.string()});
  Expect(physicalSame.code == 0 &&
             physicalSame.output.find("Identical: yes") != std::string::npos,
         "physical comparison should report identical save bytes");
  const auto physicalDifferent =
      Run({"compare", "physical", validSavePath.string(), generatedA.string()});
  Expect(physicalDifferent.code == 0 &&
             physicalDifferent.output.find("Differing bytes:") !=
                 std::string::npos,
         "physical comparison should report byte differences");
  const auto semanticSame = Run(
      {"compare", "semantic", includedJson.string(), excludedJson.string()});
  Expect(semanticSame.code == 0,
         "semantic comparison should ignore archival physical-image presence");
  const auto physicalJson =
      Run({"compare", "physical", generatedA.string(), generatedA.string(),
           "--format", "json"});
  Expect(physicalJson.code == 0 &&
             nlohmann::ordered_json::parse(physicalJson.output)
                 .at("identical")
                 .get<bool>(),
         "comparison commands should support machine-readable JSON stdout");
  const fs::path semanticReportJson = temp / "semantic-report.json";
  const fs::path semanticReportMarkdown = temp / "semantic-report.md";
  Expect(Run({"compare", "semantic", includedJson.string(),
              excludedJson.string(), "--output-json",
              semanticReportJson.string(), "--output-markdown",
              semanticReportMarkdown.string()})
                 .code == 0 &&
             fs::exists(semanticReportJson) &&
             fs::exists(semanticReportMarkdown),
         "comparison commands should write collision-safe JSON and Markdown "
         "reports");
  Expect(Run({"compare", "semantic", includedJson.string(),
              excludedJson.string(), "--output-json",
              semanticReportJson.string()})
                 .code == pkmn::cli::ToInt(pkmn::cli::ExitCode::OutputFailure),
         "comparison report outputs should never be overwritten");
  auto derivedDifference = excludedDocument;
  derivedDifference["decoded"]["trainer"]["name"]["rawHex"] = "00";
  const fs::path derivedDifferenceJson = temp / "derived-difference.json";
  std::ofstream(derivedDifferenceJson) << derivedDifference.dump(2);
  const auto derivedComparison =
      Run({"compare", "semantic", excludedJson.string(),
           derivedDifferenceJson.string(), "--format", "json"});
  Expect(derivedComparison.code == 0 &&
             nlohmann::ordered_json::parse(derivedComparison.output)
                     .at("classificationCounts")
                     .at("derived_match") ==
                 1,
         "semantic comparison should classify derived raw-field differences");
  auto semanticDifference = excludedDocument;
  semanticDifference["decoded"]["moneyAndCoins"]["money"] = 123456;
  const fs::path semanticDifferenceJson = temp / "semantic-difference.json";
  std::ofstream(semanticDifferenceJson) << semanticDifference.dump(2);
  Expect(Run({"compare", "semantic", excludedJson.string(),
              semanticDifferenceJson.string()})
                 .code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::SemanticMismatch),
         "semantic comparison should return the mismatch exit code");

  const fs::path proofDirectory = temp / "synthetic-proof";
  const auto proof = Run({"proof", "red", validSavePath.string(),
                          "--output-dir", proofDirectory.string()});
  Expect(proof.code == 0 && fs::exists(proofDirectory / "comparison.md") &&
             fs::exists(proofDirectory / "comparison.json") &&
             fs::exists(proofDirectory / "physical-comparison.md") &&
             fs::exists(proofDirectory / "physical-comparison.json") &&
             fs::exists(proofDirectory / "semantic-comparison.md") &&
             fs::exists(proofDirectory / "semantic-comparison.json") &&
             fs::exists(proofDirectory / "summary-comparison.md") &&
             fs::exists(proofDirectory / "summary-comparison.json") &&
             fs::exists(proofDirectory / "semantic-json-comparison.md") &&
             fs::exists(proofDirectory / "semantic-json-comparison.json") &&
             fs::exists(proofDirectory / "archival-json-comparison.md") &&
             fs::exists(proofDirectory / "archival-json-comparison.json") &&
             fs::exists(proofDirectory / "proof-manifest.json") &&
             fs::exists(proofDirectory / "emulator-checklist.md"),
         "proof red should create the complete report and emulator-checklist "
         "package");
  nlohmann::ordered_json proofManifest;
  std::ifstream(proofDirectory / "proof-manifest.json") >> proofManifest;
  Expect(proofManifest.at("deterministic").get<bool>() &&
             proofManifest.at("physicalImageIsolation").get<bool>() &&
             proofManifest.at("generatedIntegrityValid").get<bool>() &&
             proofManifest.at("artifactSha256").is_object() &&
             proofManifest.at("artifactSha256").size() >= 18 &&
             proofManifest.at("emulatorValidation") == "required-manual-gate",
         "proof manifest should record automated gates without claiming "
         "emulator proof");
  Expect(Run({"proof", "red", validSavePath.string(), "--output-dir",
              proofDirectory.string()})
                 .code == pkmn::cli::ToInt(pkmn::cli::ExitCode::OutputFailure),
         "proof workflow should refuse an existing proof directory");
  Expect(Run({"proof", "red", validSavePath.string(), "--output-dir",
              proofDirectory.string(), "--zip", "--auto-suffix"})
                 .code == 0 &&
             fs::exists(temp / "synthetic-proof_2") &&
             fs::exists(temp / "synthetic-proof_2.zip"),
         "proof workflows should support explicit collision-safe directory "
         "and archive suffixing");

  auto postEmulatorBytes =
      pkmn::cli::red::save::RedSave::Read(generatedA).BytesView();
  postEmulatorBytes[0x2CEF] = 12;
  pkmn::cli::red::generation::RepairChecksums(postEmulatorBytes);
  const fs::path postEmulatorSave = temp / "post-emulator.sav";
  {
    std::ofstream save(postEmulatorSave, std::ios::binary);
    save.write(reinterpret_cast<const char *>(postEmulatorBytes.data()),
               static_cast<std::streamsize>(postEmulatorBytes.size()));
  }
  const fs::path postEmulatorDirectory = temp / "post-emulator-validation";
  const auto postEmulator =
      Run({"red", "validate-post-emulator", generatedA.string(),
           postEmulatorSave.string(), "--output-dir",
           postEmulatorDirectory.string()});
  Expect(postEmulator.code == 0 &&
             fs::exists(postEmulatorDirectory /
                        "post-emulator-validation.json") &&
             fs::exists(postEmulatorDirectory /
                        "post-emulator-validation.md"),
         "post-emulator validation should accept checksum-valid expected "
         "runtime drift and write reports");
  Expect(Run({"proof", "post-emulator", "--before", generatedA.string(),
              "--after", postEmulatorSave.string(), "--proof-dir",
              proofDirectory.string()})
                 .code == 0,
         "post-emulator proof continuation should update an existing proof "
         "package when explicitly requested");
  {
    std::ifstream manifestInput(proofDirectory / "proof-manifest.json");
    manifestInput >> proofManifest;
  }
  Expect(proofManifest.at("emulatorValidation") == "passed" &&
             proofManifest.at("postEmulatorValidation").at("passed") == true,
         "proof continuation should record the completed emulator gate");

  auto unexpectedPostDocument = excludedDocument;
  unexpectedPostDocument["decoded"]["trainer"]["name"]["value"] = "BLUE";
  unexpectedPostDocument["decoded"]["trainer"]["name"]["losslessValue"] =
      "BLUE";
  const auto unexpectedPostResult =
      pkmn::cli::red::generation::Generate(unexpectedPostDocument);
  const fs::path unexpectedPostSave = temp / "unexpected-post-emulator.sav";
  {
    std::ofstream save(unexpectedPostSave, std::ios::binary);
    save.write(
        reinterpret_cast<const char *>(unexpectedPostResult.bytes.data()),
        static_cast<std::streamsize>(unexpectedPostResult.bytes.size()));
  }
  Expect(Run({"proof", "post-emulator", "--before", generatedA.string(),
              "--after", unexpectedPostSave.string(), "--output-dir",
              (temp / "unexpected-post-report").string()})
                 .code == pkmn::cli::ToInt(
                              pkmn::cli::ExitCode::PostEmulatorValidationFailure),
         "post-emulator validation should fail on unexpected identity drift");

  const fs::path zippedProofA = temp / "zipped-proof-a";
  const fs::path zippedProofB = temp / "zipped-proof-b";
  const fs::path proofZipA = temp / "proof-a.zip";
  const fs::path proofZipB = temp / "proof-b.zip";
  Expect(Run({"proof", "red", validSavePath.string(), "--output-dir",
              zippedProofA.string(), "--zip-output", proofZipA.string()})
                 .code == 0 &&
             Run({"proof", "red", validSavePath.string(), "--output-dir",
                  zippedProofB.string(), "--zip-output", proofZipB.string()})
                     .code == 0,
         "proof red should create explicitly requested standalone ZIP "
         "packages");
  auto ReadBinary = [](const fs::path &path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    const auto size = input.tellg();
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    input.seekg(0);
    input.read(reinterpret_cast<char *>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    return bytes;
  };
  const auto zipA = ReadBinary(proofZipA);
  const auto zipB = ReadBinary(proofZipB);
  const std::string zipText(zipA.begin(), zipA.end());
  Expect(zipA == zipB && zipA.size() > 4 && zipA[0] == 'P' && zipA[1] == 'K',
         "proof ZIP packages should be deterministic ZIP archives");
  Expect(zipText.find(temp.string()) == std::string::npos &&
             zipText.find("proof-manifest.json") != std::string::npos,
         "proof ZIPs should contain logical artifact names without private "
         "absolute paths");
  Expect(Run({"proof", "verify", zippedProofA.string()}).code == 0 &&
             Run({"proof", "verify", proofZipA.string(), "--format", "json"})
                     .code == 0,
         "proof verify should validate both directories and deterministic ZIPs");
  {
    std::ofstream tamper(zippedProofA / "summary-comparison.md",
                         std::ios::app);
    tamper << "tampered\n";
  }
  Expect(Run({"proof", "verify", zippedProofA.string()}).code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidInput),
         "proof verify should reject artifact hash mismatches");

  const auto sourceBeforeEdit =
      pkmn::cli::red::save::RedSave::Read(validSavePath).BytesView();
  const fs::path editSession = temp / "multi.edit-session.json";
  const auto beginEdit =
      Run({"red", "begin-edit", validSavePath.string(), "--output",
           editSession.string()});
  Expect(beginEdit.code == 0 && fs::exists(editSession),
         "begin-edit should create a semantic-only copy-first session");
  Expect(Run({"red", "begin-edit", validSavePath.string(), "--output",
              editSession.string()})
                 .code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::EditValidationFailure),
         "begin-edit should refuse an existing session path");
  const auto addEdits =
      Run({"red", "edit-session", editSession.string(), "--trainer-name",
           "RED", "--money", "123456", "--set",
           "/decoded/party/pokemon", "[]"});
  Expect(addEdits.code == 0 &&
             addEdits.output.find("Pending edits updated and validated: 3") !=
                 std::string::npos,
         "edit-session should preserve and validate multiple pending edits");
  const auto pendingEdits =
      Run({"red", "pending-edits", editSession.string()});
  Expect(pendingEdits.code == 0 &&
             pendingEdits.output.find("Pending edits: 3") != std::string::npos,
         "pending-edits should report every staged edit");
  Expect(Run({"red", "edit-session", editSession.string(), "--coins", "42",
              "--dry-run", "--format", "json"}).code == 0 &&
             Run({"red", "pending-edits", editSession.string()})
                     .output.find("Pending edits: 3") != std::string::npos,
         "edit-session dry runs should validate without persisting changes");
  Expect(Run({"red", "validate-edit", editSession.string()}).code == 0,
         "validate-edit should generate and checksum-check without writing a "
         "save");
  Expect(Run({"red", "edit-session", editSession.string(), "--set",
              "/decoded/location/mapId", "1"})
                 .code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::EditValidationFailure),
         "edit sessions should reject arbitrary location changes");
  const fs::path editedSave = temp / "explicit-manual-edit.sav";
  const auto endEdit = Run({"red", "end-edit", editSession.string(),
                            "--output", editedSave.string()});
  Expect(endEdit.code == 0 && fs::exists(editedSave) &&
             fs::exists(editedSave.string() + ".edit-report.json") &&
             fs::exists(editedSave.string() + ".edit-report.md"),
         "end-edit should write a validated save copy and edit reports");
  Expect(pkmn::cli::red::validation::SaveValidator::Validate(
             pkmn::cli::red::save::RedSave::Read(editedSave))
             .Valid(),
         "end-edit should regenerate and validate every checksum");
  const auto editedLoaded =
      pkmn::cli::red::save::RedSave::Read(editedSave);
  const auto editedDecoded = pkmn::cli::red::json::Decode(
      editedLoaded, "edited.sav",
      pkmn::cli::red::validation::SaveValidator::Validate(editedLoaded),
      {false});
  Expect(editedDecoded.at("decoded").at("trainer").at("name").at("value") ==
                 "RED" &&
             editedDecoded.at("decoded").at("moneyAndCoins").at("money") ==
                 123456,
         "end-edit should apply staged semantic values to the output save");
  Expect(pkmn::cli::red::save::RedSave::Read(validSavePath).BytesView() ==
             sourceBeforeEdit,
         "editing must never overwrite the source save");
  Expect(Run({"red", "end-edit", editSession.string(), "--output",
              editedSave.string()})
                 .code == pkmn::cli::ToInt(pkmn::cli::ExitCode::OutputFailure),
         "end-edit should refuse output and report collisions");
  Expect(Run({"red", "end-edit", editSession.string(), "--output",
              editedSave.string(), "--auto-suffix"})
                 .code == 0 &&
             fs::exists(temp / "explicit-manual-edit_2.sav"),
         "end-edit should support explicit collision-safe output suffixing");

  const fs::path invalidEditSession = temp / "invalid.edit-session.json";
  Expect(Run({"red", "begin-edit", validSavePath.string(), "--output",
              invalidEditSession.string()})
                 .code == 0,
         "a second edit session should be independent");
  Expect(Run({"red", "edit-session", invalidEditSession.string(), "--set",
              "/decoded/currentBoxCache/selectedBoxNumber", "99"})
                 .code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::EditValidationFailure),
         "invalid pending state should fail validation before session write");
  Expect(Run({"red", "edit-session", invalidEditSession.string(), "--set",
              "/decoded/moneyAndCoins/money", "1000000"})
                 .code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::EditValidationFailure),
         "advanced edit pointers should not bypass semantic range validation");

  const fs::path broadEditSession = temp / "broad.edit-session.json";
  const fs::path partyValueFile = temp / "party-value.json";
  const fs::path bagValueFile = temp / "bag-value.json";
  std::ofstream(partyValueFile)
      << excludedDocument.at("decoded").at("party").dump(2);
  std::ofstream(bagValueFile)
      << excludedDocument.at("decoded").at("inventory").at("bag").dump(2);
  Expect(Run({"red", "begin-edit", validSavePath.string(), "--output",
              broadEditSession.string()})
                 .code == 0 &&
             Run({"red", "edit-session", broadEditSession.string(),
                  "--badges", "all", "--selected-box", "1", "--party-file",
                  partyValueFile.string(), "--bag-file",
                  bagValueFile.string()})
                     .code == 0 &&
             Run({"red", "validate-edit", broadEditSession.string()}).code ==
                 0,
         "named broad-edit options should support badges, storage selection, "
         "party, and inventory JSON values");
  {
    nlohmann::ordered_json broadSessionDocument;
    std::ifstream(broadEditSession) >> broadSessionDocument;
    Expect(broadSessionDocument.at("document")
                   .at("decoded")
                   .at("badges")
                   .at("raw") == 255,
           "--badges all should stage all eight badges");
  }

  const fs::path eventSession = temp / "event.edit-session.json";
  const fs::path eventOutput = temp / "named-event-output.sav";
  Expect(Run({"red", "begin-edit", validSavePath.string(), "--output",
              eventSession.string()}).code == 0 &&
             Run({"red", "edit-session", eventSession.string(), "--event",
                  "EVENT_GOT_STARTER", "on"}).code == 0 &&
             Run({"red", "end-edit", eventSession.string(), "--dry-run",
                  "--format", "json"}).code == 0 &&
             Run({"red", "end-edit", eventSession.string(), "--output",
                  eventOutput.string()}).code == 0,
         "named verified event flags should be editable and generatable");
  const auto eventSave = pkmn::cli::red::save::RedSave::Read(eventOutput);
  const auto eventDecoded = pkmn::cli::red::json::Decode(
      eventSave, "named-event-output.sav",
      pkmn::cli::red::validation::SaveValidator::Validate(eventSave), {false});
  const auto &eventFlags = eventDecoded.at("decoded").at("events").at("flags");
  const auto starter = std::find_if(
      eventFlags.begin(), eventFlags.end(), [](const auto &record) {
        return record.at("name") == "EVENT_GOT_STARTER";
      });
  Expect(starter != eventFlags.end() && starter->at("value") == true &&
             (eventSave.At(0x29F3 + 34 / 8) & (1U << (34 % 8))) != 0,
         "named event edits should synchronize canonical and raw event state");
  Expect(Run({"red", "edit-session", eventSession.string(), "--event",
              "EVENT_NOT_A_REAL_FLAG", "on"}).code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::EditValidationFailure),
         "unknown named events should fail closed");

  const fs::path historySession = temp / "history.edit-session.json";
  Expect(Run({"red", "begin-edit", validSavePath.string(), "--output",
              historySession.string()}).code == 0 &&
             Run({"red", "edit-session", historySession.string(), "--money",
                  "100", "--coins", "20"}).code == 0 &&
             Run({"red", "annotate-edit", historySession.string(),
                  "automation sample"}).code == 0 &&
             Run({"red", "undo-edit", historySession.string()}).code == 0 &&
             Run({"red", "pending-edits", historySession.string()})
                     .output.find("Pending edits: 1") != std::string::npos &&
             Run({"red", "edit-history", historySession.string(), "--format",
                  "json"}).code == 0,
         "edit sessions should support annotations, history, and validated undo");
  const fs::path presetSession = temp / "preset.edit-session.json";
  Expect(Run({"red", "begin-edit", validSavePath.string(), "--output",
              presetSession.string()}).code == 0 &&
             Run({"red", "edit-session", presetSession.string(),
                  "--location-preset", "reds-house-2f"}).code == 0 &&
             Run({"red", "undo-edit", presetSession.string()}).code == 0,
         "the verified Red's-house location preset should be editable and undoable");
  std::string changedScripts(0x200, '0');
  changedScripts[1] = '1';
  Expect(Run({"red", "edit-session", presetSession.string(), "--set",
              "/decoded/worldStateRaw/scriptsHex",
              nlohmann::ordered_json(changedScripts).dump()}).code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::EditValidationFailure),
         "unverified raw script changes should have dedicated fail-closed diagnostics");

  const fs::path interactiveOutput =
      temp / "synthetic-valid_generated_trainer.sav";
  std::istringstream interactiveInput("1\nRED\n23\n");
  auto *originalInputBuffer = std::cin.rdbuf(interactiveInput.rdbuf());
  const auto interactiveEdit =
      Run({"red", "edit", validSavePath.string()});
  std::cin.rdbuf(originalInputBuffer);
  Expect(interactiveEdit.code == 0 && fs::exists(interactiveOutput) &&
             interactiveEdit.output.find("Pokemon Red copy-first editor") !=
                 std::string::npos,
         "interactive edit should stage, validate, and save from its menu");

  const fs::path moneySession = temp / "money.edit-session.json";
  Expect(Run({"red", "begin-edit", validSavePath.string(), "--output",
              moneySession.string()})
                 .code == 0 &&
             Run({"red", "edit-session", moneySession.string(), "--money",
                  "42"})
                     .code == 0 &&
             Run({"red", "end-edit", moneySession.string()}).code == 0 &&
             fs::exists(temp / "synthetic-valid_generated_money.sav"),
         "a single money edit should use the documented output name");

  auto missingFieldDocument = excludedDocument;
  missingFieldDocument["decoded"].erase("party");
  const fs::path missingFieldJson = temp / "missing-field.json";
  std::ofstream(missingFieldJson) << missingFieldDocument.dump(2);
  Expect(Run({"rjson", "validate", missingFieldJson.string()}).code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidInput),
         "rjson validate should reject a missing required semantic field");
  auto unsupportedSchema = excludedDocument;
  unsupportedSchema["schema"]["schemaVersion"] = "9.0.0";
  const fs::path unsupportedJson = temp / "unsupported-schema.json";
  std::ofstream(unsupportedJson) << unsupportedSchema.dump(2);
  Expect(Run({"rjson", "validate", unsupportedJson.string()}).code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidInput),
         "rjson validate should reject an unsupported schema version");
  const fs::path invalidJson = temp / "invalid.json";
  std::ofstream(invalidJson) << "{not-json";
  Expect(Run({"rjson", "validate", invalidJson.string()}).code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidInput),
         "rjson validate should reject malformed JSON");
  const auto deterministicA = pkmn::cli::red::json::Serialize(
      pkmn::cli::red::json::Decode(loaded, "logical.sav", validation, {true}));
  const auto deterministicB = pkmn::cli::red::json::Serialize(
      pkmn::cli::red::json::Decode(loaded, "logical.sav", validation, {true}));
  Expect(deterministicA == deterministicB,
         "canonical decode serialization should be deterministic");
  const auto collision = Run({"red", "decode", validSavePath.string(),
                              "--output", includedJson.string()});
  Expect(collision.code == pkmn::cli::ToInt(pkmn::cli::ExitCode::OutputFailure),
         "red decode should refuse an existing output");
  Expect(Run({"red", "decode", validSavePath.string(), "--output",
              includedJson.string(), "--auto-suffix"})
                 .code == 0 &&
             fs::exists(temp / "included_2.red.json"),
         "red decode should support explicit collision-safe output suffixing");

  for (std::size_t bank = 0; bank < 2; ++bank) {
    auto corrupted = ValidSyntheticSave();
    corrupted[pkmn::cli::red::validation::SaveValidator::BankStored[bank]] ^=
        0x01U;
    const auto report = pkmn::cli::red::validation::SaveValidator::Validate(
        pkmn::cli::red::save::RedSave(std::move(corrupted)));
    Expect(!report.banks[bank].Valid(),
           "internal validator should detect each bank checksum corruption");
  }
  for (std::size_t box = 0; box < 12; ++box) {
    auto corrupted = ValidSyntheticSave();
    const std::size_t bank = box / 6;
    const std::size_t withinBank = box % 6;
    corrupted
        [pkmn::cli::red::validation::SaveValidator::BoxChecksumTables[bank] +
         withinBank] ^= 0x01U;
    const auto report = pkmn::cli::red::validation::SaveValidator::Validate(
        pkmn::cli::red::save::RedSave(std::move(corrupted)));
    Expect(!report.boxes[box].Valid(),
           "internal validator should detect every box checksum corruption");
  }

  const auto standaloneInspect =
      Run({"red", "inspect", validSavePath.string()});
  const auto standaloneValidate =
      Run({"red", "validate", validSavePath.string()});
  Expect(standaloneInspect.code == 0 &&
             standaloneInspect.output.find("Main checksum: valid") !=
                 std::string::npos,
         "red inspect should use the internal engine when helper executables "
         "are absent");
  Expect(standaloneValidate.code == 0 &&
             standaloneValidate.output.find("Validation: passed") !=
                 std::string::npos,
         "red validate should use the internal engine when helper executables "
         "are absent");
  const auto standaloneValidateJson =
      Run({"red", "validate", validSavePath.string(), "--format", "json"});
  Expect(standaloneValidateJson.code == 0 &&
             nlohmann::ordered_json::parse(standaloneValidateJson.output)
                 .at("valid")
                 .get<bool>(),
         "red validation should support machine-readable JSON output");

  validSave[pkmn::cli::red::validation::SaveValidator::MainStored] ^= 0x01U;
  const fs::path corruptSavePath = temp / "synthetic-corrupt.sav";
  {
    std::ofstream saveFile(corruptSavePath, std::ios::binary);
    saveFile.write(reinterpret_cast<const char *>(validSave.data()),
                   static_cast<std::streamsize>(validSave.size()));
  }
  const auto corruptValidate =
      Run({"red", "validate", corruptSavePath.string()});
  Expect(corruptValidate.code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::ChecksumFailure),
         "red validate should map checksum corruption to checksum failure");
  Expect(corruptValidate.output.find("Main checksum: invalid") !=
             std::string::npos,
         "red validate should identify the corrupted checksum");
  const auto corruptDecode =
      Run({"red", "decode", corruptSavePath.string(), "--output",
           (temp / "corrupt.red.json").string()});
  Expect(corruptDecode.code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::ChecksumFailure),
         "red decode should reject a checksum-invalid save");
  const fs::path repairedSave = temp / "repaired.sav";
  const auto repair = Run({"red", "repair-checksums",
                           corruptSavePath.string(), "--output",
                           repairedSave.string()});
  Expect(repair.code == 0 && fs::exists(repairedSave) &&
             fs::exists(repairedSave.string() + ".repair-report.json") &&
             pkmn::cli::red::validation::SaveValidator::Validate(
                 pkmn::cli::red::save::RedSave::Read(repairedSave)).Valid() &&
             pkmn::cli::red::save::RedSave::Read(corruptSavePath).BytesView() ==
                 validSave,
         "checksum repair should create a valid copy and leave the corrupt "
         "source unchanged");

  const auto missingSave =
      Run({"red", "validate", (temp / "missing.sav").string()});
  Expect(missingSave.code ==
             pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidInput),
         "red validate should map an unreadable input to invalid input");
  const auto wrongSize = pkmn::cli::red::validation::SaveValidator::Validate(
      pkmn::cli::red::save::RedSave(std::vector<std::uint8_t>(100, 0)));
  Expect(!wrongSize.expectedSize && !wrongSize.Valid(),
         "internal validator should reject non-32-KiB saves before checksum "
         "access");

  bool boundsRejected = false;
  try {
    static_cast<void>(loaded.At(loaded.Size()));
  } catch (const std::out_of_range &) {
    boundsRejected = true;
  }
  Expect(boundsRejected,
         "internal Red save access should reject out-of-bounds reads");
  fs::remove_all(temp);

  if (failures == 0) {
    std::cout << "All command-router tests passed.\n";
  }
  return failures == 0 ? 0 : 1;
}
