#include "commands/proof/ProofCommand.hpp"

#include <filesystem>
#include <fstream>
#include <ostream>
#include <stdexcept>

#include "app/ExitCode.hpp"
#include "red/comparison/Comparison.hpp"
#include "red/generation/SemanticGenerator.hpp"
#include "red/json/RedDecoder.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/Sha256.hpp"

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
} // namespace

int Run(const std::vector<std::string> &args, std::ostream &output,
        std::ostream &error) {
  if (args.empty() || args[0] == "help" || args[0] == "--help") {
    output << "Usage: pkmn proof red <source.sav> [--output-dir <directory>]\n";
    return 0;
  }
  if (args.size() < 2 || args[0] != "red") {
    error << "pkmn proof: expected 'red <source.sav>'\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  std::filesystem::path source = args[1];
  auto directory = DefaultDirectory(source);
  if (args.size() == 4 && args[2] == "--output-dir")
    directory = args[3];
  else if (args.size() != 2) {
    error << "pkmn proof red: invalid arguments\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  if (std::filesystem::exists(directory)) {
    error << "pkmn proof red: refusing existing proof directory\n";
    return ToInt(ExitCode::OutputFailure);
  }
  try {
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
    std::filesystem::create_directories(directory);
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
    Text(directory / "generation-report.json", generated.report.dump(2) + "\n");
    Text(directory / "generation-report.md", generated.markdownReport);
    Json manifest = {
        {"tool", "pkmn"},
        {"sourceLogicalName", source.filename().string()},
        {"sourceSha256", util::Sha256Hex(original.BytesView())},
        {"generatedSha256", util::Sha256Hex(generated.bytes)},
        {"deterministic", deterministic},
        {"physicalImageIsolation", isolated},
        {"generatedIntegrityValid", generatedIntegrity.Valid()},
        {"semanticEquivalentUnderPolicy", semantic.at("equivalent")},
        {"emulatorValidation", "required-manual-gate"}};
    Text(directory / "proof-manifest.json", manifest.dump(2) + "\n");
    Text(directory / "emulator-checklist.md",
         "# Emulator Checklist\n\n- [ ] Continue screen renders normally\n- [ "
         "] Continue loads without corruption\n- [ ] Player appears in Red's "
         "house second floor\n- [ ] Movement, menus, party, and PC work\n- [ ] "
         "Save normally, fully shut down, and reload\n- [ ] Return the "
         "post-emulator save for internal validation\n\nAutomated proof does "
         "not replace this manual gate.\n");
    output << "Pokemon Red proof package created\nOutput: "
           << directory.filename().string()
           << "\nDeterminism: passed\nPhysical-image isolation: "
              "passed\nEmulator validation: required\n";
    return semantic.at("equivalent").get<bool>()
               ? 0
               : ToInt(ExitCode::SemanticMismatch);
  } catch (const std::exception &e) {
    error << "pkmn proof red: " << e.what() << '\n';
    return ToInt(ExitCode::GeneralFailure);
  }
}
} // namespace pkmn::cli::commands::proof
