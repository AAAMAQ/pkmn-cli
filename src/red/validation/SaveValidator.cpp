#include "red/validation/SaveValidator.hpp"

#include <algorithm>
#include <stdexcept>

namespace pkmn::cli::red::validation {

// Adapted from the MIT-licensed Pkmn Red Save Genie Gen1Checksum rules,
// copyright 2026 MAQ / BiG MAQ Studios.
bool ValidationReport::ChecksumsValid() const noexcept {
    return main.Valid() &&
           std::all_of(banks.begin(), banks.end(), [](const auto& item) { return item.Valid(); }) &&
           std::all_of(boxes.begin(), boxes.end(), [](const auto& item) { return item.Valid(); });
}

std::uint8_t SaveValidator::ComputeInvertedSum(const save::RedSave& input,
                                               std::size_t start,
                                               std::size_t endInclusive) {
    if (start > endInclusive || endInclusive >= input.Size())
        throw std::out_of_range("checksum range is outside the Red save");
    std::uint32_t sum = 0;
    for (std::size_t index = start; index <= endInclusive; ++index) sum += input.At(index);
    return static_cast<std::uint8_t>(~static_cast<std::uint8_t>(sum & 0xFFU));
}

std::size_t SaveValidator::BoxOffset(std::size_t indexZeroBased) {
    if (indexZeroBased >= 12) throw std::out_of_range("Red box index must be 0..11");
    const std::size_t bank = indexZeroBased / 6;
    const std::size_t withinBank = indexZeroBased % 6;
    return BankStarts[bank] + withinBank * BoxBlockSize;
}

ValidationReport SaveValidator::Validate(const save::RedSave& input) {
    ValidationReport report;
    report.actualSize = input.Size();
    report.expectedSize = input.Size() >= save::RedSave::ExpectedSize;
    if (!report.expectedSize) return report;

    report.main = {ComputeInvertedSum(input, MainStart, MainEnd), input.At(MainStored)};
    for (std::size_t bank = 0; bank < report.banks.size(); ++bank) {
        report.banks[bank] = {
            ComputeInvertedSum(input, BankStarts[bank], BankStored[bank] - 1),
            input.At(BankStored[bank])};
    }
    for (std::size_t box = 0; box < report.boxes.size(); ++box) {
        const std::size_t bank = box / 6;
        const std::size_t withinBank = box % 6;
        const std::size_t start = BoxOffset(box);
        report.boxes[box] = {
            ComputeInvertedSum(input, start, start + BoxBlockSize - 1),
            input.At(BoxChecksumTables[bank] + withinBank)};
    }
    return report;
}

}  // namespace pkmn::cli::red::validation
