#include "FireRedSaveStructure.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace firered {

namespace {
void CheckRange(std::size_t size, std::size_t offset, std::size_t length) {
    if (offset > size || length > size - offset) {
        throw std::out_of_range("bounded save access exceeded input size");
    }
}
} // namespace

std::uint16_t ReadLe16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    CheckRange(bytes.size(), offset, 2);
    return static_cast<std::uint16_t>(bytes[offset])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

std::uint32_t ReadLe32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    CheckRange(bytes.size(), offset, 4);
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

void WriteLe16(std::span<std::uint8_t> bytes, std::size_t offset, std::uint16_t value) {
    CheckRange(bytes.size(), offset, 2);
    bytes[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFFU);
}

void WriteLe32(std::span<std::uint8_t> bytes, std::size_t offset, std::uint32_t value) {
    CheckRange(bytes.size(), offset, 4);
    for (std::size_t i = 0; i < 4; ++i) {
        bytes[offset + i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xFFU);
    }
}

std::span<const std::uint8_t> StandardFlash(std::span<const std::uint8_t> image) {
    if (image.size() < Layout::kStandardFlashSize) {
        throw std::invalid_argument("FireRed save is smaller than the standard 128 KiB flash image");
    }
    return image.first(Layout::kStandardFlashSize);
}

std::string HexOffset(std::size_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << value;
    return out.str();
}

} // namespace firered
