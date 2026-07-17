#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace pkmn::cli::util {

using ZipEntry = std::pair<std::string, std::vector<std::uint8_t>>;

std::vector<ZipEntry>
ReadProofDirectoryEntries(const std::filesystem::path &directory);
void WriteDeterministicZip(const std::filesystem::path &output,
                           const std::vector<ZipEntry> &entries);

} // namespace pkmn::cli::util
