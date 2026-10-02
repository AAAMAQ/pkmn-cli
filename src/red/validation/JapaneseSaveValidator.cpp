#include "red/validation/JapaneseSaveValidator.hpp"

namespace pkmn::cli::red::validation {

std::size_t JapaneseSaveValidator::BoxOffset(std::size_t indexZeroBased) {
    if (indexZeroBased >= 8)
        throw std::out_of_range("Japanese Red box index must be 0..7");
    return BankStarts[indexZeroBased / 4] +
           (indexZeroBased % 4) * BoxBlockSize;
}

JapaneseValidationReport JapaneseSaveValidator::Validate(const save::RedSave& input) {
    JapaneseValidationReport report;
    report.actualSize = input.Size();
    report.expectedSize = input.Size() >= save::RedSave::ExpectedSize;
    if (!report.expectedSize) {
        report.errors.push_back("Japanese Red save is shorter than 0x8000 bytes");
        return report;
    }
    report.main = {SaveValidator::ComputeInvertedSum(input, MainStart, MainEnd),
                   input.At(MainStored)};
    if (!report.main.Valid())
        report.errors.push_back("Japanese Red main checksum is invalid");
    // The source's bank checksum writer includes the previous checksum byte;
    // this comparison is diagnostic only, never an acceptance condition.
    for (std::size_t bank = 0; bank < 2; ++bank) {
        report.bankDiagnostics[bank] = {
            SaveValidator::ComputeInvertedSum(input, BankStarts[bank],
                                               BankStored[bank] - 1),
            input.At(BankStored[bank])};
    }
    const auto party = input.At(0x2ED5);
    if (party != 0xFF && party > 6)
        report.errors.push_back("Japanese Red party count exceeds six");
    else if (party != 0xFF && input.At(0x2ED6 + party) != 0xFF)
        report.errors.push_back("Japanese Red party species list lacks its terminator");
    const auto selected = input.At(0x2842) & 0x7F;
    if (selected >= 8)
        report.errors.push_back("Japanese Red selected box exceeds eight boxes");
    const auto checkBox = [&](std::size_t base, const std::string& label) {
        const auto count = input.At(base);
        if (count != 0xFF && count > 30)
            report.errors.push_back(label + " count exceeds 30");
        else if (count != 0xFF && input.At(base + 1 + count) != 0xFF)
            report.errors.push_back(label + " species list lacks its terminator");
    };
    checkBox(0x302D, "Japanese Red current box cache");
    for (std::size_t index = 0; index < 8; ++index)
        checkBox(BoxOffset(index), "Japanese Red box " + std::to_string(index + 1));
    return report;
}

}  // namespace pkmn::cli::red::validation
