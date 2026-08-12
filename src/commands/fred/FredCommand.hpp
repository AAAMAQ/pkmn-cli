#pragma once
#include <ostream>
#include <string>
#include <vector>
namespace pkmn::cli::commands::fred {
int Run(const std::vector<std::string> &arguments, std::ostream &output,
        std::ostream &error);
}
