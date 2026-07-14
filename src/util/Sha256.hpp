#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pkmn::cli::util {
std::string Sha256Hex(const std::vector<std::uint8_t>& bytes);
}  // namespace pkmn::cli::util
