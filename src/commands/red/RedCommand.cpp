#include "commands/red/RedCommand.hpp"

#include <filesystem>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <set>

#include "app/ExitCode.hpp"
#include "commands/proof/ProofCommand.hpp"
#include "commands/red/EditCommand.hpp"
#include "red/json/RedDecoder.hpp"
#include "red/events/EventCatalog.hpp"
#include "red/generation/SemanticGenerator.hpp"
#include "red/reporting/SaveSummary.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/OutputPath.hpp"
#include "util/AtomicOutput.hpp"
#include "util/Sha256.hpp"

namespace pkmn::cli::commands::red {
namespace {
void PrintHelp(std::ostream& output) {
    output << "Usage:\n"
           << "  pkmn red summary <input.sav> [--format text|json|markdown] "
              "[--output <file>]\n"
           << "  pkmn red decode <input.sav> [--output <file.red.json>]\n"
           << "                       [--include-physical-image|--no-physical-image] "
              "[--auto-suffix]\n"
           << "  pkmn red inspect <input.sav> [--format json]\n"
           << "  pkmn red validate <input.sav> [--format json]\n"
           << "  pkmn red repair-checksums <input.sav> [--output <copy.sav>] "
              "[--auto-suffix]\n"
           << "  pkmn red events list [--category <category>] [--format json]\n"
           << "  pkmn red events search <query> [--format json]\n"
           << "  pkmn red events show <EVENT_NAME> [--format json]\n"
           << "  pkmn red validate-batch <save.sav>... [--format json]\n"
           << "  pkmn red decode-batch <save.sav>... --output-dir <directory> "
              "[--no-physical-image]\n"
           << "  pkmn red validate-post-emulator <before.sav> <after.sav> "
              "[--output-dir <directory>]\n"
           << "  pkmn red edit <input.sav>\n"
           << "  pkmn red begin-edit <input.sav> [--output <session.json>]\n"
           << "  pkmn red edit-session <session.json> <edits...>\n"
           << "  pkmn red pokemon <session.json> <selector> <value> "
              "rename|level|move ...\n"
           << "  pkmn red bag <session.json> add|remove <item> [quantity]\n"
           << "  pkmn red progress <session.json> fly-destinations all\n"
           << "  pkmn red pending-edits <session.json> [--format json]\n"
           << "  pkmn red undo-edit <session.json> [--count <number>]\n"
           << "  pkmn red edit-history <session.json> [--format json]\n"
           << "  pkmn red annotate-edit <session.json> <note>\n"
           << "  pkmn red validate-edit <session.json>\n"
           << "  pkmn red end-edit <session.json> [--output <save.sav>]\n";
}

int RunSummary(const std::vector<std::string> &arguments,
               std::ostream &output, std::ostream &error) {
    if (arguments.size() < 2) {
        error << "pkmn red summary: input save is required\n";
        return ToInt(ExitCode::InvalidArguments);
    }
    const std::filesystem::path inputPath = arguments[1];
    std::filesystem::path outputPath;
    std::string format = "text";
    for (std::size_t index = 2; index < arguments.size(); ++index) {
        if (arguments[index] == "--format" && index + 1 < arguments.size())
            format = arguments[++index];
        else if (arguments[index] == "--output" && index + 1 < arguments.size())
            outputPath = arguments[++index];
        else {
            error << "pkmn red summary: invalid option\n";
            return ToInt(ExitCode::InvalidArguments);
        }
    }
    if (format != "text" && format != "json" && format != "markdown") {
        error << "pkmn red summary: format must be text, json, or markdown\n";
        return ToInt(ExitCode::InvalidArguments);
    }
    try {
        const auto save = pkmn::cli::red::save::RedSave::Read(inputPath);
        const auto integrity =
            pkmn::cli::red::validation::SaveValidator::Validate(save);
        if (!integrity.expectedSize)
            return ToInt(ExitCode::InvalidInput);
        if (!integrity.Valid()) {
            error << "pkmn red summary: checksum validation failed\n";
            return ToInt(ExitCode::ChecksumFailure);
        }
        const auto document = pkmn::cli::red::json::Decode(
            save, inputPath.filename().string(), integrity,
            {.includePhysicalImage = false});
        const auto summary =
            pkmn::cli::red::reporting::BuildSaveSummary(document);
        const auto contents =
            format == "json"
                ? summary.dump(2) + '\n'
                : (format == "markdown"
                       ? pkmn::cli::red::reporting::SaveSummaryMarkdown(summary)
                       : pkmn::cli::red::reporting::SaveSummaryText(summary));
        if (outputPath.empty() || outputPath == "-")
            output << contents;
        else {
            util::WriteTextAtomic(outputPath, contents);
            output << "Readable save summary written: "
                   << outputPath.filename().string() << '\n';
        }
        return 0;
    } catch (const std::exception &exception) {
        error << "pkmn red summary: " << exception.what() << '\n';
        return std::string(exception.what()).find("overwrite") != std::string::npos
                   ? ToInt(ExitCode::OutputFailure)
                   : ToInt(ExitCode::InvalidInput);
    }
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

pkmn::cli::red::json::OrderedJson ReportJson(
    const pkmn::cli::red::validation::ValidationReport& report,
    const std::string& logicalName) {
    auto boxes = pkmn::cli::red::json::OrderedJson::array();
    for (std::size_t index = 0; index < report.boxes.size(); ++index)
        boxes.push_back({{"box", index + 1},
                         {"valid", report.boxes[index].Valid()},
                         {"stored", report.boxes[index].stored},
                         {"calculated", report.boxes[index].expected}});
    return {{"format", "pkmn-red-validation"},
            {"reportVersion", "1.0.0"},
            {"command", "red validate"},
            {"logicalName", logicalName},
            {"size", report.actualSize},
            {"expectedSize", report.expectedSize},
            {"valid", report.Valid()},
            {"mainChecksumValid", report.expectedSize && report.main.Valid()},
            {"bank2ChecksumValid", report.expectedSize && report.banks[0].Valid()},
            {"bank3ChecksumValid", report.expectedSize && report.banks[1].Valid()},
            {"boxChecksums", boxes}};
}

std::filesystem::path DefaultJsonPath(const std::filesystem::path& input) {
    auto output = input;
    if (output.extension() == ".sav" || output.extension() == ".srm") output.replace_extension();
    output += ".red.json";
    return output;
}

std::filesystem::path DefaultRepairPath(const std::filesystem::path &input) {
    return input.parent_path() /
           (input.stem().string() + "_repaired-checksums.sav");
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    return value;
}

pkmn::cli::red::json::OrderedJson EventRecord(
    const pkmn::cli::red::events::EventDefinition &definition) {
    return {{"flagIndex", definition.flagIndex},
            {"name", definition.name},
            {"label", pkmn::cli::red::events::Label(definition.name)},
            {"category", pkmn::cli::red::events::Category(definition.name)},
            {"trainerBattle",
             pkmn::cli::red::events::IsTrainerBattle(definition.name)},
            {"staticEncounter",
             pkmn::cli::red::events::IsStaticEncounter(definition.name)},
            {"majorStory",
             pkmn::cli::red::events::IsMajorStory(definition.name)}};
}

int RunEvents(const std::vector<std::string> &arguments,
              std::ostream &output, std::ostream &error) {
    if (arguments.size() < 2 ||
        (arguments[1] != "list" && arguments[1] != "search" &&
         arguments[1] != "show")) {
        error << "pkmn red events: expected list, search, or show\n";
        return ToInt(ExitCode::InvalidArguments);
    }
    const auto action = arguments[1];
    std::string query;
    std::string category;
    bool json = false;
    std::size_t index = 2;
    if (action != "list") {
        if (index >= arguments.size()) {
            error << "pkmn red events " << action << ": a query is required\n";
            return ToInt(ExitCode::InvalidArguments);
        }
        query = arguments[index++];
    }
    for (; index < arguments.size(); ++index) {
        if (arguments[index] == "--format" && index + 1 < arguments.size() &&
            arguments[index + 1] == "json") {
            json = true;
            ++index;
        } else if (action == "list" && arguments[index] == "--category" &&
                   index + 1 < arguments.size()) {
            category = arguments[++index];
        } else {
            error << "pkmn red events: invalid option\n";
            return ToInt(ExitCode::InvalidArguments);
        }
    }
    auto records = pkmn::cli::red::json::OrderedJson::array();
    const auto queryLower = Lower(query);
    for (const auto &definition : pkmn::cli::red::events::Catalog()) {
        const auto record = EventRecord(definition);
        if (!category.empty() && record.at("category") != category)
            continue;
        if (action == "search") {
            const auto searchable = Lower(std::string(definition.name) + " " +
                                          record.at("label").get<std::string>());
            if (searchable.find(queryLower) == std::string::npos)
                continue;
        }
        if (action == "show" && definition.name != query)
            continue;
        records.push_back(record);
    }
    if (action == "show" && records.empty()) {
        error << "pkmn red events show: unknown verified event name\n";
        return ToInt(ExitCode::InvalidInput);
    }
    if (json) {
        output << pkmn::cli::red::json::OrderedJson(
                      {{"catalog", "pokemon-red-usa-europe-v1"},
                       {"verifiedCount", pkmn::cli::red::events::Catalog().size()},
                       {"resultCount", records.size()}, {"records", records}})
                      .dump(2)
               << '\n';
    } else {
        output << "Verified Pokemon Red events: " << records.size() << '\n';
        for (const auto &record : records)
            output << record.at("flagIndex") << "  "
                   << record.at("name").get<std::string>() << "  ["
                   << record.at("category").get<std::string>() << "]  "
                   << record.at("label").get<std::string>() << '\n';
    }
    return 0;
}

int RunRepair(const std::vector<std::string> &arguments,
              std::ostream &output, std::ostream &error) {
    if (arguments.size() < 2)
        return ToInt(ExitCode::InvalidArguments);
    const std::filesystem::path inputPath = arguments[1];
    auto outputPath = DefaultRepairPath(inputPath);
    bool autoSuffix = false;
    for (std::size_t index = 2; index < arguments.size(); ++index) {
        if (arguments[index] == "--output" && index + 1 < arguments.size())
            outputPath = arguments[++index];
        else if (arguments[index] == "--auto-suffix")
            autoSuffix = true;
        else
            return ToInt(ExitCode::InvalidArguments);
    }
    auto reportPath = outputPath;
    reportPath += ".repair-report.json";
    auto available = [](const auto &save, const auto &report) {
        return !std::filesystem::exists(save) && !std::filesystem::exists(report);
    };
    const auto preferred = outputPath;
    if (autoSuffix && !available(outputPath, reportPath))
        for (std::size_t number = 2;; ++number) {
            outputPath = util::NumberedOutputPath(preferred, number);
            reportPath = outputPath.string() + ".repair-report.json";
            if (available(outputPath, reportPath)) break;
        }
    if (!available(outputPath, reportPath)) {
        error << "pkmn red repair-checksums: refusing existing output/report\n";
        return ToInt(ExitCode::OutputFailure);
    }
    try {
        const auto source = pkmn::cli::red::save::RedSave::Read(inputPath);
        const auto before = pkmn::cli::red::validation::SaveValidator::Validate(source);
        if (!before.expectedSize)
            throw std::runtime_error("input is smaller than standard Red SRAM");
        auto bytes = source.BytesView();
        pkmn::cli::red::generation::RepairChecksums(bytes);
        const auto repaired = pkmn::cli::red::save::RedSave(bytes);
        const auto after = pkmn::cli::red::validation::SaveValidator::Validate(repaired);
        if (!after.Valid())
            throw std::runtime_error("repaired copy failed checksum validation");
        auto changed = pkmn::cli::red::json::OrderedJson::array();
        for (std::size_t offset = 0; offset < bytes.size(); ++offset)
            if (bytes[offset] != source.At(offset))
                changed.push_back(offset);
        const auto report = pkmn::cli::red::json::OrderedJson{
            {"workflow", "copy-first-checksum-repair"},
            {"sourceLogicalName", inputPath.filename().string()},
            {"sourceSha256", util::Sha256Hex(source.BytesView())},
            {"outputSha256", util::Sha256Hex(bytes)},
            {"beforeValid", before.Valid()}, {"afterValid", after.Valid()},
            {"changedOffsets", changed}, {"sourceOverwritten", false}};
        util::OutputTransaction transaction;
        transaction.StageBinary(outputPath, bytes);
        transaction.StageText(reportPath, report.dump(2) + '\n');
        transaction.Commit();
        output << "Checksum repair copy created\nOutput: "
               << outputPath.filename().string() << "\nChanged checksum bytes: "
               << changed.size() << "\nSource overwritten: no\n";
        return 0;
    } catch (const std::exception &exception) {
        error << "pkmn red repair-checksums: " << exception.what() << '\n';
        return ToInt(ExitCode::ChecksumFailure);
    }
}

int RunValidateBatch(const std::vector<std::string> &arguments,
                     std::ostream &output, std::ostream &error) {
    bool json = false;
    std::vector<std::filesystem::path> inputs;
    for (std::size_t index = 1; index < arguments.size(); ++index) {
        if (arguments[index] == "--format" && index + 1 < arguments.size() &&
            arguments[index + 1] == "json") {
            json = true;
            ++index;
        } else if (arguments[index].starts_with("--")) {
            error << "pkmn red validate-batch: invalid option\n";
            return ToInt(ExitCode::InvalidArguments);
        } else
            inputs.emplace_back(arguments[index]);
    }
    if (inputs.empty()) return ToInt(ExitCode::InvalidArguments);
    auto results = pkmn::cli::red::json::OrderedJson::array();
    std::size_t valid = 0;
    bool unreadable = false;
    for (const auto &input : inputs) {
        try {
            const auto save = pkmn::cli::red::save::RedSave::Read(input);
            const auto report = pkmn::cli::red::validation::SaveValidator::Validate(save);
            auto row = ReportJson(report, input.filename().string());
            row["path"] = input.generic_string();
            results.push_back(row);
            valid += report.Valid();
        } catch (const std::exception &exception) {
            unreadable = true;
            results.push_back({{"path", input.generic_string()},
                               {"logicalName", input.filename().string()},
                               {"valid", false}, {"error", exception.what()}});
        }
    }
    const auto report = pkmn::cli::red::json::OrderedJson{
        {"command", "red validate-batch"}, {"inputCount", inputs.size()},
        {"validCount", valid}, {"invalidCount", inputs.size() - valid},
        {"results", results}};
    if (json)
        output << report.dump(2) << '\n';
    else {
        output << "Pokemon Red batch validation\nValid: " << valid << '/'
               << inputs.size() << '\n';
        for (const auto &row : results)
            output << (row.value("valid", false) ? "[ok] " : "[fail] ")
                   << row.at("logicalName").get<std::string>() << '\n';
    }
    if (valid == inputs.size()) return 0;
    return unreadable ? ToInt(ExitCode::InvalidInput)
                      : ToInt(ExitCode::ChecksumFailure);
}

int RunDecodeBatch(const std::vector<std::string> &arguments,
                   std::ostream &output, std::ostream &error) {
    std::vector<std::filesystem::path> inputs;
    std::filesystem::path directory;
    bool physical = true;
    for (std::size_t index = 1; index < arguments.size(); ++index) {
        if (arguments[index] == "--output-dir" && index + 1 < arguments.size())
            directory = arguments[++index];
        else if (arguments[index] == "--no-physical-image")
            physical = false;
        else if (arguments[index] == "--include-physical-image")
            physical = true;
        else if (arguments[index].starts_with("--"))
            return ToInt(ExitCode::InvalidArguments);
        else
            inputs.emplace_back(arguments[index]);
    }
    if (inputs.empty() || directory.empty())
        return ToInt(ExitCode::InvalidArguments);
    if (std::filesystem::exists(directory)) {
        error << "pkmn red decode-batch: refusing existing output directory\n";
        return ToInt(ExitCode::OutputFailure);
    }
    try {
        util::DirectoryTransaction transaction(directory);
        std::set<std::string> names;
        auto rows = pkmn::cli::red::json::OrderedJson::array();
        for (const auto &input : inputs) {
            auto name = input.stem().string() + ".red.json";
            if (!names.insert(name).second)
                throw std::runtime_error("batch inputs produce duplicate output names");
            const auto save = pkmn::cli::red::save::RedSave::Read(input);
            const auto integrity =
                pkmn::cli::red::validation::SaveValidator::Validate(save);
            if (!integrity.Valid())
                throw std::runtime_error(input.filename().string() +
                                         " failed checksum validation");
            const auto document = pkmn::cli::red::json::Decode(
                save, input.filename().string(), integrity,
                {.includePhysicalImage = physical});
            util::WriteTextAtomic(transaction.Path() / name,
                                  pkmn::cli::red::json::Serialize(document));
            rows.push_back({{"input", input.filename().string()}, {"output", name},
                            {"sourceSha256", util::Sha256Hex(save.BytesView())}});
        }
        util::WriteTextAtomic(
            transaction.Path() / "batch-manifest.json",
            pkmn::cli::red::json::OrderedJson(
                {{"command", "red decode-batch"}, {"count", inputs.size()},
                 {"physicalImageIncluded", physical}, {"results", rows}}).dump(2) +
                '\n');
        transaction.Commit();
        output << "Batch decode complete\nOutput directory: "
               << directory.filename().string() << "\nFiles: " << inputs.size()
               << '\n';
        return 0;
    } catch (const std::exception &exception) {
        error << "pkmn red decode-batch: " << exception.what() << '\n';
        return ToInt(ExitCode::InvalidInput);
    }
}

int RunDecode(const std::vector<std::string>& arguments,
              std::ostream& output, std::ostream& error) {
    if (arguments.size() < 2) return ToInt(ExitCode::InvalidArguments);
    const std::filesystem::path inputPath = arguments[1];
    auto outputPath = DefaultJsonPath(inputPath);
    bool includePhysical = true;
    bool physicalChoiceSeen = false;
    bool autoSuffix = false;
    for (std::size_t index = 2; index < arguments.size(); ++index) {
        if (arguments[index] == "--output" && index + 1 < arguments.size()) {
            outputPath = arguments[++index];
        } else if (arguments[index] == "--include-physical-image" && !physicalChoiceSeen) {
            includePhysical = true;
            physicalChoiceSeen = true;
        } else if (arguments[index] == "--no-physical-image" && !physicalChoiceSeen) {
            includePhysical = false;
            physicalChoiceSeen = true;
        } else if (arguments[index] == "--auto-suffix") {
            autoSuffix = true;
        } else {
            error << "pkmn red decode: invalid or conflicting option '" << arguments[index] << "'\n";
            return ToInt(ExitCode::InvalidArguments);
        }
    }
    const bool stdoutOutput = outputPath == std::filesystem::path("-");
    const auto preferredOutputPath = outputPath;
    if (!stdoutOutput && autoSuffix && std::filesystem::exists(outputPath)) {
        std::size_t number = 2;
        do {
            outputPath = util::NumberedOutputPath(preferredOutputPath,
                                                  number++, ".red.json");
        } while (std::filesystem::exists(outputPath));
    }
    if (!stdoutOutput && std::filesystem::exists(outputPath)) {
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
        const auto serialized = pkmn::cli::red::json::Serialize(document);
        if (stdoutOutput) {
            output << serialized;
            return ToInt(ExitCode::Success);
        }
        util::WriteTextAtomic(outputPath, serialized);
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
    if (arguments.front() == "summary")
        return RunSummary(arguments, output, error);
    if (arguments.front() == "decode") return RunDecode(arguments, output, error);
    if (arguments.front() == "events") return RunEvents(arguments, output, error);
    if (arguments.front() == "repair-checksums")
        return RunRepair(arguments, output, error);
    if (arguments.front() == "validate-batch")
        return RunValidateBatch(arguments, output, error);
    if (arguments.front() == "decode-batch")
        return RunDecodeBatch(arguments, output, error);
    if (arguments.front() == "validate-post-emulator" &&
        (arguments.size() == 3 ||
         (arguments.size() == 5 && arguments[3] == "--output-dir"))) {
        std::vector<std::string> proofArguments = {
            "--before", arguments[1], "--after", arguments[2]};
        if (arguments.size() == 5) {
            proofArguments.push_back("--output-dir");
            proofArguments.push_back(arguments[4]);
        }
        return commands::proof::RunPostEmulator(proofArguments, output, error);
    }
    if (arguments.front() == "edit" || arguments.front() == "begin-edit" ||
        arguments.front() == "edit-session" || arguments.front() == "pending-edits" ||
        arguments.front() == "pokemon" || arguments.front() == "bag" ||
        arguments.front() == "progress" ||
        arguments.front() == "undo-edit" || arguments.front() == "edit-history" ||
        arguments.front() == "annotate-edit" ||
        arguments.front() == "validate-edit" || arguments.front() == "end-edit")
        return edit::Run(arguments, output, error);
    if ((arguments.front() == "inspect" || arguments.front() == "validate") &&
        (arguments.size() == 2 ||
         (arguments.size() == 4 && arguments[2] == "--format" &&
          arguments[3] == "json"))) {
        try {
            const auto input = pkmn::cli::red::save::RedSave::Read(arguments[1]);
            const auto report = pkmn::cli::red::validation::SaveValidator::Validate(input);
            const auto logicalName =
                std::filesystem::path(arguments[1]).filename().string();
            const bool json = arguments.size() == 4;
            if (json)
                output << ReportJson(report, logicalName).dump(2) << '\n';
            else {
                output << "Red save: " << logicalName << '\n';
                PrintReport(report, output);
            }
            if (arguments.front() == "inspect")
                return report.expectedSize ? ToInt(ExitCode::Success) : ToInt(ExitCode::InvalidInput);
            if (!json)
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
