#pragma once

namespace pkmn::cli {

enum class ExitCode : int {
    Success = 0,
    GeneralFailure = 1,
    InvalidArguments = 2,
    InvalidInput = 3,
    ChecksumFailure = 4,
    GenerationFailure = 5,
    PhysicalImageIsolationFailure = 6,
    DeterminismFailure = 7,
    SemanticMismatch = 8,
    OutputFailure = 9,
    PostEmulatorValidationFailure = 10,
    EditValidationFailure = 11,
    UnsupportedOperation = 12,
};

constexpr int ToInt(ExitCode code) noexcept {
    return static_cast<int>(code);
}

}  // namespace pkmn::cli

