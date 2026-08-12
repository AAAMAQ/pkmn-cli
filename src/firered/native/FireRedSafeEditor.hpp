#pragma once

#include "FireRedLogicalSave.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace firered {

struct EditChange {
    std::string field;
    std::string before;
    std::string after;
};

class FireRedSafeEditor {
public:
    FireRedSafeEditor(Bytes sourceImage, SaveAnalysis sourceAnalysis);

    void SetPlayerName(const std::string& name);
    void SetRivalName(const std::string& name);
    void SetMoney(std::uint32_t money);
    void SetCoins(std::uint16_t coins);
    void SetBadge(std::size_t badgeNumber, bool obtained);
    void SetPartyPokemonNickname(std::size_t partySlot, const std::string& name);
    void SetExistingItemQuantity(
        const std::string& pocket,
        std::size_t slot,
        std::uint16_t expectedItemId,
        std::uint16_t quantity);

    Bytes Finish();
    const std::vector<EditChange>& Changes() const;
    std::string Report() const;

private:
    struct PocketRegion {
        std::size_t offset{};
        std::size_t capacity{};
        bool encrypted{};
    };

    static PocketRegion RegionFor(const std::string& pocket);
    void Record(std::string field, std::string before, std::string after);

    Bytes sourceImage_;
    SaveAnalysis sourceAnalysis_;
    LogicalSave logical_;
    std::vector<EditChange> changes_;
};

} // namespace firered
