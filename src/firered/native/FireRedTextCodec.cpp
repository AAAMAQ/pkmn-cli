#include "FireRedTextCodec.hpp"

#include <array>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace firered {

namespace {
std::string Token(std::uint8_t value) {
    std::ostringstream out;
    out << "<$" << std::uppercase << std::hex << std::setw(2)
        << std::setfill('0') << static_cast<unsigned>(value) << ">";
    return out.str();
}

std::string DecodeByte(std::uint8_t value) {
    if (value == 0x00) return " ";
    if (value >= 0xA1 && value <= 0xAA) return std::string(1, static_cast<char>('0' + value - 0xA1));
    if (value >= 0xBB && value <= 0xD4) return std::string(1, static_cast<char>('A' + value - 0xBB));
    if (value >= 0xD5 && value <= 0xEE) return std::string(1, static_cast<char>('a' + value - 0xD5));
    switch (value) {
    case 0x2D: return "&";
    case 0x2E: return "+";
    case 0x35: return "=";
    case 0x36: return ";";
    case 0x5B: return "%";
    case 0x5C: return "(";
    case 0x5D: return ")";
    case 0x85: return "<";
    case 0x86: return ">";
    case 0xAB: return "!";
    case 0xAC: return "?";
    case 0xAD: return ".";
    case 0xAE: return "-";
    case 0xAF: return "·";
    case 0xB0: return "…";
    case 0xB3: return "‘";
    case 0xB4: return "'";
    case 0xB5: return "♂";
    case 0xB6: return "♀";
    case 0xB7: return "¥";
    case 0xB8: return ",";
    case 0xB9: return "×";
    case 0xBA: return "/";
    case 0xF0: return ":";
    default: return Token(value);
    }
}

std::string Kana(const char* glyphs, std::uint8_t byte, std::uint8_t first) {
    return std::string(glyphs + (byte - first) * 3, 3);
}

std::string DecodeJapaneseByte(std::uint8_t value) {
    // pret/pokefirered@df4449a charmap.txt, Japanese font selection.
    if (value >= 0x01 && value <= 0x2E)
        return Kana("あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをん", value, 0x01);
    if (value >= 0x2F && value <= 0x36)
        return Kana("ぁぃぅぇぉゃゅょ", value, 0x2F);
    if (value >= 0x37 && value <= 0x45)
        return Kana("がぎぐげござじずぜぞだぢづでど", value, 0x37);
    if (value >= 0x46 && value <= 0x4A)
        return Kana("ばびぶべぼ", value, 0x46);
    if (value >= 0x4B && value <= 0x4F)
        return Kana("ぱぴぷぺぽ", value, 0x4B);
    if (value == 0x50) return "っ";
    if (value >= 0x51 && value <= 0x7E)
        return Kana("アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲン", value, 0x51);
    if (value >= 0x7F && value <= 0x86)
        return Kana("ァィゥェォャュョ", value, 0x7F);
    if (value >= 0x87 && value <= 0x95)
        return Kana("ガギグゲゴザジズゼゾダヂヅデド", value, 0x87);
    if (value >= 0x96 && value <= 0x9A)
        return Kana("バビブベボ", value, 0x96);
    if (value >= 0x9B && value <= 0x9F)
        return Kana("パピプペポ", value, 0x9B);
    if (value == 0xA0) return "ッ";
    if (value == 0xAB) return "！";
    if (value == 0xAC) return "？";
    if (value == 0xAD) return "。";
    if (value == 0xAE) return "ー";
    if (value == 0xEF) return "▶";
    return DecodeByte(value);
}

std::uint8_t EncodeAscii(char value) {
    if (value == ' ') return 0x00;
    if (value >= '0' && value <= '9') return static_cast<std::uint8_t>(0xA1 + value - '0');
    if (value >= 'A' && value <= 'Z') return static_cast<std::uint8_t>(0xBB + value - 'A');
    if (value >= 'a' && value <= 'z') return static_cast<std::uint8_t>(0xD5 + value - 'a');
    switch (value) {
    case '&': return 0x2D;
    case '+': return 0x2E;
    case '=': return 0x35;
    case ';': return 0x36;
    case '%': return 0x5B;
    case '(': return 0x5C;
    case ')': return 0x5D;
    case '!': return 0xAB;
    case '?': return 0xAC;
    case '.': return 0xAD;
    case '-': return 0xAE;
    case '\'': return 0xB4;
    case ',': return 0xB8;
    case '/': return 0xBA;
    case ':': return 0xF0;
    default: throw std::invalid_argument("name contains a character unsupported by FireRed English text");
    }
}
} // namespace

std::string DecodeFireRedText(std::span<const std::uint8_t> bytes) {
    std::string result;
    for (const auto value : bytes) {
        if (value == 0xFF) break;
        result += DecodeByte(value);
    }
    return result;
}

std::string DecodeFireRedJapaneseText(std::span<const std::uint8_t> bytes) {
    std::string result;
    for (const auto value : bytes) {
        if (value == 0xFF) break;
        result += DecodeJapaneseByte(value);
    }
    return result;
}

bool IsSupportedFireRedEnglishName(const std::string& text, std::size_t maxCharacters) {
    if (text.empty() || text.size() > maxCharacters) return false;
    try {
        for (const char character : text) (void)EncodeAscii(character);
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::uint8_t> EncodeFireRedEnglishName(
    const std::string& text,
    std::size_t fieldLength) {
    if (!IsSupportedFireRedEnglishName(text, fieldLength - 1)) {
        throw std::invalid_argument("name must be 1-" + std::to_string(fieldLength - 1)
                                    + " supported English characters");
    }
    std::vector<std::uint8_t> encoded(fieldLength, 0xFF);
    for (std::size_t i = 0; i < text.size(); ++i) encoded[i] = EncodeAscii(text[i]);
    return encoded;
}

} // namespace firered
