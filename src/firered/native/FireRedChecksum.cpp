#include "FireRedChecksum.hpp"

#include <stdexcept>

namespace firered {

std::uint16_t CalculateSectionChecksum(std::span<const std::uint8_t> data) {
    if (data.size() % 4 != 0) {
        throw std::invalid_argument("FireRed section checksum range must be divisible by four");
    }

    std::uint32_t checksum = 0;
    for (std::size_t i = 0; i < data.size(); i += 4) {
        const auto word = static_cast<std::uint32_t>(data[i])
            | (static_cast<std::uint32_t>(data[i + 1]) << 8)
            | (static_cast<std::uint32_t>(data[i + 2]) << 16)
            | (static_cast<std::uint32_t>(data[i + 3]) << 24);
        checksum += word;
    }
    return static_cast<std::uint16_t>((checksum >> 16) + (checksum & 0xFFFFU));
}

} // namespace firered
