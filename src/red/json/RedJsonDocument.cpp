#include "red/json/RedJsonDocument.hpp"

#include <fstream>
#include <stdexcept>

#include "red/validation/SaveValidator.hpp"
#include "util/Sha256.hpp"

namespace pkmn::cli::red::json {
namespace {

int Nibble(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'A' && value <= 'F') return 10 + value - 'A';
    return -1;
}

save::RedSave::Bytes DecodeHex(const std::string& hex) {
    if (hex.size() % 2U != 0) throw std::runtime_error("physicalImage hex has odd length");
    save::RedSave::Bytes bytes;
    bytes.reserve(hex.size() / 2U);
    for (std::size_t index = 0; index < hex.size(); index += 2U) {
        const int high = Nibble(hex[index]);
        const int low = Nibble(hex[index + 1]);
        if (high < 0 || low < 0)
            throw std::runtime_error("physicalImage must use uppercase canonical hexadecimal");
        bytes.push_back(static_cast<std::uint8_t>((high << 4) | low));
    }
    return bytes;
}

void Require(const OrderedJson& object, const char* key,
             DocumentValidation& validation, const std::string& path) {
    if (!object.is_object() || !object.contains(key))
        validation.errors.push_back("missing required field: " + path + "." + key);
}

}  // namespace

save::RedSave::Bytes PhysicalBytes(const OrderedJson& root) {
    const auto& physical = root.at("physicalImage");
    if (physical.at("encoding") != "hex-uppercase-continuous")
        throw std::runtime_error("unsupported physicalImage encoding");
    auto bytes = DecodeHex(physical.at("standardSramHex").get<std::string>());
    const auto trailing = DecodeHex(physical.at("trailingDataHex").get<std::string>());
    bytes.insert(bytes.end(), trailing.begin(), trailing.end());
    if (physical.at("standardSramLength").get<std::size_t>() != save::RedSave::ExpectedSize ||
        physical.at("trailingLength").get<std::size_t>() != trailing.size() ||
        physical.at("totalLength").get<std::size_t>() != bytes.size())
        throw std::runtime_error("physicalImage declared lengths do not match decoded bytes");
    return bytes;
}

LoadedDocument LoadAndValidate(const std::filesystem::path& path) {
    LoadedDocument result;
    try {
        std::ifstream input(path);
        if (!input) throw std::runtime_error("could not open Red JSON input");
        input >> result.root;
        result.validation.syntaxValid = true;
    } catch (const std::exception& exception) {
        result.validation.errors.push_back(std::string("invalid JSON: ") + exception.what());
        return result;
    }
    auto& validation = result.validation;
    Require(result.root, "schema", validation, "$");
    Require(result.root, "source", validation, "$");
    Require(result.root, "integrity", validation, "$");
    Require(result.root, "decoded", validation, "$");
    if (!validation.errors.empty()) return result;
    const auto& schema = result.root.at("schema");
    if (schema.value("format", "") != "pkmn-red-master-save")
        validation.errors.push_back("unsupported schema format");
    if (schema.value("schemaVersion", "") != "0.1.0")
        validation.errors.push_back("unsupported schema version (supported: 0.1.0)");
    validation.schemaValid = validation.errors.empty();
    if (!validation.schemaValid) return result;

    const auto& decoded = result.root.at("decoded");
    for (const char* field : {"trainer", "rival", "moneyAndCoins", "badges", "playtime",
                              "location", "options", "pokedex", "inventory", "party",
                              "pcStorage", "currentBoxCache", "daycare", "hallOfFame"})
        Require(decoded, field, validation, "$.decoded");
    if (!validation.errors.empty()) return result;
    try {
        const auto money = decoded.at("moneyAndCoins").at("money").get<std::uint32_t>();
        const auto coins = decoded.at("moneyAndCoins").at("coins").get<std::uint32_t>();
        if (money > 999999U) validation.errors.push_back("money exceeds Gen I range");
        if (coins > 9999U) validation.errors.push_back("coins exceed Gen I range");
        const auto& party = decoded.at("party");
        if (party.at("count").get<std::size_t>() != party.at("pokemon").size() ||
            party.at("pokemon").size() > 6U)
            validation.errors.push_back("party count/array is inconsistent");
        if (decoded.at("pcStorage").at("boxes").size() != 12U)
            validation.errors.push_back("pcStorage must contain all 12 boxes");
        const auto selected = decoded.at("currentBoxCache").at("selectedBoxNumber").get<int>();
        if (selected < 1 || selected > 12)
            validation.errors.push_back("selected box must be in 1..12");
    } catch (const std::exception& exception) {
        validation.errors.push_back(std::string("invalid semantic field type: ") + exception.what());
    }
    validation.semanticsValid = validation.errors.empty();
    validation.hasPhysicalImage = result.root.contains("physicalImage");
    if (!validation.hasPhysicalImage) {
        validation.warnings.push_back("physicalImage absent: archival reconstruction is unavailable");
        validation.warnings.push_back("semantic generation must remain independent of physicalImage");
        return result;
    }
    try {
        const auto bytes = PhysicalBytes(result.root);
        const auto wholeHash = result.root.at("source").at("hashes").at("wholeFileSha256").get<std::string>();
        if (util::Sha256Hex(bytes) != wholeHash)
            throw std::runtime_error("physicalImage SHA-256 does not match source metadata");
        const auto report = pkmn::cli::red::validation::SaveValidator::Validate(save::RedSave(bytes));
        if (!report.Valid()) throw std::runtime_error("physicalImage Red checksums are invalid");
        validation.physicalImageValid = true;
    } catch (const std::exception& exception) {
        validation.errors.push_back(std::string("invalid physicalImage: ") + exception.what());
    }
    return result;
}

}  // namespace pkmn::cli::red::json
