#pragma once

#include <cstddef>
#include <string>

#include "red/save/RedSave.hpp"

namespace pkmn::cli::red::codec {

struct JapaneseText {
    std::string value;
    bool terminated = false;
    bool ambiguousGlyph = false;
    bool unsupportedByte = false;
};

JapaneseText DecodeJapaneseText(const save::RedSave& input,
                                std::size_t offset, std::size_t length);

}  // namespace pkmn::cli::red::codec
