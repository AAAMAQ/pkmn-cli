#pragma once

#include <cstdint>
#include <span>

namespace firered {

std::uint16_t CalculateSectionChecksum(std::span<const std::uint8_t> data);

} // namespace firered
