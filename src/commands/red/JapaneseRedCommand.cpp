#include "commands/red/JapaneseRedCommand.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "app/ExitCode.hpp"
#include "commands/conversion/ConversionCommand.hpp"
#include "red/json/JapaneseRedDocument.hpp"
#include "red/json/RedDecoder.hpp"
#include "red/json/RedJsonDocument.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/JapaneseSaveValidator.hpp"
#include "util/AtomicOutput.hpp"

namespace pkmn::cli::commands::red {
namespace {

using Json = pkmn::cli::red::json::OrderedJson;

std::filesystem::path Base(const std::filesystem::path& path,
                           const std::string& suffix) {
    auto name = path.filename().string();
    if (name.ends_with(suffix)) name.resize(name.size() - suffix.size());
    else name = path.stem().string();
    return path.parent_path() / name;
}

std::string Profile(const std::vector<std::string>& arguments,
                    std::size_t start) {
    for (std::size_t i = start; i < arguments.size(); ++i)
        if (arguments[i] == "--profile" && i + 1 < arguments.size()) {
            const auto& profile = arguments[i + 1];
            if (profile == "JP_RED_REV0" || profile == "JP_RED_REV1" || profile == "JP_GREEN_REV0")
                return profile;
            throw std::runtime_error("profile must be JP_RED_REV0 or JP_RED_REV1");
        }
    throw std::runtime_error("declare --profile JP_RED_REV0 or JP_RED_REV1");
}

Json ReadJson(const std::filesystem::path& path) {
    std::ifstream stream(path);
    if (!stream) throw std::runtime_error("could not open Japanese Red JSON");
    Json document;
    stream >> document;
    return document;
}

Json ValidationJson(const pkmn::cli::red::validation::JapaneseValidationReport& report, bool green) {
    Json banks = Json::array();
    for (std::size_t i = 0; i < report.bankDiagnostics.size(); ++i)
        banks.push_back({{"bank", i + 2},
                         {"stored", report.bankDiagnostics[i].stored},
                         {"simpleCalculated", report.bankDiagnostics[i].expected},
                         {"acceptanceRole", "diagnostic-only"}});
    return {{"format", green ? "pkmn-green-jp-validation" : "pkmn-red-jp-validation"},
            {"valid", report.Valid()},
            {"size", report.actualSize},
            {"minimumSize", pkmn::cli::red::save::RedSave::ExpectedSize},
            {"mainChecksum", {{"valid", report.main.Valid()},
                                {"stored", report.main.stored},
                                {"calculated", report.main.expected}}},
            {"externalBankChecksums", banks},
            {"errors", report.errors}};
}

struct InputOptions {
    std::filesystem::path input;
    std::filesystem::path output;
    std::filesystem::path templatePath;
    std::string profile;
    std::string salt;
    bool includePhysicalImage = true;
    bool json = false;
    bool retainPlayerName = false;
};

InputOptions ParseSaveOptions(const std::vector<std::string>& arguments,
                              bool convert) {
    if (arguments.size() < 2)
        throw std::runtime_error("input Japanese Red save is required");
    InputOptions result;
    result.input = arguments[1];
    for (std::size_t i = 2; i < arguments.size(); ++i) {
        const auto& option = arguments[i];
        if (option == "--profile" && i + 1 < arguments.size()) {
            result.profile = arguments[++i];
            if (result.profile != "JP_RED_REV0" &&
                result.profile != "JP_RED_REV1" && result.profile != "JP_GREEN_REV0")
                throw std::runtime_error("unsupported Japanese Red profile");
        } else if (option == "--output" && i + 1 < arguments.size())
            result.output = arguments[++i];
        else if (convert && option == "--template" && i + 1 < arguments.size())
            result.templatePath = arguments[++i];
        else if (convert && option == "--salt" && i + 1 < arguments.size())
            result.salt = arguments[++i];
        else if (convert && option == "--retain-playername")
            result.retainPlayerName = true;
        else if (!convert && option == "--no-physical-image")
            result.includePhysicalImage = false;
        else if (!convert && option == "--format" && i + 1 < arguments.size() &&
                 arguments[i + 1] == "json") {
            result.json = true;
            ++i;
        } else
            throw std::runtime_error("invalid Japanese Red option: " + option);
    }
    if (result.profile.empty()) result.profile = Profile(arguments, 2);
    return result;
}

void RequireArchive(const Json& document) {
    const auto report = pkmn::cli::red::json::ValidateJapaneseDocument(document);
    if (!report.Valid())
        throw std::runtime_error("invalid Japanese Red archive: " + report.errors.front());
}

}  // namespace

int RunJapaneseRed(const std::vector<std::string>& arguments,
                   std::ostream& output, std::ostream& error, bool green) {
    if (green && (arguments.empty() || arguments.front()=="help" || arguments.front()=="--help")) {
        output << "Usage: pkmn green-jp validate|decode|convert <save.sav> --profile JP_GREEN_REV0 [--output <file>]\nExperimental Green 1.0; conversion targets international FireRed.\n";
        return 0;
    }
    if (arguments.empty() || arguments.front() == "help" ||
        arguments.front() == "--help") {
        output << "Usage:\n"
               << "  pkmn red-jp validate <save.sav> --profile JP_RED_REV0|JP_RED_REV1 [--format json]\n"
               << "  pkmn red-jp decode <save.sav> --profile JP_RED_REV0|JP_RED_REV1 [--output <file.red.jp.json>] [--no-physical-image]\n"
               << "  pkmn red-jp convert <save.sav> --profile JP_RED_REV0|JP_RED_REV1 [--output <file_fr.sav>] [--template <clean.sav>] [--salt <value>] [--retain-playername]\n";
        return 0;
    }
    try {
        const auto action = arguments.front();
        if (action != "validate" && action != "decode" && action != "convert")
            throw std::runtime_error("unknown Japanese Red action");
        const auto options = ParseSaveOptions(arguments, action == "convert");
        if (green != (options.profile == "JP_GREEN_REV0"))
            throw std::runtime_error("source profile does not match the selected Japanese game");
        const auto save = pkmn::cli::red::save::RedSave::Read(options.input);
        const auto report = pkmn::cli::red::validation::JapaneseSaveValidator::Validate(save);
        if (action == "validate") {
            if (options.json) output << ValidationJson(report, green).dump(2) << '\n';
            else output << (green ? "Japanese Green main checksum: " : "Japanese Red main checksum: ")
                        << (report.main.Valid() ? "valid" : "invalid") << '\n'
                        << "Save layout: " << (report.Valid() ? "valid" : "invalid") << '\n';
            return report.Valid() ? 0 : ToInt(ExitCode::InvalidInput);
        }
        if (!report.Valid())
            throw std::runtime_error("Japanese Red save validation failed: " +
                                     report.errors.front());
        const auto archive = pkmn::cli::red::json::DecodeJapanese(
            save, options.input.filename().string(), report, options.profile,
            {.includePhysicalImage = action == "convert" || options.includePhysicalImage});
        const auto archivePath = Base(options.input, options.input.extension().string())
                                     .string() + (green ? ".green.jp.json" : ".red.jp.json");
        if (action == "decode") {
            const auto target = options.output.empty()
                ? std::filesystem::path(archivePath) : options.output;
            pkmn::cli::util::WriteTextAtomic(target,
                                             pkmn::cli::red::json::Serialize(archive));
            output << (green ? "Japanese Green archive written: " : "Japanese Red archive written: ") << target.string() << '\n';
            return 0;
        }
        const auto projection = pkmn::cli::red::json::ProjectJapanese(
            archive, options.retainPlayerName);
        const auto englishReport =
            pkmn::cli::red::json::ValidateDocument(projection.document);
        if (!englishReport.Valid())
            throw std::runtime_error("Japanese bridge projection failed English validation: " +
                                     englishReport.errors.front());
        const auto projectionPath = Base(options.input, options.input.extension().string())
                                        .string() + ".red.json";
        pkmn::cli::util::OutputTransaction transaction;
        transaction.StageText(archivePath, pkmn::cli::red::json::Serialize(archive));
        transaction.StageText(projectionPath,
                              pkmn::cli::red::json::Serialize(projection.document));
        transaction.Commit();
        std::vector<std::string> forward = {"convert", projectionPath};
        if (!options.output.empty()) {
            forward.push_back("--output");
            forward.push_back(options.output.string());
        }
        if (!options.templatePath.empty()) {
            forward.push_back("--template");
            forward.push_back(options.templatePath.string());
        }
        if (!options.salt.empty()) {
            forward.push_back("--salt");
            forward.push_back(options.salt);
        }
        return conversion::RunRjsonExtension(forward, output, error);
    } catch (const std::exception& exception) {
        error << (green ? "pkmn green-jp: " : "pkmn red-jp: ") << exception.what() << '\n';
        return ToInt(ExitCode::InvalidInput);
    }
}

int RunJapaneseJson(const std::vector<std::string>& arguments,
                    std::ostream& output, std::ostream& error, bool green) {
    const auto readArchive = [green](const std::filesystem::path& path) {
        auto archive = ReadJson(path);
        const auto profile = archive.at("source").at("profile").get<std::string>();
        if (green != (profile == "JP_GREEN_REV0"))
            throw std::runtime_error("archive profile does not match the selected Japanese game");
        return archive;
    };
    if (green && (arguments.empty() || arguments.front()=="help" || arguments.front()=="--help")) {
        output << "Usage: pkmn gjpjson inspect|validate|reconstruct|project|compare <file.green.jp.json> [options]\nExperimental Green 1.0 archive tools.\n";
        return 0;
    }
    if (arguments.empty() || arguments.front() == "help" ||
        arguments.front() == "--help") {
        output << "Usage:\n"
               << "  pkmn rjpjson inspect|validate <file.red.jp.json>\n"
               << "  pkmn rjpjson reconstruct <file.red.jp.json> [--output <save.sav>]\n"
               << "  pkmn rjpjson project <file.red.jp.json> [--output <file.red.json>] [--retain-playername]\n"
               << "  pkmn rjpjson compare <file.red.jp.json> <file.red.json>\n";
        return 0;
    }
    try {
        if (arguments.size() < 2)
            throw std::runtime_error("Japanese archive input is required");
        const auto action = arguments.front();
        const std::filesystem::path input = arguments[1];
        if (action == "compare") {
            if (arguments.size() != 3)
                throw std::runtime_error("compare requires an archive and projection");
            const auto archive = readArchive(input);
            RequireArchive(archive);
            const auto actual = ReadJson(arguments[2]);
            const bool retainPlayerName = actual.contains("sourceJapanese") &&
                actual.at("sourceJapanese").value("targetPlayerNamePolicy", "english-fallback") ==
                    "retain-japanese-raw-experimental";
            const auto expected = pkmn::cli::red::json::ProjectJapanese(
                archive, retainPlayerName);
            const auto actualValidation = pkmn::cli::red::json::ValidateDocument(actual);
            const bool matches = actualValidation.Valid() && actual == expected.document;
            output << Json({{"format", green ? "pkmn-green-jp-projection-comparison" : "pkmn-red-jp-projection-comparison"},
                            {"exactProjectionMatch", matches},
                            {"projectionValid", actualValidation.Valid()},
                            {"partyCount", expected.document.at("decoded").at("party").at("count")},
                            {"pcPokemonCount", expected.document.at("sourceJapanese").at("pcPokemonCount")},
                            {"slotMappings", expected.mapping.size()},
                            {"selectedBoxPermanentDiffers", expected.document.at("sourceJapanese").at("selectedBoxPermanentDiffers")},
                            {"errors", actualValidation.errors}}).dump(2) << '\n';
            return matches ? 0 : ToInt(ExitCode::InvalidInput);
        }
        std::filesystem::path destination;
        bool retainPlayerName = false;
        for (std::size_t i = 2; i < arguments.size(); ++i) {
            if (arguments[i] == "--output" && i + 1 < arguments.size())
                destination = arguments[++i];
            else if (action == "project" && arguments[i] == "--retain-playername")
                retainPlayerName = true;
            else throw std::runtime_error("invalid Japanese JSON option");
        }
        const auto archive = readArchive(input);
        const auto report = pkmn::cli::red::json::ValidateJapaneseDocument(archive);
        if (action == "inspect" || action == "validate") {
            output << Json({{"valid", report.Valid()}, {"errors", report.errors},
                            {"warnings", report.warnings},
                            {"schema", archive.value("schema", Json::object())},
                            {"partyCount", archive.at("decoded").at("party").at("count")},
                            {"boxCount", archive.at("decoded").at("pcStorage").at("boxes").size()},
                            {"hasPhysicalImage", archive.contains("physicalImage")}}).dump(2)
                   << '\n';
            return report.Valid() ? 0 : ToInt(ExitCode::InvalidInput);
        }
        RequireArchive(archive);
        if (action == "reconstruct") {
            if (!archive.contains("physicalImage"))
                throw std::runtime_error("archive has no physical image");
            if (destination.empty()) destination = Base(input, green ? ".green.jp.json" : ".red.jp.json")
                                                   .string() + "_reconstructed.sav";
            const auto bytes = pkmn::cli::red::json::JapanesePhysicalBytes(archive);
            pkmn::cli::util::WriteBinaryAtomic(destination, bytes);
            output << (green ? "Japanese Green save reconstructed: " : "Japanese Red save reconstructed: ") << destination.string() << '\n';
            return 0;
        }
        if (action == "project") {
            if (destination.empty())
                destination = Base(input, green ? ".green.jp.json" : ".red.jp.json").string() + ".red.json";
            const auto projected = pkmn::cli::red::json::ProjectJapanese(
                archive, retainPlayerName);
            const auto validation =
                pkmn::cli::red::json::ValidateDocument(projected.document);
            if (!validation.Valid())
                throw std::runtime_error("English projection invalid: " +
                                         validation.errors.front());
            pkmn::cli::util::WriteTextAtomic(
                destination, pkmn::cli::red::json::Serialize(projected.document));
            output << "English-shaped Red projection written: "
                   << destination.string() << '\n';
            return 0;
        }
        throw std::runtime_error("unknown Japanese JSON action");
    } catch (const std::exception& exception) {
        error << (green ? "pkmn gjpjson: " : "pkmn rjpjson: ") << exception.what() << '\n';
        return ToInt(ExitCode::InvalidInput);
    }
}

}  // namespace pkmn::cli::commands::red
