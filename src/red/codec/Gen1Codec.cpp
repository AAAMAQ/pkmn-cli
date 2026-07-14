#include "red/codec/Gen1Codec.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace pkmn::cli::red::codec {
namespace {

std::string Unknown(std::uint8_t value) {
    std::ostringstream output;
    output << "<0x" << std::uppercase << std::hex << std::setw(2)
           << std::setfill('0') << static_cast<int>(value) << '>';
    return output.str();
}

std::string Token(std::uint8_t value, bool lossless) {
    if (value >= 0x80 && value <= 0x99) return {static_cast<char>('A' + value - 0x80)};
    if (value >= 0xA0 && value <= 0xB9) return {static_cast<char>('a' + value - 0xA0)};
    if (value >= 0xF6) return {static_cast<char>('0' + value - 0xF6)};
    switch (value) {
        case 0x5B: return lossless ? "<PC>" : "PC";
        case 0x5C: return lossless ? "<TM>" : "TM";
        case 0x5D: return lossless ? "<TRAINER>" : "TRAINER";
        case 0x7F: return " ";
        case 0x9A: return "(";
        case 0x9B: return ")";
        case 0x9C: return ":";
        case 0x9D: return ";";
        case 0xBA: return "é";
        case 0xE0: return "'";
        case 0xE3: return "-";
        case 0xE6: return "?";
        case 0xE7: return "!";
        case 0xE8: return lossless ? "<PERIOD>" : ".";
        case 0xEF: return "♂";
        case 0xF0: return "¥";
        case 0xF1: return "×";
        case 0xF2: return lossless ? "<DOT>" : ".";
        case 0xF3: return "/";
        case 0xF4: return ",";
        case 0xF5: return "♀";
        default: return Unknown(value);
    }
}

}  // namespace

std::string DecodeText(const save::RedSave& input, std::size_t offset,
                       std::size_t length, bool lossless) {
    std::string result;
    for (const auto value : input.Slice(offset, length)) {
        if (value == 0x50) break;
        result += Token(value, lossless);
    }
    return result;
}

std::uint16_t ReadU16BE(const save::RedSave& input, std::size_t offset) {
    return static_cast<std::uint16_t>((input.At(offset) << 8U) | input.At(offset + 1));
}

std::uint32_t ReadU24BE(const save::RedSave& input, std::size_t offset) {
    return (static_cast<std::uint32_t>(input.At(offset)) << 16U) |
           (static_cast<std::uint32_t>(input.At(offset + 1)) << 8U) |
           input.At(offset + 2);
}

std::uint32_t ReadBcd(const save::RedSave& input, std::size_t offset,
                      std::size_t length) {
    std::uint32_t value = 0;
    for (const auto byte : input.Slice(offset, length))
        value = value * 100U + ((byte >> 4U) & 0x0FU) * 10U + (byte & 0x0FU);
    return value;
}

std::string Hex(const save::RedSave::Bytes& bytes, bool uppercase) {
    std::ostringstream output;
    if (uppercase) output << std::uppercase;
    output << std::hex << std::setfill('0');
    for (const auto byte : bytes) output << std::setw(2) << static_cast<int>(byte);
    return output.str();
}

std::size_t CountSetBits(const save::RedSave& input, std::size_t offset,
                         std::size_t length, std::size_t usedBits) {
    std::size_t count = 0;
    const auto bytes = input.Slice(offset, length);
    for (std::size_t bit = 0; bit < std::min(usedBits, length * 8U); ++bit)
        if ((bytes[bit / 8U] & (1U << (bit % 8U))) != 0) ++count;
    return count;
}

}  // namespace pkmn::cli::red::codec
