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
void AddNamedEventEdit(Json &session, const std::string &name, bool value);
void UndoLast(Json &session, std::size_t count = 1);
void AddAnnotation(Json &session, const std::string &note);
void ApplySafeLocationPreset(Json &session, const std::string &preset);
generation::Result Validate(const Json &session);
std::filesystem::path DefaultSessionPath(const std::filesystem::path &source);
std::filesystem::path DefaultOutputPath(const Json &session);

} // namespace pkmn::cli::red::editing
