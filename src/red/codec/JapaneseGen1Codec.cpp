#include "red/codec/JapaneseGen1Codec.hpp"

#include <iomanip>
#include <sstream>

namespace pkmn::cli::red::codec {
namespace {

// Derived from Narishma-gb/pokegreen@953f41b constants/charmap.asm.
// A few distinct source spellings share a rendered tile. We choose the first
// spelling from that charmap and expose ambiguity separately.
std::string Unknown(std::uint8_t byte) {
    std::ostringstream out;
    out << "<0x" << std::uppercase << std::hex << std::setw(2)
        << std::setfill('0') << static_cast<unsigned>(byte) << '>';
    return out.str();
}

std::string Kana(const char* glyphs, std::uint8_t byte, std::uint8_t first) {
    return std::string(glyphs + (byte - first) * 3, 3);
}

std::string Token(std::uint8_t byte, bool& unsupported) {
    if (byte >= 0x80 && byte <= 0xAB)
        return Kana("アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフホマミムメモヤユヨラルレロワヲン", byte, 0x80);
    if (byte >= 0xAC && byte <= 0xB0)
        return Kana("ッャュョィ", byte, 0xAC);
    if (byte >= 0xB1 && byte <= 0xDE)
        return Kana("あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをん", byte, 0xB1);
    if (byte >= 0xDF && byte <= 0xE2)
        return Kana("っゃゅょ", byte, 0xDF);
    if (byte >= 0xF6)
        return Kana("０１２３４５６７８９", byte, 0xF6);
    if (byte >= 0x05 && byte <= 0x13)
        return Kana("ガギグゲゴザジズゼゾダヂヅデド", byte, 0x05);
    if (byte >= 0x26 && byte <= 0x34)
        return Kana("がぎぐげござじずぜぞだぢづでど", byte, 0x26);
    switch (byte) {
    case 0x19: return "バ"; case 0x1A: return "ビ";
    case 0x1B: return "ブ"; case 0x1C: return "ボ";
    case 0x3A: return "ば"; case 0x3B: return "び";
    case 0x3C: return "ぶ"; case 0x3D: return "べ";
    case 0x3E: return "ぼ";
    case 0x40: return "パ"; case 0x41: return "ピ";
    case 0x42: return "プ"; case 0x43: return "ポ";
    case 0x44: return "ぱ"; case 0x45: return "ぴ";
    case 0x46: return "ぷ"; case 0x47: return "ぺ";
    case 0x48: return "ぽ";
    case 0x60: return "Ａ"; case 0x61: return "Ｂ";
    case 0x62: return "Ｃ"; case 0x63: return "Ｄ";
    case 0x64: return "Ｅ"; case 0x65: return "Ｆ";
    case 0x66: return "Ｇ"; case 0x67: return "Ｈ";
    case 0x68: return "Ｉ"; case 0x69: return "Ｖ";
    case 0x6A: return "Ｓ"; case 0x6B: return "Ｌ";
    case 0x6C: return "Ｍ"; case 0x6D: return ":";
    case 0x6E: return "ぃ"; case 0x6F: return "ぅ";
    case 0x70: return "「"; case 0x71: return "」";
    case 0x72: return "『"; case 0x73: return "』";
    case 0x74: return "·"; case 0x75: return "⋯";
    case 0x76: return "ぁ"; case 0x77: return "ぇ";
    case 0x78: return "ぉ"; case 0x7F: return "　";
    case 0xE3: return "ー"; case 0xE4: return "゜";
    case 0xE5: return "゛"; case 0xE6: return "？";
    case 0xE7: return "！"; case 0xE8: return "。";
    case 0xE9: return "ァ"; case 0xEA: return "ゥ";
    case 0xEB: return "ェ"; case 0xEC: return "▷";
    case 0xED: return "▶"; case 0xEE: return "▼";
    case 0xEF: return "♂"; case 0xF0: return "円";
    case 0xF1: return "×"; case 0xF2: return "．";
    case 0xF3: return "／"; case 0xF4: return "ォ";
    case 0xF5: return "♀";
    default:
        unsupported = true;
        return Unknown(byte);
    }
}

}  // namespace

JapaneseText DecodeJapaneseText(const save::RedSave& input,
                                std::size_t offset, std::size_t length) {
    JapaneseText result;
    for (const auto byte : input.Slice(offset, length)) {
        if (byte == 0x50) {
            result.terminated = true;
            break;
        }
        result.ambiguousGlyph |= byte == 0x3D || byte == 0x47 ||
                                 byte == 0xCD || byte == 0xD8;
        result.value += Token(byte, result.unsupportedByte);
    }
    return result;
}

}  // namespace pkmn::cli::red::codec
