#pragma once

#include "FireRedSectionMap.hpp"

#include <filesystem>
#include <string>

namespace firered {

std::string ExportMasterJson(
    const std::filesystem::path& sourcePath,
    std::span<const std::uint8_t> image,
    const SaveAnalysis& analysis);
Bytes ImportPhysicalImage(const std::string& jsonText, std::string& sourceFilename);

} // namespace firered
