#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace firered {

struct PokemonMove {
    std::uint16_t id{};
    std::string name;
    std::uint8_t pp{};
    std::uint8_t ppUps{};
};

struct PokemonRibbons {
    std::uint8_t cool{};
    std::uint8_t beauty{};
    std::uint8_t cute{};
    std::uint8_t smart{};
    std::uint8_t tough{};
    bool champion{};
    bool winning{};
    bool victory{};
    bool artist{};
    bool effort{};
    bool marine{};
    bool land{};
    bool sky{};
    bool country{};
    bool national{};
    bool earth{};
    bool world{};
    std::uint8_t unused{};
};

struct PokemonRecord {
    bool occupied{};
    bool partyDataPresent{};
    bool checksumValid{};
    bool badEgg{};
    bool egg{};
    bool hasSpecies{};
    std::uint32_t personality{};
    std::uint32_t otId{};
    std::uint16_t publicTrainerId{};
    std::uint16_t secretTrainerId{};
    std::uint16_t shinyValue{};
    bool isShiny{};
    std::string nickname;
    std::string otName;
    std::uint8_t language{};
    std::uint8_t markings{};
    std::uint8_t headerFlags{};
    bool blockBoxRs{};
    std::uint8_t unusedHeaderFlags{};
    std::uint16_t storedChecksum{};
    std::uint16_t calculatedChecksum{};
    std::uint16_t headerUnknown{};
    std::uint16_t speciesId{};
    std::uint16_t nationalDex{};
    std::string speciesName;
    std::uint16_t heldItemId{};
    std::string heldItemName;
    std::uint32_t experience{};
    std::uint16_t growthFiller{};
    std::uint8_t natureId{};
    std::string natureName;
    std::uint8_t ppBonuses{};
    std::uint8_t friendship{};
    std::array<PokemonMove, 4> moves;
    std::array<std::uint8_t, 6> evs{};
    std::array<std::uint8_t, 5> contest{};
    std::uint8_t sheen{};
    std::uint8_t pokerus{};
    std::uint8_t pokerusStrain{};
    std::uint8_t pokerusDays{};
    std::uint8_t metLocation{};
    std::uint8_t metLevel{};
    std::uint8_t metGame{};
    std::uint8_t pokeball{};
    bool otFemale{};
    std::uint16_t originsRaw{};
    std::array<std::uint8_t, 6> ivs{};
    bool abilitySlot{};
    std::uint32_t ivWordRaw{};
    std::uint32_t ribbonWordRaw{};
    PokemonRibbons ribbons;
    bool fatefulEncounter{};
    std::uint8_t level{};
    std::uint32_t status{};
    std::uint8_t mail{};
    std::uint16_t currentHp{};
    std::uint16_t maxHp{};
    std::array<std::uint16_t, 5> stats{};
    std::string rawBoxHex;
    std::string decryptedSecureHex;
    std::string rawPartyExtensionHex;
    std::vector<std::string> warnings;
};

PokemonRecord DecodeBoxPokemon(std::span<const std::uint8_t> bytes);
PokemonRecord DecodePartyPokemon(std::span<const std::uint8_t> bytes);

} // namespace firered
