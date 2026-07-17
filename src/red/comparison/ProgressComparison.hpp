#pragma once

#include <string>

#include "red/json/RedDecoder.hpp"

namespace pkmn::cli::red::comparison {

json::OrderedJson CompareProgress(const json::OrderedJson &older,
                                  const json::OrderedJson &newer);
std::string ProgressMarkdown(const json::OrderedJson &report);

} // namespace pkmn::cli::red::comparison
