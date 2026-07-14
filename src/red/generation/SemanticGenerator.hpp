#pragma once

#include <string>
#include <vector>

#include "red/json/RedDecoder.hpp"
#include "red/save/RedSave.hpp"

namespace pkmn::cli::red::generation {

struct Result {
  save::RedSave::Bytes bytes;
  json::OrderedJson report;
  std::string markdownReport;
};

Result Generate(const json::OrderedJson &document);
void RepairChecksums(save::RedSave::Bytes &bytes);

} // namespace pkmn::cli::red::generation
