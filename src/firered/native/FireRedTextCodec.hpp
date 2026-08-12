#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace firered {

std::string DecodeFireRedText(std::span<const std::uint8_t> bytes);
std::vector<std::uint8_t> EncodeFireRedEnglishName(
    const std::string& text,
    std::size_t fieldLength);
bool IsSupportedFireRedEnglishName(const std::string& text, std::size_t maxCharacters);

} // namespace firered
