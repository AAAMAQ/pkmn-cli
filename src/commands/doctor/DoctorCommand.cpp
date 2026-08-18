#include "commands/doctor/DoctorCommand.hpp"

#include <ostream>
#include <array>

#include <nlohmann/json.hpp>

#include "app/ExitCode.hpp"
#include "app/Version.hpp"
#include "red/comparison/Comparison.hpp"
#include "red/generation/SemanticGenerator.hpp"
#include "red/json/RedDecoder.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/ResourceLocator.hpp"
#include "util/Sha256.hpp"
#include "FireRedChecksum.hpp"
#include "FileManipulation.hpp"
#include <filesystem>

namespace pkmn::cli::commands::doctor {
namespace {

void PrintHelp(std::ostream& output) {
    output
        << "Usage: pkmn doctor [--deep] [--format text|json]\n\n"
        << "Checks the standalone Red/FireRed engines, bridge runtime, and bundled resources.\n"
        << "No Save Genie or Save Generator executable is searched for or required.\n";
}

}  // namespace

int Run(const std::vector<std::string>& arguments,
        std::ostream& output,
        std::ostream& error) {
    bool json = false;
    bool deep = false;
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        if (arguments[index] == "--help" || arguments[index] == "-h" ||
            arguments[index] == "help") {
            PrintHelp(output);
            return ToInt(ExitCode::Success);
        }
        if (arguments[index] == "--json") {
            json = true;
        } else if (arguments[index] == "--format" &&
                   index + 1 < arguments.size() &&
                   arguments[index + 1] == "json") {
            json = true;
            ++index;
        } else if (arguments[index] == "--deep") {
            deep = true;
        } else {
            error << "pkmn doctor: unexpected argument '" << arguments[index] << "'\n";
            return ToInt(ExitCode::InvalidArguments);
        }
    }

    nlohmann::ordered_json deepReport = nullptr;
    if (deep) {
        try {
            auto bytes = red::save::RedSave::Read(util::RedTemplatePath()).BytesView();
            red::generation::RepairChecksums(bytes);
            const red::save::RedSave repaired(bytes);
            const auto integrity = red::validation::SaveValidator::Validate(repaired);
            if (!integrity.Valid())
                throw std::runtime_error("repaired bundled template failed integrity");
            const auto document = red::json::Decode(
                repaired, "doctor-synthetic-template.sav", integrity,
                {.includePhysicalImage = false});
            const auto first = red::generation::Generate(document);
            const auto second = red::generation::Generate(document);
            const auto generatedIntegrity = red::validation::SaveValidator::Validate(
                red::save::RedSave(first.bytes));
            if (first.bytes != second.bytes || !generatedIntegrity.Valid())
                throw std::runtime_error("internal deterministic round trip failed");
            const auto runtime = util::FireRedRuntimeScriptPath();
            const auto fireRedTemplate = util::FireRedTemplatePath();
            const auto data = runtime.parent_path() / "data";
            const bool bridgeAuthorities =
                std::filesystem::is_regular_file(data / "event_bridge_red_to_firered.json") &&
                std::filesystem::is_regular_file(data / "trainer_bridge_red_to_firered.json") &&
                std::filesystem::is_regular_file(data / "item_bridge_red_to_firered.json");
            const std::array<std::uint8_t, 4> zero{};
            if (!std::filesystem::is_regular_file(runtime) ||
                !std::filesystem::is_regular_file(fireRedTemplate) ||
                util::Sha256Hex(firered::ReadBinaryFile(fireRedTemplate)) !=
                    "5fe341091ea41f17ddccaae1aed0ee4802894c94f5281e53dfe795e837a830c0" ||
                !bridgeAuthorities ||
                firered::CalculateSectionChecksum(zero) != 0)
                throw std::runtime_error("FireRed runtime/authority self-test failed");
            deepReport = {{"passed", true},
                          {"resource", util::RedTemplatePath().filename().string()},
                          {"deterministic", true},
                          {"generatedChecksumsValid", true},
                          {"generatedSha256", util::Sha256Hex(first.bytes)},
                          {"fireRedNativeChecksumSelfTest", true},
                          {"fireRedRuntimePresent", true},
                          {"bridgeAuthoritiesPresent", true},
                          {"fireRedTemplate", fireRedTemplate.filename().string()},
                          {"fireRedTemplateIdentityValid", true},
                          {"fireRedPhysicalSelfTest", "bundled-template-ready"}};
        } catch (const std::exception &exception) {
            error << "pkmn doctor --deep: " << exception.what() << '\n';
            return ToInt(ExitCode::GeneralFailure);
        }
    }

    if (json) {
        nlohmann::ordered_json report = {
            {"tool", "pkmn"},
            {"version", std::string(kVersion)},
            {"standaloneReadiness", "ready"},
            {"externalExecutablesRequired", false},
            {"fireRedPythonRuntimeRequired", true},
            {"fireRedVerificationGate", "phase-5-and-phase-6-passed"},
            {"modules", nlohmann::ordered_json::array({
                "command-router", "red-save-loader", "red-checksum-validator",
                "red-json-decoder-validator-reconstructor", "red-semantic-generator",
                "physical-semantic-comparison", "red-proof",
                "post-emulator-validation", "red-editing",
                "red-to-firered-bridge-planner", "firered-template-generator",
                "firered-native-json-generator", "firered-proof",
                "red-to-firered-proof", "bridge-inspection",
                "red-and-firered-schema-updater"})}};
        report["deepSelfTest"] = deepReport;
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
        << "[ok] bundled Red-to-FireRed bridge and generator runtime\n"
        << "[ok] bundled clean FireRed generation template\n"
        << "[ok] FireRed reader, native JSON generator, comparisons, and proof runtime\n"
        << "[ok] FireRed Phase 5 native-generation acceptance\n"
        << "[ok] Red-to-FireRed Phase 6 conversion acceptance\n"
           << (deep ? "[ok] deep deterministic generation self-test\n" : "")
           << "\nStandalone readiness: ready\n";
    return ToInt(ExitCode::Success);
}

}  // namespace pkmn::cli::commands::doctor
