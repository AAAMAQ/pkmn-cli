#pragma once

#include "FireRedSaveStructure.hpp"
#include "FireRedSectionMap.hpp"

#include <cstddef>

namespace firered {

struct LogicalSave {
    Bytes saveBlock2;
    Bytes saveBlock1;
    Bytes pokemonStorage;
};

LogicalSave AssembleLogicalSave(
    std::span<const std::uint8_t> image,
    const SaveAnalysis& analysis);
void ScatterLogicalSave(
    Bytes& image,
    const SaveAnalysis& analysis,
    const LogicalSave& logical);

} // namespace firered
