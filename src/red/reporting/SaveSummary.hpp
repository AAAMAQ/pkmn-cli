#pragma once

#include <string>

#include "red/json/RedDecoder.hpp"

namespace pkmn::cli::red::reporting {

json::OrderedJson BuildSaveSummary(const json::OrderedJson &document);
std::string SaveSummaryText(const json::OrderedJson &summary);
std::string SaveSummaryMarkdown(const json::OrderedJson &summary);

} // namespace pkmn::cli::red::reporting
