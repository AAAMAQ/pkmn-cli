#include "FireRedSectionMap.hpp"

#include "FireRedChecksum.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <set>

namespace firered {

namespace {
SlotInfo AnalyzeSlot(std::span<const std::uint8_t> flash, std::size_t slotIndex) {
    SlotInfo slot;
    slot.slotIndex = slotIndex;
    std::set<std::uint16_t> validIds;
    std::optional<std::uint32_t> firstCounter;

    for (std::size_t localSector = 0; localSector < Layout::kSectorsPerSlot; ++localSector) {
        const std::size_t physicalSector = slotIndex * Layout::kSectorsPerSlot + localSector;
        const std::size_t base = physicalSector * Layout::kSectorSize;
        const auto sector = flash.subspan(base, Layout::kSectorSize);

        SectionInfo info;
        info.physicalSector = physicalSector;
        info.fileOffset = base;
        info.id = ReadLe16(sector, Layout::kSectionIdOffset);
        info.storedChecksum = ReadLe16(sector, Layout::kChecksumOffset);
        info.signature = ReadLe32(sector, Layout::kSignatureOffset);
        info.counter = ReadLe32(sector, Layout::kCounterOffset);
        info.idValid = info.id < Layout::kSectorsPerSlot;
        info.signatureValid = info.signature == Layout::kSectorSignature;
        slot.hasAnySignature = slot.hasAnySignature || info.signatureValid;

        if (info.idValid) {
            info.checksumDataSize = Layout::kSectionDataSizes[info.id];
            info.calculatedChecksum =
                CalculateSectionChecksum(sector.first(info.checksumDataSize));
            info.checksumValid = info.storedChecksum == info.calculatedChecksum;
            if (info.signatureValid && info.checksumValid) {
                validIds.insert(info.id);
            }
        }

        if (info.signatureValid && info.idValid && info.checksumValid) {
            if (!firstCounter) {
                firstCounter = info.counter;
            }
        }
        slot.sections.push_back(info);
    }

    slot.counter = firstCounter.value_or(0);
    slot.countersConsistent = firstCounter.has_value()
        && std::all_of(slot.sections.begin(), slot.sections.end(), [&](const SectionInfo& section) {
            return !section.signatureValid
                || !section.idValid
                || !section.checksumValid
                || section.counter == *firstCounter;
        });
    slot.allSectionIdsPresent = validIds.size() == Layout::kSectorsPerSlot;
    slot.valid = slot.allSectionIdsPresent && slot.countersConsistent;

    if (!slot.hasAnySignature) {
        slot.errors.emplace_back("slot contains no recognized FireRed section signatures");
    }
    if (!slot.allSectionIdsPresent) {
        slot.errors.emplace_back("slot does not contain one valid copy of every section ID 0-13");
    }
    if (!slot.countersConsistent) {
        slot.errors.emplace_back("valid sections do not share one save counter");
    }
    return slot;
}

std::optional<std::size_t> SelectNewerSlot(
    const SlotInfo& first,
    const SlotInfo& second,
    bool& ambiguous) {
    ambiguous = false;
    if (first.valid && !second.valid) {
        return 0;
    }
    if (!first.valid && second.valid) {
        return 1;
    }
    if (!first.valid && !second.valid) {
        return std::nullopt;
    }
    if (first.counter == second.counter) {
        ambiguous = true;
        return std::nullopt;
    }

    const auto max = std::numeric_limits<std::uint32_t>::max();
    if (first.counter == max && second.counter == 0) {
        return 1;
    }
    if (second.counter == max && first.counter == 0) {
        return 0;
    }
    return first.counter > second.counter ? std::optional<std::size_t>(0)
                                          : std::optional<std::size_t>(1);
}
} // namespace

SaveAnalysis AnalyzeSave(std::span<const std::uint8_t> image) {
    SaveAnalysis analysis;
    if (image.size() < Layout::kStandardFlashSize) {
        analysis.diagnostics.emplace_back(
            "input is smaller than the standard 128 KiB FireRed flash image");
        return analysis;
    }

    analysis.standardFlashPresent = true;
    const auto flash = image.first(Layout::kStandardFlashSize);
    analysis.slots[0] = AnalyzeSlot(flash, 0);
    analysis.slots[1] = AnalyzeSlot(flash, 1);
    analysis.atLeastOneValidSlot = analysis.slots[0].valid || analysis.slots[1].valid;
    analysis.activeSlot =
        SelectNewerSlot(analysis.slots[0], analysis.slots[1], analysis.activeSlotAmbiguous);

    if (analysis.activeSlotAmbiguous) {
        analysis.diagnostics.emplace_back(
            "both slots are valid with equal counters; active slot is intentionally unresolved");
    } else if (!analysis.activeSlot) {
        analysis.diagnostics.emplace_back("no complete valid FireRed save slot was found");
    }
    if (image.size() > Layout::kStandardFlashSize) {
        analysis.diagnostics.emplace_back(
            "bytes beyond the standard 128 KiB image are preserved as trailing data");
    }
    return analysis;
}

std::string SectionRole(std::uint16_t id) {
    if (id == 0) return "saveBlock2";
    if (id >= 1 && id <= 4) return "saveBlock1";
    if (id >= 5 && id <= 13) return "pokemonStorage";
    return "invalid";
}

} // namespace firered
