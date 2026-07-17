#pragma once
#include <iosfwd>
#include <string>
#include <vector>
namespace pkmn::cli::commands::proof {
int Run(const std::vector<std::string> &, std::ostream &, std::ostream &);
int RunPostEmulator(const std::vector<std::string> &, std::ostream &,
                    std::ostream &);
}
