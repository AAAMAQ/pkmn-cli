#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"

namespace pkmn::cli::red::validation {

struct JapaneseValidationReport {
    std::size_t actualSize = 0;
    bool expectedSize = false;
    ChecksumStatus main;
    std::array<ChecksumStatus, 2> bankDiagnostics;
    std::vector<std::string> errors;

    [[nodiscard]] bool Valid() const noexcept {
        return expectedSize && main.Valid() && errors.empty();
    }
};

class JapaneseSaveValidator {
public:
    static constexpr std::size_t MainStart = 0x2598;
    static constexpr std::size_t MainEnd = 0x3593;
    static constexpr std::size_t MainStored = 0x3594;
    static constexpr std::size_t BoxBlockSize = 0x566;
    static constexpr std::array<std::size_t, 2> BankStarts = {0x4000, 0x6000};
    static constexpr std::array<std::size_t, 2> BankStored = {0x5598, 0x7598};

    [[nodiscard]] static std::size_t BoxOffset(std::size_t indexZeroBased);
    [[nodiscard]] static JapaneseValidationReport Validate(const save::RedSave& input);
};

}  // namespace pkmn::cli::red::validation
