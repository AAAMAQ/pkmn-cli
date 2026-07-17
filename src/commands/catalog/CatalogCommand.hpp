#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace pkmn::cli::commands::catalog {

int Run(const std::vector<std::string> &arguments, std::ostream &output,
        std::ostream &error);

} // namespace pkmn::cli::commands::catalog
