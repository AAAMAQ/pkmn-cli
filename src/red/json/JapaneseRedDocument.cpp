#include "red/json/JapaneseRedDocument.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "red/codec/Gen1Codec.hpp"
#include "red/codec/JapaneseGen1Codec.hpp"
#include "red/validation/JapaneseSaveValidator.hpp"
#include "util/Sha256.hpp"

namespace pkmn::cli::red::json {
namespace {

using pkmn::cli::red::save::RedSave;

void CheckText(const OrderedJson& value, const std::string& path,
               JapaneseDocumentValidation& report) {
    try {
        const auto raw = codec::DecodeHex(value.at("rawHex").get<std::string>());
        if (raw.size() != 6)
            throw std::runtime_error("raw field is not six bytes");
        const RedSave field(raw);
        const auto decoded = codec::DecodeJapaneseText(field, 0, 6);
        if (value.at("value").get<std::string>() != decoded.value ||
            value.at("terminated").get<bool>() != decoded.terminated ||
            value.at("ambiguousGlyph").get<bool>() != decoded.ambiguousGlyph ||
            value.at("unsupportedByte").get<bool>() != decoded.unsupportedByte)
            throw std::runtime_error("glyphs or flags disagree with raw bytes");
    } catch (const std::exception& exception) {
        report.errors.push_back(path + ": " + exception.what());
    }
}

void CheckPokemon(const OrderedJson& mon, const std::string& path,
                  JapaneseDocumentValidation& report) {
    CheckText(mon.at("nickname"), path + ".nickname", report);
    CheckText(mon.at("otName"), path + ".otName", report);
}

OrderedJson EnglishText(const std::string& value) {
    const auto encoded = codec::EncodeText(value, 11);
    RedSave source(encoded);
    return {{"value", value}, {"losslessValue", codec::DecodeText(source, 0, 11, true)},
            {"rawHex", codec::Hex(encoded)}};
}

void ProjectPokemon(OrderedJson& mon, const std::string& sourceLocator) {
    const auto nickname = mon.at("nickname");
    const auto otName = mon.at("otName");
    if (nickname.at("unsupportedByte").get<bool>() ||
        otName.at("unsupportedByte").get<bool>() ||
        !nickname.at("terminated").get<bool>() ||
        !otName.at("terminated").get<bool>())
        throw std::runtime_error(sourceLocator +
                                 ": malformed or unsupported Japanese name bytes");
    mon["sourceJapanese"] = {
        {"provenanceVersion", "1.0.0"},
        {"sourceLocator", sourceLocator},
        {"nickname", nickname},
        {"otName", otName}};
    auto fallback = mon.at("speciesName").get<std::string>();
    // Internal species identifiers are not always representable Gen I names.
    if (fallback == "NIDORAN_M") fallback = "NIDORAN♂";
    else if (fallback == "NIDORAN_F") fallback = "NIDORAN♀";
    else if (fallback == "MR_MIME") fallback = "MR.MIME";
    mon["nickname"] = EnglishText(fallback);
    mon["otName"] = EnglishText("RED");
}

}  // namespace

RedSave::Bytes JapanesePhysicalBytes(const OrderedJson& document) {
    const auto& image = document.at("physicalImage");
    if (image.at("encoding") != "hex-uppercase-continuous")
        throw std::runtime_error("unsupported Japanese physical image encoding");
    auto bytes = codec::DecodeHex(image.at("standardSramHex").get<std::string>());
    auto trailing = codec::DecodeHex(image.at("trailingDataHex").get<std::string>());
    if (bytes.size() != RedSave::ExpectedSize ||
        trailing.size() != image.at("trailingLength").get<std::size_t>() ||
        bytes.size() + trailing.size() != image.at("totalLength").get<std::size_t>())
        throw std::runtime_error("Japanese physical image lengths disagree");
    bytes.insert(bytes.end(), trailing.begin(), trailing.end());
    return bytes;
}

JapaneseDocumentValidation ValidateJapaneseDocument(const OrderedJson& document) {
    JapaneseDocumentValidation result;
    try {
        const auto& schema = document.at("schema");
        const auto profile = document.at("source").at("profile").get<std::string>();
        const bool green = profile == "JP_GREEN_REV0";
        if (schema.at("format") != (green ? "pkmn-green-jp-master-save" : "pkmn-red-jp-master-save") ||
            schema.at("schemaVersion") != "0.1.0")
            throw std::runtime_error("unsupported Japanese Red archive schema");
        if (profile != "JP_RED_REV0" && profile != "JP_RED_REV1" && !green)
            throw std::runtime_error("source profile must be JP_RED_REV0 or JP_RED_REV1");
        const auto& decoded = document.at("decoded");
        CheckText(decoded.at("trainer").at("name"), "trainer.name", result);
        CheckText(decoded.at("rival").at("name"), "rival.name", result);
        const auto& party = decoded.at("party");
        if (party.at("pokemon").size() > 6 ||
            party.at("count").get<std::size_t>() != party.at("pokemon").size())
            result.errors.push_back("Japanese party count is inconsistent");
        for (std::size_t i = 0; i < party.at("pokemon").size(); ++i)
            CheckPokemon(party.at("pokemon").at(i),
                         "party/" + std::to_string(i), result);
        const auto& boxes = decoded.at("pcStorage").at("boxes");
        if (boxes.size() != 8)
            result.errors.push_back("Japanese archive requires eight PC boxes");
        for (std::size_t b = 0; b < boxes.size(); ++b) {
            const auto& box = boxes.at(b);
            if (box.at("pokemon").size() > 30 ||
                box.at("count").get<std::size_t>() != box.at("pokemon").size())
                result.errors.push_back("Japanese box count is inconsistent");
            for (std::size_t i = 0; i < box.at("pokemon").size(); ++i)
                CheckPokemon(box.at("pokemon").at(i),
                             "box/" + std::to_string(b) + "/" +
                                 std::to_string(i), result);
        }
        const auto selected = decoded.at("currentBoxCache")
                                  .at("selectedBoxNumber").get<int>();
        if (selected < 1 || selected > 8)
            result.errors.push_back("selected Japanese box must be 1..8");
        const auto& cache = decoded.at("currentBoxCache").at("cache");
        if (cache.at("pokemon").size() > 30)
            result.errors.push_back("Japanese current box cache exceeds 30");
        for (std::size_t i = 0; i < cache.at("pokemon").size(); ++i)
            CheckPokemon(cache.at("pokemon").at(i),
                         "currentBoxCache/" + std::to_string(i), result);
        const auto& daycare = decoded.at("daycare");
        if (daycare.at("inUse").get<bool>())
            CheckPokemon(daycare.at("pokemon"), "daycare", result);
        for (const auto& entry : decoded.at("hallOfFame").at("entries"))
            for (const auto& mon : entry.at("pokemon"))
                CheckText(mon.at("nickname"), "hallOfFame.nickname", result);
        if (document.contains("physicalImage")) {
            const auto bytes = JapanesePhysicalBytes(document);
            if (util::Sha256Hex(bytes) !=
                document.at("source").at("hashes").at("wholeFileSha256").get<std::string>())
                throw std::runtime_error("Japanese physical image hash mismatch");
            const RedSave save(bytes);
            const auto physicalReport = validation::JapaneseSaveValidator::Validate(save);
            if (!physicalReport.Valid())
                throw std::runtime_error("Japanese physical image fails save validation");
            const auto redone = DecodeJapanese(
                save, document.at("source").at("fileName").get<std::string>(),
                physicalReport, profile, {.includePhysicalImage = false});
            if (redone.at("decoded") != decoded)
                throw std::runtime_error("Japanese decoded state disagrees with physical image");
        } else {
            result.warnings.push_back("no physical image: byte reconstruction is unavailable");
        }
    } catch (const std::exception& exception) {
        result.errors.push_back(exception.what());
    }
    return result;
}

JapaneseProjection ProjectJapanese(const OrderedJson& archive,
                                   bool retainPlayerName) {
    const auto validation = ValidateJapaneseDocument(archive);
    if (!validation.Valid())
        throw std::runtime_error("invalid Japanese archive: " + validation.errors.front());
    auto projection = archive;
    auto& decoded = projection["decoded"];
    const int selected = decoded.at("currentBoxCache")
                             .at("selectedBoxNumber").get<int>() - 1;
    auto sourceBoxes = decoded.at("pcStorage").at("boxes");
    const auto storedSelected = sourceBoxes.at(selected);
    sourceBoxes[selected] = decoded.at("currentBoxCache").at("cache");
    const auto rawTrainer = decoded.at("trainer").at("name");
    const auto rawRival = decoded.at("rival").at("name");
    decoded["trainer"]["name"] = EnglishText("RED");
    decoded["rival"]["name"] = EnglishText("BLUE");

    auto mapping = OrderedJson::array();
    for (std::size_t i = 0; i < decoded.at("party").at("pokemon").size(); ++i)
        ProjectPokemon(decoded["party"]["pokemon"][i],
                       "party/" + std::to_string(i));

    OrderedJson boxes = OrderedJson::array();
    for (int b = 0; b < 12; ++b)
        boxes.push_back({{"boxNumber", b + 1}, {"declaredCount", 0},
                         {"count", 0}, {"pokemon", OrderedJson::array()}});
    std::size_t packed = 0;
    for (std::size_t b = 0; b < sourceBoxes.size(); ++b) {
        for (std::size_t i = 0; i < sourceBoxes.at(b).at("pokemon").size(); ++i) {
            auto mon = sourceBoxes.at(b).at("pokemon").at(i);
            const auto sourceLocator = "pcStorage/boxes/" + std::to_string(b) +
                                       "/pokemon/" + std::to_string(i);
            ProjectPokemon(mon, sourceLocator);
            const auto targetBox = packed / 20;
            if (targetBox >= 12)
                throw std::runtime_error("Japanese PC exceeds 240 slots");
            const auto targetSlot = packed % 20;
            mon["position"] = targetSlot + 1;
            boxes[targetBox]["pokemon"].push_back(mon);
            mapping.push_back({{"source", sourceLocator},
                               {"target", "pcStorage/boxes/" +
                                  std::to_string(targetBox) + "/pokemon/" +
                                  std::to_string(targetSlot)}});
            ++packed;
        }
    }
    for (auto& box : boxes) {
        const auto count = box.at("pokemon").size();
        box["count"] = count;
        box["declaredCount"] = count;
    }
    decoded["pcStorage"]["boxes"] = boxes;
    decoded["currentBoxCache"] = {
        {"rawSelectedBoxValue", 0}, {"selectedBoxNumber", 1},
        {"hasChangedBoxesBefore", false}, {"cache", boxes.at(0)}};
    if (decoded.at("daycare").at("inUse").get<bool>())
        ProjectPokemon(decoded["daycare"]["pokemon"], "daycare/pokemon");
    for (auto& entry : decoded["hallOfFame"]["entries"])
        for (auto& mon : entry["pokemon"]) {
            mon["sourceJapanese"] = mon.at("nickname");
            mon["nickname"] = EnglishText(mon.at("speciesName").get<std::string>());
        }

    projection["schema"] = {
        {"format", "pkmn-red-master-save"}, {"schemaVersion", "0.1.0"},
        {"game", "Pokemon Red"}, {"generation", 1},
        {"regionAssumption", "Japanese semantic projection"},
        {"canonicalExtension", ".red.json"}, {"lossless", false},
        {"stability", "draft"}};
    projection["sourceJapanese"] = {
        {"provenanceVersion", "1.0.0"},
        {"profile", archive.at("source").at("profile")},
        {"sourceSha256", archive.at("source").at("hashes").at("wholeFileSha256")},
        {"trainerName", rawTrainer}, {"rivalName", rawRival},
        {"targetPlayerNamePolicy", retainPlayerName
            ? "retain-japanese-raw-experimental" : "english-fallback"},
        {"selectedBox", selected + 1},
        {"selectedBoxCacheReplacedPermanent", true},
        {"selectedBoxPermanentDiffers", storedSelected.at("rawBlockHex") !=
                                       sourceBoxes.at(selected).at("rawBlockHex")},
        {"selectedBoxPermanentCopy", storedSelected},
        {"slotMapping", mapping},
        {"pcPokemonCount", packed},
        {"namePolicy", "Japanese Pokémon provenance overrides English display fallback in FireRed"}};
    projection["integrity"] = {
        {"allValid", true}, {"semanticProjection", true},
        {"sourceMainChecksum", archive.at("integrity").at("mainChecksum")}};
    projection.erase("physicalImage");
    projection["reconstruction"] = {
        {"available", false},
        {"policy", "generate-English-Red-semantics-only"},
        {"semanticFieldsAreInformational", false}};
    return {projection, mapping};
}

}  // namespace pkmn::cli::red::json
