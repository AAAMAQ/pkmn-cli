#include "red/generation/SemanticGenerator.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "red/codec/Gen1Codec.hpp"
#include "red/events/EventCatalog.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/ResourceLocator.hpp"
#include "util/Sha256.hpp"

namespace pkmn::cli::red::generation {
namespace {
using Bytes = save::RedSave::Bytes;
using Json = json::OrderedJson;

Bytes LoadTemplate() {
  std::ifstream input(util::RedTemplatePath(),
                      std::ios::binary | std::ios::ate);
  if (!input)
    throw std::runtime_error("could not open installed Red template");
  const auto size = input.tellg();
  Bytes bytes(static_cast<std::size_t>(size));
  input.seekg(0);
  input.read(reinterpret_cast<char *>(bytes.data()), size);
  if (bytes.size() != save::RedSave::ExpectedSize ||
      util::Sha256Hex(bytes) !=
          "248bc35328be435b16b47e2bb87c4e9732c2b5c92a95450839ed4619f74eb2e7")
    throw std::runtime_error(
        "installed Red template failed identity validation");
  return bytes;
}

void Range(Bytes &bytes, std::size_t offset, const Bytes &values) {
  if (offset > bytes.size() || values.size() > bytes.size() - offset)
    throw std::runtime_error("generation write out of range");
  std::copy(values.begin(), values.end(),
            bytes.begin() + static_cast<std::ptrdiff_t>(offset));
}
void U8(Bytes &b, std::size_t o, std::uint8_t v) { b.at(o) = v; }
void U16(Bytes &b, std::size_t o, std::uint16_t v) {
  U8(b, o, v >> 8);
  U8(b, o + 1, v & 255);
}
void U24(Bytes &b, std::size_t o, std::uint32_t v) {
  U8(b, o, v >> 16);
  U8(b, o + 1, (v >> 8) & 255);
  U8(b, o + 2, v & 255);
}
void Bcd(Bytes &b, std::size_t o, std::size_t len, std::uint32_t v) {
  for (std::size_t i = 0; i < len; ++i) {
    const auto p = o + len - 1 - i;
    U8(b, p, static_cast<std::uint8_t>((v % 10) + (v / 10 % 10) * 16));
    v /= 100;
  }
}
void Zero(Bytes &b, std::size_t o, std::size_t n) {
  std::fill(b.begin() + static_cast<std::ptrdiff_t>(o),
            b.begin() + static_cast<std::ptrdiff_t>(o + n), 0);
}

void Name(Bytes &bytes, std::size_t offset, const Json &text) {
  Range(bytes, offset,
        codec::EncodeText(text.value("losslessValue", text.value("value", "")),
                          11));
}

void Pokemon(Bytes &bytes, std::size_t record, const Json &mon, bool party) {
  U8(bytes, record, mon.at("speciesId").get<std::uint8_t>());
  U16(bytes, record + 1, mon.at("currentHp").get<std::uint16_t>());
  U8(bytes, record + 3, mon.at("level").get<std::uint8_t>());
  U8(bytes, record + 4, mon.at("status").get<std::uint8_t>());
  U8(bytes, record + 5, mon.at("types").at(0).get<std::uint8_t>());
  U8(bytes, record + 6, mon.at("types").at(1).get<std::uint8_t>());
  U8(bytes, record + 7, mon.at("catchRate").get<std::uint8_t>());
  for (std::size_t i = 0; i < 4; ++i) {
    const auto &move = mon.at("moves").at(i);
    U8(bytes, record + 8 + i, move.at("moveId").get<std::uint8_t>());
    U8(bytes, record + 0x1D + i,
       static_cast<std::uint8_t>(move.at("pp").get<int>() |
                                 (move.at("ppUps").get<int>() << 6)));
  }
  U16(bytes, record + 0x0C, mon.at("trainerId").get<std::uint16_t>());
  U24(bytes, record + 0x0E, mon.at("experience").get<std::uint32_t>());
  const auto &se = mon.at("statExperience");
  U16(bytes, record + 0x11, se.at("hp"));
  U16(bytes, record + 0x13, se.at("attack"));
  U16(bytes, record + 0x15, se.at("defense"));
  U16(bytes, record + 0x17, se.at("speed"));
  U16(bytes, record + 0x19, se.at("special"));
  const auto &dv = mon.at("dvs");
  U8(bytes, record + 0x1B,
     static_cast<std::uint8_t>((dv.at("attack").get<int>() << 4) |
                               dv.at("defense").get<int>()));
  U8(bytes, record + 0x1C,
     static_cast<std::uint8_t>((dv.at("speed").get<int>() << 4) |
                               dv.at("special").get<int>()));
  if (party) {
    const auto &s = mon.at("stats");
    U8(bytes, record + 0x21, mon.at("level"));
    U16(bytes, record + 0x22, s.at("maxHp"));
    U16(bytes, record + 0x24, s.at("attack"));
    U16(bytes, record + 0x26, s.at("defense"));
    U16(bytes, record + 0x28, s.at("speed"));
    U16(bytes, record + 0x2A, s.at("special"));
  }
}

void Box(Bytes &bytes, std::size_t base, const Json &box) {
  Zero(bytes, base, 0x462);
  const auto &mons = box.at("pokemon");
  if (mons.size() > 20)
    throw std::runtime_error("box exceeds 20 Pokemon");
  U8(bytes, base, static_cast<std::uint8_t>(mons.size()));
  U8(bytes, base + 1 + mons.size(), 0xFF);
  for (std::size_t i = 0; i < mons.size(); ++i) {
    const auto &mon = mons.at(i);
    U8(bytes, base + 1 + i, mon.at("speciesId"));
    Pokemon(bytes, base + 0x16 + i * 0x21, mon, false);
    Name(bytes, base + 0x2AA + i * 11, mon.at("otName"));
    Name(bytes, base + 0x386 + i * 11, mon.at("nickname"));
  }
}

void Items(Bytes &bytes, std::size_t count, std::size_t pairs,
           std::size_t capacity, const Json &list) {
  Zero(bytes, count, 1 + capacity * 2 + 1);
  const auto &items = list.at("items");
  if (items.size() > capacity)
    throw std::runtime_error("inventory capacity exceeded");
  U8(bytes, count, static_cast<std::uint8_t>(items.size()));
  for (std::size_t i = 0; i < items.size(); ++i) {
    U8(bytes, pairs + i * 2, items.at(i).at("itemId"));
    U8(bytes, pairs + i * 2 + 1, items.at(i).at("quantity"));
  }
  U8(bytes, pairs + items.size() * 2, 0xFF);
}

void HexRange(Bytes &bytes, std::size_t offset, std::size_t expected,
              const Json &raw, const char *key) {
  auto v = codec::DecodeHex(raw.at(key));
  if (v.size() != expected)
    throw std::runtime_error(std::string(key) + " has wrong length");
  Range(bytes, offset, v);
}

} // namespace

void RepairChecksums(Bytes &bytes) {
  for (std::size_t box = 0; box < 12; ++box) {
    const auto start = validation::SaveValidator::BoxOffset(box);
    std::uint32_t sum = 0;
    for (std::size_t i = start; i < start + 0x462; ++i)
      sum += bytes[i];
    bytes[(box < 6 ? 0x5A4D : 0x7A4D) + (box % 6)] =
        static_cast<std::uint8_t>(~(sum & 255));
  }
  for (std::size_t bank = 0; bank < 2; ++bank) {
    std::uint32_t sum = 0;
    for (std::size_t i = bank ? 0x6000 : 0x4000; i < (bank ? 0x7A4C : 0x5A4C);
         ++i)
      sum += bytes[i];
    bytes[bank ? 0x7A4C : 0x5A4C] = static_cast<std::uint8_t>(~(sum & 255));
  }
  std::uint32_t sum = 0;
  for (std::size_t i = 0x2598; i <= 0x3522; ++i)
    sum += bytes[i];
  bytes[0x3523] = static_cast<std::uint8_t>(~(sum & 255));
}

Result Generate(const Json &document) {
  if (document.at("schema").value("schemaVersion", "") != "0.1.0")
    throw std::runtime_error("unsupported schema version");
  auto bytes = LoadTemplate();
  const auto &d = document.at("decoded");
  std::vector<std::string> warnings;
  Name(bytes, 0x2598, d.at("trainer").at("name"));
  Name(bytes, 0x25F6, d.at("rival").at("name"));
  U16(bytes, 0x2605, d.at("trainer").at("trainerId"));
  Bcd(bytes, 0x25F3, 3, d.at("moneyAndCoins").at("money"));
  Bcd(bytes, 0x2850, 2, d.at("moneyAndCoins").at("coins"));
  U8(bytes, 0x2601, d.at("options").at("raw"));
  U8(bytes, 0x2609, d.at("options").at("contrast"));
  U8(bytes, 0x2602, d.at("badges").at("raw"));
  U8(bytes, 0x29D6, d.at("badges").at("raw"));
  const auto &pt = d.at("playtime");
  U8(bytes, 0x2CED, pt.at("hours"));
  U8(bytes, 0x2CEE, pt.at("maxed").get<bool>() ? 1 : 0);
  U8(bytes, 0x2CEF, pt.at("minutes"));
  U8(bytes, 0x2CF0, pt.at("seconds"));
  U8(bytes, 0x2CF1, pt.at("frames"));
  const auto &loc = d.at("location");
  if (loc.at("mapId") != 38 || loc.at("x") != 3 || loc.at("y") != 6)
    warnings.push_back(
        "Unsafe source location canonicalized to Red's house second floor.");
  U8(bytes, 0x260A, 38);
  U8(bytes, 0x260E, 3);
  U8(bytes, 0x260D, 6);
  U8(bytes, 0x2610, 1);
  U8(bytes, 0x260F, 0);
  U8(bytes, 0x2611, 0);
  Range(bytes, 0x25A3,
        codec::DecodeHex(d.at("pokedex").at("ownedBitfieldHex")));
  Range(bytes, 0x25B6, codec::DecodeHex(d.at("pokedex").at("seenBitfieldHex")));
  Items(bytes, 0x25C9, 0x25CA, 20, d.at("inventory").at("bag"));
  Items(bytes, 0x27E6, 0x27E7, 50, d.at("inventory").at("pcItems"));
  Zero(bytes, 0x2F2C, 0x194);
  const auto &party = d.at("party").at("pokemon");
  if (party.size() > 6)
    throw std::runtime_error("party exceeds six Pokemon");
  U8(bytes, 0x2F2C, static_cast<std::uint8_t>(party.size()));
  U8(bytes, 0x2F2D + party.size(), 0xFF);
  for (std::size_t i = 0; i < party.size(); ++i) {
    const auto &mon = party.at(i);
    U8(bytes, 0x2F2D + i, mon.at("speciesId"));
    Pokemon(bytes, 0x2F34 + i * 0x2C, mon, true);
    Name(bytes, 0x303C + i * 11, mon.at("otName"));
    Name(bytes, 0x307E + i * 11, mon.at("nickname"));
  }
  const auto &boxes = d.at("pcStorage").at("boxes");
  if (boxes.size() != 12)
    throw std::runtime_error("generation requires all 12 boxes");
  for (std::size_t i = 0; i < 12; ++i)
    Box(bytes, validation::SaveValidator::BoxOffset(i), boxes.at(i));
  const auto &current = d.at("currentBoxCache");
  const auto selected = current.at("selectedBoxNumber").get<int>();
  if (selected < 1 || selected > 12)
    throw std::runtime_error("selected box must be in 1..12");
  U8(bytes, 0x284C,
     static_cast<std::uint8_t>(
         (selected - 1) |
         (current.at("hasChangedBoxesBefore").get<bool>() ? 0x80 : 0)));
  Box(bytes, 0x30C0, current.at("cache"));
  Zero(bytes, 0x2CF4, 0x39);
  const auto &daycare = d.at("daycare");
  U8(bytes, 0x2CF4, daycare.at("inUse").get<bool>() ? 1 : 0);
  if (daycare.at("inUse").get<bool>()) {
    const auto &mon = daycare.at("pokemon");
    Pokemon(bytes, 0x2D0B, mon, false);
    Name(bytes, 0x2D00, mon.at("otName"));
    Name(bytes, 0x2CF5, mon.at("nickname"));
  }
  Zero(bytes, 0x0598, 0x12C0);
  const auto &hof = d.at("hallOfFame").at("entries");
  if (hof.size() > 50)
    throw std::runtime_error("Hall of Fame exceeds 50 entries");
  U8(bytes, 0x284E, static_cast<std::uint8_t>(hof.size()));
  for (std::size_t e = 0; e < hof.size(); ++e)
    for (const auto &mon : hof.at(e).at("pokemon")) {
      const auto slot = mon.at("partyOrder").get<std::size_t>() - 1;
      const auto o = 0x0598 + e * 0x60 + slot * 0x10;
      U8(bytes, o, mon.at("speciesId"));
      U8(bytes, o + 1, mon.at("level"));
      Name(bytes, o + 2, mon.at("nickname"));
    }
  const auto &raw = d.at("worldStateRaw");
  auto eventBytes = codec::DecodeHex(raw.at("eventFlagsHex").get<std::string>());
  if (eventBytes.size() != 0x140)
    throw std::runtime_error("eventFlagsHex has the wrong byte length");
  if (d.contains("events") || d.contains("trainerBattles") ||
      d.contains("staticBattles") || d.contains("storyProgress"))
    events::ApplyNamedState(d, eventBytes);
  std::copy(eventBytes.begin(), eventBytes.end(), bytes.begin() + 0x29F3);
  HexRange(bytes, 0x289C, 0x100, raw, "scriptsHex");
  HexRange(bytes, 0x2852, 29, raw, "missableObjectsHex");
  HexRange(bytes, 0x299C, 7, raw, "hiddenItemsHex");
  HexRange(bytes, 0x29AA, 2, raw, "hiddenCoinsHex");
  HexRange(bytes, 0x29B7, 2, raw, "visitedTownsHex");
  RepairChecksums(bytes);
  const auto integrity =
      validation::SaveValidator::Validate(save::RedSave(bytes));
  if (!integrity.Valid())
    throw std::runtime_error(
        "generated save failed internal integrity validation");
  Json ranges = Json::array(
      {{{"name", "trainer name"}, {"start", 0x2598}, {"endInclusive", 0x25A2}},
       {{"name", "pokedex owned"}, {"start", 0x25A3}, {"endInclusive", 0x25B5}},
       {{"name", "pokedex seen"}, {"start", 0x25B6}, {"endInclusive", 0x25C8}},
       {{"name", "bag"}, {"start", 0x25C9}, {"endInclusive", 0x25F2}},
       {{"name", "money"}, {"start", 0x25F3}, {"endInclusive", 0x25F5}},
       {{"name", "rival name"}, {"start", 0x25F6}, {"endInclusive", 0x2600}},
       {{"name", "options"}, {"start", 0x2601}, {"endInclusive", 0x2602}},
       {{"name", "trainer id"}, {"start", 0x2605}, {"endInclusive", 0x2606}},
       {{"name", "contrast"}, {"start", 0x2609}, {"endInclusive", 0x2609}},
       {{"name", "safe location"}, {"start", 0x260A}, {"endInclusive", 0x2611}},
       {{"name", "pc items"}, {"start", 0x27E6}, {"endInclusive", 0x284B}},
       {{"name", "current box selector"},
        {"start", 0x284C},
        {"endInclusive", 0x284C}},
       {{"name", "hall count and coins"},
        {"start", 0x284E},
        {"endInclusive", 0x2851}},
       {{"name", "missables"}, {"start", 0x2852}, {"endInclusive", 0x286E}},
       {{"name", "scripts"}, {"start", 0x289C}, {"endInclusive", 0x299B}},
       {{"name", "hidden items"}, {"start", 0x299C}, {"endInclusive", 0x29A2}},
       {{"name", "hidden coins"}, {"start", 0x29AA}, {"endInclusive", 0x29AB}},
       {{"name", "visited towns"}, {"start", 0x29B7}, {"endInclusive", 0x29B8}},
       {{"name", "badge mirror"}, {"start", 0x29D6}, {"endInclusive", 0x29D6}},
       {{"name", "events"}, {"start", 0x29F3}, {"endInclusive", 0x2B32}},
       {{"name", "playtime"}, {"start", 0x2CED}, {"endInclusive", 0x2CF1}},
       {{"name", "daycare"}, {"start", 0x2CF4}, {"endInclusive", 0x2D2C}},
       {{"name", "party"}, {"start", 0x2F2C}, {"endInclusive", 0x30BF}},
       {{"name", "current box cache"},
        {"start", 0x30C0},
        {"endInclusive", 0x3521}},
       {{"name", "main checksum"}, {"start", 0x3523}, {"endInclusive", 0x3523}},
       {{"name", "hall of fame"},
        {"start", 0x0598},
        {"endInclusive", 0x1857}}});
  for (std::size_t i = 0; i < 12; ++i)
    ranges.push_back(
        {{"name", "permanent box " + std::to_string(i + 1)},
         {"start", validation::SaveValidator::BoxOffset(i)},
         {"endInclusive", validation::SaveValidator::BoxOffset(i) + 0x461}});
  ranges.push_back({{"name", "bank 2 integrity"},
                    {"start", 0x5A4C},
                    {"endInclusive", 0x5A52}});
  ranges.push_back({{"name", "bank 3 integrity"},
                    {"start", 0x7A4C},
                    {"endInclusive", 0x7A52}});
  std::size_t overlaps = 0;
  for (std::size_t i = 0; i < ranges.size(); ++i)
    for (std::size_t j = i + 1; j < ranges.size(); ++j)
      if (ranges[i]["start"].get<std::size_t>() <=
              ranges[j]["endInclusive"].get<std::size_t>() &&
          ranges[j]["start"].get<std::size_t>() <=
              ranges[i]["endInclusive"].get<std::size_t>())
        ++overlaps;
  if (overlaps != 0)
    throw std::runtime_error("declared generation write ranges overlap");
  Json report = {
      {"generator", "pkmn internal Red semantic generator"},
      {"physicalImageIgnored", true},
      {"templateSha256",
       "248bc35328be435b16b47e2bb87c4e9732c2b5c92a95450839ed4619f74eb2e7"},
      {"outputSha256", util::Sha256Hex(bytes)},
      {"outputSize", bytes.size()},
      {"integrityValid", true},
      {"locationCanonicalized", !warnings.empty()},
      {"warnings", warnings},
      {"writeOverlapCount", overlaps},
      {"writeRanges", ranges}};
  std::ostringstream md;
  md << "# Pokemon Red Generation Report\n\n- Physical image ignored: yes\n- "
        "Integrity valid: yes\n- Output SHA-256: `"
     << util::Sha256Hex(bytes)
     << "`\n- Location canonicalized: " << (!warnings.empty() ? "yes" : "no")
     << "\n";
  return {std::move(bytes), std::move(report), md.str()};
}
} // namespace pkmn::cli::red::generation
