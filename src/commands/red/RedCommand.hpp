#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace pkmn::cli::commands::red {
int Run(const std::vector<std::string>& arguments, std::ostream& output, std::ostream& error);
}
