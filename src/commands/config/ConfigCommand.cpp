#include "commands/config/ConfigCommand.hpp"

#include <ostream>

#include <nlohmann/json.hpp>

#include "app/ExitCode.hpp"
#include "util/ResourceLocator.hpp"

namespace pkmn::cli::commands::config {

int Run(const std::vector<std::string> &arguments, std::ostream &output,
        std::ostream &error) {
  if (arguments.empty() || arguments[0] == "help" ||
      arguments[0] == "--help") {
    output << "Usage: pkmn config show [--format text|json]\n\n"
              "Shows the compiled safety/default policy. pkmn does not use "
              "external engine paths or hidden mutable configuration.\n";
    return 0;
  }
  if (arguments[0] != "show" || arguments.size() > 3) {
    error << "pkmn config: invalid arguments\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  bool json = false;
  if (arguments.size() == 3 && arguments[1] == "--format" &&
      arguments[2] == "json")
    json = true;
  else if (arguments.size() != 1) {
    error << "pkmn config: --format must be text or json\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  nlohmann::ordered_json policy = {
      {"externalEngineExecutables", false},
      {"overwriteInputs", false},
      {"overwriteOutputs", false},
      {"automaticSuffixing", "opt-in (--auto-suffix)"},
      {"semanticGenerationUsesPhysicalImage", false},
      {"reconstructionRequiresPhysicalImage", true},
      {"arbitraryLocationEditing", false},
      {"safeGeneratedLocation", "Red's house second floor"},
      {"canonicalRedJsonSchema", "0.1.0"},
      {"canonicalFireRedJsonSchema", "0.4.0"},
      {"pokemonConversionProfile", "PCCS ORIGINAL pinned deterministic profile"},
      {"ambiguousTrainerPolicy", "target remains undefeated"},
      {"fireRedOnlyProgressionPolicy", "locked unless explicitly derived"},
      {"rawCrossGenerationIdCopying", false},
      {"nativeFireRedGeneration", "implemented; Phase 5 MAQ acceptance passed"},
      {"redToFireRedConversion", "implemented; Phase 6 MAQ acceptance passed"},
      {"version3Phase", "phase-3-complete"},
      {"conversionRouteRegistry", "typed; all four routes available"},
      {"checksumRepairDuringConversion", "in-memory; source unchanged by default"},
      {"interactiveMode", "implemented; all four routes with evidence labels"},
      {"fireRedTemplate", "bundled clean default; strict user override supported"},
      {"environmentVariables",
       nlohmann::ordered_json::array({"PKMN_QUIET=1", "PKMN_VERBOSE=1",
                                      "NO_COLOR", "PKMN_FIRERED_TEMPLATE"})},
      {"runtimeMode", util::UsesBundledRuntime()
                          ? "bundled-private-executable"
                          : "developer-python-fallback"},
      {"pythonRuntimeRequiredForFireRedConversion",
       !util::UsesBundledRuntime()},
      {"transactionalOutputWrites", true},
      {"fireRedSupport", "phase-5-and-phase-6-accepted"}};
  if (json) {
    output << policy.dump(2) << '\n';
  } else {
    output << "pkmn compiled policy\n"
              "External engines: not required\n"
              "Input/output overwrite: disabled\n"
              "Collision suffixing: opt-in with --auto-suffix\n"
              "Semantic physicalImage authority: disabled\n"
              "Reconstruction physicalImage authority: required\n"
              "Arbitrary location editing: disabled\n"
              "Safe generated location: Red's house second floor\n"
              "Pokemon conversion: pinned deterministic PCCS ORIGINAL profile\n"
              "Ambiguous trainers: remain undefeated\n"
              "FireRed-only progression: locked unless explicitly derived\n"
              "Raw event/trainer/item IDs copied between games: never\n"
              "Environment controls: PKMN_QUIET=1, PKMN_VERBOSE=1, NO_COLOR, "
              "PKMN_FIRERED_TEMPLATE\n"
              "Transactional output writes: enabled\n"
              "FireRed template: bundled clean default; strict user override supported\n"
              "FireRed generation: Phase 5 accepted\n"
              "Red-to-FireRed conversion: Phase 6 accepted\n";
    output << "Version 3 status: Phase 3 complete\n"
              "Route registry: all four Red/Blue-to-FireRed/LeafGreen routes available\n"
              "Conversion checksum repair: in-memory; source unchanged by default\n"
              "Interactive mode: available for all four routes\n"
           << "Runtime mode: "
           << (util::UsesBundledRuntime() ? "bundled-private-executable"
                                          : "developer-python-fallback")
           << '\n';
  }
  return 0;
}

} // namespace pkmn::cli::commands::config
