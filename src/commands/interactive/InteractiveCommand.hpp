#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace pkmn::cli::commands::interactive {

int Run(const std::vector<std::string> &arguments, std::istream &input,
        std::ostream &output, std::ostream &error);

} // namespace pkmn::cli::commands::interactive
