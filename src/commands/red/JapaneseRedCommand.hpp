#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace pkmn::cli::commands::red {

int RunJapaneseRed(const std::vector<std::string>& arguments,
                   std::ostream& output, std::ostream& error, bool green = false);
int RunJapaneseJson(const std::vector<std::string>& arguments,
                    std::ostream& output, std::ostream& error, bool green = false);

}  // namespace pkmn::cli::commands::red
