#include "FireRedReferenceData.hpp"

#include "GeneratedFireRedTables.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <sstream>

namespace firered {

namespace {
template <typename Collection>
std::string Lookup(std::uint32_t id, const Collection& entries, const char* fallback) {
    const auto found = std::lower_bound(
        std::begin(entries), std::end(entries), id,
        [](const reference::IdName& entry, std::uint32_t value) {
            return entry.id < value;
        });
    if (found != std::end(entries) && found->id == id) return found->name;
    return std::string(fallback) + "_" + std::to_string(id);
}
} // namespace

std::string HumanizeConstant(const std::string& constant) {
    const auto firstUnderscore = constant.find('_');
    std::string value = firstUnderscore == std::string::npos
        ? constant : constant.substr(firstUnderscore + 1);
    std::replace(value.begin(), value.end(), '_', ' ');
    bool newWord = true;
    for (char& character : value) {
        if (character == ' ') {
            newWord = true;
        } else {
            character = static_cast<char>(
                newWord ? std::toupper(static_cast<unsigned char>(character))
                        : std::tolower(static_cast<unsigned char>(character)));
            newWord = false;
        }
    }
    return value;
}

std::string ItemName(std::uint16_t id) {
    return HumanizeConstant(Lookup(id, reference::kItems, "ITEM_UNKNOWN"));
}

std::string MoveName(std::uint16_t id) {
    return HumanizeConstant(Lookup(id, reference::kMoves, "MOVE_UNKNOWN"));
}

std::string SpeciesName(std::uint16_t id) {
    return HumanizeConstant(Lookup(id, reference::kSpecies, "SPECIES_UNKNOWN"));
}

std::uint16_t SpeciesNationalDex(std::uint16_t id) {
    if (id < std::size(reference::kSpeciesToNationalDex)) {
        return reference::kSpeciesToNationalDex[id];
    }
    return 0;
}

std::string NationalDexSpeciesName(std::uint16_t nationalDex) {
    for (std::uint16_t species = 1;
         species < std::size(reference::kSpeciesToNationalDex); ++species) {
        if (reference::kSpeciesToNationalDex[species] == nationalDex) {
            return SpeciesName(species);
        }
    }
    return "National Dex #" + std::to_string(nationalDex);
}

std::string FlagName(std::uint16_t id) {
    const auto name = Lookup(id, reference::kFlags, "FLAG_UNKNOWN");
    return name.rfind("FLAG_UNKNOWN", 0) == 0 ? name : HumanizeConstant(name);
}

std::string VariableName(std::uint16_t id) {
    const auto name = Lookup(id, reference::kVariables, "VAR_UNKNOWN");
    return name.rfind("VAR_UNKNOWN", 0) == 0 ? name : HumanizeConstant(name);
}

std::string GameStatName(std::uint16_t id) {
    const auto name = Lookup(id, reference::kGameStats, "GAME_STAT_UNKNOWN");
    return name.rfind("GAME_STAT_UNKNOWN", 0) == 0 ? name : HumanizeConstant(name);
}

std::string MapName(std::uint8_t group, std::uint8_t number) {
    for (const auto& map : reference::kMaps) {
        if (map.group == group && map.number == number) return map.name;
    }
    return "UnknownMap(" + std::to_string(group) + "," + std::to_string(number) + ")";
}

const char* PretReferenceCommit() {
    return reference::kPretCommit;
}

} // namespace firered
