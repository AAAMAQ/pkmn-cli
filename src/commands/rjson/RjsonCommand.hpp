#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace pkmn::cli::commands::rjson {
int Run(const std::vector<std::string>& arguments, std::ostream& output, std::ostream& error);
}  // namespace pkmn::cli::commands::rjson
