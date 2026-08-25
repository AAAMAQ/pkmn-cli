#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace pkmn::cli::commands::paired {

int RunBlue(const std::vector<std::string>& arguments, std::ostream& output,
            std::ostream& error);
int RunBlueJson(const std::vector<std::string>& arguments, std::ostream& output,
                std::ostream& error);
int RunLeafGreen(const std::vector<std::string>& arguments, std::ostream& output,
                 std::ostream& error);
int RunLeafGreenJson(const std::vector<std::string>& arguments,
                     std::ostream& output, std::ostream& error);

}  // namespace pkmn::cli::commands::paired
