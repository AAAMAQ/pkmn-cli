#pragma once

#include <cstdint>
#include <string>

namespace firered {

std::string ItemName(std::uint16_t id);
std::string MoveName(std::uint16_t id);
std::string SpeciesName(std::uint16_t id);
std::uint16_t SpeciesNationalDex(std::uint16_t id);
std::string NationalDexSpeciesName(std::uint16_t nationalDex);
std::string FlagName(std::uint16_t id);
std::string VariableName(std::uint16_t id);
std::string GameStatName(std::uint16_t id);
std::string MapName(std::uint8_t group, std::uint8_t number);
std::string HumanizeConstant(const std::string& constant);
const char* PretReferenceCommit();

} // namespace firered
