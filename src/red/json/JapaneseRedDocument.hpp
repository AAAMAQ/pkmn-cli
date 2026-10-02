#pragma once

#include <string>
#include <vector>

#include "red/json/RedDecoder.hpp"

namespace pkmn::cli::red::json {

struct JapaneseDocumentValidation {
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    [[nodiscard]] bool Valid() const noexcept { return errors.empty(); }
};

JapaneseDocumentValidation ValidateJapaneseDocument(const OrderedJson& document);
save::RedSave::Bytes JapanesePhysicalBytes(const OrderedJson& document);

struct JapaneseProjection {
    OrderedJson document;
    OrderedJson mapping;
};

JapaneseProjection ProjectJapanese(const OrderedJson& archive,
                                   bool retainPlayerName = false);

}  // namespace pkmn::cli::red::json
