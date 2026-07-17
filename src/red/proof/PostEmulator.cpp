#include "red/proof/PostEmulator.hpp"

#include <sstream>

#include "red/comparison/Comparison.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/Sha256.hpp"

namespace pkmn::cli::red::proof {

PostEmulatorResult ValidatePostEmulator(
    const std::filesystem::path &beforePath,
    const std::filesystem::path &afterPath) {
  const auto before = save::RedSave::Read(beforePath);
  const auto after = save::RedSave::Read(afterPath);
  const auto beforeIntegrity = validation::SaveValidator::Validate(before);
  const auto afterIntegrity = validation::SaveValidator::Validate(after);
  if (!beforeIntegrity.Valid())
    throw std::runtime_error("pre-emulator save failed checksum validation");
  if (!afterIntegrity.Valid())
    throw std::runtime_error("post-emulator save failed checksum validation");

  const auto beforeDocument =
      json::Decode(before, beforePath.filename().string(), beforeIntegrity,
                   {.includePhysicalImage = false});
  const auto afterDocument =
      json::Decode(after, afterPath.filename().string(), afterIntegrity,
                   {.includePhysicalImage = false});
  const auto physical =
      comparison::ComparePhysical(before.BytesView(), after.BytesView());
  const auto semantic = comparison::CompareSemantic(
      beforeDocument, afterDocument, {.postEmulator = true});
  const bool passed = semantic.at("equivalent").get<bool>();
  json::OrderedJson report = {
      {"format", "pkmn-post-emulator-validation"},
      {"reportVersion", "1.0.0"},
      {"workflow", "post-emulator-validation"},
      {"before",
       {{"logicalName", beforePath.filename().string()},
        {"size", before.Size()},
        {"sha256", util::Sha256Hex(before.BytesView())},
        {"checksumsValid", beforeIntegrity.Valid()}}},
      {"after",
       {{"logicalName", afterPath.filename().string()},
        {"size", after.Size()},
        {"sha256", util::Sha256Hex(after.BytesView())},
        {"checksumsValid", afterIntegrity.Valid()}}},
      {"sourceFilesModified", false},
      {"physicalComparison", physical},
      {"semanticComparison", semantic},
      {"unexpectedCorruptionDetected", !passed},
      {"passed", passed}};

  std::ostringstream markdown;
  markdown << "# Post-Emulator Validation\n\n"
           << "- Result: " << (passed ? "passed" : "failed") << '\n'
           << "- Before SHA-256: `" << util::Sha256Hex(before.BytesView())
           << "`\n- After SHA-256: `" << util::Sha256Hex(after.BytesView())
           << "`\n- Before checksums: valid\n- After checksums: valid\n"
           << "- Source files modified: no\n\n"
           << comparison::SemanticMarkdown(semantic) << "\n"
           << comparison::PhysicalMarkdown(physical);
  return {std::move(report), markdown.str(), passed};
}

} // namespace pkmn::cli::red::proof
