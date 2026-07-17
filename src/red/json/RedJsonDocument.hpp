#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "red/json/RedDecoder.hpp"
#include "red/save/RedSave.hpp"

namespace pkmn::cli::red::json {

struct DocumentValidation {
    bool syntaxValid = false;
    bool schemaValid = false;
    bool semanticsValid = false;
    bool hasPhysicalImage = false;
    bool physicalImageValid = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    [[nodiscard]] bool Valid() const noexcept {
        return syntaxValid && schemaValid && semanticsValid &&
               (!hasPhysicalImage || physicalImageValid) && errors.empty();
    }
};

struct LoadedDocument {
    OrderedJson root;
    DocumentValidation validation;
};

LoadedDocument LoadAndValidate(const std::filesystem::path& path);
DocumentValidation ValidateDocument(const OrderedJson& root);
save::RedSave::Bytes PhysicalBytes(const OrderedJson& root);

}  // namespace pkmn::cli::red::json
