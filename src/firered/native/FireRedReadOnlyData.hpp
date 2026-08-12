#pragma once

#include "FireRedLogicalSave.hpp"
#include "FireRedPokemonCodec.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace firered {

struct Warp {
    std::int8_t mapGroup{};
    std::int8_t mapNumber{};
    std::int8_t warpId{};
    std::int16_t x{};
    std::int16_t y{};
    std::string mapName;
};

struct ItemStack {
    std::string pocket;
    std::size_t slot{};
    std::uint16_t id{};
    std::string name;
    std::uint16_t quantity{};
    bool encryptedQuantity{};
};

struct NamedFlag {
    std::uint16_t id{};
    std::string name;
};

struct NamedVariable {
    std::uint16_t id{};
    std::string name;
    std::uint16_t value{};
};

struct PokemonBox {
    std::size_t index{};
    std::string name;
    std::uint8_t wallpaper{};
    struct Entry {
        std::size_t slot{};
        PokemonRecord pokemon;
    };
    std::vector<Entry> pokemon;
};

struct GameOptions {
    std::uint8_t buttonMode{};
    std::uint8_t textSpeed{};
    std::uint8_t windowFrameType{};
    bool stereo{};
    bool setBattleStyle{};
    bool battleSceneOff{};
    bool regionMapZoom{};
    std::uint16_t raw{};
};

struct MailRecord {
    std::size_t slot{};
    std::array<std::uint16_t, 9> words{};
    std::string playerName;
    std::uint32_t trainerId{};
    std::uint16_t species{};
    std::uint16_t itemId{};
    std::string rawHex;
};

struct DaycareRecord {
    std::string facility;
    std::size_t slot{};
    PokemonRecord pokemon;
    std::uint32_t steps{};
    std::string mailOtName;
    std::string mailPokemonName;
    std::uint8_t gameLanguage{};
    std::uint8_t pokemonLanguage{};
    std::string rawMailHex;
};

struct TrainerTowerRecord {
    std::size_t challengeType{};
    std::uint32_t timer{};
    std::uint32_t bestTimeStored{};
    std::uint32_t bestTime{};
    std::uint8_t floorsCleared{};
    std::uint8_t dataSetId{};
    bool receivedPrize{};
    bool checkedFinalTime{};
    bool spokeToOwner{};
    bool hasLost{};
    bool unknownFlag{};
    bool validated{};
    std::uint8_t rawFlags{};
};

struct FireRedDecodedSave {
    std::string playerName;
    std::string rivalName;
    std::uint8_t playerGender{};
    std::uint8_t specialSaveWarpFlags{};
    std::uint16_t publicTrainerId{};
    std::uint16_t secretTrainerId{};
    std::uint16_t playTimeHours{};
    std::uint8_t playTimeMinutes{};
    std::uint8_t playTimeSeconds{};
    std::uint8_t playTimeVBlanks{};
    GameOptions options;
    std::uint32_t encryptionKey{};
    std::uint32_t money{};
    std::uint16_t coins{};
    std::uint16_t registeredItem{};
    std::int16_t playerX{};
    std::int16_t playerY{};
    Warp location;
    Warp continueWarp;
    Warp dynamicWarp;
    Warp lastHealLocation;
    Warp escapeWarp;
    std::uint16_t savedMusic{};
    std::uint8_t weather{};
    std::uint8_t weatherCycleStage{};
    std::uint8_t flashLevel{};
    std::uint16_t mapLayoutId{};
    std::uint8_t partyCount{};
    std::vector<PokemonRecord> party;
    std::uint8_t currentBox{};
    std::vector<PokemonBox> boxes;
    std::vector<ItemStack> items;
    std::array<bool, 8> badges{};
    std::vector<std::uint16_t> pokedexOwned;
    std::vector<std::uint16_t> pokedexSeen;
    bool pokedexSeenMirrorsConsistent{};
    std::vector<NamedFlag> setFlags;
    std::vector<NamedVariable> nonzeroVariables;
    std::array<std::uint32_t, 64> gameStats{};
    std::vector<MailRecord> mail;
    std::vector<DaycareRecord> daycare;
    std::uint16_t daycareOffspringPersonality{};
    std::uint8_t daycareStepCounter{};
    std::uint32_t towerChallengeId{};
    std::vector<TrainerTowerRecord> trainerTower;
    std::vector<std::string> warnings;
};

FireRedDecodedSave DecodeFireRedSave(const LogicalSave& logical);
std::string DumpFireRedCompactSummary(const FireRedDecodedSave& decoded);
std::string DumpFireRedDetailedSummary(const FireRedDecodedSave& decoded);
std::string DumpFireRedSummary(const FireRedDecodedSave& decoded);

} // namespace firered
