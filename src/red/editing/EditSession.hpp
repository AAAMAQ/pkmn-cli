#pragma once

#include <filesystem>
#include <string>

#include "red/generation/SemanticGenerator.hpp"
#include "red/json/RedDecoder.hpp"

namespace pkmn::cli::red::editing {

using Json = json::OrderedJson;

Json Begin(const std::filesystem::path &source);
Json Load(const std::filesystem::path &sessionPath);
void Save(const std::filesystem::path &sessionPath, const Json &session,
          bool refuseCollision);
void AddEdit(Json &session, const std::string &pointer, const Json &value);
generation::Result Validate(const Json &session);
std::filesystem::path DefaultSessionPath(const std::filesystem::path &source);
std::filesystem::path DefaultOutputPath(const Json &session);

} // namespace pkmn::cli::red::editing
