#include "app/CommandRouter.hpp"

#include <algorithm>
#include <array>
#include <ostream>
#include <string_view>

#include "app/ExitCode.hpp"
#include "app/Version.hpp"
#include "commands/doctor/DoctorCommand.hpp"
#include "commands/compare/CompareCommand.hpp"
#include "commands/proof/ProofCommand.hpp"
#include "commands/red/RedCommand.hpp"
#include "commands/rjson/RjsonCommand.hpp"

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
    if (arguments.front() == "compare") {
        return commands::compare::Run({arguments.begin() + 1, arguments.end()}, output, error);
    }
    if (arguments.front() == "proof") {
        return commands::proof::Run({arguments.begin() + 1, arguments.end()}, output, error);
    }

    if (arguments.front() == "red") {
        return commands::red::Run({arguments.begin() + 1, arguments.end()}, output, error);
    }
    if (arguments.front() == "rjson") {
        return commands::rjson::Run({arguments.begin() + 1, arguments.end()}, output, error);
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
        << "  doctor               Check standalone internal-engine readiness\n"
        << "  red inspect          Inspect save integrity using the internal Red engine\n"
        << "  red validate         Validate all known Red save checksums internally\n"
        << "  red validate-post-emulator  Validate emulator round-trip drift\n"
        << "  red decode           Export canonical .red.json internally\n"
        << "  red edit             Interactive validated copy editing\n"
        << "  red begin-edit       Start a scriptable multi-edit session\n"
        << "  rjson inspect        Inspect canonical Red JSON internally\n"
        << "  rjson validate       Validate canonical Red JSON internally\n"
        << "  rjson generate       Generate a Red save from semantic fields\n"
        << "  rjson reconstruct    Restore an archival physical image\n"
        << "\n"
        << "  compare physical     Compare save bytes and difference ranges\n"
        << "  compare semantic     Compare canonical Red semantic JSON\n"
        << "  proof red            Run the internal Red proof pipeline\n"
        << "  proof post-emulator  Continue proof after manual emulator testing\n"
        << "\nReserved/planned command domains:\n"
        << "  config               Future local CLI configuration\n"
        << "  fred, frjson         Future FireRed workflows (not implemented)\n"
        << "  convert              Future Red-to-FireRed conversion (not implemented)\n\n"
        << "Core safety model:\n"
        << "  generate             Semantic generation; physicalImage is never authority\n"
        << "  reconstruct          Archival reconstruction; physicalImage is required\n"
        << "  edit                 Writes a validated copy; never overwrites input by default\n\n"
        << "Examples:\n"
        << "  pkmn red decode savefile.sav\n"
        << "  pkmn red inspect savefile.sav\n"
        << "  pkmn red validate savefile.sav\n"
        << "  pkmn doctor\n";
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
