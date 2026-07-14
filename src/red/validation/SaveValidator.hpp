#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "red/save/RedSave.hpp"

namespace pkmn::cli::red::validation {

struct ChecksumStatus {
    std::uint8_t expected = 0;
    std::uint8_t stored = 0;

    [[nodiscard]] bool Valid() const noexcept { return expected == stored; }
};

struct ValidationReport {
    std::size_t actualSize = 0;
    bool expectedSize = false;
    ChecksumStatus main;
    std::array<ChecksumStatus, 2> banks;
    std::array<ChecksumStatus, 12> boxes;

    [[nodiscard]] bool ChecksumsValid() const noexcept;
    [[nodiscard]] bool Valid() const noexcept { return expectedSize && ChecksumsValid(); }
};

class SaveValidator {
public:
    static constexpr std::size_t MainStart = 0x2598;
    static constexpr std::size_t MainEnd = 0x3522;
    static constexpr std::size_t MainStored = 0x3523;
    static constexpr std::size_t BoxBlockSize = 0x0462;
    static constexpr std::array<std::size_t, 2> BankStarts = {0x4000, 0x6000};
    static constexpr std::array<std::size_t, 2> BankStored = {0x5A4C, 0x7A4C};
    static constexpr std::array<std::size_t, 2> BoxChecksumTables = {0x5A4D, 0x7A4D};

    [[nodiscard]] static ValidationReport Validate(const save::RedSave& input);
    [[nodiscard]] static std::uint8_t ComputeInvertedSum(
        const save::RedSave& input, std::size_t start, std::size_t endInclusive);
    [[nodiscard]] static std::size_t BoxOffset(std::size_t indexZeroBased);
};

}  // namespace pkmn::cli::red::validation
