#include "red/json/RedDecoder.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "app/Version.hpp"
#include "red/codec/Gen1Codec.hpp"
#include "red/codec/JapaneseGen1Codec.hpp"
#include "red/events/EventCatalog.hpp"
#include "red/data/Gen1Names.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/Sha256.hpp"

namespace pkmn::cli::red::json {
namespace {

namespace c = pkmn::cli::red::codec;
using pkmn::cli::red::save::RedSave;

struct Layout {
  std::size_t nameLength, party, boxCache, boxSize, boxCount;
  std::size_t hofCount, rivalName, money, coins, options, badges, badgeMirror;
  std::size_t trainerId, map, x, y, xBlock, yBlock, previousMap, contrast;
  std::size_t dexOwned, dexSeen, bagCount, bagPairs, pcItemCount, pcItemPairs;
  std::size_t selectedBox, missable, hiddenItems, hiddenCoins, visitedTowns;
  std::size_t scripts, events, rivalStarter, playerStarter;
  std::size_t playtime, daycareInUse, daycareNickname, daycareOt, daycareRecord;
};

constexpr Layout kEnglish{11, 0x2F2C, 0x30C0, 0x462, 12,
    0x284E, 0x25F6, 0x25F3, 0x2850, 0x2601, 0x2602, 0x29D6,
    0x2605, 0x260A, 0x260E, 0x260D, 0x2610, 0x260F, 0x2611, 0x2609,
    0x25A3, 0x25B6, 0x25C9, 0x25CA, 0x27E6, 0x27E7,
    0x284C, 0x2852, 0x299C, 0x29AA, 0x29B7,
    0x289C, 0x29F3, 0x29C1, 0x29C3,
    0x2CED, 0x2CF4, 0x2CF5, 0x2D00, 0x2D0B};
constexpr Layout kJapanese{6, 0x2ED5, 0x302D, 0x566, 8,
    0x2844, 0x25F1, 0x25EE, 0x2846, 0x25F7, 0x25F8, 0x29CC,
    0x25FB, 0x2600, 0x2604, 0x2603, 0x2606, 0x2605, 0x2607, 0x25FF,
    0x259E, 0x25B1, 0x25C4, 0x25C5, 0x27DC, 0x27DD,
    0x2842, 0x2848, 0x2992, 0x29A0, 0x29AD,
    0x2892, 0x29E9, 0x29B7, 0x29B9,
    0x2CA0, 0x2CA7, 0x2CA8, 0x2CAE, 0x2CB4};

std::string StarterName(std::uint8_t species) {
  // Pinned pret/pokered internal species constants used by wPlayerStarter and
  // wRivalStarter. These bytes are part of wMainData and therefore SRAM.
  switch (species) {
  case 0x99:
    return "bulbasaur";
  case 0xB0:
    return "charmander";
  case 0xB1:
    return "squirtle";
  default:
    return "unknown";
  }
}

OrderedJson Text(const RedSave &save, std::size_t offset, std::size_t length,
                 bool japanese) {
  if (japanese) {
    const auto text = c::DecodeJapaneseText(save, offset, length);
    return {{"value", text.value}, {"losslessValue", text.value},
            {"rawHex", c::Hex(save.Slice(offset, length))},
            {"terminated", text.terminated},
            {"ambiguousGlyph", text.ambiguousGlyph},
            {"unsupportedByte", text.unsupportedByte}};
  }
  return {{"value", c::DecodeText(save, offset, length)},
          {"losslessValue", c::DecodeText(save, offset, length, true)},
          {"rawHex", c::Hex(save.Slice(offset, length))}};
}

OrderedJson Items(const RedSave &save, std::size_t countOffset,
                  std::size_t pairsOffset, std::size_t capacity) {
  const auto rawDeclared = save.At(countOffset);
  const auto declared = rawDeclared == 0xFF ? 0U : rawDeclared;
  const auto count = std::min<std::size_t>(declared, capacity);
  OrderedJson items = OrderedJson::array();
  for (std::size_t slot = 0; slot < count; ++slot) {
    const auto id = save.At(pairsOffset + slot * 2U);
    if (id == 0xFF)
      break;
    items.push_back({{"slot", slot + 1},
                     {"itemId", id},
                     {"itemName", data::ItemName(id)},
                     {"quantity", save.At(pairsOffset + slot * 2U + 1)}});
  }
  return {{"declaredCount", declared},
          {"count", items.size()},
          {"capacity", capacity},
          {"items", items}};
}

OrderedJson Pokemon(const RedSave &save, std::size_t record,
                    std::size_t recordSize, std::size_t otName,
                    std::size_t nickname, std::size_t position, bool party,
                    const Layout& layout, bool japanese) {
  const auto dv1 = save.At(record + 0x1B);
  const auto dv2 = save.At(record + 0x1C);
  OrderedJson moves = OrderedJson::array();
  for (std::size_t index = 0; index < 4; ++index) {
    const auto rawPp = save.At(record + 0x1D + index);
    const auto moveId = save.At(record + 0x08 + index);
    moves.push_back({{"slot", index + 1},
                     {"moveId", moveId},
                     {"moveName", data::MoveName(moveId)},
                     {"pp", rawPp & 0x3F},
                     {"ppUps", rawPp >> 6},
                     {"rawPp", rawPp}});
  }
  const auto speciesId = save.At(record);
  OrderedJson result = {
      {"position", position},
      {"speciesId", speciesId},
      {"speciesName", data::SpeciesName(speciesId)},
      {"pokedexNumber", data::PokedexNumber(speciesId) < 0
                            ? OrderedJson(nullptr)
                            : OrderedJson(data::PokedexNumber(speciesId))},
      {"level", save.At(record + (party ? 0x21 : 0x03))},
      {"nickname", Text(save, nickname, layout.nameLength, japanese)},
      {"otName", Text(save, otName, layout.nameLength, japanese)},
      {"currentHp", c::ReadU16BE(save, record + 0x01)},
      {"status", save.At(record + 0x04)},
      {"types",
       OrderedJson::array({save.At(record + 0x05), save.At(record + 0x06)})},
      {"catchRate", save.At(record + 0x07)},
      {"moves", moves},
      {"trainerId", c::ReadU16BE(save, record + 0x0C)},
      {"experience", c::ReadU24BE(save, record + 0x0E)},
      {"statExperience",
       {{"hp", c::ReadU16BE(save, record + 0x11)},
        {"attack", c::ReadU16BE(save, record + 0x13)},
        {"defense", c::ReadU16BE(save, record + 0x15)},
        {"speed", c::ReadU16BE(save, record + 0x17)},
        {"special", c::ReadU16BE(save, record + 0x19)}}},
      {"dvs",
       {{"attack", dv1 >> 4},
        {"defense", dv1 & 0x0F},
        {"speed", dv2 >> 4},
        {"special", dv2 & 0x0F},
        {"hp", ((dv1 >> 4) & 1U) * 8U + (dv1 & 1U) * 4U +
                   ((dv2 >> 4) & 1U) * 2U + (dv2 & 1U)}}},
      {"rawRecordHex", c::Hex(save.Slice(record, recordSize))}};
  if (party) {
    result["stats"] = {{"maxHp", c::ReadU16BE(save, record + 0x22)},
                       {"attack", c::ReadU16BE(save, record + 0x24)},
                       {"defense", c::ReadU16BE(save, record + 0x26)},
                       {"speed", c::ReadU16BE(save, record + 0x28)},
                       {"special", c::ReadU16BE(save, record + 0x2A)}};
  }
  return result;
}

OrderedJson Box(const RedSave &save, std::size_t base, int number,
                const Layout& layout, bool japanese) {
  const auto rawCount = save.At(base);
  // Unused external PC-box banks in otherwise valid saves may still contain
  // erased SRAM (0xFF). Treat that erased count byte as an empty box instead
  // of clamping it to 20 and decoding twenty 0xFF records as Pokemon.
  const auto declaredCount = rawCount == 0xFF ? 0U : rawCount;
  const auto capacity = japanese ? 30U : 20U;
  const auto count = std::min<std::size_t>(declaredCount, capacity);
  OrderedJson pokemon = OrderedJson::array();
  for (std::size_t index = 0; index < count; ++index) {
    pokemon.push_back(Pokemon(save, base + (japanese ? 0x20 : 0x16) + index * 0x21, 0x21,
                              base + (japanese ? 0x3FE : 0x2AA) + index * layout.nameLength,
                              base + (japanese ? 0x4B2 : 0x386) + index * layout.nameLength,
                              index + 1, false, layout, japanese));
  }
  return {{"boxNumber", number},
          {"declaredCount", declaredCount},
          {"count", pokemon.size()},
          {"speciesListHex", c::Hex(save.Slice(base + 1, capacity))},
          {"pokemon", pokemon},
          {"rawBlockHex", c::Hex(save.Slice(base, layout.boxSize))}};
}

OrderedJson Party(const RedSave &save, const Layout& layout, bool japanese) {
  const std::size_t base = layout.party;
  const auto storedCount = save.At(base);
  const auto rawCount = storedCount == 0xFF ? 0U : storedCount;
  const auto count = std::min<std::size_t>(rawCount, 6);
  OrderedJson pokemon = OrderedJson::array();
  for (std::size_t index = 0; index < count; ++index) {
    pokemon.push_back(Pokemon(save, base + 0x08 + index * 0x2C, 0x2C,
                              base + 0x110 + index * layout.nameLength,
                              base + (japanese ? 0x134 : 0x152) + index * layout.nameLength,
                              index + 1, true, layout, japanese));
  }
  return {{"declaredCount", rawCount},
          {"count", pokemon.size()},
          {"speciesListHex", c::Hex(save.Slice(base + 1, 6))},
          {"pokemon", pokemon}};
}

OrderedJson HallOfFame(const RedSave &save, const Layout& layout, bool japanese) {
  const auto storedCount = save.At(layout.hofCount);
  const auto count = storedCount == 0xFF
                         ? 0U
                         : std::min<std::size_t>(storedCount, 50);
  OrderedJson entries = OrderedJson::array();
  for (std::size_t entry = 0; entry < count; ++entry) {
    OrderedJson pokemon = OrderedJson::array();
    for (std::size_t slot = 0; slot < 6; ++slot) {
      const auto offset = 0x0598 + entry * 0x60 + slot * 0x10;
      const auto species = save.At(offset);
      if (species == 0 || species == 0xFF)
        continue;
      pokemon.push_back({{"partyOrder", slot + 1},
                         {"speciesId", species},
                         {"speciesName", data::SpeciesName(species)},
                         {"pokedexNumber", data::PokedexNumber(species)},
                         {"level", save.At(offset + 1)},
                         {"nickname", Text(save, offset + 2, layout.nameLength, japanese)},
                         {"rawSlotHex", c::Hex(save.Slice(offset, 0x10))}});
    }
    entries.push_back({{"entryNumber", entry + 1}, {"pokemon", pokemon}});
  }
  return {{"recordCount", count}, {"entries", entries}};
}

} // namespace

OrderedJson DecodeImpl(const RedSave &input, const std::string &logicalName,
                       const OrderedJson& integrity, bool japanese,
                       const std::string& sourceProfile,
                       const DecodeOptions &options) {
  const auto& layout = japanese ? kJapanese : kEnglish;
  const auto &all = input.BytesView();
  const auto standard = input.Slice(0, RedSave::ExpectedSize);
  const RedSave::Bytes trailing(
      all.begin() + static_cast<std::ptrdiff_t>(RedSave::ExpectedSize),
      all.end());
  const auto wholeHash = util::Sha256Hex(all);
  const auto standardHash = util::Sha256Hex(standard);
  OrderedJson boxes = OrderedJson::array();
  for (std::size_t index = 0; index < layout.boxCount; ++index)
    boxes.push_back(Box(input, japanese
                         ? validation::JapaneseSaveValidator::BoxOffset(index)
                         : validation::SaveValidator::BoxOffset(index),
                        static_cast<int>(index + 1), layout, japanese));
  const auto currentRaw = input.At(layout.selectedBox);
  const auto daycareMarker = input.At(layout.daycareInUse);
  const bool daycareInUse = daycareMarker != 0 && daycareMarker != 0xFF;
  const RedSave emptySave(
      RedSave::Bytes(RedSave::ExpectedSize, static_cast<std::uint8_t>(0)));
  const auto daycarePokemon =
      daycareInUse
          ? Pokemon(input, layout.daycareRecord, 0x21, layout.daycareOt,
                    layout.daycareNickname, 1, false, layout, japanese)
          : Pokemon(emptySave, layout.daycareRecord, 0x21, layout.daycareOt,
                    layout.daycareNickname, 1, false, layout, japanese);

  OrderedJson badges = OrderedJson::array();
  for (std::size_t bit = 0; bit < 8; ++bit)
    badges.push_back(
        {{"index", bit + 1}, {"owned", (input.At(layout.badges) & (1U << bit)) != 0}});

  OrderedJson decoded = {
      {"trainer",
       {{"name", Text(input, 0x2598, layout.nameLength, japanese)},
        {"trainerId", c::ReadU16BE(input, layout.trainerId)}}},
      {"rival", {{"name", Text(input, layout.rivalName, layout.nameLength, japanese)}}},
      {"moneyAndCoins",
       {{"money", c::ReadBcd(input, layout.money, 3)},
        {"coins", c::ReadBcd(input, layout.coins, 2)}}},
      {"badges",
       {{"raw", input.At(layout.badges)},
        {"mirrorRaw", input.At(layout.badgeMirror)},
        {"entries", badges}}},
      {"playtime",
       {{"hours", input.At(layout.playtime)},
        {"maxed", input.At(layout.playtime + 1) != 0},
        {"minutes", input.At(layout.playtime + 2)},
        {"seconds", input.At(layout.playtime + 3)},
        {"frames", input.At(layout.playtime + 4)}}},
      {"location",
       {{"mapId", input.At(layout.map)},
        {"mapName", data::MapName(input.At(layout.map))},
        {"x", input.At(layout.x)},
        {"y", input.At(layout.y)},
        {"xBlock", input.At(layout.xBlock)},
        {"yBlock", input.At(layout.yBlock)},
        {"previousMapId", input.At(layout.previousMap)},
        {"previousMapName", data::MapName(input.At(layout.previousMap))}}},
      {"options",
       {{"raw", input.At(layout.options)},
        {"textSpeed", input.At(layout.options) & 0x07},
        {"battleAnimationsDisabled", (input.At(layout.options) & 0x80) != 0},
        {"battleStyleSet", (input.At(layout.options) & 0x40) != 0},
        {"contrast", input.At(layout.contrast)}}},
      {"pokedex",
       {{"ownedCount", c::CountSetBits(input, layout.dexOwned, 19, 151)},
        {"seenCount", c::CountSetBits(input, layout.dexSeen, 19, 151)},
        {"ownedBitfieldHex", c::Hex(input.Slice(layout.dexOwned, 19))},
        {"seenBitfieldHex", c::Hex(input.Slice(layout.dexSeen, 19))}}},
      {"inventory",
       {{"bag", Items(input, layout.bagCount, layout.bagPairs, 20)},
        {"pcItems", Items(input, layout.pcItemCount, layout.pcItemPairs, 50)}}},
      {"party", Party(input, layout, japanese)},
      {"pcStorage", {{"boxes", boxes}}},
      {"currentBoxCache",
       {{"rawSelectedBoxValue", currentRaw},
        {"selectedBoxNumber", (currentRaw & 0x7F) + 1},
        {"hasChangedBoxesBefore", (currentRaw & 0x80) != 0},
        {"cache", Box(input, layout.boxCache, (currentRaw & 0x7F) + 1,
                       layout, japanese)}}},
      {"daycare",
       {{"inUse", daycareInUse}, {"pokemon", daycarePokemon}}},
      {"hallOfFame", HallOfFame(input, layout, japanese)},
      {"summaryCounts",
       {{"eventFlagsSet", c::CountSetBits(input, layout.events, 0x140, 0xA00)},
        {"missableObjectsSet", c::CountSetBits(input, layout.missable, 29, 228)},
        {"hiddenItemsSet", c::CountSetBits(input, layout.hiddenItems, 7, 54)},
        {"hiddenCoinsSet", c::CountSetBits(input, layout.hiddenCoins, 2, 12)},
        {"visitedTownsSet", c::CountSetBits(input, layout.visitedTowns, 2, 11)},
        {"nonzeroScriptBytes", 0},
        {"trainerFlagsSet", 0},
        {"staticEncounterFlagsSet", 0},
        {"storyFlagsSet", 0},
        {"classification",
         "aggregate counts; named classification deferred"}}}};
  decoded["worldStateRaw"] = {
      {"eventFlagsHex", c::Hex(input.Slice(layout.events, 0x140))},
      {"scriptsHex", c::Hex(input.Slice(layout.scripts, 0x100))},
      {"missableObjectsHex", c::Hex(input.Slice(layout.missable, 29))},
      {"hiddenItemsHex", c::Hex(input.Slice(layout.hiddenItems, 7))},
      {"hiddenCoinsHex", c::Hex(input.Slice(layout.hiddenCoins, 2))},
      {"visitedTownsHex", c::Hex(input.Slice(layout.visitedTowns, 2))}};
  const auto namedState = events::DecodeNamedState(input.Slice(layout.events, 0x140));
  for (const auto &[key, value] : namedState.items())
    decoded[key] = value;
  bool gotStarter = false;
  for (const auto &record : decoded.at("storyProgress").at("storyFlags")) {
    if (record.value("name", "") == "EVENT_GOT_STARTER") {
      gotStarter = record.value("completed", false);
      break;
    }
  }
  // Pinned pret/pokered lays these fields out as wRivalStarter, one reserved
  // byte, then wPlayerStarter inside saved wMainData. Their standard SRAM file
  // offsets are 0x29C1 and 0x29C3 respectively.
  decoded["worldState"] = {
      {"storyEvidence",
       {{"gotStarter", gotStarter},
        {"starterChoice", StarterName(input.At(layout.playerStarter))},
        {"rivalStarterChoice", StarterName(input.At(layout.rivalStarter))},
        {"starterSpeciesId", input.At(layout.playerStarter)},
        {"rivalStarterSpeciesId", input.At(layout.rivalStarter)}}}};
  std::size_t scripts = 0;
  for (const auto byte : input.Slice(layout.scripts, 97))
    if (byte != 0)
      ++scripts;
  decoded["summaryCounts"]["nonzeroScriptBytes"] = scripts;
  decoded["summaryCounts"]["trainerFlagsSet"] =
      decoded.at("trainerBattles").at("complete");
  decoded["summaryCounts"]["staticEncounterFlagsSet"] =
      decoded.at("staticBattles").at("complete");
  decoded["summaryCounts"]["storyFlagsSet"] =
      decoded.at("storyProgress").at("complete");
  decoded["summaryCounts"]["classification"] =
      "verified named catalog plus lossless raw event bytes";

  OrderedJson document = {
      {"schema",
       {{"format", japanese ? "pkmn-red-jp-master-save" : "pkmn-red-master-save"},
        {"schemaVersion", "0.1.0"},
        {"game", japanese ? "Pocket Monsters Red (Japan)" : "Pokemon Red"},
        {"generation", 1},
        {"regionAssumption", japanese ? "Japan" : "USA-Europe"},
        {"canonicalExtension", japanese ? ".red.jp.json" : ".red.json"},
        {"lossless", options.includePhysicalImage},
        {"stability", "draft"}}},
      {"tool", {{"name", "pkmn"}, {"version", std::string(kVersion)}}},
      {"source",
       {{"fileName", logicalName},
        {"profile", sourceProfile},
        {"fileSize", {{"decimal", input.Size()}}},
        {"standardSramSize", {{"decimal", RedSave::ExpectedSize}}},
        {"trailingByteCount", trailing.size()},
        {"hashes",
         {{"wholeFileSha256", wholeHash},
          {"standardSramSha256", standardHash},
          {"trailingDataSha256",
           trailing.empty() ? OrderedJson(nullptr)
                            : OrderedJson(util::Sha256Hex(trailing))}}}}},
      {"integrity", integrity},
      {"decoded", decoded},
      {"reconstruction",
       {{"available", options.includePhysicalImage},
        {"policy", "use-physical-image-for-no-edit-reconstruction"},
        {"semanticFieldsAreInformational", true}}},
      {"diagnostics",
       {{"namedEventClassification", "deferred"},
        {"arbitraryLocationGeneration", "restricted"}}}};
  if (options.includePhysicalImage) {
    document["physicalImage"] = {{"encoding", "hex-uppercase-continuous"},
                                 {"standardSramHex", c::Hex(standard)},
                                 {"trailingDataHex", c::Hex(trailing)},
                                 {"totalLength", input.Size()},
                                 {"standardSramLength", RedSave::ExpectedSize},
                                 {"trailingLength", trailing.size()}};
  }
  return document;
}

OrderedJson Decode(const RedSave &input, const std::string &logicalName,
                   const validation::ValidationReport &report,
                   const DecodeOptions &options) {
  OrderedJson boxChecksums = OrderedJson::array();
  for (std::size_t index = 0; index < report.boxes.size(); ++index)
    boxChecksums.push_back({{"box", index + 1},
                            {"valid", report.boxes[index].Valid()},
                            {"stored", report.boxes[index].stored},
                            {"calculated", report.boxes[index].expected}});
  OrderedJson integrity = {
      {"allValid", report.Valid()},
      {"mainChecksum", {{"valid", report.main.Valid()},
                        {"storedValue", report.main.stored},
                        {"calculatedValue", report.main.expected}}},
      {"bank2AllChecksumValid", report.banks[0].Valid()},
      {"bank3AllChecksumValid", report.banks[1].Valid()},
      {"boxChecksums", boxChecksums}};
  auto document = DecodeImpl(input, logicalName, integrity, false,
                             "GEN1_RED_INTL", options);
  document["source"].erase("profile");
  return document;
}

OrderedJson DecodeJapanese(const RedSave &input, const std::string &logicalName,
                           const validation::JapaneseValidationReport &report,
                           const std::string& sourceProfile,
                           const DecodeOptions &options) {
  OrderedJson banks = OrderedJson::array();
  for (std::size_t index = 0; index < report.bankDiagnostics.size(); ++index)
    banks.push_back({{"bank", index + 2},
                     {"stored", report.bankDiagnostics[index].stored},
                     {"simpleCalculated", report.bankDiagnostics[index].expected},
                     {"acceptanceRole", "diagnostic-only"}});
  const OrderedJson integrity = {
      {"allValid", report.Valid()},
      {"mainChecksum", {{"valid", report.main.Valid()},
                        {"storedValue", report.main.stored},
                        {"calculatedValue", report.main.expected}}},
      {"externalBankChecksums", banks},
      {"errors", report.errors}};
  auto document = DecodeImpl(input, logicalName, integrity, true,
                             sourceProfile, options);
  if (sourceProfile == "JP_GREEN_REV0") {
    document["schema"]["format"] = "pkmn-green-jp-master-save";
    document["schema"]["game"] = "Pocket Monsters Green (Japan)";
    document["schema"]["canonicalExtension"] = ".green.jp.json";
  }
  document["diagnostics"]["japaneseLayoutSource"] =
      "Narishma-gb/pokegreen@953f41b34108621b2bf13c3b1e53abfc9c3e5aec";
  return document;
}

std::string Serialize(const OrderedJson &document) {
  return document.dump(2) + "\n";
}

} // namespace pkmn::cli::red::json
