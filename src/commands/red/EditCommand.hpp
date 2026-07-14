#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace pkmn::cli::commands::red::edit {

int Run(const std::vector<std::string> &arguments, std::ostream &output,
        std::ostream &error);

} // namespace pkmn::cli::commands::red::edit
