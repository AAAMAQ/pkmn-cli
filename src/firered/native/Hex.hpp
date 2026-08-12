#pragma once

#include "FireRedSaveStructure.hpp"

#include <span>
#include <string>

namespace firered {

std::string EncodeHex(std::span<const std::uint8_t> bytes);
Bytes DecodeHex(const std::string& hex);

} // namespace firered
