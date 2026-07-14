#include "red/save/RedSave.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

namespace pkmn::cli::red::save {

RedSave::RedSave(Bytes bytes) : bytes_(std::move(bytes)) {}

RedSave RedSave::Read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("could not open input save");
    const std::streamsize size = input.tellg();
    if (size < 0) throw std::runtime_error("could not determine input save size");
    Bytes bytes(static_cast<std::size_t>(size));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), size);
        if (!input) throw std::runtime_error("could not read complete input save");
    }
    return RedSave(std::move(bytes));
}

RedSave::Byte RedSave::At(std::size_t offset) const {
    if (offset >= bytes_.size()) throw std::out_of_range("Red save byte offset is out of range");
    return bytes_[offset];
}

RedSave::Bytes RedSave::Slice(std::size_t offset, std::size_t length) const {
    if (offset > bytes_.size() || length > bytes_.size() - offset) {
        throw std::out_of_range("Red save byte range is out of bounds");
    }
    return Bytes(bytes_.begin() + static_cast<std::ptrdiff_t>(offset),
                 bytes_.begin() + static_cast<std::ptrdiff_t>(offset + length));
}

}  // namespace pkmn::cli::red::save
