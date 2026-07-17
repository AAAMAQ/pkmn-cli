#include "app/CommandRouter.hpp"

#include <algorithm>
#include <array>
#include <ostream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <string_view>

#include "app/CommandCatalog.hpp"
#include "app/ExitCode.hpp"
#include "app/Version.hpp"
#include "commands/catalog/CatalogCommand.hpp"
#include "commands/completion/CompletionCommand.hpp"
#include "commands/config/ConfigCommand.hpp"
#include "commands/doctor/DoctorCommand.hpp"
#include "commands/compare/CompareCommand.hpp"
#include "commands/proof/ProofCommand.hpp"
#include "commands/red/RedCommand.hpp"
#include "commands/rjson/RjsonCommand.hpp"

namespace pkmn::cli {
namespace {

constexpr std::array<std::string_view, 7> kPlannedDomains = {
    "red", "rjson", "proof", "compare", "fred", "frjson", "convert"};

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
    static thread_local bool environmentApplied = false;
    if (!environmentApplied &&
        (arguments.empty() ||
         (arguments.front() != "--quiet" && arguments.front() != "--verbose" &&
          arguments.front() != "--no-color"))) {
        std::vector<std::string> configured;
        if (const char *quiet = std::getenv("PKMN_QUIET");
            quiet != nullptr && std::string_view(quiet) == "1")
            configured.push_back("--quiet");
        if (const char *verbose = std::getenv("PKMN_VERBOSE");
            verbose != nullptr && std::string_view(verbose) == "1")
            configured.push_back("--verbose");
        if (std::getenv("NO_COLOR") != nullptr)
            configured.push_back("--no-color");
        if (!configured.empty()) {
            configured.insert(configured.end(), arguments.begin(), arguments.end());
            environmentApplied = true;
            const int result = Run(configured, output, error);
            environmentApplied = false;
            return result;
        }
    }
    if (!arguments.empty() &&
        (arguments.front() == "--quiet" || arguments.front() == "--verbose" ||
         arguments.front() == "--no-color")) {
        bool quiet = false;
        bool verbose = false;
        std::size_t begin = 0;
        while (begin < arguments.size()) {
            if (arguments[begin] == "--quiet") quiet = true;
            else if (arguments[begin] == "--verbose") verbose = true;
            else if (arguments[begin] != "--no-color") break;
            ++begin;
        }
        if (quiet && verbose) {
            error << "pkmn: --quiet and --verbose cannot be combined\n";
            return ToInt(ExitCode::InvalidArguments);
        }
        const std::vector<std::string> stripped(arguments.begin() + begin,
                                                arguments.end());
        if (quiet) {
            std::ostringstream discarded;
            return Run(stripped, discarded, error);
        }
        if (verbose)
            output << "[pkmn] version=" << kVersion
                   << " standalone=true color=false\n";
        return Run(stripped, output, error);
    }
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
    if (arguments.front() == "completion") {
        return commands::completion::Run(
            {arguments.begin() + 1, arguments.end()}, output, error);
    }
    if (arguments.front() == "config") {
        return commands::config::Run(
            {arguments.begin() + 1, arguments.end()}, output, error);
    }
    if (arguments.front() == "get-all-cmds") {
        return commands::catalog::Run(
            {arguments.begin() + 1, arguments.end()}, output, error);
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
        << "Global controls (before the command): --quiet, --verbose, --no-color\n\n"
        << "Available command endpoints (" << AvailableCommands().size()
        << "):\n";
    std::string_view category;
    for (const auto &command : AvailableCommands()) {
        if (command.category != category) {
            category = command.category;
            output << "\n" << category << ":\n";
        }
        output << "  " << std::left << std::setw(28) << command.path
               << command.description << '\n';
    }
    output
        << "\nUse 'pkmn get-all-cmds' for complete usage lines and descriptions.\n"
        << "Use 'pkmn get-all-cmds --format markdown --output commands.md' "
           "to save them.\n\n"
        << "Reserved/planned command domains:\n"
        << "  fred, frjson         Future FireRed workflows (not implemented)\n"
        << "  convert              Future Red-to-FireRed conversion (not implemented)\n\n"
        << "Core safety model:\n"
        << "  generate             Semantic generation; physicalImage is never authority\n"
        << "  reconstruct          Archival reconstruction; physicalImage is required\n"
        << "  edit                 Writes a validated copy; never overwrites input by default\n\n"
        << "Examples:\n"
        << "  pkmn red summary savefile.sav\n"
        << "  pkmn red validate savefile.sav\n"
        << "  pkmn red decode savefile.sav\n"
        << "  pkmn compare progress older.sav newer.sav\n"
        << "  pkmn get-all-cmds\n"
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
