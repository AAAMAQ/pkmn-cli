#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "red/save/RedSave.hpp"

namespace pkmn::cli::red::codec {

std::string DecodeText(const save::RedSave &input, std::size_t offset,
                       std::size_t length, bool lossless = false);
save::RedSave::Bytes EncodeText(const std::string &value, std::size_t length);
save::RedSave::Bytes DecodeHex(const std::string &hex);
std::uint16_t ReadU16BE(const save::RedSave &input, std::size_t offset);
std::uint32_t ReadU24BE(const save::RedSave &input, std::size_t offset);
std::uint32_t ReadBcd(const save::RedSave &input, std::size_t offset,
                      std::size_t length);
std::string Hex(const save::RedSave::Bytes &bytes, bool uppercase = true);
std::size_t CountSetBits(const save::RedSave &input, std::size_t offset,
                         std::size_t length, std::size_t usedBits);

} // namespace pkmn::cli::red::codec
