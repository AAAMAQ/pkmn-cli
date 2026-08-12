#include "FireRedLogicalSave.hpp"

#include "FireRedChecksum.hpp"

#include <algorithm>
#include <stdexcept>

namespace firered {

namespace {
const SectionInfo& FindSection(const SlotInfo& slot, std::uint16_t id) {
    const auto found = std::find_if(
        slot.sections.begin(), slot.sections.end(),
        [id](const SectionInfo& section) {
            return section.id == id && section.signatureValid && section.checksumValid;
        });
    if (found == slot.sections.end()) {
        throw std::runtime_error("active slot is missing logical section " + std::to_string(id));
    }
    return *found;
}

Bytes AssembleRange(
    std::span<const std::uint8_t> image,
    const SlotInfo& slot,
    std::uint16_t firstId,
    std::uint16_t lastId) {
    Bytes output;
    for (std::uint16_t id = firstId; id <= lastId; ++id) {
        const auto& section = FindSection(slot, id);
        const auto begin = image.begin() + static_cast<std::ptrdiff_t>(section.fileOffset);
        output.insert(
            output.end(), begin,
            begin + static_cast<std::ptrdiff_t>(Layout::kSectionDataSizes[id]));
    }
    return output;
}

void ScatterRange(
    Bytes& image,
    const SlotInfo& slot,
    std::uint16_t firstId,
    std::uint16_t lastId,
    std::span<const std::uint8_t> logical) {
    std::size_t logicalOffset = 0;
    for (std::uint16_t id = firstId; id <= lastId; ++id) {
        const auto& section = FindSection(slot, id);
        const auto size = Layout::kSectionDataSizes[id];
        if (logicalOffset + size > logical.size()) {
            throw std::runtime_error("logical save block is shorter than its verified layout");
        }
        auto sector = std::span<std::uint8_t>(image).subspan(
            section.fileOffset, Layout::kSectorSize);
        std::copy_n(logical.begin() + static_cast<std::ptrdiff_t>(logicalOffset),
                    size, sector.begin());
        WriteLe16(
            sector, Layout::kChecksumOffset,
            CalculateSectionChecksum(sector.first(size)));
        logicalOffset += size;
    }
    if (logicalOffset != logical.size()) {
        throw std::runtime_error("logical save block size does not match verified layout");
    }
}
} // namespace

LogicalSave AssembleLogicalSave(
    std::span<const std::uint8_t> image,
    const SaveAnalysis& analysis) {
    if (!analysis.activeSlot || analysis.activeSlotAmbiguous) {
        throw std::runtime_error("cannot assemble logical data without an unambiguous active slot");
    }
    const auto& slot = analysis.slots[*analysis.activeSlot];
    return {
        AssembleRange(image, slot, 0, 0),
        AssembleRange(image, slot, 1, 4),
        AssembleRange(image, slot, 5, 13)
    };
}

void ScatterLogicalSave(
    Bytes& image,
    const SaveAnalysis& analysis,
    const LogicalSave& logical) {
    if (image.size() < Layout::kStandardFlashSize || !analysis.activeSlot) {
        throw std::runtime_error("cannot scatter logical data into an invalid save");
    }
    const auto& slot = analysis.slots[*analysis.activeSlot];
    ScatterRange(image, slot, 0, 0, logical.saveBlock2);
    ScatterRange(image, slot, 1, 4, logical.saveBlock1);
    ScatterRange(image, slot, 5, 13, logical.pokemonStorage);
}

} // namespace firered
