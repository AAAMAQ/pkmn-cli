#include "red/codec/Gen1Codec.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace pkmn::cli::red::codec {
namespace {

std::string Unknown(std::uint8_t value) {
  std::ostringstream output;
  output << "<0x" << std::uppercase << std::hex << std::setw(2)
         << std::setfill('0') << static_cast<int>(value) << '>';
  return output.str();
}

std::string Token(std::uint8_t value, bool lossless) {
  if (value >= 0x80 && value <= 0x99)
    return {static_cast<char>('A' + value - 0x80)};
  if (value >= 0xA0 && value <= 0xB9)
    return {static_cast<char>('a' + value - 0xA0)};
  if (value >= 0xF6)
    return {static_cast<char>('0' + value - 0xF6)};
  switch (value) {
  case 0x5B:
    return lossless ? "<PC>" : "PC";
  case 0x5C:
    return lossless ? "<TM>" : "TM";
  case 0x5D:
    return lossless ? "<TRAINER>" : "TRAINER";
  case 0x7F:
    return " ";
  case 0x9A:
    return "(";
  case 0x9B:
    return ")";
  case 0x9C:
    return ":";
  case 0x9D:
    return ";";
  case 0xBA:
    return "é";
  case 0xE0:
    return "'";
  case 0xE3:
    return "-";
  case 0xE6:
    return "?";
  case 0xE7:
    return "!";
  case 0xE8:
    return lossless ? "<PERIOD>" : ".";
  case 0xEF:
    return "♂";
  case 0xF0:
    return "¥";
  case 0xF1:
    return "×";
  case 0xF2:
    return lossless ? "<DOT>" : ".";
  case 0xF3:
    return "/";
  case 0xF4:
    return ",";
  case 0xF5:
    return "♀";
  default:
    return Unknown(value);
  }
}

} // namespace

std::string DecodeText(const save::RedSave &input, std::size_t offset,
                       std::size_t length, bool lossless) {
  std::string result;
  for (const auto value : input.Slice(offset, length)) {
    if (value == 0x50)
      break;
    result += Token(value, lossless);
  }
  return result;
}

save::RedSave::Bytes EncodeText(const std::string &value, std::size_t length) {
  save::RedSave::Bytes result(length, 0x50);
  std::size_t out = 0;
  for (std::size_t index = 0; index < value.size() && out + 1 < length;) {
    std::uint8_t byte = 0;
    std::size_t consumed = 1;
    const auto c = static_cast<unsigned char>(value[index]);
    if (value.compare(index, 5, "<DOT>") == 0) {
      byte = 0xF2;
      consumed = 5;
    } else if (value.compare(index, 8, "<PERIOD>") == 0) {
      byte = 0xE8;
      consumed = 8;
    } else if (index + 6 <= value.size() &&
               value.compare(index, 3, "<0x") == 0 && value[index + 5] == '>') {
      const int high = std::isdigit(value[index + 3])
                           ? value[index + 3] - '0'
                           : 10 + std::toupper(value[index + 3]) - 'A';
      const int low = std::isdigit(value[index + 4])
                          ? value[index + 4] - '0'
                          : 10 + std::toupper(value[index + 4]) - 'A';
      if (high < 0 || high > 15 || low < 0 || low > 15)
        throw std::runtime_error("invalid lossless Gen I byte token");
      byte = static_cast<std::uint8_t>((high << 4) | low);
      consumed = 6;
    } else if (c >= 'A' && c <= 'Z')
      byte = static_cast<std::uint8_t>(0x80 + c - 'A');
    else if (c >= 'a' && c <= 'z')
      byte = static_cast<std::uint8_t>(0xA0 + c - 'a');
    else if (c >= '0' && c <= '9')
      byte = static_cast<std::uint8_t>(0xF6 + c - '0');
    else {
      switch (c) {
      case ' ':
        byte = 0x7F;
        break;
      case '\'':
        byte = 0xE0;
        break;
      case '-':
        byte = 0xE3;
        break;
      case '?':
        byte = 0xE6;
        break;
      case '!':
        byte = 0xE7;
        break;
      case '.':
        byte = 0xE8;
        break;
      case '/':
        byte = 0xF3;
        break;
      case ',':
        byte = 0xF4;
        break;
      default:
        throw std::runtime_error("unsupported Gen I text character");
      }
    }
    if (byte == 0x50)
      throw std::runtime_error("terminator cannot occur inside Gen I text");
    result[out++] = byte;
    index += consumed;
  }
  return result;
}

save::RedSave::Bytes DecodeHex(const std::string &hex) {
  if (hex.size() % 2U != 0)
    throw std::runtime_error("hex string has odd length");
  save::RedSave::Bytes result;
  result.reserve(hex.size() / 2U);
  auto nibble = [](char c) -> int {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'A' && c <= 'F')
      return 10 + c - 'A';
    if (c >= 'a' && c <= 'f')
      return 10 + c - 'a';
    return -1;
  };
  for (std::size_t index = 0; index < hex.size(); index += 2U) {
    const int high = nibble(hex[index]);
    const int low = nibble(hex[index + 1]);
    if (high < 0 || low < 0)
      throw std::runtime_error("invalid hexadecimal string");
    result.push_back(static_cast<std::uint8_t>((high << 4) | low));
  }
  return result;
}

std::uint16_t ReadU16BE(const save::RedSave &input, std::size_t offset) {
  return static_cast<std::uint16_t>((input.At(offset) << 8U) |
                                    input.At(offset + 1));
}

std::uint32_t ReadU24BE(const save::RedSave &input, std::size_t offset) {
  return (static_cast<std::uint32_t>(input.At(offset)) << 16U) |
         (static_cast<std::uint32_t>(input.At(offset + 1)) << 8U) |
         input.At(offset + 2);
}

std::uint32_t ReadBcd(const save::RedSave &input, std::size_t offset,
                      std::size_t length) {
  std::uint32_t value = 0;
  for (const auto byte : input.Slice(offset, length))
    value = value * 100U + ((byte >> 4U) & 0x0FU) * 10U + (byte & 0x0FU);
  return value;
}

std::string Hex(const save::RedSave::Bytes &bytes, bool uppercase) {
  std::ostringstream output;
  if (uppercase)
    output << std::uppercase;
  output << std::hex << std::setfill('0');
  for (const auto byte : bytes)
    output << std::setw(2) << static_cast<int>(byte);
  return output.str();
}

std::size_t CountSetBits(const save::RedSave &input, std::size_t offset,
                         std::size_t length, std::size_t usedBits) {
  std::size_t count = 0;
  const auto bytes = input.Slice(offset, length);
  for (std::size_t bit = 0; bit < std::min(usedBits, length * 8U); ++bit)
    if ((bytes[bit / 8U] & (1U << (bit % 8U))) != 0)
      ++count;
  return count;
}

} // namespace pkmn::cli::red::codec
