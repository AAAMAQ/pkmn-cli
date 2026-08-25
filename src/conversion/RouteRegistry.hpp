#pragma once

#include <optional>
#include <string_view>
#include <vector>

namespace pkmn::cli::conversion {

enum class Generation { Gen1, Gen3 };
enum class GameId { Red, Blue, FireRed, LeafGreen };
enum class Capability { Available, Planned };

struct GameProfile {
  GameId id;
  std::string_view key;
  std::string_view displayName;
  Generation generation;
  std::string_view jsonSuffix;
};

struct RouteProfile {
  std::string_view key;
  GameId source;
  GameId target;
  Capability capability;
  std::string_view evidence;
  std::string_view conversionPolicy;
};

[[nodiscard]] const std::vector<GameProfile> &GameProfiles();
[[nodiscard]] const std::vector<RouteProfile> &Routes();
[[nodiscard]] const GameProfile &Profile(GameId id);
[[nodiscard]] std::optional<RouteProfile> FindRoute(std::string_view key);
[[nodiscard]] std::string_view CapabilityName(Capability capability);

} // namespace pkmn::cli::conversion
