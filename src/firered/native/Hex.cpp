#include "Hex.hpp"

#include <stdexcept>

namespace firered {

std::string EncodeHex(std::span<const std::uint8_t> bytes) {
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string result;
    result.resize(bytes.size() * 2);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        result[i * 2] = digits[bytes[i] >> 4];
        result[i * 2 + 1] = digits[bytes[i] & 0x0F];
    }
    return result;
}

Bytes DecodeHex(const std::string& hex) {
    if (hex.size() % 2 != 0) throw std::invalid_argument("hex payload has odd length");
    auto nibble = [](char c) -> std::uint8_t {
        if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
        if (c >= 'A' && c <= 'F') return static_cast<std::uint8_t>(c - 'A' + 10);
        throw std::invalid_argument("hex payload must use canonical uppercase characters");
    };
    Bytes result(hex.size() / 2);
    for (std::size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<std::uint8_t>(
            static_cast<std::uint8_t>(nibble(hex[i * 2]) << 4)
            | nibble(hex[i * 2 + 1]));
    }
    return result;
}

} // namespace firered
