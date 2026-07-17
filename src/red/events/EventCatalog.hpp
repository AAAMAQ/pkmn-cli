#pragma once

#include <span>
#include <string>
#include <string_view>

#include "red/json/RedDecoder.hpp"

namespace pkmn::cli::red::events {

struct EventDefinition {
  int flagIndex;
  std::string_view name;
};

std::span<const EventDefinition> Catalog();
bool IsTrainerBattle(std::string_view name);
bool IsStaticEncounter(std::string_view name);
bool IsMajorStory(std::string_view name);
std::string Category(std::string_view name);
std::string Label(std::string_view name);

json::OrderedJson DecodeNamedState(std::span<const std::uint8_t> eventBytes);
void ValidateNamedState(const json::OrderedJson &decoded,
                        std::vector<std::string> &errors);
void ApplyNamedState(const json::OrderedJson &decoded,
                     std::vector<std::uint8_t> &eventBytes);

} // namespace pkmn::cli::red::events
