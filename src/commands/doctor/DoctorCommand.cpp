#include "commands/doctor/DoctorCommand.hpp"

#include <filesystem>
#include <ostream>
#include <string_view>

#include "app/ExitCode.hpp"
#include "app/Version.hpp"
#include "integration/ToolDiscovery.hpp"

namespace pkmn::cli::commands::doctor {
namespace {

void PrintHelp(std::ostream& output) {
    output
        << "Usage: pkmn doctor\n\n"
        << "Checks whether the unified CLI can locate the two verified Pokemon Red helper tools.\n"
        << "Discovery order: environment override, PATH, then a nearby sibling-project build.\n\n"
        << "Environment overrides:\n"
        << "  PKMN_RED_SAVE_GENIE       Path to the Save Genie executable\n"
        << "  PKMN_RED_SAVE_GENERATOR   Path to pkmn-red-save-generator\n";
}

void PrintTool(const integration::ToolLocation& tool, std::ostream& output) {
    output << (tool.Found() ? "[ok]   " : "[miss] ") << tool.displayName << '\n';
    if (tool.Found()) {
        output << "       path: " << tool.path->string() << '\n'
               << "       via:  " << tool.source << '\n';
    } else {
        output << "       " << tool.guidance << '\n';
    }
}

}  // namespace

int Run(const std::vector<std::string>& arguments,
        std::ostream& output,
        std::ostream& error) {
    if (!arguments.empty()) {
        if (arguments.size() == 1 &&
            (arguments.front() == "--help" || arguments.front() == "-h" ||
             arguments.front() == "help")) {
            PrintHelp(output);
            return ToInt(ExitCode::Success);
        }
        error << "pkmn doctor: unexpected argument '" << arguments.front() << "'\n";
        return ToInt(ExitCode::InvalidArguments);
    }

    output << "pkmn doctor\n"
           << "CLI version: " << kVersion << '\n'
           << "Working directory: " << std::filesystem::current_path().string() << "\n\n";

    const auto genie = integration::DiscoverTool(integration::SaveGenieSpec());
    const auto generator = integration::DiscoverTool(integration::SaveGeneratorSpec());
    PrintTool(genie, output);
    PrintTool(generator, output);

    const bool ready = genie.Found() && generator.Found();
    output << "\nRed wrapper readiness: " << (ready ? "ready" : "not ready") << '\n';
    if (!ready) {
        output << "The base CLI is healthy; one or more external Red engines still need configuration.\n";
    }
    return ready ? ToInt(ExitCode::Success) : ToInt(ExitCode::GeneralFailure);
}

}  // namespace pkmn::cli::commands::doctor

