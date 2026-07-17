#pragma once

#include <cstdint>

#include "red/data/Gen1PokemonData.hpp"

namespace pkmn::cli::red::data {

struct CalculatedStats {
  std::uint16_t maxHp = 0;
  std::uint16_t attack = 0;
  std::uint16_t defense = 0;
  std::uint16_t speed = 0;
  std::uint16_t special = 0;
};

std::uint32_t ExperienceForLevel(GrowthRate growthRate, std::uint8_t level);
std::uint8_t LevelFromExperience(const SpeciesData &species,
                                 std::uint32_t experience);
std::uint8_t MoveMaxPp(std::uint8_t moveId, std::uint8_t ppUps);
std::uint16_t CalculateStat(std::uint8_t base, std::uint8_t dv,
                            std::uint16_t statExperience,
                            std::uint8_t level, bool hp);

} // namespace pkmn::cli::red::data
