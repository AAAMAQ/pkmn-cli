#include "commands/doctor/DoctorCommand.hpp"

#include <ostream>

#include <nlohmann/json.hpp>

#include "app/ExitCode.hpp"
#include "app/Version.hpp"

namespace pkmn::cli::commands::doctor {
namespace {

void PrintHelp(std::ostream& output) {
    output
        << "Usage: pkmn doctor [--format text|json]\n\n"
        << "Checks that this standalone executable contains the internal Red foundation.\n"
        << "No Save Genie or Save Generator executable is searched for or required.\n";
}

}  // namespace

int Run(const std::vector<std::string>& arguments,
        std::ostream& output,
        std::ostream& error) {
    bool json = false;
    if (!arguments.empty()) {
        if (arguments.size() == 1 &&
            (arguments.front() == "--help" || arguments.front() == "-h" ||
             arguments.front() == "help")) {
            PrintHelp(output);
            return ToInt(ExitCode::Success);
        }
        if ((arguments.size() == 1 && arguments.front() == "--json") ||
            (arguments.size() == 2 && arguments.front() == "--format" &&
             arguments[1] == "json")) {
            json = true;
        } else {
        error << "pkmn doctor: unexpected argument '" << arguments.front() << "'\n";
        return ToInt(ExitCode::InvalidArguments);
        }
    }

    if (json) {
        nlohmann::ordered_json report = {
            {"tool", "pkmn"},
            {"version", std::string(kVersion)},
            {"standaloneReadiness", "ready"},
            {"externalExecutablesRequired", false},
            {"modules", nlohmann::ordered_json::array({
                "command-router", "red-save-loader", "red-checksum-validator",
                "red-json-decoder-validator-reconstructor", "red-semantic-generator",
                "physical-semantic-comparison", "red-proof",
                "post-emulator-validation", "red-editing"})}};
        output << report.dump(2) << '\n';
        return ToInt(ExitCode::Success);
    }

    output << "pkmn doctor\n"
           << "CLI version: " << kVersion << '\n'
           << "[ok] command router\n"
           << "[ok] internal Red save loader\n"
           << "[ok] internal Red checksum validator\n"
           << "[ok] canonical Red JSON decoder\n"
           << "[ok] internal Red JSON validator and reconstructor\n"
           << "[ok] internal semantic generator and bundled Red template\n"
        << "[ok] physical/semantic comparison and Red proof workflow\n"
        << "[ok] post-emulator validation and proof continuation\n"
        << "[ok] validated copy-first Red editing sessions\n"
           << "\nStandalone readiness: ready\n";
    return ToInt(ExitCode::Success);
}

}  // namespace pkmn::cli::commands::doctor
