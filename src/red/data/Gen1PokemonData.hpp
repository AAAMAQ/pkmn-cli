#pragma once

// Adapted from the MIT-licensed Pkmn Red Save Generator verified Gen I data
// tables. See THIRD_PARTY_NOTICES.md for source-project provenance.

#include <array>
#include <cstdint>

namespace pkmn::cli::red::data {

enum class GrowthRate : std::uint8_t {
    MediumFast = 0,
    SlightlyFast = 1,
    SlightlySlow = 2,
    MediumSlow = 3,
    Fast = 4,
    Slow = 5,
};

struct SpeciesData {
    bool valid;
    std::uint8_t internalId;
    std::uint8_t nationalDexNumber;
    std::uint8_t baseHp;
    std::uint8_t baseAttack;
    std::uint8_t baseDefense;
    std::uint8_t baseSpeed;
    std::uint8_t baseSpecial;
    std::uint8_t type1;
    std::uint8_t type2;
    std::uint8_t catchRate;
    GrowthRate growthRate;
};

struct MoveData {
    bool valid;
    std::uint8_t id;
    std::uint8_t basePp;
};

inline constexpr std::array<SpeciesData, 256> kSpeciesData = {
    SpeciesData{false, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{true, 0x01, 112, 105, 130, 120, 40, 45, 4, 5, 60, GrowthRate::Slow}, // RHYDON
    SpeciesData{true, 0x02, 115, 105, 95, 80, 90, 40, 0, 0, 45, GrowthRate::MediumFast}, // KANGASKHAN
    SpeciesData{true, 0x03, 32, 46, 57, 40, 50, 40, 3, 3, 235, GrowthRate::MediumSlow}, // NIDORAN_M
    SpeciesData{true, 0x04, 35, 70, 45, 48, 35, 60, 0, 0, 150, GrowthRate::Fast}, // CLEFAIRY
    SpeciesData{true, 0x05, 21, 40, 60, 30, 70, 31, 0, 2, 255, GrowthRate::MediumFast}, // SPEAROW
    SpeciesData{true, 0x06, 100, 40, 30, 50, 100, 55, 23, 23, 190, GrowthRate::MediumFast}, // VOLTORB
    SpeciesData{true, 0x07, 34, 81, 92, 77, 85, 75, 3, 4, 45, GrowthRate::MediumSlow}, // NIDOKING
    SpeciesData{true, 0x08, 80, 95, 75, 110, 30, 80, 21, 24, 75, GrowthRate::MediumFast}, // SLOWBRO
    SpeciesData{true, 0x09, 2, 60, 62, 63, 60, 80, 22, 3, 45, GrowthRate::MediumSlow}, // IVYSAUR
    SpeciesData{true, 0x0A, 103, 95, 95, 85, 55, 125, 22, 24, 45, GrowthRate::Slow}, // EXEGGUTOR
    SpeciesData{true, 0x0B, 108, 90, 55, 75, 30, 60, 0, 0, 45, GrowthRate::MediumFast}, // LICKITUNG
    SpeciesData{true, 0x0C, 102, 60, 40, 80, 40, 60, 22, 24, 90, GrowthRate::Slow}, // EXEGGCUTE
    SpeciesData{true, 0x0D, 88, 80, 80, 50, 25, 40, 3, 3, 190, GrowthRate::MediumFast}, // GRIMER
    SpeciesData{true, 0x0E, 94, 60, 65, 60, 110, 130, 8, 3, 45, GrowthRate::MediumSlow}, // GENGAR
    SpeciesData{true, 0x0F, 29, 55, 47, 52, 41, 40, 3, 3, 235, GrowthRate::MediumSlow}, // NIDORAN_F
    SpeciesData{true, 0x10, 31, 90, 82, 87, 76, 75, 3, 4, 45, GrowthRate::MediumSlow}, // NIDOQUEEN
    SpeciesData{true, 0x11, 104, 50, 50, 95, 35, 40, 4, 4, 190, GrowthRate::MediumFast}, // CUBONE
    SpeciesData{true, 0x12, 111, 80, 85, 95, 25, 30, 4, 5, 120, GrowthRate::Slow}, // RHYHORN
    SpeciesData{true, 0x13, 131, 130, 85, 80, 60, 95, 21, 25, 45, GrowthRate::Slow}, // LAPRAS
    SpeciesData{true, 0x14, 59, 90, 110, 80, 95, 80, 20, 20, 75, GrowthRate::Slow}, // ARCANINE
    SpeciesData{true, 0x15, 151, 100, 100, 100, 100, 100, 24, 24, 45, GrowthRate::MediumSlow}, // MEW
    SpeciesData{true, 0x16, 130, 95, 125, 79, 81, 100, 21, 2, 45, GrowthRate::Slow}, // GYARADOS
    SpeciesData{true, 0x17, 90, 30, 65, 100, 40, 45, 21, 21, 190, GrowthRate::Slow}, // SHELLDER
    SpeciesData{true, 0x18, 72, 40, 40, 35, 70, 100, 21, 3, 190, GrowthRate::Slow}, // TENTACOOL
    SpeciesData{true, 0x19, 92, 30, 35, 30, 80, 100, 8, 3, 190, GrowthRate::MediumSlow}, // GASTLY
    SpeciesData{true, 0x1A, 123, 70, 110, 80, 105, 55, 7, 2, 45, GrowthRate::MediumFast}, // SCYTHER
    SpeciesData{true, 0x1B, 120, 30, 45, 55, 85, 70, 21, 21, 225, GrowthRate::Slow}, // STARYU
    SpeciesData{true, 0x1C, 9, 79, 83, 100, 78, 85, 21, 21, 45, GrowthRate::MediumSlow}, // BLASTOISE
    SpeciesData{true, 0x1D, 127, 65, 125, 100, 85, 55, 7, 7, 45, GrowthRate::Slow}, // PINSIR
    SpeciesData{true, 0x1E, 114, 65, 55, 115, 60, 100, 22, 22, 45, GrowthRate::MediumFast}, // TANGELA
    SpeciesData{false, 0x1F, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x21, 58, 55, 70, 45, 60, 50, 20, 20, 190, GrowthRate::Slow}, // GROWLITHE
    SpeciesData{true, 0x22, 95, 35, 45, 160, 70, 30, 5, 4, 45, GrowthRate::MediumFast}, // ONIX
    SpeciesData{true, 0x23, 22, 65, 90, 65, 100, 61, 0, 2, 90, GrowthRate::MediumFast}, // FEAROW
    SpeciesData{true, 0x24, 16, 40, 45, 40, 56, 35, 0, 2, 255, GrowthRate::MediumSlow}, // PIDGEY
    SpeciesData{true, 0x25, 79, 90, 65, 65, 15, 40, 21, 24, 190, GrowthRate::MediumFast}, // SLOWPOKE
    SpeciesData{true, 0x26, 64, 40, 35, 30, 105, 120, 24, 24, 100, GrowthRate::MediumSlow}, // KADABRA
    SpeciesData{true, 0x27, 75, 55, 95, 115, 35, 45, 5, 4, 120, GrowthRate::MediumSlow}, // GRAVELER
    SpeciesData{true, 0x28, 113, 250, 5, 5, 50, 105, 0, 0, 30, GrowthRate::Fast}, // CHANSEY
    SpeciesData{true, 0x29, 67, 80, 100, 70, 45, 50, 1, 1, 90, GrowthRate::MediumSlow}, // MACHOKE
    SpeciesData{true, 0x2A, 122, 40, 45, 65, 90, 100, 24, 24, 45, GrowthRate::MediumFast}, // MR_MIME
    SpeciesData{true, 0x2B, 106, 50, 120, 53, 87, 35, 1, 1, 45, GrowthRate::MediumFast}, // HITMONLEE
    SpeciesData{true, 0x2C, 107, 50, 105, 79, 76, 35, 1, 1, 45, GrowthRate::MediumFast}, // HITMONCHAN
    SpeciesData{true, 0x2D, 24, 60, 85, 69, 80, 65, 3, 3, 90, GrowthRate::MediumFast}, // ARBOK
    SpeciesData{true, 0x2E, 47, 60, 95, 80, 30, 80, 7, 22, 75, GrowthRate::MediumFast}, // PARASECT
    SpeciesData{true, 0x2F, 54, 50, 52, 48, 55, 50, 21, 21, 190, GrowthRate::MediumFast}, // PSYDUCK
    SpeciesData{true, 0x30, 96, 60, 48, 45, 42, 90, 24, 24, 190, GrowthRate::MediumFast}, // DROWZEE
    SpeciesData{true, 0x31, 76, 80, 110, 130, 45, 55, 5, 4, 45, GrowthRate::MediumSlow}, // GOLEM
    SpeciesData{false, 0x32, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x33, 126, 65, 95, 57, 93, 85, 20, 20, 45, GrowthRate::MediumFast}, // MAGMAR
    SpeciesData{false, 0x34, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x35, 125, 65, 83, 57, 105, 85, 23, 23, 45, GrowthRate::MediumFast}, // ELECTABUZZ
    SpeciesData{true, 0x36, 82, 50, 60, 95, 70, 120, 23, 23, 60, GrowthRate::MediumFast}, // MAGNETON
    SpeciesData{true, 0x37, 109, 40, 65, 95, 35, 60, 3, 3, 190, GrowthRate::MediumFast}, // KOFFING
    SpeciesData{false, 0x38, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x39, 56, 40, 80, 35, 70, 35, 1, 1, 190, GrowthRate::MediumFast}, // MANKEY
    SpeciesData{true, 0x3A, 86, 65, 45, 55, 45, 70, 21, 21, 190, GrowthRate::MediumFast}, // SEEL
    SpeciesData{true, 0x3B, 50, 10, 55, 25, 95, 45, 4, 4, 255, GrowthRate::MediumFast}, // DIGLETT
    SpeciesData{true, 0x3C, 128, 75, 100, 95, 110, 70, 0, 0, 45, GrowthRate::Slow}, // TAUROS
    SpeciesData{false, 0x3D, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0x3E, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0x3F, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x40, 83, 52, 65, 55, 60, 58, 0, 2, 45, GrowthRate::MediumFast}, // FARFETCHD
    SpeciesData{true, 0x41, 48, 60, 55, 50, 45, 40, 7, 3, 190, GrowthRate::MediumFast}, // VENONAT
    SpeciesData{true, 0x42, 149, 91, 134, 95, 80, 100, 26, 2, 45, GrowthRate::Slow}, // DRAGONITE
    SpeciesData{false, 0x43, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0x44, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0x45, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x46, 84, 35, 85, 45, 75, 35, 0, 2, 190, GrowthRate::MediumFast}, // DODUO
    SpeciesData{true, 0x47, 60, 40, 50, 40, 90, 40, 21, 21, 255, GrowthRate::MediumSlow}, // POLIWAG
    SpeciesData{true, 0x48, 124, 65, 50, 35, 95, 95, 25, 24, 45, GrowthRate::MediumFast}, // JYNX
    SpeciesData{true, 0x49, 146, 90, 100, 90, 90, 125, 20, 2, 3, GrowthRate::Slow}, // MOLTRES
    SpeciesData{true, 0x4A, 144, 90, 85, 100, 85, 125, 25, 2, 3, GrowthRate::Slow}, // ARTICUNO
    SpeciesData{true, 0x4B, 145, 90, 90, 85, 100, 125, 23, 2, 3, GrowthRate::Slow}, // ZAPDOS
    SpeciesData{true, 0x4C, 132, 48, 48, 48, 48, 48, 0, 0, 35, GrowthRate::MediumFast}, // DITTO
    SpeciesData{true, 0x4D, 52, 40, 45, 35, 90, 40, 0, 0, 255, GrowthRate::MediumFast}, // MEOWTH
    SpeciesData{true, 0x4E, 98, 30, 105, 90, 50, 25, 21, 21, 225, GrowthRate::MediumFast}, // KRABBY
    SpeciesData{false, 0x4F, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0x51, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x52, 37, 38, 41, 40, 65, 65, 20, 20, 190, GrowthRate::MediumFast}, // VULPIX
    SpeciesData{true, 0x53, 38, 73, 76, 75, 100, 100, 20, 20, 75, GrowthRate::MediumFast}, // NINETALES
    SpeciesData{true, 0x54, 25, 35, 55, 30, 90, 50, 23, 23, 190, GrowthRate::MediumFast}, // PIKACHU
    SpeciesData{true, 0x55, 26, 60, 90, 55, 100, 90, 23, 23, 75, GrowthRate::MediumFast}, // RAICHU
    SpeciesData{false, 0x56, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0x57, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x58, 147, 41, 64, 45, 50, 50, 26, 26, 45, GrowthRate::Slow}, // DRATINI
    SpeciesData{true, 0x59, 148, 61, 84, 65, 70, 70, 26, 26, 45, GrowthRate::Slow}, // DRAGONAIR
    SpeciesData{true, 0x5A, 140, 30, 80, 90, 55, 45, 5, 21, 45, GrowthRate::MediumFast}, // KABUTO
    SpeciesData{true, 0x5B, 141, 60, 115, 105, 80, 70, 5, 21, 45, GrowthRate::MediumFast}, // KABUTOPS
    SpeciesData{true, 0x5C, 116, 30, 40, 70, 60, 70, 21, 21, 225, GrowthRate::MediumFast}, // HORSEA
    SpeciesData{true, 0x5D, 117, 55, 65, 95, 85, 95, 21, 21, 75, GrowthRate::MediumFast}, // SEADRA
    SpeciesData{false, 0x5E, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0x5F, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x60, 27, 50, 75, 85, 40, 30, 4, 4, 255, GrowthRate::MediumFast}, // SANDSHREW
    SpeciesData{true, 0x61, 28, 75, 100, 110, 65, 55, 4, 4, 90, GrowthRate::MediumFast}, // SANDSLASH
    SpeciesData{true, 0x62, 138, 35, 40, 100, 35, 90, 5, 21, 45, GrowthRate::MediumFast}, // OMANYTE
    SpeciesData{true, 0x63, 139, 70, 60, 125, 55, 115, 5, 21, 45, GrowthRate::MediumFast}, // OMASTAR
    SpeciesData{true, 0x64, 39, 115, 45, 20, 20, 25, 0, 0, 170, GrowthRate::Fast}, // JIGGLYPUFF
    SpeciesData{true, 0x65, 40, 140, 70, 45, 45, 85, 0, 0, 50, GrowthRate::Fast}, // WIGGLYTUFF
    SpeciesData{true, 0x66, 133, 55, 55, 50, 55, 65, 0, 0, 45, GrowthRate::MediumFast}, // EEVEE
    SpeciesData{true, 0x67, 136, 65, 130, 60, 65, 110, 20, 20, 45, GrowthRate::MediumFast}, // FLAREON
    SpeciesData{true, 0x68, 135, 65, 65, 60, 130, 110, 23, 23, 45, GrowthRate::MediumFast}, // JOLTEON
    SpeciesData{true, 0x69, 134, 130, 65, 60, 65, 110, 21, 21, 45, GrowthRate::MediumFast}, // VAPOREON
    SpeciesData{true, 0x6A, 66, 70, 80, 50, 35, 35, 1, 1, 180, GrowthRate::MediumSlow}, // MACHOP
    SpeciesData{true, 0x6B, 41, 40, 45, 35, 55, 40, 3, 2, 255, GrowthRate::MediumFast}, // ZUBAT
    SpeciesData{true, 0x6C, 23, 35, 60, 44, 55, 40, 3, 3, 255, GrowthRate::MediumFast}, // EKANS
    SpeciesData{true, 0x6D, 46, 35, 70, 55, 25, 55, 7, 22, 190, GrowthRate::MediumFast}, // PARAS
    SpeciesData{true, 0x6E, 61, 65, 65, 65, 90, 50, 21, 21, 120, GrowthRate::MediumSlow}, // POLIWHIRL
    SpeciesData{true, 0x6F, 62, 90, 85, 95, 70, 70, 21, 1, 45, GrowthRate::MediumSlow}, // POLIWRATH
    SpeciesData{true, 0x70, 13, 40, 35, 30, 50, 20, 7, 3, 255, GrowthRate::MediumFast}, // WEEDLE
    SpeciesData{true, 0x71, 14, 45, 25, 50, 35, 25, 7, 3, 120, GrowthRate::MediumFast}, // KAKUNA
    SpeciesData{true, 0x72, 15, 65, 80, 40, 75, 45, 7, 3, 45, GrowthRate::MediumFast}, // BEEDRILL
    SpeciesData{false, 0x73, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x74, 85, 60, 110, 70, 100, 60, 0, 2, 45, GrowthRate::MediumFast}, // DODRIO
    SpeciesData{true, 0x75, 57, 65, 105, 60, 95, 60, 1, 1, 75, GrowthRate::MediumFast}, // PRIMEAPE
    SpeciesData{true, 0x76, 51, 35, 80, 50, 120, 50, 4, 4, 50, GrowthRate::MediumFast}, // DUGTRIO
    SpeciesData{true, 0x77, 49, 70, 65, 60, 90, 90, 7, 3, 75, GrowthRate::MediumFast}, // VENOMOTH
    SpeciesData{true, 0x78, 87, 90, 70, 80, 70, 95, 21, 25, 75, GrowthRate::MediumFast}, // DEWGONG
    SpeciesData{false, 0x79, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0x7A, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x7B, 10, 45, 30, 35, 45, 20, 7, 7, 255, GrowthRate::MediumFast}, // CATERPIE
    SpeciesData{true, 0x7C, 11, 50, 20, 55, 30, 25, 7, 7, 120, GrowthRate::MediumFast}, // METAPOD
    SpeciesData{true, 0x7D, 12, 60, 45, 50, 70, 80, 7, 2, 45, GrowthRate::MediumFast}, // BUTTERFREE
    SpeciesData{true, 0x7E, 68, 90, 130, 80, 55, 65, 1, 1, 45, GrowthRate::MediumSlow}, // MACHAMP
    SpeciesData{false, 0x7F, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x80, 55, 80, 82, 78, 85, 80, 21, 21, 75, GrowthRate::MediumFast}, // GOLDUCK
    SpeciesData{true, 0x81, 97, 85, 73, 70, 67, 115, 24, 24, 75, GrowthRate::MediumFast}, // HYPNO
    SpeciesData{true, 0x82, 42, 75, 80, 70, 90, 75, 3, 2, 90, GrowthRate::MediumFast}, // GOLBAT
    SpeciesData{true, 0x83, 150, 106, 110, 90, 130, 154, 24, 24, 3, GrowthRate::Slow}, // MEWTWO
    SpeciesData{true, 0x84, 143, 160, 110, 65, 30, 65, 0, 0, 25, GrowthRate::Slow}, // SNORLAX
    SpeciesData{true, 0x85, 129, 20, 10, 55, 80, 20, 21, 21, 255, GrowthRate::Slow}, // MAGIKARP
    SpeciesData{false, 0x86, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0x87, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x88, 89, 105, 105, 75, 50, 65, 3, 3, 75, GrowthRate::MediumFast}, // MUK
    SpeciesData{false, 0x89, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x8A, 99, 55, 130, 115, 75, 50, 21, 21, 60, GrowthRate::MediumFast}, // KINGLER
    SpeciesData{true, 0x8B, 91, 50, 95, 180, 70, 85, 21, 25, 60, GrowthRate::Slow}, // CLOYSTER
    SpeciesData{false, 0x8C, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x8D, 101, 60, 50, 70, 140, 80, 23, 23, 60, GrowthRate::MediumFast}, // ELECTRODE
    SpeciesData{true, 0x8E, 36, 95, 70, 73, 60, 85, 0, 0, 25, GrowthRate::Fast}, // CLEFABLE
    SpeciesData{true, 0x8F, 110, 65, 90, 120, 60, 85, 3, 3, 60, GrowthRate::MediumFast}, // WEEZING
    SpeciesData{true, 0x90, 53, 65, 70, 60, 115, 65, 0, 0, 90, GrowthRate::MediumFast}, // PERSIAN
    SpeciesData{true, 0x91, 105, 60, 80, 110, 45, 50, 4, 4, 75, GrowthRate::MediumFast}, // MAROWAK
    SpeciesData{false, 0x92, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x93, 93, 45, 50, 45, 95, 115, 8, 3, 90, GrowthRate::MediumSlow}, // HAUNTER
    SpeciesData{true, 0x94, 63, 25, 20, 15, 90, 105, 24, 24, 200, GrowthRate::MediumSlow}, // ABRA
    SpeciesData{true, 0x95, 65, 55, 50, 45, 120, 135, 24, 24, 50, GrowthRate::MediumSlow}, // ALAKAZAM
    SpeciesData{true, 0x96, 17, 63, 60, 55, 71, 50, 0, 2, 120, GrowthRate::MediumSlow}, // PIDGEOTTO
    SpeciesData{true, 0x97, 18, 83, 80, 75, 91, 70, 0, 2, 45, GrowthRate::MediumSlow}, // PIDGEOT
    SpeciesData{true, 0x98, 121, 60, 75, 85, 115, 100, 21, 24, 60, GrowthRate::Slow}, // STARMIE
    SpeciesData{true, 0x99, 1, 45, 49, 49, 45, 65, 22, 3, 45, GrowthRate::MediumSlow}, // BULBASAUR
    SpeciesData{true, 0x9A, 3, 80, 82, 83, 80, 100, 22, 3, 45, GrowthRate::MediumSlow}, // VENUSAUR
    SpeciesData{true, 0x9B, 73, 80, 70, 65, 100, 120, 21, 3, 60, GrowthRate::Slow}, // TENTACRUEL
    SpeciesData{false, 0x9C, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0x9D, 118, 45, 67, 60, 63, 50, 21, 21, 225, GrowthRate::MediumFast}, // GOLDEEN
    SpeciesData{true, 0x9E, 119, 80, 92, 65, 68, 80, 21, 21, 60, GrowthRate::MediumFast}, // SEAKING
    SpeciesData{false, 0x9F, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0xA0, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0xA1, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0xA2, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0xA3, 77, 50, 85, 55, 90, 65, 20, 20, 190, GrowthRate::MediumFast}, // PONYTA
    SpeciesData{true, 0xA4, 78, 65, 100, 70, 105, 80, 20, 20, 60, GrowthRate::MediumFast}, // RAPIDASH
    SpeciesData{true, 0xA5, 19, 30, 56, 35, 72, 25, 0, 0, 255, GrowthRate::MediumFast}, // RATTATA
    SpeciesData{true, 0xA6, 20, 55, 81, 60, 97, 50, 0, 0, 127, GrowthRate::MediumFast}, // RATICATE
    SpeciesData{true, 0xA7, 33, 61, 72, 57, 56, 55, 3, 3, 120, GrowthRate::MediumSlow}, // NIDORINO
    SpeciesData{true, 0xA8, 30, 70, 62, 67, 56, 55, 3, 3, 120, GrowthRate::MediumSlow}, // NIDORINA
    SpeciesData{true, 0xA9, 74, 40, 80, 100, 20, 30, 5, 4, 255, GrowthRate::MediumSlow}, // GEODUDE
    SpeciesData{true, 0xAA, 137, 65, 60, 70, 40, 75, 0, 0, 45, GrowthRate::MediumFast}, // PORYGON
    SpeciesData{true, 0xAB, 142, 80, 105, 65, 130, 60, 5, 2, 45, GrowthRate::Slow}, // AERODACTYL
    SpeciesData{false, 0xAC, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0xAD, 81, 25, 35, 70, 45, 95, 23, 23, 190, GrowthRate::MediumFast}, // MAGNEMITE
    SpeciesData{false, 0xAE, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0xAF, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0xB0, 4, 39, 52, 43, 65, 50, 20, 20, 45, GrowthRate::MediumSlow}, // CHARMANDER
    SpeciesData{true, 0xB1, 7, 44, 48, 65, 43, 50, 21, 21, 45, GrowthRate::MediumSlow}, // SQUIRTLE
    SpeciesData{true, 0xB2, 5, 58, 64, 58, 80, 65, 20, 20, 45, GrowthRate::MediumSlow}, // CHARMELEON
    SpeciesData{true, 0xB3, 8, 59, 63, 80, 58, 65, 21, 21, 45, GrowthRate::MediumSlow}, // WARTORTLE
    SpeciesData{true, 0xB4, 6, 78, 84, 78, 100, 85, 20, 2, 45, GrowthRate::MediumSlow}, // CHARIZARD
    SpeciesData{false, 0xB5, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0xB6, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0xB7, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{false, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // MISSINGNO
    SpeciesData{true, 0xB9, 43, 45, 50, 55, 30, 75, 22, 3, 255, GrowthRate::MediumSlow}, // ODDISH
    SpeciesData{true, 0xBA, 44, 60, 65, 70, 40, 85, 22, 3, 120, GrowthRate::MediumSlow}, // GLOOM
    SpeciesData{true, 0xBB, 45, 75, 80, 85, 50, 100, 22, 3, 45, GrowthRate::MediumSlow}, // VILEPLUME
    SpeciesData{true, 0xBC, 69, 50, 75, 35, 40, 70, 22, 3, 255, GrowthRate::MediumSlow}, // BELLSPROUT
    SpeciesData{true, 0xBD, 70, 65, 90, 50, 55, 85, 22, 3, 120, GrowthRate::MediumSlow}, // WEEPINBELL
    SpeciesData{true, 0xBE, 71, 80, 105, 65, 70, 100, 22, 3, 45, GrowthRate::MediumSlow}, // VICTREEBEL
    SpeciesData{false, 0xBF, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xC0, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xC1, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xC2, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xC3, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xC4, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xC5, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xC6, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xC7, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xC8, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xC9, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xCA, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xCB, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xCC, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xCD, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xCE, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xCF, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xD0, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xD1, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xD2, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xD3, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xD4, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xD5, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xD6, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xD7, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xD8, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xD9, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xDA, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xDB, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xDC, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xDD, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xDE, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xDF, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xE0, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xE2, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xE3, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xE4, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xE5, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xE6, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xE7, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xE8, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xE9, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xEA, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xEB, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xEC, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xED, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xEE, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xEF, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xF0, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xF1, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xF2, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xF3, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xF4, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xF5, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xF6, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xF7, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xF8, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xF9, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xFA, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xFB, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xFC, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xFD, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xFE, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
    SpeciesData{false, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, GrowthRate::MediumFast}, // INVALID
};

inline constexpr std::array<MoveData, 256> kMoveData = {
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{true, 0x01, 35}, // POUND
    MoveData{true, 0x02, 25}, // KARATE_CHOP
    MoveData{true, 0x03, 10}, // DOUBLESLAP
    MoveData{true, 0x04, 15}, // COMET_PUNCH
    MoveData{true, 0x05, 20}, // MEGA_PUNCH
    MoveData{true, 0x06, 20}, // PAY_DAY
    MoveData{true, 0x07, 15}, // FIRE_PUNCH
    MoveData{true, 0x08, 15}, // ICE_PUNCH
    MoveData{true, 0x09, 15}, // THUNDERPUNCH
    MoveData{true, 0x0A, 35}, // SCRATCH
    MoveData{true, 0x0B, 30}, // VICEGRIP
    MoveData{true, 0x0C, 5}, // GUILLOTINE
    MoveData{true, 0x0D, 10}, // RAZOR_WIND
    MoveData{true, 0x0E, 30}, // SWORDS_DANCE
    MoveData{true, 0x0F, 30}, // CUT
    MoveData{true, 0x10, 35}, // GUST
    MoveData{true, 0x11, 35}, // WING_ATTACK
    MoveData{true, 0x12, 20}, // WHIRLWIND
    MoveData{true, 0x13, 15}, // FLY
    MoveData{true, 0x14, 20}, // BIND
    MoveData{true, 0x15, 20}, // SLAM
    MoveData{true, 0x16, 10}, // VINE_WHIP
    MoveData{true, 0x17, 20}, // STOMP
    MoveData{true, 0x18, 30}, // DOUBLE_KICK
    MoveData{true, 0x19, 5}, // MEGA_KICK
    MoveData{true, 0x1A, 25}, // JUMP_KICK
    MoveData{true, 0x1B, 15}, // ROLLING_KICK
    MoveData{true, 0x1C, 15}, // SAND_ATTACK
    MoveData{true, 0x1D, 15}, // HEADBUTT
    MoveData{true, 0x1E, 25}, // HORN_ATTACK
    MoveData{true, 0x1F, 20}, // FURY_ATTACK
    MoveData{true, 0x20, 5}, // HORN_DRILL
    MoveData{true, 0x21, 35}, // TACKLE
    MoveData{true, 0x22, 15}, // BODY_SLAM
    MoveData{true, 0x23, 20}, // WRAP
    MoveData{true, 0x24, 20}, // TAKE_DOWN
    MoveData{true, 0x25, 20}, // THRASH
    MoveData{true, 0x26, 15}, // DOUBLE_EDGE
    MoveData{true, 0x27, 30}, // TAIL_WHIP
    MoveData{true, 0x28, 35}, // POISON_STING
    MoveData{true, 0x29, 20}, // TWINEEDLE
    MoveData{true, 0x2A, 20}, // PIN_MISSILE
    MoveData{true, 0x2B, 30}, // LEER
    MoveData{true, 0x2C, 25}, // BITE
    MoveData{true, 0x2D, 40}, // GROWL
    MoveData{true, 0x2E, 20}, // ROAR
    MoveData{true, 0x2F, 15}, // SING
    MoveData{true, 0x30, 20}, // SUPERSONIC
    MoveData{true, 0x31, 20}, // SONICBOOM
    MoveData{true, 0x32, 20}, // DISABLE
    MoveData{true, 0x33, 30}, // ACID
    MoveData{true, 0x34, 25}, // EMBER
    MoveData{true, 0x35, 15}, // FLAMETHROWER
    MoveData{true, 0x36, 30}, // MIST
    MoveData{true, 0x37, 25}, // WATER_GUN
    MoveData{true, 0x38, 5}, // HYDRO_PUMP
    MoveData{true, 0x39, 15}, // SURF
    MoveData{true, 0x3A, 10}, // ICE_BEAM
    MoveData{true, 0x3B, 5}, // BLIZZARD
    MoveData{true, 0x3C, 20}, // PSYBEAM
    MoveData{true, 0x3D, 20}, // BUBBLEBEAM
    MoveData{true, 0x3E, 20}, // AURORA_BEAM
    MoveData{true, 0x3F, 5}, // HYPER_BEAM
    MoveData{true, 0x40, 35}, // PECK
    MoveData{true, 0x41, 20}, // DRILL_PECK
    MoveData{true, 0x42, 25}, // SUBMISSION
    MoveData{true, 0x43, 20}, // LOW_KICK
    MoveData{true, 0x44, 20}, // COUNTER
    MoveData{true, 0x45, 20}, // SEISMIC_TOSS
    MoveData{true, 0x46, 15}, // STRENGTH
    MoveData{true, 0x47, 20}, // ABSORB
    MoveData{true, 0x48, 10}, // MEGA_DRAIN
    MoveData{true, 0x49, 10}, // LEECH_SEED
    MoveData{true, 0x4A, 40}, // GROWTH
    MoveData{true, 0x4B, 25}, // RAZOR_LEAF
    MoveData{true, 0x4C, 10}, // SOLARBEAM
    MoveData{true, 0x4D, 35}, // POISONPOWDER
    MoveData{true, 0x4E, 30}, // STUN_SPORE
    MoveData{true, 0x4F, 15}, // SLEEP_POWDER
    MoveData{true, 0x50, 20}, // PETAL_DANCE
    MoveData{true, 0x51, 40}, // STRING_SHOT
    MoveData{true, 0x52, 10}, // DRAGON_RAGE
    MoveData{true, 0x53, 15}, // FIRE_SPIN
    MoveData{true, 0x54, 30}, // THUNDERSHOCK
    MoveData{true, 0x55, 15}, // THUNDERBOLT
    MoveData{true, 0x56, 20}, // THUNDER_WAVE
    MoveData{true, 0x57, 10}, // THUNDER
    MoveData{true, 0x58, 15}, // ROCK_THROW
    MoveData{true, 0x59, 10}, // EARTHQUAKE
    MoveData{true, 0x5A, 5}, // FISSURE
    MoveData{true, 0x5B, 10}, // DIG
    MoveData{true, 0x5C, 10}, // TOXIC
    MoveData{true, 0x5D, 25}, // CONFUSION
    MoveData{true, 0x5E, 10}, // PSYCHIC_M
    MoveData{true, 0x5F, 20}, // HYPNOSIS
    MoveData{true, 0x60, 40}, // MEDITATE
    MoveData{true, 0x61, 30}, // AGILITY
    MoveData{true, 0x62, 30}, // QUICK_ATTACK
    MoveData{true, 0x63, 20}, // RAGE
    MoveData{true, 0x64, 20}, // TELEPORT
    MoveData{true, 0x65, 15}, // NIGHT_SHADE
    MoveData{true, 0x66, 10}, // MIMIC
    MoveData{true, 0x67, 40}, // SCREECH
    MoveData{true, 0x68, 15}, // DOUBLE_TEAM
    MoveData{true, 0x69, 20}, // RECOVER
    MoveData{true, 0x6A, 30}, // HARDEN
    MoveData{true, 0x6B, 20}, // MINIMIZE
    MoveData{true, 0x6C, 20}, // SMOKESCREEN
    MoveData{true, 0x6D, 10}, // CONFUSE_RAY
    MoveData{true, 0x6E, 40}, // WITHDRAW
    MoveData{true, 0x6F, 40}, // DEFENSE_CURL
    MoveData{true, 0x70, 30}, // BARRIER
    MoveData{true, 0x71, 30}, // LIGHT_SCREEN
    MoveData{true, 0x72, 30}, // HAZE
    MoveData{true, 0x73, 20}, // REFLECT
    MoveData{true, 0x74, 30}, // FOCUS_ENERGY
    MoveData{true, 0x75, 10}, // BIDE
    MoveData{true, 0x76, 10}, // METRONOME
    MoveData{true, 0x77, 20}, // MIRROR_MOVE
    MoveData{true, 0x78, 5}, // SELFDESTRUCT
    MoveData{true, 0x79, 10}, // EGG_BOMB
    MoveData{true, 0x7A, 30}, // LICK
    MoveData{true, 0x7B, 20}, // SMOG
    MoveData{true, 0x7C, 20}, // SLUDGE
    MoveData{true, 0x7D, 20}, // BONE_CLUB
    MoveData{true, 0x7E, 5}, // FIRE_BLAST
    MoveData{true, 0x7F, 15}, // WATERFALL
    MoveData{true, 0x80, 10}, // CLAMP
    MoveData{true, 0x81, 20}, // SWIFT
    MoveData{true, 0x82, 15}, // SKULL_BASH
    MoveData{true, 0x83, 15}, // SPIKE_CANNON
    MoveData{true, 0x84, 35}, // CONSTRICT
    MoveData{true, 0x85, 20}, // AMNESIA
    MoveData{true, 0x86, 15}, // KINESIS
    MoveData{true, 0x87, 10}, // SOFTBOILED
    MoveData{true, 0x88, 20}, // HI_JUMP_KICK
    MoveData{true, 0x89, 30}, // GLARE
    MoveData{true, 0x8A, 15}, // DREAM_EATER
    MoveData{true, 0x8B, 40}, // POISON_GAS
    MoveData{true, 0x8C, 20}, // BARRAGE
    MoveData{true, 0x8D, 15}, // LEECH_LIFE
    MoveData{true, 0x8E, 10}, // LOVELY_KISS
    MoveData{true, 0x8F, 5}, // SKY_ATTACK
    MoveData{true, 0x90, 10}, // TRANSFORM
    MoveData{true, 0x91, 30}, // BUBBLE
    MoveData{true, 0x92, 10}, // DIZZY_PUNCH
    MoveData{true, 0x93, 15}, // SPORE
    MoveData{true, 0x94, 20}, // FLASH
    MoveData{true, 0x95, 15}, // PSYWAVE
    MoveData{true, 0x96, 40}, // SPLASH
    MoveData{true, 0x97, 40}, // ACID_ARMOR
    MoveData{true, 0x98, 10}, // CRABHAMMER
    MoveData{true, 0x99, 5}, // EXPLOSION
    MoveData{true, 0x9A, 15}, // FURY_SWIPES
    MoveData{true, 0x9B, 10}, // BONEMERANG
    MoveData{true, 0x9C, 10}, // REST
    MoveData{true, 0x9D, 10}, // ROCK_SLIDE
    MoveData{true, 0x9E, 15}, // HYPER_FANG
    MoveData{true, 0x9F, 30}, // SHARPEN
    MoveData{true, 0xA0, 30}, // CONVERSION
    MoveData{true, 0xA1, 10}, // TRI_ATTACK
    MoveData{true, 0xA2, 10}, // SUPER_FANG
    MoveData{true, 0xA3, 20}, // SLASH
    MoveData{true, 0xA4, 10}, // SUBSTITUTE
    MoveData{true, 0xA5, 10}, // STRUGGLE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
    MoveData{false, 0x00, 0}, // NO_MOVE
};

inline constexpr const SpeciesData* FindSpeciesData(std::uint8_t internalId) {
    const auto& entry = kSpeciesData[internalId];
    return entry.valid ? &entry : nullptr;
}

inline constexpr const MoveData* FindMoveData(std::uint8_t moveId) {
    const auto& entry = kMoveData[moveId];
    return entry.valid ? &entry : nullptr;
}

}  // namespace pkmn::cli::red::data
