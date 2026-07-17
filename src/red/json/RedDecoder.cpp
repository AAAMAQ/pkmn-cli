#include "red/json/RedDecoder.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "app/Version.hpp"
#include "red/codec/Gen1Codec.hpp"
#include "red/events/EventCatalog.hpp"
#include "red/data/Gen1Names.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/Sha256.hpp"

namespace pkmn::cli::red::json {
namespace {

namespace c = pkmn::cli::red::codec;
using pkmn::cli::red::save::RedSave;

OrderedJson Text(const RedSave &save, std::size_t offset, std::size_t length) {
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
                    std::size_t nickname, std::size_t position, bool party) {
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
      {"nickname", Text(save, nickname, 11)},
      {"otName", Text(save, otName, 11)},
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

OrderedJson Box(const RedSave &save, std::size_t base, int number) {
  const auto rawCount = save.At(base);
  // Unused external PC-box banks in otherwise valid saves may still contain
  // erased SRAM (0xFF). Treat that erased count byte as an empty box instead
  // of clamping it to 20 and decoding twenty 0xFF records as Pokemon.
  const auto declaredCount = rawCount == 0xFF ? 0U : rawCount;
  const auto count = std::min<std::size_t>(declaredCount, 20);
  OrderedJson pokemon = OrderedJson::array();
  for (std::size_t index = 0; index < count; ++index) {
    pokemon.push_back(Pokemon(save, base + 0x16 + index * 0x21, 0x21,
                              base + 0x2AA + index * 11,
                              base + 0x386 + index * 11, index + 1, false));
  }
  return {{"boxNumber", number},
          {"declaredCount", declaredCount},
          {"count", pokemon.size()},
          {"speciesListHex", c::Hex(save.Slice(base + 1, 20))},
          {"pokemon", pokemon},
          {"rawBlockHex", c::Hex(save.Slice(base, 0x462))}};
}

OrderedJson Party(const RedSave &save) {
  constexpr std::size_t base = 0x2F2C;
  const auto storedCount = save.At(base);
  const auto rawCount = storedCount == 0xFF ? 0U : storedCount;
  const auto count = std::min<std::size_t>(rawCount, 6);
  OrderedJson pokemon = OrderedJson::array();
  for (std::size_t index = 0; index < count; ++index) {
    pokemon.push_back(Pokemon(save, base + 0x08 + index * 0x2C, 0x2C,
                              base + 0x110 + index * 11,
                              base + 0x152 + index * 11, index + 1, true));
  }
  return {{"declaredCount", rawCount},
          {"count", pokemon.size()},
          {"speciesListHex", c::Hex(save.Slice(base + 1, 6))},
          {"pokemon", pokemon}};
}

OrderedJson HallOfFame(const RedSave &save) {
  const auto storedCount = save.At(0x284E);
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
                         {"nickname", Text(save, offset + 2, 11)},
                         {"rawSlotHex", c::Hex(save.Slice(offset, 0x10))}});
    }
    entries.push_back({{"entryNumber", entry + 1}, {"pokemon", pokemon}});
  }
  return {{"recordCount", count}, {"entries", entries}};
}

} // namespace

OrderedJson Decode(const RedSave &input, const std::string &logicalName,
                   const validation::ValidationReport &report,
                   const DecodeOptions &options) {
  const auto &all = input.BytesView();
  const auto standard = input.Slice(0, RedSave::ExpectedSize);
  const RedSave::Bytes trailing(
      all.begin() + static_cast<std::ptrdiff_t>(RedSave::ExpectedSize),
      all.end());
  const auto wholeHash = util::Sha256Hex(all);
  const auto standardHash = util::Sha256Hex(standard);
  OrderedJson boxes = OrderedJson::array();
  for (std::size_t index = 0; index < 12; ++index)
    boxes.push_back(Box(input, validation::SaveValidator::BoxOffset(index),
                        static_cast<int>(index + 1)));
  const auto currentRaw = input.At(0x284C);
  const auto daycareMarker = input.At(0x2CF4);
  const bool daycareInUse = daycareMarker != 0 && daycareMarker != 0xFF;
  const RedSave emptySave(
      RedSave::Bytes(RedSave::ExpectedSize, static_cast<std::uint8_t>(0)));
  const auto daycarePokemon =
      daycareInUse
          ? Pokemon(input, 0x2D0B, 0x21, 0x2D00, 0x2CF5, 1, false)
          : Pokemon(emptySave, 0x2D0B, 0x21, 0x2D00, 0x2CF5, 1, false);

  OrderedJson boxChecksums = OrderedJson::array();
  for (std::size_t index = 0; index < report.boxes.size(); ++index)
    boxChecksums.push_back({{"box", index + 1},
                            {"valid", report.boxes[index].Valid()},
                            {"stored", report.boxes[index].stored},
                            {"calculated", report.boxes[index].expected}});

  OrderedJson badges = OrderedJson::array();
  for (std::size_t bit = 0; bit < 8; ++bit)
    badges.push_back(
        {{"index", bit + 1}, {"owned", (input.At(0x2602) & (1U << bit)) != 0}});

  OrderedJson decoded = {
      {"trainer",
       {{"name", Text(input, 0x2598, 11)},
        {"trainerId", c::ReadU16BE(input, 0x2605)}}},
      {"rival", {{"name", Text(input, 0x25F6, 11)}}},
      {"moneyAndCoins",
       {{"money", c::ReadBcd(input, 0x25F3, 3)},
        {"coins", c::ReadBcd(input, 0x2850, 2)}}},
      {"badges",
       {{"raw", input.At(0x2602)},
        {"mirrorRaw", input.At(0x29D6)},
        {"entries", badges}}},
      {"playtime",
       {{"hours", input.At(0x2CED)},
        {"maxed", input.At(0x2CEE) != 0},
        {"minutes", input.At(0x2CEF)},
        {"seconds", input.At(0x2CF0)},
        {"frames", input.At(0x2CF1)}}},
      {"location",
       {{"mapId", input.At(0x260A)},
        {"mapName", data::MapName(input.At(0x260A))},
        {"x", input.At(0x260E)},
        {"y", input.At(0x260D)},
        {"xBlock", input.At(0x2610)},
        {"yBlock", input.At(0x260F)},
        {"previousMapId", input.At(0x2611)},
        {"previousMapName", data::MapName(input.At(0x2611))}}},
      {"options",
       {{"raw", input.At(0x2601)},
        {"textSpeed", input.At(0x2601) & 0x07},
        {"battleAnimationsDisabled", (input.At(0x2601) & 0x80) != 0},
        {"battleStyleSet", (input.At(0x2601) & 0x40) != 0},
        {"contrast", input.At(0x2609)}}},
      {"pokedex",
       {{"ownedCount", c::CountSetBits(input, 0x25A3, 19, 151)},
        {"seenCount", c::CountSetBits(input, 0x25B6, 19, 151)},
        {"ownedBitfieldHex", c::Hex(input.Slice(0x25A3, 19))},
        {"seenBitfieldHex", c::Hex(input.Slice(0x25B6, 19))}}},
      {"inventory",
       {{"bag", Items(input, 0x25C9, 0x25CA, 20)},
        {"pcItems", Items(input, 0x27E6, 0x27E7, 50)}}},
      {"party", Party(input)},
      {"pcStorage", {{"boxes", boxes}}},
      {"currentBoxCache",
       {{"rawSelectedBoxValue", currentRaw},
        {"selectedBoxNumber", (currentRaw & 0x7F) + 1},
        {"hasChangedBoxesBefore", (currentRaw & 0x80) != 0},
        {"cache", Box(input, 0x30C0, (currentRaw & 0x7F) + 1)}}},
      {"daycare",
       {{"inUse", daycareInUse}, {"pokemon", daycarePokemon}}},
      {"hallOfFame", HallOfFame(input)},
      {"summaryCounts",
       {{"eventFlagsSet", c::CountSetBits(input, 0x29F3, 0x140, 0xA00)},
        {"missableObjectsSet", c::CountSetBits(input, 0x2852, 29, 228)},
        {"hiddenItemsSet", c::CountSetBits(input, 0x299C, 7, 54)},
        {"hiddenCoinsSet", c::CountSetBits(input, 0x29AA, 2, 12)},
        {"visitedTownsSet", c::CountSetBits(input, 0x29B7, 2, 11)},
        {"nonzeroScriptBytes", 0},
        {"trainerFlagsSet", 0},
        {"staticEncounterFlagsSet", 0},
        {"storyFlagsSet", 0},
        {"classification",
         "aggregate counts; named classification deferred"}}}};
  decoded["worldStateRaw"] = {
      {"eventFlagsHex", c::Hex(input.Slice(0x29F3, 0x140))},
      {"scriptsHex", c::Hex(input.Slice(0x289C, 0x100))},
      {"missableObjectsHex", c::Hex(input.Slice(0x2852, 29))},
      {"hiddenItemsHex", c::Hex(input.Slice(0x299C, 7))},
      {"hiddenCoinsHex", c::Hex(input.Slice(0x29AA, 2))},
      {"visitedTownsHex", c::Hex(input.Slice(0x29B7, 2))}};
  const auto namedState = events::DecodeNamedState(input.Slice(0x29F3, 0x140));
  for (const auto &[key, value] : namedState.items())
    decoded[key] = value;
  std::size_t scripts = 0;
  for (const auto byte : input.Slice(0x289C, 97))
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
       {{"format", "pkmn-red-master-save"},
        {"schemaVersion", "0.1.0"},
        {"game", "Pokemon Red"},
        {"generation", 1},
        {"regionAssumption", "USA-Europe"},
        {"canonicalExtension", ".red.json"},
        {"lossless", options.includePhysicalImage},
        {"stability", "draft"}}},
      {"tool", {{"name", "pkmn"}, {"version", std::string(kVersion)}}},
      {"source",
       {{"fileName", logicalName},
        {"fileSize", {{"decimal", input.Size()}}},
        {"standardSramSize", {{"decimal", RedSave::ExpectedSize}}},
        {"trailingByteCount", trailing.size()},
        {"hashes",
         {{"wholeFileSha256", wholeHash},
          {"standardSramSha256", standardHash},
          {"trailingDataSha256",
           trailing.empty() ? OrderedJson(nullptr)
                            : OrderedJson(util::Sha256Hex(trailing))}}}}},
      {"integrity",
       {{"allValid", report.Valid()},
        {"mainChecksum",
         {{"valid", report.main.Valid()},
          {"storedValue", report.main.stored},
          {"calculatedValue", report.main.expected}}},
        {"bank2AllChecksumValid", report.banks[0].Valid()},
        {"bank3AllChecksumValid", report.banks[1].Valid()},
        {"boxChecksums", boxChecksums}}},
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

std::string Serialize(const OrderedJson &document) {
  return document.dump(2) + "\n";
}

} // namespace pkmn::cli::red::json
