#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "app/CommandRouter.hpp"
#include "app/ExitCode.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"

namespace {

int failures = 0;

void Expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

struct Result {
    int code;
    std::string output;
    std::string error;
};

Result Run(std::vector<std::string> arguments) {
    std::ostringstream output;
    std::ostringstream error;
    const int code = pkmn::cli::CommandRouter{}.Run(arguments, output, error);
    return {code, output.str(), error.str()};
}

std::uint8_t InvertedSum(const std::vector<std::uint8_t>& bytes,
                         std::size_t start,
                         std::size_t endInclusive) {
    std::uint32_t sum = 0;
    for (std::size_t index = start; index <= endInclusive; ++index) sum += bytes[index];
    return static_cast<std::uint8_t>(~static_cast<std::uint8_t>(sum & 0xFFU));
}

std::vector<std::uint8_t> ValidSyntheticSave() {
    using Validator = pkmn::cli::red::validation::SaveValidator;
    std::vector<std::uint8_t> bytes(pkmn::cli::red::save::RedSave::ExpectedSize, 0);
    bytes[Validator::MainStored] = InvertedSum(bytes, Validator::MainStart, Validator::MainEnd);
    for (std::size_t bank = 0; bank < 2; ++bank) {
        bytes[Validator::BankStored[bank]] =
            InvertedSum(bytes, Validator::BankStarts[bank], Validator::BankStored[bank] - 1);
    }
    for (std::size_t box = 0; box < 12; ++box) {
        const std::size_t bank = box / 6;
        const std::size_t withinBank = box % 6;
        const std::size_t start = Validator::BoxOffset(box);
        bytes[Validator::BoxChecksumTables[bank] + withinBank] =
            InvertedSum(bytes, start, start + Validator::BoxBlockSize - 1);
    }
    return bytes;
}

}  // namespace

int main() {
    const auto noArguments = Run({});
    Expect(noArguments.code == 0, "no arguments should show help successfully");
    Expect(noArguments.output.find("Usage:") != std::string::npos, "help should contain Usage");

    const auto help = Run({"--help"});
    Expect(help.code == 0, "--help should succeed");
    Expect(help.output.find("physicalImage") != std::string::npos,
           "help should state the physicalImage safety boundary");

    const auto version = Run({"--version"});
    Expect(version.code == 0, "--version should succeed");
    Expect(version.output == "pkmn 0.1.0\n", "version output should be stable");

    const auto unknown = Run({"wat"});
    Expect(unknown.code == pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidArguments),
           "unknown command should use invalid-arguments exit code");

    const auto planned = Run({"red", "decode", "sample.sav"});
    Expect(planned.code == pkmn::cli::ToInt(pkmn::cli::ExitCode::UnsupportedOperation),
           "planned domain should fail honestly as unsupported");
    Expect(planned.error.find("internal canonical .red.json export") != std::string::npos,
           "Red decode placeholder should explain the internal JSON-export migration gate");

    const auto pendingRjson = Run({"rjson", "generate"});
    Expect(pendingRjson.code == pkmn::cli::ToInt(pkmn::cli::ExitCode::UnsupportedOperation),
           "rjson should remain an honest internal-workflow placeholder");

    const auto future = Run({"fred", "decode", "sample.sav"});
    Expect(future.error.find("emulator-proven") != std::string::npos,
           "FireRed placeholder should state its verification gate");

    const auto doctorHelp = Run({"doctor", "--help"});
    Expect(doctorHelp.code == 0, "doctor help should succeed without dependencies");
    Expect(doctorHelp.output.find("No Save Genie or Save Generator") != std::string::npos,
           "doctor help should state that external executables are not required");
    const auto doctor = Run({"doctor"});
    Expect(doctor.code == 0 && doctor.output.find("Standalone readiness: ready") != std::string::npos,
           "doctor should report standalone readiness without external tools");

    namespace fs = std::filesystem;
    const fs::path temp = fs::temp_directory_path() / "pkmn-cli-red-foundation-tests";
    fs::remove_all(temp);
    fs::create_directories(temp);

    const fs::path validSavePath = temp / "synthetic-valid.sav";
    auto validSave = ValidSyntheticSave();
    {
        std::ofstream saveFile(validSavePath, std::ios::binary);
        saveFile.write(reinterpret_cast<const char*>(validSave.data()),
                       static_cast<std::streamsize>(validSave.size()));
    }
    const pkmn::cli::red::save::RedSave loaded =
        pkmn::cli::red::save::RedSave::Read(validSavePath);
    const auto validation = pkmn::cli::red::validation::SaveValidator::Validate(loaded);
    Expect(validation.Valid(), "internal Red validator should accept a synthetic valid save");
    Expect(validation.boxes.size() == 12, "internal Red validator should check all 12 boxes");

    for (std::size_t bank = 0; bank < 2; ++bank) {
        auto corrupted = ValidSyntheticSave();
        corrupted[pkmn::cli::red::validation::SaveValidator::BankStored[bank]] ^= 0x01U;
        const auto report = pkmn::cli::red::validation::SaveValidator::Validate(
            pkmn::cli::red::save::RedSave(std::move(corrupted)));
        Expect(!report.banks[bank].Valid(), "internal validator should detect each bank checksum corruption");
    }
    for (std::size_t box = 0; box < 12; ++box) {
        auto corrupted = ValidSyntheticSave();
        const std::size_t bank = box / 6;
        const std::size_t withinBank = box % 6;
        corrupted[pkmn::cli::red::validation::SaveValidator::BoxChecksumTables[bank] +
                  withinBank] ^= 0x01U;
        const auto report = pkmn::cli::red::validation::SaveValidator::Validate(
            pkmn::cli::red::save::RedSave(std::move(corrupted)));
        Expect(!report.boxes[box].Valid(), "internal validator should detect every box checksum corruption");
    }

    const auto standaloneInspect = Run({"red", "inspect", validSavePath.string()});
    const auto standaloneValidate = Run({"red", "validate", validSavePath.string()});
    Expect(standaloneInspect.code == 0 && standaloneInspect.output.find("Main checksum: valid") !=
               std::string::npos,
           "red inspect should use the internal engine when helper executables are absent");
    Expect(standaloneValidate.code == 0 && standaloneValidate.output.find("Validation: passed") !=
               std::string::npos,
           "red validate should use the internal engine when helper executables are absent");

    validSave[pkmn::cli::red::validation::SaveValidator::MainStored] ^= 0x01U;
    const fs::path corruptSavePath = temp / "synthetic-corrupt.sav";
    {
        std::ofstream saveFile(corruptSavePath, std::ios::binary);
        saveFile.write(reinterpret_cast<const char*>(validSave.data()),
                       static_cast<std::streamsize>(validSave.size()));
    }
    const auto corruptValidate = Run({"red", "validate", corruptSavePath.string()});
    Expect(corruptValidate.code == pkmn::cli::ToInt(pkmn::cli::ExitCode::ChecksumFailure),
           "red validate should map checksum corruption to checksum failure");
    Expect(corruptValidate.output.find("Main checksum: invalid") != std::string::npos,
           "red validate should identify the corrupted checksum");

    const auto missingSave = Run({"red", "validate", (temp / "missing.sav").string()});
    Expect(missingSave.code == pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidInput),
           "red validate should map an unreadable input to invalid input");
    const auto wrongSize = pkmn::cli::red::validation::SaveValidator::Validate(
        pkmn::cli::red::save::RedSave(std::vector<std::uint8_t>(100, 0)));
    Expect(!wrongSize.expectedSize && !wrongSize.Valid(),
           "internal validator should reject non-32-KiB saves before checksum access");

    bool boundsRejected = false;
    try {
        static_cast<void>(loaded.At(loaded.Size()));
    } catch (const std::out_of_range&) {
        boundsRejected = true;
    }
    Expect(boundsRejected, "internal Red save access should reject out-of-bounds reads");
    fs::remove_all(temp);

    if (failures == 0) {
        std::cout << "All command-router tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
