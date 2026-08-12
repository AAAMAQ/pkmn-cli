#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace firered {

std::string Sha256Hex(std::span<const std::uint8_t> bytes);

} // namespace firered
