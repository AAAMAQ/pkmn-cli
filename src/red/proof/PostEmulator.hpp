#pragma once

#include <filesystem>
#include <string>

#include "red/json/RedDecoder.hpp"

namespace pkmn::cli::red::proof {

struct PostEmulatorResult {
  json::OrderedJson report;
  std::string markdown;
  bool passed = false;
};

PostEmulatorResult ValidatePostEmulator(
    const std::filesystem::path &beforePath,
    const std::filesystem::path &afterPath);

} // namespace pkmn::cli::red::proof
