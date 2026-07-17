#include "commands/config/ConfigCommand.hpp"

#include <ostream>

#include <nlohmann/json.hpp>

#include "app/ExitCode.hpp"

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
      {"environmentVariables",
       nlohmann::ordered_json::array({"PKMN_QUIET=1", "PKMN_VERBOSE=1",
                                      "NO_COLOR"})},
      {"transactionalOutputWrites", true},
      {"fireRedSupport", "not-implemented"}};
  if (json)
    output << policy.dump(2) << '\n';
  else
    output << "pkmn compiled policy\n"
              "External engines: not required\n"
              "Input/output overwrite: disabled\n"
              "Collision suffixing: opt-in with --auto-suffix\n"
              "Semantic physicalImage authority: disabled\n"
              "Reconstruction physicalImage authority: required\n"
              "Arbitrary location editing: disabled\n"
              "Safe generated location: Red's house second floor\n"
              "Environment controls: PKMN_QUIET=1, PKMN_VERBOSE=1, NO_COLOR\n"
              "Transactional output writes: enabled\n"
              "FireRed support: not implemented\n";
  return 0;
}

} // namespace pkmn::cli::commands::config
