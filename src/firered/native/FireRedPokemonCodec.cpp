#include "FireRedPokemonCodec.hpp"

#include "FireRedReferenceData.hpp"
#include "FireRedSaveStructure.hpp"
#include "FireRedTextCodec.hpp"
#include "Hex.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace firered {

namespace {
constexpr std::array<const char*, 25> kNatureNames{
    "Hardy", "Lonely", "Brave", "Adamant", "Naughty",
    "Bold", "Docile", "Relaxed", "Impish", "Lax",
    "Timid", "Hasty", "Serious", "Jolly", "Naive",
    "Modest", "Mild", "Quiet", "Bashful", "Rash",
    "Calm", "Gentle", "Sassy", "Careful", "Quirky"
};
constexpr std::array<std::array<std::uint8_t, 4>, 24> kLogicalToPhysical{{
    {{0,1,2,3}}, {{0,1,3,2}}, {{0,2,1,3}}, {{0,3,1,2}},
    {{0,2,3,1}}, {{0,3,2,1}}, {{1,0,2,3}}, {{1,0,3,2}},
    {{2,0,1,3}}, {{3,0,1,2}}, {{2,0,3,1}}, {{3,0,2,1}},
    {{1,2,0,3}}, {{1,3,0,2}}, {{2,1,0,3}}, {{3,1,0,2}},
    {{2,3,0,1}}, {{3,2,0,1}}, {{1,2,3,0}}, {{1,3,2,0}},
    {{2,1,3,0}}, {{3,1,2,0}}, {{2,3,1,0}}, {{3,2,1,0}}
}};

std::array<std::uint8_t, 48> DecryptSecure(std::span<const std::uint8_t> bytes) {
    const auto personality = ReadLe32(bytes, 0);
    const auto otId = ReadLe32(bytes, 4);
    const auto key = personality ^ otId;
    std::array<std::uint8_t, 48> decrypted{};
    for (std::size_t offset = 0; offset < decrypted.size(); offset += 4) {
        const auto value = ReadLe32(bytes, 32 + offset) ^ key;
        WriteLe32(decrypted, offset, value);
    }
    return decrypted;
}

std::span<const std::uint8_t> LogicalSubstruct(
    const std::array<std::uint8_t, 48>& decrypted,
    std::uint32_t personality,
    std::size_t logicalType) {
    const auto physical = kLogicalToPhysical[personality % 24][logicalType];
    return std::span<const std::uint8_t>(decrypted).subspan(physical * 12, 12);
}

std::uint16_t PokemonChecksum(const std::array<std::uint8_t, 48>& decrypted) {
    std::uint16_t result = 0;
    for (std::size_t offset = 0; offset < decrypted.size(); offset += 2) {
        result = static_cast<std::uint16_t>(result + ReadLe16(decrypted, offset));
    }
    return result;
}
} // namespace

PokemonRecord DecodeBoxPokemon(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 80) throw std::invalid_argument("BoxPokemon requires 80 bytes");
    PokemonRecord record;
    record.personality = ReadLe32(bytes, 0);
    record.otId = ReadLe32(bytes, 4);
    record.publicTrainerId = static_cast<std::uint16_t>(record.otId);
    record.secretTrainerId = static_cast<std::uint16_t>(record.otId >> 16);
    const auto personalityLow = static_cast<std::uint16_t>(record.personality);
    const auto personalityHigh = static_cast<std::uint16_t>(record.personality >> 16);
    record.shinyValue = static_cast<std::uint16_t>(
        record.publicTrainerId ^ record.secretTrainerId ^ personalityLow ^ personalityHigh);
    record.isShiny = record.shinyValue < 8;
    record.nickname = DecodeFireRedText(bytes.subspan(8, 10));
    record.language = bytes[18];
    record.headerFlags = bytes[19];
    record.badEgg = (record.headerFlags & 0x01U) != 0;
    record.hasSpecies = (record.headerFlags & 0x02U) != 0;
    record.egg = (record.headerFlags & 0x04U) != 0;
    record.blockBoxRs = (record.headerFlags & 0x08U) != 0;
    record.unusedHeaderFlags = static_cast<std::uint8_t>(record.headerFlags >> 4);
    record.otName = DecodeFireRedText(bytes.subspan(20, 7));
    record.markings = bytes[27];
    record.storedChecksum = ReadLe16(bytes, 28);
    record.headerUnknown = ReadLe16(bytes, 30);
    record.rawBoxHex = EncodeHex(bytes.first(80));
    record.occupied = record.hasSpecies || record.personality != 0 || record.otId != 0;
    if (!record.occupied) return record;

    const auto decrypted = DecryptSecure(bytes);
    record.decryptedSecureHex = EncodeHex(decrypted);
    record.calculatedChecksum = PokemonChecksum(decrypted);
    record.checksumValid = record.storedChecksum == record.calculatedChecksum;
    if (!record.checksumValid) {
        record.warnings.emplace_back(
            "Pokémon checksum mismatch; semantic fields may represent corrupt or Bad Egg data");
    }

    const auto growth = LogicalSubstruct(decrypted, record.personality, 0);
    const auto attacks = LogicalSubstruct(decrypted, record.personality, 1);
    const auto effort = LogicalSubstruct(decrypted, record.personality, 2);
    const auto misc = LogicalSubstruct(decrypted, record.personality, 3);

    record.speciesId = ReadLe16(growth, 0);
    record.nationalDex = SpeciesNationalDex(record.speciesId);
    record.speciesName = SpeciesName(record.speciesId);
    record.heldItemId = ReadLe16(growth, 2);
    record.heldItemName = ItemName(record.heldItemId);
    record.experience = ReadLe32(growth, 4);
    record.ppBonuses = growth[8];
    record.friendship = growth[9];
    record.growthFiller = ReadLe16(growth, 10);
    record.natureId = static_cast<std::uint8_t>(record.personality % 25);
    record.natureName = kNatureNames[record.natureId];

    for (std::size_t index = 0; index < 4; ++index) {
        record.moves[index].id = ReadLe16(attacks, index * 2);
        record.moves[index].name = MoveName(record.moves[index].id);
        record.moves[index].pp = attacks[8 + index];
        record.moves[index].ppUps = static_cast<std::uint8_t>(
            (record.ppBonuses >> (index * 2)) & 0x3U);
    }
    for (std::size_t index = 0; index < 6; ++index) record.evs[index] = effort[index];
    for (std::size_t index = 0; index < 5; ++index) record.contest[index] = effort[6 + index];
    record.sheen = effort[11];
    record.pokerus = misc[0];
    record.pokerusStrain = static_cast<std::uint8_t>(record.pokerus >> 4);
    record.pokerusDays = static_cast<std::uint8_t>(record.pokerus & 0x0FU);
    record.metLocation = misc[1];
    const auto origins = ReadLe16(misc, 2);
    record.originsRaw = origins;
    record.metLevel = static_cast<std::uint8_t>(origins & 0x7FU);
    record.metGame = static_cast<std::uint8_t>((origins >> 7) & 0x0FU);
    record.pokeball = static_cast<std::uint8_t>((origins >> 11) & 0x0FU);
    record.otFemale = ((origins >> 15) & 1U) != 0;
    const auto ivs = ReadLe32(misc, 4);
    record.ivWordRaw = ivs;
    for (std::size_t index = 0; index < 6; ++index) {
        record.ivs[index] = static_cast<std::uint8_t>((ivs >> (index * 5)) & 0x1FU);
    }
    record.egg = record.egg || ((ivs >> 30) & 1U) != 0;
    record.abilitySlot = ((ivs >> 31) & 1U) != 0;
    const auto ribbons = ReadLe32(misc, 8);
    record.ribbonWordRaw = ribbons;
    record.ribbons.cool = static_cast<std::uint8_t>(ribbons & 0x7U);
    record.ribbons.beauty = static_cast<std::uint8_t>((ribbons >> 3) & 0x7U);
    record.ribbons.cute = static_cast<std::uint8_t>((ribbons >> 6) & 0x7U);
    record.ribbons.smart = static_cast<std::uint8_t>((ribbons >> 9) & 0x7U);
    record.ribbons.tough = static_cast<std::uint8_t>((ribbons >> 12) & 0x7U);
    record.ribbons.champion = ((ribbons >> 15) & 1U) != 0;
    record.ribbons.winning = ((ribbons >> 16) & 1U) != 0;
    record.ribbons.victory = ((ribbons >> 17) & 1U) != 0;
    record.ribbons.artist = ((ribbons >> 18) & 1U) != 0;
    record.ribbons.effort = ((ribbons >> 19) & 1U) != 0;
    record.ribbons.marine = ((ribbons >> 20) & 1U) != 0;
    record.ribbons.land = ((ribbons >> 21) & 1U) != 0;
    record.ribbons.sky = ((ribbons >> 22) & 1U) != 0;
    record.ribbons.country = ((ribbons >> 23) & 1U) != 0;
    record.ribbons.national = ((ribbons >> 24) & 1U) != 0;
    record.ribbons.earth = ((ribbons >> 25) & 1U) != 0;
    record.ribbons.world = ((ribbons >> 26) & 1U) != 0;
    record.ribbons.unused = static_cast<std::uint8_t>((ribbons >> 27) & 0xFU);
    record.fatefulEncounter = ((ribbons >> 31) & 1U) != 0;
    return record;
}

PokemonRecord DecodePartyPokemon(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 100) throw std::invalid_argument("party Pokemon requires 100 bytes");
    auto record = DecodeBoxPokemon(bytes.first(80));
    record.partyDataPresent = true;
    record.status = ReadLe32(bytes, 80);
    record.level = bytes[84];
    record.mail = bytes[85];
    record.currentHp = ReadLe16(bytes, 86);
    record.maxHp = ReadLe16(bytes, 88);
    for (std::size_t index = 0; index < 5; ++index) {
        record.stats[index] = ReadLe16(bytes, 90 + index * 2);
    }
    record.rawPartyExtensionHex = EncodeHex(bytes.subspan(80, 20));
    return record;
}

} // namespace firered
