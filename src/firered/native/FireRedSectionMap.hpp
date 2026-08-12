#pragma once

#include "FireRedSaveStructure.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace firered {

struct SectionInfo {
    std::size_t physicalSector{};
    std::size_t fileOffset{};
    std::uint16_t id{};
    std::uint16_t storedChecksum{};
    std::uint16_t calculatedChecksum{};
    std::uint32_t signature{};
    std::uint32_t counter{};
    std::size_t checksumDataSize{};
    bool idValid{};
    bool signatureValid{};
    bool checksumValid{};
};

struct SlotInfo {
    std::size_t slotIndex{};
    std::vector<SectionInfo> sections;
    std::uint32_t counter{};
    bool hasAnySignature{};
    bool allSectionIdsPresent{};
    bool countersConsistent{};
    bool valid{};
    std::vector<std::string> errors;
};

struct SaveAnalysis {
    std::array<SlotInfo, Layout::kSaveSlotCount> slots;
    std::optional<std::size_t> activeSlot;
    bool activeSlotAmbiguous{};
    bool standardFlashPresent{};
    bool atLeastOneValidSlot{};
    std::vector<std::string> diagnostics;
};

SaveAnalysis AnalyzeSave(std::span<const std::uint8_t> image);
std::string SectionRole(std::uint16_t id);

} // namespace firered
