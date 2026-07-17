#include "red/data/Gen1PokemonMath.hpp"

#include <algorithm>
#include <stdexcept>

namespace pkmn::cli::red::data {
namespace {

std::uint32_t CeilSqrt(std::uint16_t value) {
  std::uint32_t root = 0;
  while ((root + 1U) * (root + 1U) <= value)
    ++root;
  return root * root == value ? root : root + 1U;
}

} // namespace

std::uint32_t ExperienceForLevel(GrowthRate growthRate, std::uint8_t level) {
  if (level == 0 || level > 100)
    throw std::runtime_error("Pokemon level must be in 1..100");
  const std::int64_t n = level;
  const std::int64_t cube = n * n * n;
  const std::int64_t square = n * n;
  std::int64_t result = 0;
  switch (growthRate) {
  case GrowthRate::MediumFast:
    result = cube;
    break;
  case GrowthRate::SlightlyFast:
    result = ((3 * cube) / 4) + (10 * square) - 30;
    break;
  case GrowthRate::SlightlySlow:
    result = ((3 * cube) / 4) + (20 * square) - 70;
    break;
  case GrowthRate::MediumSlow:
    result = ((6 * cube) / 5) - (15 * square) + (100 * n) - 140;
    break;
  case GrowthRate::Fast:
    result = (4 * cube) / 5;
    break;
  case GrowthRate::Slow:
    result = (5 * cube) / 4;
    break;
  }
  return static_cast<std::uint32_t>(std::max<std::int64_t>(0, result));
}

std::uint8_t LevelFromExperience(const SpeciesData &species,
                                 std::uint32_t experience) {
  std::uint8_t level = 1;
  for (std::uint16_t candidate = 2; candidate <= 100; ++candidate) {
    if (ExperienceForLevel(species.growthRate,
                           static_cast<std::uint8_t>(candidate)) > experience)
      break;
    level = static_cast<std::uint8_t>(candidate);
  }
  return level;
}

std::uint8_t MoveMaxPp(std::uint8_t moveId, std::uint8_t ppUps) {
  if (ppUps > 3)
    throw std::runtime_error("move PP Ups must be in 0..3");
  if (moveId == 0)
    return 0;
  const auto *move = FindMoveData(moveId);
  if (move == nullptr)
    throw std::runtime_error("unsupported Gen I move ID");
  return static_cast<std::uint8_t>(move->basePp +
                                   (move->basePp / 5U) * ppUps);
}

std::uint16_t CalculateStat(std::uint8_t base, std::uint8_t dv,
                            std::uint16_t statExperience,
                            std::uint8_t level, bool hp) {
  const auto baseAndDv = static_cast<std::uint32_t>((base + dv) * 2U);
  const auto statExperienceTerm = CeilSqrt(statExperience) / 4U;
  const auto scaled = ((baseAndDv + statExperienceTerm) * level) / 100U;
  const auto total = hp ? scaled + level + 10U : scaled + 5U;
  return static_cast<std::uint16_t>(std::min<std::uint32_t>(total, 999U));
}

} // namespace pkmn::cli::red::data
