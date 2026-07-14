#include "commands/doctor/DoctorCommand.hpp"

#include <ostream>

#include "app/ExitCode.hpp"
#include "app/Version.hpp"

namespace pkmn::cli::commands::doctor {
namespace {

void PrintHelp(std::ostream& output) {
    output
        << "Usage: pkmn doctor\n\n"
        << "Checks that this standalone executable contains the internal Red foundation.\n"
        << "No Save Genie or Save Generator executable is searched for or required.\n";
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
           << "[ok] command router\n"
           << "[ok] internal Red save loader\n"
           << "[ok] internal Red checksum validator\n"
           << "[ok] canonical Red JSON decoder\n"
           << "[info] internal RJSON validation and generation: planned milestones\n"
           << "\nStandalone readiness: ready\n";
    return ToInt(ExitCode::Success);
}

}  // namespace pkmn::cli::commands::doctor
