#include "app/CommandRouter.hpp"

#include <algorithm>
#include <array>
#include <ostream>
#include <string_view>

#include "app/ExitCode.hpp"
#include "app/Version.hpp"
#include "commands/doctor/DoctorCommand.hpp"

namespace pkmn::cli {
namespace {

constexpr std::array<std::string_view, 8> kPlannedDomains = {
    "red", "rjson", "proof", "compare", "config", "fred", "frjson", "convert"};

bool IsHelp(std::string_view argument) {
    return argument == "help" || argument == "--help" || argument == "-h";
}

bool IsVersion(std::string_view argument) {
    return argument == "version" || argument == "--version" || argument == "-V";
}

}  // namespace

int CommandRouter::Run(const std::vector<std::string>& arguments,
                       std::ostream& output,
                       std::ostream& error) const {
    if (arguments.empty() || IsHelp(arguments.front())) {
        PrintHelp(output);
        return ToInt(ExitCode::Success);
    }

    if (IsVersion(arguments.front())) {
        PrintVersion(output);
        return ToInt(ExitCode::Success);
    }

    if (arguments.front() == "doctor") {
        const std::vector<std::string> doctorArguments(arguments.begin() + 1, arguments.end());
        return commands::doctor::Run(doctorArguments, output, error);
    }

    if (std::find(kPlannedDomains.begin(), kPlannedDomains.end(), arguments.front()) !=
        kPlannedDomains.end()) {
        return RunPlannedDomain(arguments, output, error);
    }

    error << "pkmn: unknown command or domain '" << arguments.front() << "'\n"
          << "Run 'pkmn --help' to see available commands.\n";
    return ToInt(ExitCode::InvalidArguments);
}

void CommandRouter::PrintVersion(std::ostream& output) {
    output << kProgramName << ' ' << kVersion << '\n';
}

void CommandRouter::PrintHelp(std::ostream& output) {
    output
        << "pkmn - unified Pokemon save research command line\n\n"
        << "Usage:\n"
        << "  pkmn <domain> <command> [arguments] [options]\n"
        << "  pkmn doctor\n"
        << "  pkmn --help\n"
        << "  pkmn --version\n\n"
        << "Available now:\n"
        << "  doctor               Check the CLI and Red helper-tool environment\n\n"
        << "Planned command domains:\n"
        << "  red                  Pokemon Red .sav workflows\n"
        << "  rjson                Pokemon Red .red.json workflows\n"
        << "  proof                End-to-end proof workflows\n"
        << "  compare              Physical and semantic comparisons\n"
        << "  config               Local helper-tool configuration\n"
        << "  fred, frjson         Future FireRed workflows (not implemented)\n"
        << "  convert              Future Red-to-FireRed conversion (not implemented)\n\n"
        << "Core safety model:\n"
        << "  generate             Semantic generation; physicalImage is never authority\n"
        << "  reconstruct          Archival reconstruction; physicalImage is required\n"
        << "  edit                 Writes a validated copy; never overwrites input by default\n\n"
        << "Examples planned for the Red integration milestones:\n"
        << "  pkmn red decode savefile.sav\n"
        << "  pkmn red validate savefile.sav\n"
        << "  pkmn rjson generate savefile.red.json\n"
        << "  pkmn rjson reconstruct savefile.red.json\n"
        << "  pkmn proof red savefile.sav\n";
}

int CommandRouter::RunPlannedDomain(const std::vector<std::string>& arguments,
                                    std::ostream& output,
                                    std::ostream& error) {
    if (arguments.size() > 1 && IsHelp(arguments[1])) {
        output << "The '" << arguments.front()
               << "' domain is reserved by the command router but is not implemented yet.\n";
        return ToInt(ExitCode::Success);
    }

    error << "pkmn: the '" << arguments.front()
          << "' domain is planned but not implemented in this foundation release.\n";
    if (arguments.front() == "fred" || arguments.front() == "frjson" ||
        arguments.front() == "convert") {
        error << "FireRed and Red-to-FireRed support will remain disabled until their "
                 "separate engines are verified and emulator-proven.\n";
    }
    return ToInt(ExitCode::UnsupportedOperation);
}

}  // namespace pkmn::cli

