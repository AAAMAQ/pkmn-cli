#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace firered {

using Bytes = std::vector<std::uint8_t>;

struct Layout {
    static constexpr std::size_t kStandardFlashSize = 0x20000;
    static constexpr std::size_t kSectorSize = 0x1000;
    static constexpr std::size_t kSectorDataSize = 0xF80;
    static constexpr std::size_t kSectorCount = 32;
    static constexpr std::size_t kSaveSlotCount = 2;
    static constexpr std::size_t kSectorsPerSlot = 14;
    static constexpr std::size_t kHallOfFameFirstSector = 28;
    static constexpr std::size_t kTrainerTowerFirstSector = 30;

    static constexpr std::size_t kSectionIdOffset = 0xFF4;
    static constexpr std::size_t kChecksumOffset = 0xFF6;
    static constexpr std::size_t kSignatureOffset = 0xFF8;
    static constexpr std::size_t kCounterOffset = 0xFFC;
    static constexpr std::uint32_t kSectorSignature = 0x08012025;

    static constexpr std::array<std::size_t, kSectorsPerSlot> kSectionDataSizes{
        0xF24, // SaveBlock2
        0xF80, 0xF80, 0xF80, 0xEE8, // SaveBlock1 (size 0x3D68)
        0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80,
        0x7D0 // PokemonStorage (size 0x83D0)
    };
};

std::uint16_t ReadLe16(std::span<const std::uint8_t> bytes, std::size_t offset);
std::uint32_t ReadLe32(std::span<const std::uint8_t> bytes, std::size_t offset);
void WriteLe16(std::span<std::uint8_t> bytes, std::size_t offset, std::uint16_t value);
void WriteLe32(std::span<std::uint8_t> bytes, std::size_t offset, std::uint32_t value);
std::span<const std::uint8_t> StandardFlash(std::span<const std::uint8_t> image);
std::string HexOffset(std::size_t value);

} // namespace firered
