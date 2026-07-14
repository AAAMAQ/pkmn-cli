#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace pkmn::cli {

class CommandRouter {
public:
    int Run(const std::vector<std::string>& arguments,
            std::ostream& output,
            std::ostream& error) const;

    static void PrintHelp(std::ostream& output);
    static void PrintVersion(std::ostream& output);

private:
    static int RunPlannedDomain(const std::vector<std::string>& arguments,
                                std::ostream& output,
                                std::ostream& error);
};

}  // namespace pkmn::cli

