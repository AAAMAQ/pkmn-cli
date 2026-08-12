#include "FireRedMasterJson.hpp"

#include "FireRedLogicalSave.hpp"
#include "FireRedReadOnlyData.hpp"
#include "FireRedReferenceData.hpp"
#include "FireRedSaveStructure.hpp"
#include "FireRedTextCodec.hpp"
#include "Hex.hpp"
#include "Sha256.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>

namespace firered {

using nlohmann::ordered_json;

namespace {
constexpr std::size_t kSectorSize = 0x1000;
constexpr std::size_t kSectorDataSize = 0xF80;

std::uint16_t SpecialChecksum(std::span<const std::uint8_t> bytes) {
    std::uint32_t sum = 0;
    for (std::size_t offset = 0; offset + 4 <= bytes.size(); offset += 4) {
        sum += ReadLe32(bytes, offset);
    }
    return static_cast<std::uint16_t>((sum >> 16) + (sum & 0xFFFFU));
}

ordered_json SpecialSectorsJson(std::span<const std::uint8_t> image) {
    ordered_json sectors = ordered_json::array();
    for (std::size_t number = 28; number < 32; ++number) {
        const auto sector = image.subspan(number * kSectorSize, kSectorSize);
        const auto signature = ReadLe32(sector, 0xFF8);
        ordered_json entry{
            {"physicalSector", number},
            {"role", number < 30 ? "hall-of-fame" : "downloadable-trainer-tower"},
            {"signature", signature},
            {"signatureValid", signature == Layout::kSectorSignature},
            {"footerIdOrChecksum", ReadLe16(sector, 0xFF4)},
            {"footerChecksum", ReadLe16(sector, 0xFF6)},
            {"counter", ReadLe32(sector, 0xFFC)},
            {"fullSectorHex", EncodeHex(sector)},
            {"fullSectorSha256", Sha256Hex(sector)}
        };
        if (number < 30) {
            entry["calculatedDataChecksum"] = SpecialChecksum(sector.first(kSectorDataSize));
            entry["dataChecksumValid"] = signature == Layout::kSectorSignature
                && ReadLe16(sector, 0xFF4) == SpecialChecksum(sector.first(kSectorDataSize));
        } else {
            entry["sentinel"] = ReadLe32(sector, 0);
            entry["sentinelValid"] = ReadLe32(sector, 0) == 0xB39DU;
        }
        sectors.push_back(std::move(entry));
    }

    ordered_json teams = ordered_json::array();
    const auto first = image.subspan(28 * kSectorSize, kSectorSize);
    const auto second = image.subspan(29 * kSectorSize, kSectorSize);
    const bool valid = ReadLe32(first, 0xFF8) == Layout::kSectorSignature
        && ReadLe32(second, 0xFF8) == Layout::kSectorSignature
        && ReadLe16(first, 0xFF4) == SpecialChecksum(first.first(kSectorDataSize))
        && ReadLe16(second, 0xFF4) == SpecialChecksum(second.first(kSectorDataSize));
    if (valid) {
        Bytes data;
        data.insert(data.end(), first.begin(), first.begin() + kSectorDataSize);
        data.insert(data.end(), second.begin(), second.begin() + kSectorDataSize);
        for (std::size_t teamIndex = 0; teamIndex < 50; ++teamIndex) {
            const auto teamOffset = teamIndex * 120;
            if ((ReadLe16(data, teamOffset + 8) & 0x1FFU) == 0) break;
            ordered_json mons = ordered_json::array();
            for (std::size_t monIndex = 0; monIndex < 6; ++monIndex) {
                const auto offset = teamOffset + monIndex * 20;
                const auto speciesLevel = ReadLe16(data, offset + 8);
                const auto species = static_cast<std::uint16_t>(speciesLevel & 0x1FFU);
                if (species == 0) continue;
                const auto trainerId = ReadLe32(data, offset);
                const auto personality = ReadLe32(data, offset + 4);
                const auto shinyValue = static_cast<std::uint16_t>(
                    static_cast<std::uint16_t>(trainerId)
                    ^ static_cast<std::uint16_t>(trainerId >> 16)
                    ^ static_cast<std::uint16_t>(personality)
                    ^ static_cast<std::uint16_t>(personality >> 16));
                mons.push_back({
                    {"partySlot", monIndex}, {"trainerId", trainerId},
                    {"personality", personality},
                    {"shinyValue", shinyValue}, {"isShiny", shinyValue < 8},
                    {"species", species}, {"speciesName", SpeciesName(species)},
                    {"nationalDex", SpeciesNationalDex(species)},
                    {"level", speciesLevel >> 9},
                    {"nickname", DecodeFireRedText(std::span<const std::uint8_t>(data).subspan(offset + 10, 10))},
                    {"rawHex", EncodeHex(std::span<const std::uint8_t>(data).subspan(offset, 20))}
                });
            }
            teams.push_back({{"index", teamIndex}, {"pokemon", std::move(mons)}});
        }
    }
    return {
        {"physicalSectors", std::move(sectors)},
        {"hallOfFame", {{"valid", valid}, {"teamCount", teams.size()}, {"teams", std::move(teams)}}},
        {"trainerTowerPayload", {
            {"classification", "downloadable e-Reader/card payload; raw-preserved"},
            {"sector30Valid", ReadLe32(image, 30 * kSectorSize) == 0xB39DU},
            {"sector31Valid", ReadLe32(image, 31 * kSectorSize) == 0xB39DU}
        }}
    };
}

ordered_json LogicalCoverageJson(const LogicalSave& logical) {
    return {
        {"contract", "Every active logical byte is included exactly; semantic fields are additive views."},
        {"saveBlock2", {
            {"size", logical.saveBlock2.size()}, {"sha256", Sha256Hex(logical.saveBlock2)},
            {"rawHex", EncodeHex(logical.saveBlock2)},
            {"semanticallyDecodedRanges", {
                {{"offset", "0x0000"}, {"size", "0x0016"}, {"role", "trainer-time-options"}},
                {{"offset", "0x0018"}, {"size", "0x0078"}, {"role", "pokedex"}},
                {{"offset", "0x0F20"}, {"size", "0x0004"}, {"role", "encryption-key"}}
            }},
            {"remainingBytes", "raw-preserved with pret struct names documented in coverage ledger"}
        }},
        {"saveBlock1", {
            {"size", logical.saveBlock1.size()}, {"sha256", Sha256Hex(logical.saveBlock1)},
            {"rawHex", EncodeHex(logical.saveBlock1)},
            {"semanticallyDecodedRanges", {
                {{"offset", "0x0000"}, {"size", "0x0034"}, {"role", "map-state"}},
                {{"offset", "0x0034"}, {"size", "0x0260"}, {"role", "party"}},
                {{"offset", "0x0290"}, {"size", "0x03BC"}, {"role", "currency-inventory-pokedex-mirror"}},
                {{"offset", "0x0EE0"}, {"size", "0x0420"}, {"role", "flags-vars-stats"}},
                {{"offset", "0x2CD0"}, {"size", "0x03CC"}, {"role", "mail-daycare"}},
                {{"offset", "0x3A18"}, {"size", "0x0040"}, {"role", "pokedex-mirror-rival"}},
                {{"offset", "0x3C98"}, {"size", "0x00D0"}, {"role", "route5-daycare-trainer-tower"}}
            }},
            {"remainingBytes", "raw-preserved with pret struct names documented in coverage ledger"}
        }},
        {"pokemonStorage", {
            {"size", logical.pokemonStorage.size()}, {"sha256", Sha256Hex(logical.pokemonStorage)},
            {"rawHex", EncodeHex(logical.pokemonStorage)},
            {"semanticCoverage", "current box, all 420 exact slots, box names, wallpapers; every occupied record fully raw-backed"}
        }}
    };
}

ordered_json SectionJson(const SectionInfo& section) {
    return {
        {"physicalSector", section.physicalSector},
        {"fileOffset", HexOffset(section.fileOffset)},
        {"id", section.id},
        {"role", SectionRole(section.id)},
        {"checksumDataSize", section.checksumDataSize},
        {"storedChecksum", section.storedChecksum},
        {"calculatedChecksum", section.calculatedChecksum},
        {"signature", section.signature},
        {"counter", section.counter},
        {"idValid", section.idValid},
        {"signatureValid", section.signatureValid},
        {"checksumValid", section.checksumValid}
    };
}

ordered_json SlotJson(const SlotInfo& slot) {
    ordered_json sections = ordered_json::array();
    for (const auto& section : slot.sections) sections.push_back(SectionJson(section));
    return {
        {"slotIndex", slot.slotIndex},
        {"counter", slot.counter},
        {"valid", slot.valid},
        {"hasAnySignature", slot.hasAnySignature},
        {"allSectionIdsPresent", slot.allSectionIdsPresent},
        {"countersConsistent", slot.countersConsistent},
        {"errors", slot.errors},
        {"physicalSections", std::move(sections)}
    };
}

ordered_json PokemonJson(const PokemonRecord& pokemon) {
    ordered_json moves = ordered_json::array();
    for (const auto& move : pokemon.moves) {
        moves.push_back({{"id", move.id}, {"name", move.name}, {"pp", move.pp},
                         {"ppUps", move.ppUps}});
    }
    ordered_json partyData = nullptr;
    if (pokemon.partyDataPresent) {
        partyData = {
            {"level", pokemon.level},
            {"status", pokemon.status},
            {"mailIndex", pokemon.mail},
            {"currentHp", pokemon.currentHp},
            {"maxHp", pokemon.maxHp},
            {"attack", pokemon.stats[0]},
            {"defense", pokemon.stats[1]},
            {"speed", pokemon.stats[2]},
            {"specialAttack", pokemon.stats[3]},
            {"specialDefense", pokemon.stats[4]}
        };
    }
    return {
        {"occupied", pokemon.occupied},
        {"partyDataPresent", pokemon.partyDataPresent},
        {"checksumValid", pokemon.checksumValid},
        {"storedChecksum", pokemon.storedChecksum},
        {"calculatedChecksum", pokemon.calculatedChecksum},
        {"badEgg", pokemon.badEgg},
        {"isEgg", pokemon.egg},
        {"hasSpecies", pokemon.hasSpecies},
        {"personality", pokemon.personality},
        {"otId", pokemon.otId},
        {"publicTrainerId", pokemon.publicTrainerId},
        {"secretTrainerId", pokemon.secretTrainerId},
        {"shinyValue", pokemon.shinyValue},
        {"isShiny", pokemon.isShiny},
        {"nickname", pokemon.nickname},
        {"otName", pokemon.otName},
        {"language", pokemon.language},
        {"markings", pokemon.markings},
        {"headerFlagsRaw", pokemon.headerFlags},
        {"blockBoxRS", pokemon.blockBoxRs},
        {"unusedHeaderFlags", pokemon.unusedHeaderFlags},
        {"species", {
            {"internalId", pokemon.speciesId},
            {"nationalDex", pokemon.nationalDex},
            {"name", pokemon.speciesName}
        }},
        {"heldItem", {{"id", pokemon.heldItemId}, {"name", pokemon.heldItemName}}},
        {"experience", pokemon.experience},
        {"nature", {{"id", pokemon.natureId}, {"name", pokemon.natureName}}},
        {"friendship", pokemon.friendship},
        {"growthFiller", pokemon.growthFiller},
        {"ppBonuses", pokemon.ppBonuses},
        {"moves", std::move(moves)},
        {"evs", {
            {"hp", pokemon.evs[0]}, {"attack", pokemon.evs[1]},
            {"defense", pokemon.evs[2]}, {"speed", pokemon.evs[3]},
            {"specialAttack", pokemon.evs[4]}, {"specialDefense", pokemon.evs[5]}
        }},
        {"contest", {
            {"cool", pokemon.contest[0]}, {"beauty", pokemon.contest[1]},
            {"cute", pokemon.contest[2]}, {"smart", pokemon.contest[3]},
            {"tough", pokemon.contest[4]},
            {"sheen", pokemon.sheen}
        }},
        {"ivs", {
            {"hp", pokemon.ivs[0]}, {"attack", pokemon.ivs[1]},
            {"defense", pokemon.ivs[2]}, {"speed", pokemon.ivs[3]},
            {"specialAttack", pokemon.ivs[4]}, {"specialDefense", pokemon.ivs[5]}
        }},
        {"origins", {
            {"raw", pokemon.originsRaw},
            {"metLocation", pokemon.metLocation},
            {"metLevel", pokemon.metLevel},
            {"metGame", pokemon.metGame},
            {"pokeball", pokemon.pokeball},
            {"otFemale", pokemon.otFemale},
            {"fatefulEncounter", pokemon.fatefulEncounter}
        }},
        {"abilitySlot", pokemon.abilitySlot ? 1 : 0},
        {"pokerus", {{"raw", pokemon.pokerus}, {"strain", pokemon.pokerusStrain},
                       {"daysRemaining", pokemon.pokerusDays}}},
        {"ribbons", {
            {"raw", pokemon.ribbonWordRaw},
            {"cool", pokemon.ribbons.cool}, {"beauty", pokemon.ribbons.beauty},
            {"cute", pokemon.ribbons.cute}, {"smart", pokemon.ribbons.smart},
            {"tough", pokemon.ribbons.tough}, {"champion", pokemon.ribbons.champion},
            {"winning", pokemon.ribbons.winning}, {"victory", pokemon.ribbons.victory},
            {"artist", pokemon.ribbons.artist}, {"effort", pokemon.ribbons.effort},
            {"marine", pokemon.ribbons.marine}, {"land", pokemon.ribbons.land},
            {"sky", pokemon.ribbons.sky}, {"country", pokemon.ribbons.country},
            {"national", pokemon.ribbons.national}, {"earth", pokemon.ribbons.earth},
            {"world", pokemon.ribbons.world}, {"unusedBits", pokemon.ribbons.unused}
        }},
        {"raw", {
            {"boxRecordHex", pokemon.rawBoxHex},
            {"decryptedSecureHex", pokemon.decryptedSecureHex},
            {"partyExtensionHex", pokemon.rawPartyExtensionHex},
            {"headerUnknown", pokemon.headerUnknown},
            {"ivWord", pokemon.ivWordRaw}
        }},
        {"partyData", std::move(partyData)},
        {"warnings", pokemon.warnings}
    };
}

ordered_json SemanticJson(const FireRedDecodedSave& decoded) {
    ordered_json party = ordered_json::array();
    for (const auto& pokemon : decoded.party) party.push_back(PokemonJson(pokemon));
    ordered_json boxes = ordered_json::array();
    for (const auto& box : decoded.boxes) {
        ordered_json pokemon = ordered_json::array();
        for (const auto& entry : box.pokemon) {
            auto value = PokemonJson(entry.pokemon);
            value["slot"] = entry.slot;
            pokemon.push_back(std::move(value));
        }
        boxes.push_back({
            {"index", box.index},
            {"name", box.name},
            {"wallpaper", box.wallpaper},
            {"count", box.pokemon.size()},
            {"pokemon", std::move(pokemon)}
        });
    }
    ordered_json items = ordered_json::array();
    for (const auto& item : decoded.items) {
        items.push_back({
            {"pocket", item.pocket}, {"slot", item.slot}, {"id", item.id},
            {"name", item.name}, {"quantity", item.quantity},
            {"encryptedQuantity", item.encryptedQuantity}
        });
    }
    ordered_json flags = ordered_json::array();
    for (const auto& flag : decoded.setFlags) {
        flags.push_back({{"id", flag.id}, {"name", flag.name}});
    }
    ordered_json variables = ordered_json::array();
    for (const auto& variable : decoded.nonzeroVariables) {
        variables.push_back({
            {"id", variable.id}, {"name", variable.name}, {"value", variable.value}
        });
    }
    ordered_json gameStats = ordered_json::array();
    for (std::size_t index = 0; index < decoded.gameStats.size(); ++index) {
        gameStats.push_back({
            {"id", index},
            {"name", GameStatName(static_cast<std::uint16_t>(index))},
            {"value", decoded.gameStats[index]}
        });
    }
    ordered_json badges = ordered_json::array();
    static constexpr std::array<const char*, 8> kBadgeNames{
        "Boulder", "Cascade", "Thunder", "Rainbow",
        "Soul", "Marsh", "Volcano", "Earth"
    };
    for (std::size_t index = 0; index < decoded.badges.size(); ++index) {
        badges.push_back({
            {"number", index + 1},
            {"name", kBadgeNames[index]},
            {"obtained", decoded.badges[index]},
            {"flagId", 0x820 + index}
        });
    }
    const auto warpJson = [](const Warp& warp) {
        return ordered_json{
            {"mapGroup", warp.mapGroup}, {"mapNumber", warp.mapNumber},
            {"mapName", warp.mapName}, {"warpId", warp.warpId},
            {"x", warp.x}, {"y", warp.y}
        };
    };
    ordered_json mail = ordered_json::array();
    for (const auto& value : decoded.mail) {
        mail.push_back({
            {"slot", value.slot}, {"words", value.words},
            {"playerName", value.playerName}, {"trainerId", value.trainerId},
            {"species", value.species}, {"itemId", value.itemId},
            {"rawHex", value.rawHex}
        });
    }
    ordered_json daycare = ordered_json::array();
    for (const auto& value : decoded.daycare) {
        daycare.push_back({
            {"facility", value.facility}, {"slot", value.slot},
            {"pokemon", PokemonJson(value.pokemon)}, {"steps", value.steps},
            {"mail", {{"otName", value.mailOtName},
                       {"pokemonName", value.mailPokemonName},
                       {"gameLanguage", value.gameLanguage},
                       {"pokemonLanguage", value.pokemonLanguage},
                       {"rawHex", value.rawMailHex}}}
        });
    }
    ordered_json tower = ordered_json::array();
    for (const auto& value : decoded.trainerTower) {
        const auto towerTime = [](std::uint32_t frames) {
            const auto minutes = frames / 3600;
            const auto remainder = frames % 3600;
            const auto seconds = remainder / 60;
            const auto centiseconds = (remainder % 60) * 168 / 100;
            return ordered_json{{"frames", frames}, {"minutes", minutes},
                                {"seconds", seconds}, {"centiseconds", centiseconds}};
        };
        tower.push_back({
            {"challengeType", value.challengeType}, {"timer", towerTime(value.timer)},
            {"bestTimeStoredEncrypted", value.bestTimeStored},
            {"bestTime", towerTime(value.bestTime)}, {"floorsCleared", value.floorsCleared},
            {"dataSetId", value.dataSetId}, {"receivedPrize", value.receivedPrize},
            {"checkedFinalTime", value.checkedFinalTime},
            {"spokeToOwner", value.spokeToOwner}, {"hasLost", value.hasLost},
            {"unknownFlag", value.unknownFlag}, {"validated", value.validated},
            {"rawFlags", value.rawFlags}
        });
    }
    return {
        {"trainer", {
            {"name", decoded.playerName},
            {"rivalName", decoded.rivalName},
            {"genderByte", decoded.playerGender},
            {"specialSaveWarpFlags", decoded.specialSaveWarpFlags},
            {"publicTrainerId", decoded.publicTrainerId},
            {"secretTrainerId", decoded.secretTrainerId}
        }},
        {"playtime", {
            {"hours", decoded.playTimeHours},
            {"minutes", decoded.playTimeMinutes},
            {"seconds", decoded.playTimeSeconds},
            {"vblanks", decoded.playTimeVBlanks}
        }},
        {"options", {
            {"buttonMode", decoded.options.buttonMode},
            {"textSpeed", decoded.options.textSpeed},
            {"windowFrameType", decoded.options.windowFrameType},
            {"sound", decoded.options.stereo ? "stereo" : "mono"},
            {"battleStyle", decoded.options.setBattleStyle ? "set" : "shift"},
            {"battleSceneOff", decoded.options.battleSceneOff},
            {"regionMapZoom", decoded.options.regionMapZoom},
            {"raw", decoded.options.raw}
        }},
        {"security", {{"saveEncryptionKey", decoded.encryptionKey}}},
        {"currency", {{"money", decoded.money}, {"coins", decoded.coins}}},
        {"location", {
            {"mapGroup", decoded.location.mapGroup},
            {"mapNumber", decoded.location.mapNumber},
            {"mapName", decoded.location.mapName},
            {"warpId", decoded.location.warpId},
            {"x", decoded.playerX}, {"y", decoded.playerY}
        }},
        {"continueWarp", {
            {"mapGroup", decoded.continueWarp.mapGroup},
            {"mapNumber", decoded.continueWarp.mapNumber},
            {"mapName", decoded.continueWarp.mapName},
            {"warpId", decoded.continueWarp.warpId},
            {"x", decoded.continueWarp.x}, {"y", decoded.continueWarp.y}
        }},
        {"mapState", {
            {"dynamicWarp", warpJson(decoded.dynamicWarp)},
            {"lastHealLocation", warpJson(decoded.lastHealLocation)},
            {"escapeWarp", warpJson(decoded.escapeWarp)},
            {"savedMusic", decoded.savedMusic}, {"weather", decoded.weather},
            {"weatherCycleStage", decoded.weatherCycleStage},
            {"flashLevel", decoded.flashLevel}, {"mapLayoutId", decoded.mapLayoutId}
        }},
        {"badges", std::move(badges)},
        {"pokedex", {
            {"ownedCount", decoded.pokedexOwned.size()},
            {"seenCount", decoded.pokedexSeen.size()},
            {"ownedNationalDexNumbers", decoded.pokedexOwned},
            {"seenNationalDexNumbers", decoded.pokedexSeen},
            {"seenMirrorsConsistent", decoded.pokedexSeenMirrorsConsistent}
        }},
        {"party", {{"count", decoded.party.size()}, {"pokemon", std::move(party)}}},
        {"storage", {
            {"currentBox", decoded.currentBox},
            {"boxCount", decoded.boxes.size()},
            {"boxes", std::move(boxes)}
        }},
        {"inventory", {
            {"registeredItemId", decoded.registeredItem},
            {"occupiedSlots", decoded.items.size()},
            {"items", std::move(items)}
        }},
        {"mail", std::move(mail)},
        {"daycare", {
            {"fourIslandOffspringPersonality", decoded.daycareOffspringPersonality},
            {"fourIslandStepCounter", decoded.daycareStepCounter},
            {"records", std::move(daycare)}
        }},
        {"trainerTower", {
            {"activeChallengeType", decoded.towerChallengeId},
            {"records", std::move(tower)}
        }},
        {"progress", {
            {"setFlagCount", decoded.setFlags.size()},
            {"setFlags", std::move(flags)},
            {"nonzeroVariableCount", decoded.nonzeroVariables.size()},
            {"nonzeroVariables", std::move(variables)},
            {"gameStats", std::move(gameStats)}
        }},
        {"warnings", decoded.warnings}
    };
}

ordered_json ConversionModelJson(const FireRedDecodedSave& decoded) {
    ordered_json party = ordered_json::array();
    for (const auto& pokemon : decoded.party) {
        party.push_back({
            {"speciesNationalDex", pokemon.nationalDex},
            {"nickname", pokemon.nickname},
            {"otName", pokemon.otName},
            {"sourcePersonality", pokemon.personality},
            {"sourceOtId", pokemon.otId},
            {"moves", {
                pokemon.moves[0].id, pokemon.moves[1].id,
                pokemon.moves[2].id, pokemon.moves[3].id
            }},
            {"experience", pokemon.experience},
            {"friendship", pokemon.friendship},
            {"ivs", pokemon.ivs},
            {"evs", pokemon.evs},
            {"checksumValid", pokemon.checksumValid}
        });
    }
    return {
        {"format", "pkmn-shared-conversion-model"},
        {"version", "0.1.0-draft"},
        {"role", "firered-target-and-native-source-view"},
        {"reference", {
            {"repository", "pret/pokefirered"},
            {"commit", PretReferenceCommit()}
        }},
        {"bridge", {
            {"redInput", ".red.json conversionModel"},
            {"sharedAuthority", "semantic concepts and explicit policy results"},
            {"fireredTarget", ".fred.json conversionModel"},
            {"physicalBytesAreNeverBridged", true}
        }},
        {"identity", {
            {"trainerName", decoded.playerName},
            {"publicTrainerId", decoded.publicTrainerId},
            {"secretTrainerId", decoded.secretTrainerId},
            {"genderByte", decoded.playerGender},
            {"classification", "direct_or_policy_generated"}
        }},
        {"playtime", {
            {"hours", decoded.playTimeHours},
            {"minutes", decoded.playTimeMinutes},
            {"seconds", decoded.playTimeSeconds},
            {"classification", "direct_transfer"}
        }},
        {"currency", {
            {"money", decoded.money},
            {"coins", decoded.coins},
            {"classification", "direct_with_target_caps"}
        }},
        {"pokedex", {
            {"ownedNationalDexNumbers", decoded.pokedexOwned},
            {"seenNationalDexNumbers", decoded.pokedexSeen},
            {"classification", "semantic_translation_by_national_dex"}
        }},
        {"party", std::move(party)},
        {"location", {
            {"sourceMap", decoded.location.mapName},
            {"classification", "policy_required_never_raw_id_transfer"}
        }},
        {"storyProgress", {
            {"classification", "explicit_mapping_table_required"},
            {"rawFlagsOrVariablesAreNotTransferable", true}
        }},
        {"inventory", {
            {"classification", "explicit_item_mapping_and_pocket_policy_required"}
        }},
        {"redOnlyPreservation", {
            "Red Hall of Fame history",
            "Gen I DVs and Stat Experience source values",
            "unmapped runtime and scratch state"
        }},
        {"fireredOnlyGeneratedFields", {
            "personality/PID", "nature", "ability slot", "IVs/EVs",
            "friendship", "met data", "Poké Ball", "language", "ribbons",
            "secret trainer ID"
        }},
        {"status", "bridge-defined; conversion-writer-not-implemented"}
    };
}
} // namespace

std::string ExportMasterJson(
    const std::filesystem::path& sourcePath,
    std::span<const std::uint8_t> image,
    const SaveAnalysis& analysis) {
    if (!analysis.standardFlashPresent) {
        throw std::invalid_argument("cannot export a FireRed master JSON without 128 KiB flash data");
    }

    const auto standard = image.first(Layout::kStandardFlashSize);
    const auto trailing = image.subspan(Layout::kStandardFlashSize);
    const auto logical = AssembleLogicalSave(image, analysis);
    const auto semantic = DecodeFireRedSave(logical);
    ordered_json slots = ordered_json::array({SlotJson(analysis.slots[0]), SlotJson(analysis.slots[1])});

    ordered_json root{
        {"format", "pkmn-firered-master-save"},
        {"schemaVersion", "0.4.0"},
        {"generator", {
            {"name", "Pkmn FireRed Save Genie"},
            {"version", "0.4.0-complete-coverage"}
        }},
        {"source", {
            {"filename", sourcePath.filename().string()},
            {"fileSize", image.size()},
            {"sha256", Sha256Hex(image)}
        }},
        {"physicalImage", {
            {"authority", "no-edit-reconstruction"},
            {"encoding", "uppercase-hex"},
            {"fullFileHex", EncodeHex(image)},
            {"fullFileSha256", Sha256Hex(image)},
            {"standardFlashSize", Layout::kStandardFlashSize},
            {"standardFlashSha256", Sha256Hex(standard)},
            {"trailingDataSize", trailing.size()},
            {"trailingDataSha256", Sha256Hex(trailing)}
        }},
        {"decoded", {
            {"stage", "native-fireRed-reader"},
            {"activeSlot", analysis.activeSlot ? ordered_json(*analysis.activeSlot) : ordered_json(nullptr)},
            {"activeSlotAmbiguous", analysis.activeSlotAmbiguous},
            {"slots", std::move(slots)},
            {"semantic", SemanticJson(semantic)},
            {"logicalBlocks", LogicalCoverageJson(logical)},
            {"specialSectors", SpecialSectorsJson(standard)}
        }},
        {"coverage", {
            {"semanticAuthority", "known native fields decoded; all other bytes explicitly raw-preserved"},
            {"generationAuthority", "not-yet; future FireRed generator contract"},
            {"standardFlashBytes", Layout::kStandardFlashSize},
            {"trailingBytes", trailing.size()},
            {"preservedBytes", image.size()},
            {"uncoveredBytes", 0},
            {"policy", "physical image and active logical block hex are lossless authorities; no unknown byte is inferred"},
            {"activeLogicalByteCoveragePercent", 100},
            {"specialSectorByteCoveragePercent", 100}
        }},
        {"diagnostics", analysis.diagnostics},
        {"conversionModel", ConversionModelJson(semantic)}
    };
    return root.dump(2) + "\n";
}

Bytes ImportPhysicalImage(const std::string& jsonText, std::string& sourceFilename) {
    const auto root = ordered_json::parse(jsonText);
    if (root.value("format", "") != "pkmn-firered-master-save") {
        throw std::invalid_argument("unsupported master-save format");
    }
    const auto schemaVersion = root.value("schemaVersion", "");
    if (schemaVersion != "0.1.0"
        && schemaVersion != "0.2.0"
        && schemaVersion != "0.3.0"
        && schemaVersion != "0.4.0") {
        throw std::invalid_argument("unsupported .fred.json schema version");
    }
    if (!root.contains("source") || !root["source"].is_object()
        || !root["source"].contains("filename") || !root["source"]["filename"].is_string()) {
        throw std::invalid_argument("source.filename is required");
    }
    if (!root.contains("physicalImage") || !root["physicalImage"].is_object()) {
        throw std::invalid_argument("physicalImage is required");
    }
    const auto& physical = root["physicalImage"];
    if (physical.value("encoding", "") != "uppercase-hex"
        || !physical.contains("fullFileHex") || !physical["fullFileHex"].is_string()
        || !physical.contains("fullFileSha256") || !physical["fullFileSha256"].is_string()) {
        throw std::invalid_argument("canonical physicalImage fields are missing");
    }

    sourceFilename = root["source"]["filename"].get<std::string>();
    if (std::filesystem::path(sourceFilename).filename().string() != sourceFilename) {
        throw std::invalid_argument("source filename must not contain a directory path");
    }
    auto bytes = DecodeHex(physical["fullFileHex"].get<std::string>());
    if (bytes.size() < Layout::kStandardFlashSize) {
        throw std::invalid_argument("physical image is smaller than 128 KiB");
    }
    if (Sha256Hex(bytes) != physical["fullFileSha256"].get<std::string>()) {
        throw std::invalid_argument("physical image SHA-256 mismatch");
    }
    if (root["source"].contains("fileSize")
        && root["source"]["fileSize"].get<std::size_t>() != bytes.size()) {
        throw std::invalid_argument("source file size does not match physical image");
    }
    return bytes;
}

} // namespace firered
