#include "commands/red/RedCommand.hpp"

#include <filesystem>
#include <ostream>
#include <stdexcept>

#include "app/ExitCode.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"

namespace pkmn::cli::commands::red {
namespace {
void PrintHelp(std::ostream& output) {
    output << "Usage:\n"
           << "  pkmn red decode <input.sav>     (JSON export migration pending)\n"
           << "  pkmn red inspect <input.sav>\n"
           << "  pkmn red validate <input.sav>\n";
}

void PrintReport(const pkmn::cli::red::validation::ValidationReport& report, std::ostream& output) {
    output << "Size: " << report.actualSize << " bytes (expected "
           << pkmn::cli::red::save::RedSave::ExpectedSize << "): "
           << (report.expectedSize ? "ok" : "invalid") << '\n';
    if (!report.expectedSize) return;
    output << "Main checksum: " << (report.main.Valid() ? "valid" : "invalid") << '\n'
           << "Bank 2 all-checksum: " << (report.banks[0].Valid() ? "valid" : "invalid") << '\n'
           << "Bank 3 all-checksum: " << (report.banks[1].Valid() ? "valid" : "invalid") << '\n';
    output << "Box checksums:";
    for (std::size_t index = 0; index < report.boxes.size(); ++index)
        output << ' ' << (index + 1) << '=' << (report.boxes[index].Valid() ? "valid" : "invalid");
    output << '\n';
}
}

int Run(const std::vector<std::string>& arguments, std::ostream& output, std::ostream& error) {
    if (arguments.empty() || arguments.front() == "help" || arguments.front() == "--help" ||
        arguments.front() == "-h") {
        PrintHelp(output);
        output << "\nInspect and validate use the internal Red engine. Decode remains pending the canonical JSON-export migration.\n";
        return ToInt(ExitCode::Success);
    }
    if (arguments.front() == "decode" && arguments.size() == 2) {
        error << "pkmn red decode: unsupported until the internal canonical .red.json export slice is complete; no external Save Genie executable is required or invoked.\n";
        return ToInt(ExitCode::UnsupportedOperation);
    }
    if ((arguments.front() == "inspect" || arguments.front() == "validate") &&
        arguments.size() == 2) {
        try {
            const auto input = pkmn::cli::red::save::RedSave::Read(arguments[1]);
            const auto report = pkmn::cli::red::validation::SaveValidator::Validate(input);
            output << "Red save: " << std::filesystem::path(arguments[1]).filename().string() << '\n';
            PrintReport(report, output);
            if (arguments.front() == "inspect")
                return report.expectedSize ? ToInt(ExitCode::Success) : ToInt(ExitCode::InvalidInput);
            output << "Validation: " << (report.Valid() ? "passed" : "failed") << '\n';
            if (!report.expectedSize) return ToInt(ExitCode::InvalidInput);
            return report.Valid() ? ToInt(ExitCode::Success) : ToInt(ExitCode::ChecksumFailure);
        } catch (const std::exception& exception) {
            error << "pkmn red " << arguments.front() << ": " << exception.what() << '\n';
            return ToInt(ExitCode::InvalidInput);
        }
    }
    error << "pkmn red: invalid or unsupported arguments\n";
    PrintHelp(error);
    return ToInt(ExitCode::InvalidArguments);
}

}  // namespace pkmn::cli::commands::red
