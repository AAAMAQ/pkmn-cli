#include "red/json/RedJsonDocument.hpp"

#include <array>
#include <fstream>
#include <set>
#include <stdexcept>

#include "red/codec/Gen1Codec.hpp"
#include "red/events/EventCatalog.hpp"
#include "red/data/Gen1Names.hpp"
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

void ValidateText(const OrderedJson& text, const std::string& path,
                  DocumentValidation& validation) {
    try {
        const auto value = text.value("losslessValue", text.at("value").get<std::string>());
        const auto encoded = codec::EncodeText(value, 11);
        save::RedSave bytes(encoded);
        if (codec::DecodeText(bytes, 0, 11, true) != value)
            validation.errors.push_back(path + " exceeds the ten-character Gen I limit");
    } catch (const std::exception& exception) {
        validation.errors.push_back(path + " is not valid Gen I text: " + exception.what());
    }
}

void ValidatePokemon(const OrderedJson& pokemon, bool party,
                     const std::string& path,
                     DocumentValidation& validation) {
    try {
        const auto species = pokemon.at("speciesId").get<unsigned>();
        const auto level = pokemon.at("level").get<unsigned>();
        const auto hp = pokemon.at("currentHp").get<unsigned>();
        if (species > 255 ||
            !data::IsValidSpecies(static_cast<std::uint8_t>(species)))
            validation.errors.push_back(path + ".speciesId is unsupported");
        if (level == 0 || level > 100)
            validation.errors.push_back(path + ".level must be in 1..100");
        if (pokemon.at("types").size() != 2)
            validation.errors.push_back(path + ".types must contain two values");
        if (pokemon.at("moves").size() != 4)
            validation.errors.push_back(path + ".moves must contain four slots");
        for (std::size_t index = 0; index < pokemon.at("moves").size(); ++index) {
            const auto& move = pokemon.at("moves").at(index);
            const auto moveId = move.at("moveId").get<unsigned>();
            if (moveId > 255 ||
                !data::IsValidMove(static_cast<std::uint8_t>(moveId)) ||
                move.at("pp").get<unsigned>() > 63 ||
                move.at("ppUps").get<unsigned>() > 3)
                validation.errors.push_back(path + ".moves[" + std::to_string(index) +
                                            "] is outside Gen I ranges");
        }
        for (const auto* key : {"attack", "defense", "speed", "special"})
            if (pokemon.at("dvs").at(key).get<unsigned>() > 15)
                validation.errors.push_back(path + ".dvs." + key + " exceeds 15");
        ValidateText(pokemon.at("nickname"), path + ".nickname", validation);
        ValidateText(pokemon.at("otName"), path + ".otName", validation);
        if (party) {
            const auto maxHp = pokemon.at("stats").at("maxHp").get<unsigned>();
            if (maxHp == 0 || hp > maxHp)
                validation.errors.push_back(path + ".currentHp exceeds maxHp or maxHp is zero");
        }
    } catch (const std::exception& exception) {
        validation.errors.push_back(path + " has an invalid Pokemon record: " + exception.what());
    }
}

void ValidateItems(const OrderedJson& collection, const std::string& path,
                   std::size_t capacity, DocumentValidation& validation) {
    try {
        const auto& items = collection.at("items");
        if (items.size() > capacity)
            validation.errors.push_back(path + " exceeds capacity");
        if (collection.at("count").get<std::size_t>() != items.size())
            validation.errors.push_back(path + " count does not match its item array");
        for (std::size_t index = 0; index < items.size(); ++index) {
            const auto id = items.at(index).at("itemId").get<unsigned>();
            const auto quantity = items.at(index).at("quantity").get<unsigned>();
            if (id > 255 ||
                !data::IsValidItem(static_cast<std::uint8_t>(id)) ||
                quantity == 0 || quantity > 99)
                validation.errors.push_back(path + ".items[" + std::to_string(index) +
                                            "] has an invalid ID or quantity");
        }
    } catch (const std::exception& exception) {
        validation.errors.push_back(path + " has invalid inventory data: " + exception.what());
    }
}

void WarnBadgeEventRelationships(const OrderedJson& decoded,
                                 DocumentValidation& validation) {
    if (!decoded.contains("events") ||
        !decoded.at("events").contains("flags"))
        return;
    const auto badges = decoded.at("badges").at("raw").get<unsigned>();
    const std::array<std::pair<int, const char*>, 8> gymEvents{{
        {119, "Boulder/Brock"}, {191, "Cascade/Misty"},
        {359, "Thunder/Lt. Surge"}, {425, "Rainbow/Erika"},
        {601, "Soul/Koga"}, {865, "Marsh/Sabrina"},
        {665, "Volcano/Blaine"}, {81, "Earth/Giovanni"}}};
    for (std::size_t bit = 0; bit < gymEvents.size(); ++bit) {
        const auto [flagIndex, label] = gymEvents[bit];
        for (const auto& record : decoded.at("events").at("flags")) {
            if (record.value("flagIndex", -1) != flagIndex) continue;
            const bool badge = (badges & (1U << bit)) != 0;
            const bool battle = record.value("value", false);
            if (badge != battle)
                validation.warnings.push_back(
                    std::string(label) +
                    " badge and verified gym-battle event differ; this can be "
                    "transient in gameplay but should be reviewed before generation");
            break;
        }
    }
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
    try {
        std::ifstream input(path);
        if (!input) throw std::runtime_error("could not open Red JSON input");
        return LoadAndValidate(input);
    } catch (const std::exception& exception) {
        LoadedDocument result;
        result.validation.errors.push_back(std::string("invalid JSON: ") + exception.what());
        return result;
    }
}

LoadedDocument LoadAndValidate(std::istream& input) {
    LoadedDocument result;
    try {
        input >> result.root;
        result.validation.syntaxValid = true;
    } catch (const std::exception& exception) {
        result.validation.errors.push_back(std::string("invalid JSON: ") + exception.what());
        return result;
    }
    result.validation = ValidateDocument(result.root);
    return result;
}

DocumentValidation ValidateDocument(const OrderedJson& root) {
    DocumentValidation validation;
    validation.syntaxValid = true;
    Require(root, "schema", validation, "$");
    Require(root, "source", validation, "$");
    Require(root, "integrity", validation, "$");
    Require(root, "decoded", validation, "$");
    if (!validation.errors.empty()) return validation;
    const auto& schema = root.at("schema");
    if (schema.value("format", "") != "pkmn-red-master-save")
        validation.errors.push_back("unsupported schema format");
    if (schema.value("schemaVersion", "") != "0.1.0")
        validation.errors.push_back("unsupported schema version (supported: 0.1.0)");
    validation.schemaValid = validation.errors.empty();
    if (!validation.schemaValid) return validation;

    const auto& decoded = root.at("decoded");
    for (const char* field : {"trainer", "rival", "moneyAndCoins", "badges", "playtime",
                              "location", "options", "pokedex", "inventory", "party",
                              "pcStorage", "currentBoxCache", "daycare", "hallOfFame",
                              "worldStateRaw"})
        Require(decoded, field, validation, "$.decoded");
    if (!validation.errors.empty()) return validation;
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
        ValidateText(decoded.at("trainer").at("name"), "$.decoded.trainer.name", validation);
        ValidateText(decoded.at("rival").at("name"), "$.decoded.rival.name", validation);
        if (decoded.at("badges").at("raw").get<unsigned>() !=
            decoded.at("badges").at("mirrorRaw").get<unsigned>())
            validation.errors.push_back("badge mirror is not synchronized");
        const auto& playtime = decoded.at("playtime");
        if (playtime.at("minutes").get<unsigned>() >= 60 ||
            playtime.at("seconds").get<unsigned>() >= 60 ||
            playtime.at("frames").get<unsigned>() >= 60)
            validation.errors.push_back("playtime minute/second/frame values exceed clock ranges");
        const auto owned = codec::DecodeHex(
            decoded.at("pokedex").at("ownedBitfieldHex").get<std::string>());
        const auto seen = codec::DecodeHex(
            decoded.at("pokedex").at("seenBitfieldHex").get<std::string>());
        if (owned.size() != 19 || seen.size() != 19)
            validation.errors.push_back("Pokedex bitfields must each contain 19 bytes");
        ValidateItems(decoded.at("inventory").at("bag"),
                      "$.decoded.inventory.bag", 20, validation);
        ValidateItems(decoded.at("inventory").at("pcItems"),
                      "$.decoded.inventory.pcItems", 50, validation);
        for (std::size_t index = 0; index < party.at("pokemon").size(); ++index)
            ValidatePokemon(party.at("pokemon").at(index), true,
                            "$.decoded.party.pokemon[" + std::to_string(index) + "]",
                            validation);
        const auto& boxes = decoded.at("pcStorage").at("boxes");
        for (std::size_t boxIndex = 0; boxIndex < boxes.size(); ++boxIndex) {
            const auto& box = boxes.at(boxIndex);
            if (box.at("pokemon").size() > 20 ||
                box.at("count").get<std::size_t>() != box.at("pokemon").size())
                validation.errors.push_back("PC box " + std::to_string(boxIndex + 1) +
                                            " count/array is inconsistent");
            for (std::size_t index = 0; index < box.at("pokemon").size(); ++index)
                ValidatePokemon(box.at("pokemon").at(index), false,
                                "$.decoded.pcStorage.boxes[" +
                                    std::to_string(boxIndex) + "].pokemon[" +
                                    std::to_string(index) + "]",
                                validation);
        }
        const auto& cache = decoded.at("currentBoxCache").at("cache");
        if (cache.at("pokemon").size() > 20 ||
            cache.at("count").get<std::size_t>() != cache.at("pokemon").size())
            validation.errors.push_back("current-box cache count/array is inconsistent");
        for (std::size_t index = 0; index < cache.at("pokemon").size(); ++index)
            ValidatePokemon(cache.at("pokemon").at(index), false,
                            "$.decoded.currentBoxCache.cache.pokemon[" +
                                std::to_string(index) + "]",
                            validation);
        const auto& daycare = decoded.at("daycare");
        if (daycare.at("inUse").get<bool>())
            ValidatePokemon(daycare.at("pokemon"), false,
                            "$.decoded.daycare.pokemon", validation);
        const auto& hall = decoded.at("hallOfFame").at("entries");
        if (hall.size() > 50)
            validation.errors.push_back("Hall of Fame exceeds 50 entries");
        for (std::size_t entry = 0; entry < hall.size(); ++entry) {
            const auto& pokemon = hall.at(entry).at("pokemon");
            if (pokemon.size() > 6)
                validation.errors.push_back("Hall of Fame entry exceeds six Pokemon");
            std::set<unsigned> orders;
            for (std::size_t index = 0; index < pokemon.size(); ++index) {
                const auto& mon = pokemon.at(index);
                const auto order = mon.at("partyOrder").get<unsigned>();
                const auto species = mon.at("speciesId").get<unsigned>();
                const auto level = mon.at("level").get<unsigned>();
                if (order < 1 || order > 6 || !orders.insert(order).second ||
                    species > 255 ||
                    !data::IsValidSpecies(static_cast<std::uint8_t>(species)) ||
                    level == 0 || level > 100)
                    validation.errors.push_back("Hall of Fame entry has invalid slot/species/level");
                ValidateText(mon.at("nickname"), "$.decoded.hallOfFame.nickname",
                             validation);
            }
        }
        const auto& world = decoded.at("worldStateRaw");
        for (const auto& [key, length] :
             std::vector<std::pair<const char*, std::size_t>>{
                 {"eventFlagsHex", 0x140}, {"scriptsHex", 0x100},
                 {"missableObjectsHex", 29}, {"hiddenItemsHex", 7},
                 {"hiddenCoinsHex", 2}, {"visitedTownsHex", 2}})
            if (codec::DecodeHex(world.at(key).get<std::string>()).size() != length)
                validation.errors.push_back(std::string(key) + " has the wrong byte length");
        const bool hasAnyNamedState = decoded.contains("events") ||
            decoded.contains("trainerBattles") || decoded.contains("staticBattles") ||
            decoded.contains("storyProgress");
        if (hasAnyNamedState)
            events::ValidateNamedState(decoded, validation.errors);
        if (hasAnyNamedState && validation.errors.empty())
            WarnBadgeEventRelationships(decoded, validation);
    } catch (const std::exception& exception) {
        validation.errors.push_back(std::string("invalid semantic field type: ") + exception.what());
    }
    validation.semanticsValid = validation.errors.empty();
    validation.hasPhysicalImage = root.contains("physicalImage");
    if (!validation.hasPhysicalImage) {
        validation.warnings.push_back("physicalImage absent: archival reconstruction is unavailable");
        validation.warnings.push_back("semantic generation must remain independent of physicalImage");
        return validation;
    }
    try {
        const auto bytes = PhysicalBytes(root);
        const auto wholeHash = root.at("source").at("hashes").at("wholeFileSha256").get<std::string>();
        if (util::Sha256Hex(bytes) != wholeHash)
            throw std::runtime_error("physicalImage SHA-256 does not match source metadata");
        const auto report = pkmn::cli::red::validation::SaveValidator::Validate(save::RedSave(bytes));
        if (!report.Valid()) throw std::runtime_error("physicalImage Red checksums are invalid");
        validation.physicalImageValid = true;
    } catch (const std::exception& exception) {
        validation.errors.push_back(std::string("invalid physicalImage: ") + exception.what());
    }
    return validation;
}

}  // namespace pkmn::cli::red::json
