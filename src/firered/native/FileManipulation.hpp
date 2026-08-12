#pragma once

#include "FireRedSaveStructure.hpp"

#include <filesystem>

namespace firered {

Bytes ReadBinaryFile(const std::filesystem::path& path);
void WriteNewBinaryFile(const std::filesystem::path& path, std::span<const std::uint8_t> bytes);
void WriteNewTextFile(const std::filesystem::path& path, const std::string& text);
std::string ReadTextFile(const std::filesystem::path& path);
std::filesystem::path DefaultJsonPath(const std::filesystem::path& savePath);
std::filesystem::path DefaultReconstructedPath(
    const std::filesystem::path& jsonPath,
    const std::string& sourceFilename);
std::filesystem::path CollisionSafePath(const std::filesystem::path& preferred);

} // namespace firered
