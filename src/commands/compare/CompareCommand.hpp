#pragma once
#include <iosfwd>
#include <string>
#include <vector>
namespace pkmn::cli::commands::compare {
int Run(const std::vector<std::string> &, std::ostream &, std::ostream &);
}
