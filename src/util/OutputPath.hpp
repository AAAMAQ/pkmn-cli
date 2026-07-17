#pragma once

#include <cstddef>
#include <filesystem>
#include <string_view>

namespace pkmn::cli::util {

std::filesystem::path NumberedOutputPath(
    const std::filesystem::path &preferred, std::size_t number,
    std::string_view compoundExtension = {});

} // namespace pkmn::cli::util
