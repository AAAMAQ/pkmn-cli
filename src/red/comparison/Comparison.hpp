#pragma once

#include "red/json/RedDecoder.hpp"
#include "red/save/RedSave.hpp"

namespace pkmn::cli::red::comparison {

struct SemanticOptions {
  bool postEmulator = false;
};

json::OrderedJson ComparePhysical(const save::RedSave::Bytes &a,
                                  const save::RedSave::Bytes &b);
json::OrderedJson CompareSemantic(const json::OrderedJson &a,
                                  const json::OrderedJson &b,
                                  const SemanticOptions &options = {});
std::string PhysicalMarkdown(const json::OrderedJson &report);
std::string SemanticMarkdown(const json::OrderedJson &report);
} // namespace pkmn::cli::red::comparison
