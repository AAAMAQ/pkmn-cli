#pragma once
#include <filesystem>
namespace pkmn::cli::util {
std::filesystem::path RedTemplatePath();
std::filesystem::path FireRedTemplatePath();
std::filesystem::path BundledRuntimeExecutablePath();
std::filesystem::path FireRedRuntimeScriptPath();
std::filesystem::path RuntimeDataPath();
bool UsesBundledRuntime();
} // namespace pkmn::cli::util
