#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"

namespace pkmn::cli::red::json {

using OrderedJson = nlohmann::ordered_json;

struct DecodeOptions {
    bool includePhysicalImage = true;
};

OrderedJson Decode(const save::RedSave& input, const std::string& logicalName,
                   const validation::ValidationReport& validation,
                   const DecodeOptions& options = {});
std::string Serialize(const OrderedJson& document);

}  // namespace pkmn::cli::red::json
