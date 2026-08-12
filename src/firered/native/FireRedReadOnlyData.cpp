#include "FireRedReadOnlyData.hpp"

#include "FireRedReferenceData.hpp"
#include "FireRedSaveStructure.hpp"
#include "FireRedTextCodec.hpp"
#include "Hex.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace firered {

namespace {
constexpr std::size_t kPokedexBytes = 52;

Warp DecodeWarp(std::span<const std::uint8_t> bytes, std::size_t offset) {
    Warp warp;
    warp.mapGroup = static_cast<std::int8_t>(bytes[offset]);
    warp.mapNumber = static_cast<std::int8_t>(bytes[offset + 1]);
    warp.warpId = static_cast<std::int8_t>(bytes[offset + 2]);
    warp.x = static_cast<std::int16_t>(ReadLe16(bytes, offset + 4));
    warp.y = static_cast<std::int16_t>(ReadLe16(bytes, offset + 6));
    warp.mapName = MapName(
        static_cast<std::uint8_t>(warp.mapGroup),
        static_cast<std::uint8_t>(warp.mapNumber));
    return warp;
}

std::vector<ItemStack> DecodePocket(
    std::span<const std::uint8_t> block,
    std::size_t offset,
    std::size_t capacity,
    std::string name,
    std::uint16_t key,
    bool encrypted) {
    std::vector<ItemStack> result;
    for (std::size_t slot = 0; slot < capacity; ++slot) {
        const auto itemId = ReadLe16(block, offset + slot * 4);
        const auto storedQuantity = ReadLe16(block, offset + slot * 4 + 2);
        if (itemId == 0) continue;
        result.push_back({
            name, slot, itemId, ItemName(itemId),
            static_cast<std::uint16_t>(storedQuantity ^ (encrypted ? key : 0)),
            encrypted
        });
    }
    return result;
}

std::vector<std::uint16_t> DecodeDexBits(std::span<const std::uint8_t> bytes) {
    std::vector<std::uint16_t> result;
    for (std::uint16_t national = 1; national <= 386; ++national) {
        const auto bit = static_cast<std::size_t>(national - 1);
        if ((bytes[bit / 8] & (1U << (bit % 8))) != 0) result.push_back(national);
    }
    return result;
}

MailRecord DecodeMail(std::span<const std::uint8_t> bytes, std::size_t slot) {
    MailRecord result;
    result.slot = slot;
    for (std::size_t i = 0; i < result.words.size(); ++i) {
        result.words[i] = ReadLe16(bytes, i * 2);
    }
    result.playerName = DecodeFireRedText(bytes.subspan(0x12, 8));
    result.trainerId = ReadLe32(bytes, 0x1A);
    result.species = ReadLe16(bytes, 0x1E);
    result.itemId = ReadLe16(bytes, 0x20);
    result.rawHex = EncodeHex(bytes.first(36));
    return result;
}

DaycareRecord DecodeDaycareMon(
    std::span<const std::uint8_t> bytes,
    std::string facility,
    std::size_t slot) {
    DaycareRecord result;
    result.facility = std::move(facility);
    result.slot = slot;
    result.pokemon = DecodeBoxPokemon(bytes.first(80));
    result.rawMailHex = EncodeHex(bytes.subspan(80, 56));
    result.mailOtName = DecodeFireRedText(bytes.subspan(116, 8));
    result.mailPokemonName = DecodeFireRedText(bytes.subspan(124, 11));
    const auto languages = bytes[135];
    result.gameLanguage = static_cast<std::uint8_t>(languages & 0xFU);
    result.pokemonLanguage = static_cast<std::uint8_t>(languages >> 4);
    result.steps = ReadLe32(bytes, 136);
    return result;
}
} // namespace

FireRedDecodedSave DecodeFireRedSave(const LogicalSave& logical) {
    if (logical.saveBlock2.size() != 0xF24
        || logical.saveBlock1.size() != 0x3D68
        || logical.pokemonStorage.size() != 0x83D0) {
        throw std::invalid_argument("logical save block sizes do not match FireRed");
    }
    const auto sb2 = std::span<const std::uint8_t>(logical.saveBlock2);
    const auto sb1 = std::span<const std::uint8_t>(logical.saveBlock1);
    const auto storage = std::span<const std::uint8_t>(logical.pokemonStorage);
    FireRedDecodedSave decoded;
    decoded.playerName = DecodeFireRedText(sb2.subspan(0, 8));
    decoded.playerGender = sb2[8];
    decoded.specialSaveWarpFlags = sb2[9];
    decoded.publicTrainerId = ReadLe16(sb2, 0xA);
    decoded.secretTrainerId = ReadLe16(sb2, 0xC);
    decoded.playTimeHours = ReadLe16(sb2, 0xE);
    decoded.playTimeMinutes = sb2[0x10];
    decoded.playTimeSeconds = sb2[0x11];
    decoded.playTimeVBlanks = sb2[0x12];
    decoded.options.buttonMode = sb2[0x13];
    decoded.options.raw = ReadLe16(sb2, 0x14);
    decoded.options.textSpeed = static_cast<std::uint8_t>(decoded.options.raw & 0x7U);
    decoded.options.windowFrameType = static_cast<std::uint8_t>((decoded.options.raw >> 3) & 0x1FU);
    decoded.options.stereo = ((decoded.options.raw >> 8) & 1U) != 0;
    decoded.options.setBattleStyle = ((decoded.options.raw >> 9) & 1U) != 0;
    decoded.options.battleSceneOff = ((decoded.options.raw >> 10) & 1U) != 0;
    decoded.options.regionMapZoom = ((decoded.options.raw >> 11) & 1U) != 0;
    decoded.encryptionKey = ReadLe32(sb2, 0xF20);
    decoded.playerX = static_cast<std::int16_t>(ReadLe16(sb1, 0));
    decoded.playerY = static_cast<std::int16_t>(ReadLe16(sb1, 2));
    decoded.location = DecodeWarp(sb1, 4);
    decoded.continueWarp = DecodeWarp(sb1, 0xC);
    decoded.dynamicWarp = DecodeWarp(sb1, 0x14);
    decoded.lastHealLocation = DecodeWarp(sb1, 0x1C);
    decoded.escapeWarp = DecodeWarp(sb1, 0x24);
    decoded.savedMusic = ReadLe16(sb1, 0x2C);
    decoded.weather = sb1[0x2E];
    decoded.weatherCycleStage = sb1[0x2F];
    decoded.flashLevel = sb1[0x30];
    decoded.mapLayoutId = ReadLe16(sb1, 0x32);
    decoded.partyCount = std::min<std::uint8_t>(sb1[0x34], 6);
    decoded.money = ReadLe32(sb1, 0x290) ^ decoded.encryptionKey;
    decoded.coins = static_cast<std::uint16_t>(
        ReadLe16(sb1, 0x294) ^ static_cast<std::uint16_t>(decoded.encryptionKey));
    decoded.registeredItem = ReadLe16(sb1, 0x296);
    decoded.rivalName = DecodeFireRedText(sb1.subspan(0x3A4C, 8));

    for (std::size_t index = 0; index < decoded.partyCount; ++index) {
        decoded.party.push_back(DecodePartyPokemon(sb1.subspan(0x38 + index * 100, 100)));
    }

    const auto key16 = static_cast<std::uint16_t>(decoded.encryptionKey);
    const std::array<std::tuple<std::size_t, std::size_t, const char*, bool>, 6> pockets{{
        {0x298, 30, "PC Items", false},
        {0x310, 42, "Items", true},
        {0x3B8, 30, "Key Items", true},
        {0x430, 13, "Poké Balls", true},
        {0x464, 58, "TM Case", true},
        {0x54C, 43, "Berry Pouch", true}
    }};
    for (const auto& [offset, capacity, name, encrypted] : pockets) {
        auto pocket = DecodePocket(sb1, offset, capacity, name, key16, encrypted);
        decoded.items.insert(decoded.items.end(), pocket.begin(), pocket.end());
    }

    decoded.pokedexOwned = DecodeDexBits(sb2.subspan(0x28, kPokedexBytes));
    const auto seenPrimary = DecodeDexBits(sb2.subspan(0x5C, kPokedexBytes));
    const auto seenMirror1 = DecodeDexBits(sb1.subspan(0x5F8, kPokedexBytes));
    const auto seenMirror2 = DecodeDexBits(sb1.subspan(0x3A18, kPokedexBytes));
    decoded.pokedexSeen = seenPrimary;
    decoded.pokedexSeenMirrorsConsistent =
        seenPrimary == seenMirror1 && seenPrimary == seenMirror2;
    if (!decoded.pokedexSeenMirrorsConsistent) {
        decoded.warnings.emplace_back("the three FireRed Pokédex seen mirrors differ");
    }

    for (std::uint16_t id = 0; id < 0x900; ++id) {
        if ((sb1[0xEE0 + id / 8] & (1U << (id % 8))) != 0) {
            decoded.setFlags.push_back({id, FlagName(id)});
        }
    }
    for (std::size_t index = 0; index < decoded.badges.size(); ++index) {
        constexpr std::uint16_t kFirstBadgeFlag = 0x820;
        const auto flag = static_cast<std::uint16_t>(kFirstBadgeFlag + index);
        decoded.badges[index] =
            (sb1[0xEE0 + flag / 8] & (1U << (flag % 8))) != 0;
    }
    for (std::uint16_t index = 0; index < 0x100; ++index) {
        const auto value = ReadLe16(sb1, 0x1000 + index * 2);
        if (value != 0) {
            const auto id = static_cast<std::uint16_t>(0x4000 + index);
            decoded.nonzeroVariables.push_back({id, VariableName(id), value});
        }
    }
    for (std::size_t index = 0; index < decoded.gameStats.size(); ++index) {
        decoded.gameStats[index] = ReadLe32(sb1, 0x1200 + index * 4) ^ decoded.encryptionKey;
    }

    for (std::size_t slot = 0; slot < 16; ++slot) {
        decoded.mail.push_back(DecodeMail(sb1.subspan(0x2CD0 + slot * 36, 36), slot));
    }
    decoded.daycare.push_back(DecodeDaycareMon(sb1.subspan(0x2F80, 140), "Four Island", 0));
    decoded.daycare.push_back(DecodeDaycareMon(sb1.subspan(0x300C, 140), "Four Island", 1));
    decoded.daycareOffspringPersonality = ReadLe16(sb1, 0x3098);
    decoded.daycareStepCounter = sb1[0x309A];
    decoded.daycare.push_back(DecodeDaycareMon(sb1.subspan(0x3C98, 140), "Route 5", 0));
    decoded.towerChallengeId = ReadLe32(sb1, 0x3D34);
    for (std::size_t i = 0; i < 4; ++i) {
        const auto offset = 0x3D38 + i * 12;
        const auto flags = sb1[offset + 10];
        const auto storedBestTime = ReadLe32(sb1, offset + 4);
        decoded.trainerTower.push_back({
            i, ReadLe32(sb1, offset), storedBestTime,
            storedBestTime ^ decoded.encryptionKey,
            sb1[offset + 8], sb1[offset + 9],
            (flags & 0x01U) != 0, (flags & 0x02U) != 0,
            (flags & 0x04U) != 0, (flags & 0x08U) != 0,
            (flags & 0x10U) != 0, (flags & 0x20U) != 0, flags
        });
    }

    decoded.currentBox = storage[0];
    if (decoded.currentBox >= 14) {
        decoded.warnings.emplace_back("current PC box index is outside 0-13");
    }
    for (std::size_t boxIndex = 0; boxIndex < 14; ++boxIndex) {
        PokemonBox box;
        box.index = boxIndex;
        box.name = DecodeFireRedText(storage.subspan(0x8344 + boxIndex * 9, 9));
        box.wallpaper = storage[0x83C2 + boxIndex];
        for (std::size_t slot = 0; slot < 30; ++slot) {
            auto pokemon = DecodeBoxPokemon(
                storage.subspan(4 + (boxIndex * 30 + slot) * 80, 80));
            if (pokemon.occupied) box.pokemon.push_back({slot, std::move(pokemon)});
        }
        decoded.boxes.push_back(std::move(box));
    }

    const auto invalidParty = static_cast<std::size_t>(std::count_if(
        decoded.party.begin(), decoded.party.end(),
        [](const PokemonRecord& pokemon) { return !pokemon.checksumValid; }));
    std::size_t invalidBoxed = 0;
    for (const auto& box : decoded.boxes) {
        invalidBoxed += static_cast<std::size_t>(std::count_if(
            box.pokemon.begin(), box.pokemon.end(),
            [](const PokemonBox::Entry& entry) { return !entry.pokemon.checksumValid; }));
    }
    if (invalidParty || invalidBoxed) {
        decoded.warnings.push_back(
            std::to_string(invalidParty + invalidBoxed)
            + " occupied Pokémon records have checksum mismatches");
    }
    return decoded;
}

std::string DumpFireRedCompactSummary(const FireRedDecodedSave& value) {
    std::ostringstream out;
    out << "=== Pkmn FireRed Save Genie ===\n\n";
    out << "Trainer: " << value.playerName << "\n";
    out << "Rival: " << value.rivalName << "\n";
    out << "Gender byte: " << static_cast<unsigned>(value.playerGender) << "\n";
    out << "Trainer ID: " << value.publicTrainerId
        << "  Secret ID: " << value.secretTrainerId << "\n";
    out << "Play time: " << value.playTimeHours << ":"
        << std::setw(2) << std::setfill('0') << static_cast<unsigned>(value.playTimeMinutes)
        << ":" << std::setw(2) << static_cast<unsigned>(value.playTimeSeconds)
        << std::setfill(' ') << "\n";
    out << "Money: " << value.money << "  Coins: " << value.coins << "\n";
    out << "Location: " << value.location.mapName
        << " (group " << static_cast<int>(value.location.mapGroup)
        << ", map " << static_cast<int>(value.location.mapNumber)
        << ", warp " << static_cast<int>(value.location.warpId)
        << ", x " << value.playerX << ", y " << value.playerY << ")\n";
    out << "Pokédex: " << value.pokedexOwned.size() << " owned, "
        << value.pokedexSeen.size() << " seen"
        << (value.pokedexSeenMirrorsConsistent ? "" : " [MIRROR WARNING]") << "\n";
    out << "Party: " << value.party.size() << "\n";
    for (std::size_t index = 0; index < value.party.size(); ++index) {
        const auto& pokemon = value.party[index];
        out << "  " << index + 1 << ". " << pokemon.nickname << " — "
            << pokemon.speciesName << " (#" << pokemon.nationalDex << "), Lv."
            << static_cast<unsigned>(pokemon.level)
            << ", checksum " << (pokemon.checksumValid ? "OK" : "BAD") << "\n";
    }
    std::size_t stored = 0;
    for (const auto& box : value.boxes) stored += box.pokemon.size();
    out << "PC storage: " << stored << " Pokémon across 14 boxes; current box "
        << static_cast<unsigned>(value.currentBox + 1) << "\n";
    out << "Inventory: " << value.items.size() << " occupied slots\n";
    out << "Set flags: " << value.setFlags.size()
        << "  Nonzero variables: " << value.nonzeroVariables.size() << "\n";
    if (!value.warnings.empty()) {
        out << "\nWarnings:\n";
        for (const auto& warning : value.warnings) out << "- " << warning << "\n";
    }
    return out.str();
}

namespace {
constexpr std::array<const char*, 8> kBadgeNames{
    "Boulder (Brock)", "Cascade (Misty)", "Thunder (Lt. Surge)",
    "Rainbow (Erika)", "Soul (Koga)", "Marsh (Sabrina)",
    "Volcano (Blaine)", "Earth (Giovanni)"
};

void DumpPokemonDetailed(
    std::ostringstream& out,
    const PokemonRecord& pokemon,
    std::size_t displayIndex) {
    out << "[" << displayIndex << "] " << pokemon.speciesName
        << " (InternalID=" << pokemon.speciesId
        << " | Dex#" << pokemon.nationalDex << ")\n";
    out << "    Nickname: \"" << pokemon.nickname << "\"\n";
    out << "    OT:       \"" << pokemon.otName << "\""
        << " (OTID32: " << pokemon.otId << ")\n";
    out << "    PID:      " << pokemon.personality << "\n";
    out << "    Shiny:    " << (pokemon.isShiny ? "YES" : "no")
        << " (value " << pokemon.shinyValue << ", Gen III threshold < 8)\n";
    out << "    Checksum: " << (pokemon.checksumValid ? "VALID" : "INVALID")
        << " (stored " << pokemon.storedChecksum
        << ", calculated " << pokemon.calculatedChecksum << ")\n";
    out << "    Flags: egg=" << (pokemon.egg ? "yes" : "no")
        << ", badEgg=" << (pokemon.badEgg ? "yes" : "no")
        << ", hasSpecies=" << (pokemon.hasSpecies ? "yes" : "no") << "\n";
    out << "    EXP: " << pokemon.experience
        << " | Friendship: " << static_cast<unsigned>(pokemon.friendship)
        << " | Held Item: " << pokemon.heldItemName
        << " (" << pokemon.heldItemId << ")\n";
    out << "    IVs: HP " << static_cast<unsigned>(pokemon.ivs[0])
        << " | ATK " << static_cast<unsigned>(pokemon.ivs[1])
        << " | DEF " << static_cast<unsigned>(pokemon.ivs[2])
        << " | SPD " << static_cast<unsigned>(pokemon.ivs[3])
        << " | SpA " << static_cast<unsigned>(pokemon.ivs[4])
        << " | SpD " << static_cast<unsigned>(pokemon.ivs[5]) << "\n";
    out << "    EVs: HP " << static_cast<unsigned>(pokemon.evs[0])
        << " | ATK " << static_cast<unsigned>(pokemon.evs[1])
        << " | DEF " << static_cast<unsigned>(pokemon.evs[2])
        << " | SPD " << static_cast<unsigned>(pokemon.evs[3])
        << " | SpA " << static_cast<unsigned>(pokemon.evs[4])
        << " | SpD " << static_cast<unsigned>(pokemon.evs[5]) << "\n";
    out << "    Contest: Cool " << static_cast<unsigned>(pokemon.contest[0])
        << " | Beauty " << static_cast<unsigned>(pokemon.contest[1])
        << " | Cute " << static_cast<unsigned>(pokemon.contest[2])
        << " | Smart " << static_cast<unsigned>(pokemon.contest[3])
        << " | Tough " << static_cast<unsigned>(pokemon.contest[4])
        << " | Sheen " << static_cast<unsigned>(pokemon.sheen) << "\n";
    out << "    Origin: metLocation=" << static_cast<unsigned>(pokemon.metLocation)
        << ", metLevel=" << static_cast<unsigned>(pokemon.metLevel)
        << ", game=" << static_cast<unsigned>(pokemon.metGame)
        << ", ball=" << static_cast<unsigned>(pokemon.pokeball)
        << ", OT female=" << (pokemon.otFemale ? "yes" : "no")
        << ", fateful=" << (pokemon.fatefulEncounter ? "yes" : "no") << "\n";
    out << "    Other: language=" << static_cast<unsigned>(pokemon.language)
        << ", markings=" << static_cast<unsigned>(pokemon.markings)
        << ", abilitySlot=" << (pokemon.abilitySlot ? 1 : 0)
        << ", pokerus=" << static_cast<unsigned>(pokemon.pokerus) << "\n";
    if (pokemon.partyDataPresent) {
        out << "    Party: Lv." << static_cast<unsigned>(pokemon.level)
            << " | HP " << pokemon.currentHp << "/" << pokemon.maxHp
            << " | ATK " << pokemon.stats[0]
            << " | DEF " << pokemon.stats[1]
            << " | SPD " << pokemon.stats[2]
            << " | SpA " << pokemon.stats[3]
            << " | SpD " << pokemon.stats[4]
            << " | Status 0x" << std::hex << std::uppercase
            << pokemon.status << std::dec << std::nouppercase << "\n";
    } else {
        out << "    Party-only level, HP, status and calculated stats: not stored in PC record\n";
    }
    out << "    Moves:\n";
    for (std::size_t index = 0; index < pokemon.moves.size(); ++index) {
        const auto& move = pokemon.moves[index];
        const auto ppUps = static_cast<unsigned>(
            (pokemon.ppBonuses >> (index * 2)) & 0x3U);
        out << "      " << index + 1 << ") " << move.name
            << " (ID " << move.id << ", current PP "
            << static_cast<unsigned>(move.pp) << ", PP Ups " << ppUps << ")\n";
    }
    for (const auto& warning : pokemon.warnings) {
        out << "    WARNING: " << warning << "\n";
    }
}

void DumpDexList(
    std::ostringstream& out,
    const char* label,
    const std::vector<std::uint16_t>& entries) {
    out << label << " (" << entries.size() << "):\n";
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto national = entries[index];
        out << "  #" << national << " " << NationalDexSpeciesName(national);
        if (index + 1 != entries.size()) out << "\n";
    }
    out << "\n";
}
} // namespace

std::string DumpFireRedDetailedSummary(const FireRedDecodedSave& value) {
    std::ostringstream out;
    out << "=== Pkmn FireRed Save Genie — Detailed Semantic Report ===\n\n";
    out << "--- Trainer / Game State ---\n";
    out << "Trainer Name: " << value.playerName << "\n";
    out << "Rival Name:   " << value.rivalName << "\n";
    out << "Gender Byte:  " << static_cast<unsigned>(value.playerGender) << "\n";
    out << "Trainer ID:   " << value.publicTrainerId << "\n";
    out << "Secret ID:    " << value.secretTrainerId << "\n";
    out << "Playtime:     " << value.playTimeHours << "h "
        << static_cast<unsigned>(value.playTimeMinutes) << "m "
        << static_cast<unsigned>(value.playTimeSeconds) << "s\n";
    out << "Money:        " << value.money << "\n";
    out << "Coins:        " << value.coins << "\n";
    out << "Encryption Key: 0x" << std::hex << std::uppercase
        << value.encryptionKey << std::dec << std::nouppercase << "\n";
    out << "Registered Item ID: " << value.registeredItem << "\n\n";

    out << "--- Location ---\n";
    out << "Current: " << value.location.mapName
        << " | group=" << static_cast<int>(value.location.mapGroup)
        << " map=" << static_cast<int>(value.location.mapNumber)
        << " warp=" << static_cast<int>(value.location.warpId)
        << " x=" << value.playerX << " y=" << value.playerY << "\n";
    out << "Continue Warp: " << value.continueWarp.mapName
        << " | group=" << static_cast<int>(value.continueWarp.mapGroup)
        << " map=" << static_cast<int>(value.continueWarp.mapNumber)
        << " warp=" << static_cast<int>(value.continueWarp.warpId)
        << " x=" << value.continueWarp.x << " y=" << value.continueWarp.y << "\n\n";

    out << "--- Badges ---\n";
    for (std::size_t index = 0; index < value.badges.size(); ++index) {
        out << index + 1 << ". " << kBadgeNames[index] << " -> "
            << (value.badges[index] ? "Yes" : "No") << "\n";
    }

    out << "\n--- Pokémon Party ---\n";
    out << "Party Count: " << value.party.size() << "\n";
    for (std::size_t index = 0; index < value.party.size(); ++index) {
        DumpPokemonDetailed(out, value.party[index], index + 1);
    }

    out << "\n--- Pokédex ---\n";
    out << "Owned: " << value.pokedexOwned.size() << " / 386\n";
    out << "Seen:  " << value.pokedexSeen.size() << " / 386\n";
    out << "Seen mirrors consistent: "
        << (value.pokedexSeenMirrorsConsistent ? "yes" : "NO") << "\n";
    DumpDexList(out, "Owned List", value.pokedexOwned);
    DumpDexList(out, "Seen List", value.pokedexSeen);

    out << "\n--- PC Storage ---\n";
    out << "Current Box: " << static_cast<unsigned>(value.currentBox + 1) << "\n";
    for (const auto& box : value.boxes) {
        out << "\nBox " << box.index + 1 << ": \"" << box.name
            << "\" | wallpaper=" << static_cast<unsigned>(box.wallpaper)
            << " | count=" << box.pokemon.size() << "\n";
        for (std::size_t index = 0; index < box.pokemon.size(); ++index) {
            out << "    Physical slot: " << box.pokemon[index].slot + 1 << "\n";
            DumpPokemonDetailed(out, box.pokemon[index].pokemon, index + 1);
        }
    }

    out << "\n--- Inventory ---\n";
    std::string currentPocket;
    for (const auto& item : value.items) {
        if (item.pocket != currentPocket) {
            currentPocket = item.pocket;
            out << "\n[" << currentPocket << "]\n";
        }
        out << "  Slot " << item.slot + 1 << ": " << item.name
            << " (ID " << item.id << ") x" << item.quantity
            << (item.encryptedQuantity ? " [encrypted]" : " [plain]") << "\n";
    }

    out << "\n--- Set Event / System Flags ---\n";
    out << "Set Count: " << value.setFlags.size() << "\n";
    for (const auto& flag : value.setFlags) {
        out << "  " << flag.id << " (0x" << std::hex << std::uppercase << flag.id
            << std::dec << std::nouppercase << "): " << flag.name << "\n";
    }

    out << "\n--- Nonzero Variables ---\n";
    out << "Nonzero Count: " << value.nonzeroVariables.size() << "\n";
    for (const auto& variable : value.nonzeroVariables) {
        out << "  " << variable.id << " (0x" << std::hex << std::uppercase
            << variable.id << std::dec << std::nouppercase << "): "
            << variable.name << " = " << variable.value << "\n";
    }

    out << "\n--- Game Statistics ---\n";
    for (std::size_t index = 0; index < value.gameStats.size(); ++index) {
        out << "  " << index << ": " << GameStatName(static_cast<std::uint16_t>(index))
            << " = " << value.gameStats[index] << "\n";
    }

    out << "\n--- Decoder Warnings ---\n";
    if (value.warnings.empty()) {
        out << "None\n";
    } else {
        for (const auto& warning : value.warnings) out << "- " << warning << "\n";
    }
    out << "\nCoverage note: the master .fred.json additionally contains decoded options, "
           "mail, daycare, Trainer Tower progress, Hall of Fame teams, exact PC slots, "
           "and raw/hash-backed coverage of every active logical and special-sector byte. "
           "Runtime, mystery-event and unused fields without a verified interpretation "
           "are explicitly raw-preserved rather than guessed.\n";
    return out.str();
}

std::string DumpFireRedSummary(const FireRedDecodedSave& value) {
    return DumpFireRedCompactSummary(value);
}

} // namespace firered
