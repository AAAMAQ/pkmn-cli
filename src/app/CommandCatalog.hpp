#pragma once

#include <span>
#include <string_view>

namespace pkmn::cli {

struct CommandSpec {
  std::string_view category;
  std::string_view path;
  std::string_view usage;
  std::string_view description;
};

std::span<const CommandSpec> AvailableCommands();

} // namespace pkmn::cli
