#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace pkmn::cli::red::save {

// Adapted from the MIT-licensed Pkmn Red Save Genie SaveStructure/FileManipulation
// modules, copyright 2026 MAQ / BiG MAQ Studios.
class RedSave {
public:
    using Byte = std::uint8_t;
    using Bytes = std::vector<Byte>;

    static constexpr std::size_t ExpectedSize = 0x8000;

    explicit RedSave(Bytes bytes);
    static RedSave Read(const std::filesystem::path& path);

    [[nodiscard]] std::size_t Size() const noexcept { return bytes_.size(); }
    [[nodiscard]] Byte At(std::size_t offset) const;
    [[nodiscard]] Bytes Slice(std::size_t offset, std::size_t length) const;
    [[nodiscard]] const Bytes& BytesView() const noexcept { return bytes_; }

private:
    Bytes bytes_;
};

}  // namespace pkmn::cli::red::save
