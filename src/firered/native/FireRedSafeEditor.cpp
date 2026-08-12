#include "FireRedSafeEditor.hpp"

#include "FireRedReadOnlyData.hpp"
#include "FireRedSaveStructure.hpp"
#include "FireRedTextCodec.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace firered {

FireRedSafeEditor::FireRedSafeEditor(Bytes sourceImage, SaveAnalysis sourceAnalysis)
    : sourceImage_(std::move(sourceImage)),
      sourceAnalysis_(std::move(sourceAnalysis)),
      logical_(AssembleLogicalSave(sourceImage_, sourceAnalysis_)) {}

void FireRedSafeEditor::Record(std::string field, std::string before, std::string after) {
    changes_.push_back({std::move(field), std::move(before), std::move(after)});
}

void FireRedSafeEditor::SetPlayerName(const std::string& name) {
    const auto before = DecodeFireRedText(
        std::span<const std::uint8_t>(logical_.saveBlock2).subspan(0, 8));
    const auto encoded = EncodeFireRedEnglishName(name, 8);
    std::copy(encoded.begin(), encoded.end(), logical_.saveBlock2.begin());
    Record("playerName", before, name);
}

void FireRedSafeEditor::SetRivalName(const std::string& name) {
    const auto before = DecodeFireRedText(
        std::span<const std::uint8_t>(logical_.saveBlock1).subspan(0x3A4C, 8));
    const auto encoded = EncodeFireRedEnglishName(name, 8);
    std::copy(encoded.begin(), encoded.end(), logical_.saveBlock1.begin() + 0x3A4C);
    Record("rivalName", before, name);
}

void FireRedSafeEditor::SetMoney(std::uint32_t money) {
    if (money > 999999) throw std::invalid_argument("money must be 0-999999");
    const auto key = ReadLe32(logical_.saveBlock2, 0xF20);
    const auto before = ReadLe32(logical_.saveBlock1, 0x290) ^ key;
    WriteLe32(logical_.saveBlock1, 0x290, money ^ key);
    Record("money", std::to_string(before), std::to_string(money));
}

void FireRedSafeEditor::SetCoins(std::uint16_t coins) {
    if (coins > 9999) throw std::invalid_argument("coins must be 0-9999");
    const auto key = static_cast<std::uint16_t>(ReadLe32(logical_.saveBlock2, 0xF20));
    const auto before = static_cast<std::uint16_t>(ReadLe16(logical_.saveBlock1, 0x294) ^ key);
    WriteLe16(logical_.saveBlock1, 0x294, static_cast<std::uint16_t>(coins ^ key));
    Record("coins", std::to_string(before), std::to_string(coins));
}

void FireRedSafeEditor::SetBadge(std::size_t badgeNumber, bool obtained) {
    if (badgeNumber < 1 || badgeNumber > 8) {
        throw std::invalid_argument("badge number must be 1-8");
    }
    constexpr std::uint16_t kFirstBadgeFlag = 0x820;
    constexpr std::size_t kFlagsOffset = 0xEE0;
    const auto flag = static_cast<std::uint16_t>(kFirstBadgeFlag + badgeNumber - 1);
    const auto offset = kFlagsOffset + flag / 8;
    const auto mask = static_cast<std::uint8_t>(1U << (flag % 8));
    const bool before = (logical_.saveBlock1[offset] & mask) != 0;
    if (obtained) {
        logical_.saveBlock1[offset] |= mask;
    } else {
        logical_.saveBlock1[offset] &= static_cast<std::uint8_t>(~mask);
    }
    static constexpr std::array<const char*, 8> kBadgeNames{
        "Boulder", "Cascade", "Thunder", "Rainbow",
        "Soul", "Marsh", "Volcano", "Earth"
    };
    Record(
        "badges." + std::string(kBadgeNames[badgeNumber - 1]),
        before ? "obtained" : "not obtained",
        obtained ? "obtained" : "not obtained");
}

void FireRedSafeEditor::SetPartyPokemonNickname(
    std::size_t partySlot, const std::string& name) {
    const auto partyCount = logical_.saveBlock1.at(0x34);
    if (partySlot < 1 || partySlot > partyCount || partySlot > 6)
        throw std::invalid_argument("party slot is not occupied");
    const auto offset = 0x38 + (partySlot - 1) * 100 + 8;
    const auto before = DecodeFireRedText(
        std::span<const std::uint8_t>(logical_.saveBlock1).subspan(offset, 10));
    const auto encoded = EncodeFireRedEnglishName(name, 10);
    std::copy(encoded.begin(), encoded.end(), logical_.saveBlock1.begin() + offset);
    Record("party[" + std::to_string(partySlot) + "].nickname", before, name);
}

FireRedSafeEditor::PocketRegion FireRedSafeEditor::RegionFor(const std::string& pocket) {
    if (pocket == "PC Items") return {0x298, 30, false};
    if (pocket == "Items") return {0x310, 42, true};
    if (pocket == "Key Items") return {0x3B8, 30, true};
    if (pocket == "Poké Balls") return {0x430, 13, true};
    if (pocket == "TM Case") return {0x464, 58, true};
    if (pocket == "Berry Pouch") return {0x54C, 43, true};
    throw std::invalid_argument("unsupported item pocket");
}

void FireRedSafeEditor::SetExistingItemQuantity(
    const std::string& pocket,
    std::size_t slot,
    std::uint16_t expectedItemId,
    std::uint16_t quantity) {
    if (quantity == 0 || quantity > 999) {
        throw std::invalid_argument("existing item quantity must be 1-999");
    }
    const auto region = RegionFor(pocket);
    if (slot >= region.capacity) throw std::invalid_argument("item slot is outside pocket capacity");
    const auto offset = region.offset + slot * 4;
    const auto actualId = ReadLe16(logical_.saveBlock1, offset);
    if (actualId == 0 || actualId != expectedItemId) {
        throw std::invalid_argument("item identity changed; refusing stale or empty slot edit");
    }
    const auto key = region.encrypted
        ? static_cast<std::uint16_t>(ReadLe32(logical_.saveBlock2, 0xF20)) : 0;
    const auto before = static_cast<std::uint16_t>(
        ReadLe16(logical_.saveBlock1, offset + 2) ^ key);
    WriteLe16(
        logical_.saveBlock1, offset + 2,
        static_cast<std::uint16_t>(quantity ^ key));
    Record(
        pocket + "[" + std::to_string(slot) + "].quantity",
        std::to_string(before), std::to_string(quantity));
}

Bytes FireRedSafeEditor::Finish() {
    if (changes_.empty()) throw std::runtime_error("no edits were requested");
    auto output = sourceImage_;
    ScatterLogicalSave(output, sourceAnalysis_, logical_);
    const auto validation = AnalyzeSave(output);
    if (!validation.activeSlot || validation.activeSlotAmbiguous
        || validation.activeSlot != sourceAnalysis_.activeSlot
        || !validation.slots[*validation.activeSlot].valid) {
        throw std::runtime_error("edited copy failed post-write FireRed validation");
    }
    return output;
}

const std::vector<EditChange>& FireRedSafeEditor::Changes() const {
    return changes_;
}

std::string FireRedSafeEditor::Report() const {
    std::string report = "# FireRed Safe Edit Report\n\n";
    report += "The source save was not overwritten. Only the active slot was changed.\n\n";
    for (const auto& change : changes_) {
        report += "- `" + change.field + "`: `" + change.before + "` → `"
            + change.after + "`\n";
    }
    return report;
}

} // namespace firered
