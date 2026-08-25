#include "conversion/RouteRegistry.hpp"

#include <stdexcept>

namespace pkmn::cli::conversion {

const std::vector<GameProfile> &GameProfiles() {
  static const std::vector<GameProfile> profiles = {
      {GameId::Red, "GEN1_RED", "Pokemon Red", Generation::Gen1,
       ".red.json"},
      {GameId::Blue, "GEN1_BLUE", "Pokemon Blue", Generation::Gen1,
       ".blue.json"},
      {GameId::FireRed, "GEN3_FIRERED", "Pokemon FireRed", Generation::Gen3,
       ".fred.json"},
      {GameId::LeafGreen, "GEN3_LEAFGREEN", "Pokemon LeafGreen",
       Generation::Gen3, ".lg.json"},
  };
  return profiles;
}

const std::vector<RouteProfile> &Routes() {
  static const std::vector<RouteProfile> routes = {
      {"red-firered", GameId::Red, GameId::FireRed, Capability::Available,
       "EMULATOR_VERIFIED", "PCCS_ORIGINAL_V1"},
      {"red-leafgreen", GameId::Red, GameId::LeafGreen, Capability::Available,
       "STATICALLY_VALIDATED_COMMUNITY_TESTING", "PCCS_ORIGINAL_V1"},
      {"blue-firered", GameId::Blue, GameId::FireRed, Capability::Available,
       "STATICALLY_VALIDATED_COMMUNITY_TESTING", "PCCS_ORIGINAL_V1"},
      {"blue-leafgreen", GameId::Blue, GameId::LeafGreen,
       Capability::Available, "STATICALLY_VALIDATED_COMMUNITY_TESTING", "PCCS_ORIGINAL_V1"},
  };
  return routes;
}

const GameProfile &Profile(GameId id) {
  for (const auto &profile : GameProfiles())
    if (profile.id == id)
      return profile;
  throw std::logic_error("unknown game profile");
}

std::optional<RouteProfile> FindRoute(std::string_view key) {
  // Retain the version 2 spelling as a permanent compatibility alias.
  if (key == "red-to-firered")
    key = "red-firered";
  for (const auto &route : Routes())
    if (route.key == key)
      return route;
  return std::nullopt;
}

std::string_view CapabilityName(Capability capability) {
  return capability == Capability::Available ? "AVAILABLE" : "PLANNED";
}

} // namespace pkmn::cli::conversion
