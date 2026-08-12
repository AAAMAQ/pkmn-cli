#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace pkmn::cli::commands::conversion {
int RunRedConvert(const std::vector<std::string> &arguments,
                  std::ostream &output, std::ostream &error);
int RunRjsonExtension(const std::vector<std::string> &arguments,
                      std::ostream &output, std::ostream &error);
int RunFrjson(const std::vector<std::string> &arguments,
              std::ostream &output, std::ostream &error);
int RunConvert(const std::vector<std::string> &arguments,
               std::ostream &output, std::ostream &error);
int RunRuntimeUtility(const std::vector<std::string> &arguments,
                      std::ostream &output, std::ostream &error);
} // namespace pkmn::cli::commands::conversion
