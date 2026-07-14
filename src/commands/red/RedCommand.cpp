#include "commands/red/RedCommand.hpp"

#include <filesystem>
#include <fstream>
#include <ostream>
#include <stdexcept>

#include "app/ExitCode.hpp"
#include "commands/red/EditCommand.hpp"
#include "red/json/RedDecoder.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"

namespace pkmn::cli::commands::red {
namespace {
void PrintHelp(std::ostream& output) {
    output << "Usage:\n"
           << "  pkmn red decode <input.sav> [--output <file.red.json>]\n"
           << "                       [--include-physical-image|--no-physical-image]\n"
           << "  pkmn red inspect <input.sav>\n"
           << "  pkmn red validate <input.sav>\n"
           << "  pkmn red edit <input.sav>\n"
           << "  pkmn red begin-edit|edit-session|pending-edits|validate-edit|end-edit ...\n";
}

void PrintReport(const pkmn::cli::red::validation::ValidationReport& report, std::ostream& output) {
    output << "Size: " << report.actualSize << " bytes (minimum standard SRAM "
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

std::filesystem::path DefaultJsonPath(const std::filesystem::path& input) {
    auto output = input;
    if (output.extension() == ".sav" || output.extension() == ".srm") output.replace_extension();
    output += ".red.json";
    return output;
}

int RunDecode(const std::vector<std::string>& arguments,
              std::ostream& output, std::ostream& error) {
    if (arguments.size() < 2) return ToInt(ExitCode::InvalidArguments);
    const std::filesystem::path inputPath = arguments[1];
    auto outputPath = DefaultJsonPath(inputPath);
    bool includePhysical = true;
    bool physicalChoiceSeen = false;
    for (std::size_t index = 2; index < arguments.size(); ++index) {
        if (arguments[index] == "--output" && index + 1 < arguments.size()) {
            outputPath = arguments[++index];
        } else if (arguments[index] == "--include-physical-image" && !physicalChoiceSeen) {
            includePhysical = true;
            physicalChoiceSeen = true;
        } else if (arguments[index] == "--no-physical-image" && !physicalChoiceSeen) {
            includePhysical = false;
            physicalChoiceSeen = true;
        } else {
            error << "pkmn red decode: invalid or conflicting option '" << arguments[index] << "'\n";
            return ToInt(ExitCode::InvalidArguments);
        }
    }
    if (std::filesystem::exists(outputPath)) {
        error << "pkmn red decode: refusing to overwrite existing output: "
              << outputPath.filename().string() << '\n';
        return ToInt(ExitCode::OutputFailure);
    }
    try {
        const auto input = pkmn::cli::red::save::RedSave::Read(inputPath);
        const auto report = pkmn::cli::red::validation::SaveValidator::Validate(input);
        if (!report.expectedSize) {
            error << "pkmn red decode: input is smaller than the 32 KiB Red SRAM image\n";
            return ToInt(ExitCode::InvalidInput);
        }
        if (!report.Valid()) {
            error << "pkmn red decode: checksum validation failed; refusing canonical export\n";
            return ToInt(ExitCode::ChecksumFailure);
        }
        const auto document = pkmn::cli::red::json::Decode(
            input, inputPath.filename().string(), report, {includePhysical});
        std::ofstream file(outputPath, std::ios::binary | std::ios::trunc);
        if (!file) throw std::runtime_error("could not create output file");
        const auto serialized = pkmn::cli::red::json::Serialize(document);
        file.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
        if (!file) throw std::runtime_error("could not write complete output file");
        output << "Decoded Pokemon Red save\n"
               << "Output: " << outputPath.filename().string() << '\n'
               << "Physical image: " << (includePhysical ? "included" : "excluded") << '\n';
        return ToInt(ExitCode::Success);
    } catch (const std::exception& exception) {
        error << "pkmn red decode: " << exception.what() << '\n';
        return ToInt(ExitCode::InvalidInput);
    }
}
}

int Run(const std::vector<std::string>& arguments, std::ostream& output, std::ostream& error) {
    if (arguments.empty() || arguments.front() == "help" || arguments.front() == "--help" ||
        arguments.front() == "-h") {
        PrintHelp(output);
        output << "\nAll listed commands use the internal Red engine.\n";
        return ToInt(ExitCode::Success);
    }
    if (arguments.front() == "decode") return RunDecode(arguments, output, error);
    if (arguments.front() == "edit" || arguments.front() == "begin-edit" ||
        arguments.front() == "edit-session" || arguments.front() == "pending-edits" ||
        arguments.front() == "validate-edit" || arguments.front() == "end-edit")
        return edit::Run(arguments, output, error);
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
