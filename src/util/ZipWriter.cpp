#include "util/ZipWriter.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace pkmn::cli::util {
namespace {

std::uint32_t Crc32(const std::vector<std::uint8_t> &bytes) {
  std::uint32_t crc = 0xFFFFFFFFU;
  for (const auto byte : bytes) {
    crc ^= byte;
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc >> 1U) ^
            (0xEDB88320U & static_cast<std::uint32_t>(-(crc & 1U)));
  }
  return ~crc;
}

void U16(std::ostream &output, std::uint16_t value) {
  output.put(static_cast<char>(value & 0xFFU));
  output.put(static_cast<char>((value >> 8U) & 0xFFU));
}

void U32(std::ostream &output, std::uint32_t value) {
  U16(output, static_cast<std::uint16_t>(value & 0xFFFFU));
  U16(output, static_cast<std::uint16_t>((value >> 16U) & 0xFFFFU));
}

struct CentralEntry {
  std::string name;
  std::uint32_t crc = 0;
  std::uint32_t size = 0;
  std::uint32_t offset = 0;
};

} // namespace

std::vector<ZipEntry>
ReadProofDirectoryEntries(const std::filesystem::path &directory) {
  std::vector<std::filesystem::path> paths;
  for (const auto &item : std::filesystem::directory_iterator(directory)) {
    if (!item.is_regular_file() || item.path().extension() == ".zip")
      continue;
    paths.push_back(item.path());
  }
  std::sort(paths.begin(), paths.end(), [](const auto &left, const auto &right) {
    return left.filename().generic_string() < right.filename().generic_string();
  });
  std::vector<ZipEntry> entries;
  for (const auto &path : paths) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input)
      throw std::runtime_error("could not read proof artifact for ZIP");
    const auto end = input.tellg();
    if (end < 0 || static_cast<std::uint64_t>(end) > 0xFFFFFFFFULL)
      throw std::runtime_error("proof artifact exceeds ZIP32 limits");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    input.seekg(0);
    input.read(reinterpret_cast<char *>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!input && !bytes.empty())
      throw std::runtime_error("could not read complete proof artifact");
    entries.emplace_back(path.filename().generic_string(), std::move(bytes));
  }
  return entries;
}

void WriteDeterministicZip(const std::filesystem::path &output,
                           const std::vector<ZipEntry> &entries) {
  if (entries.size() > 0xFFFFU)
    throw std::runtime_error("proof package exceeds ZIP32 entry limit");
  std::ofstream zip(output, std::ios::binary | std::ios::trunc);
  if (!zip)
    throw std::runtime_error("could not create proof ZIP");
  std::vector<CentralEntry> central;
  for (const auto &[name, bytes] : entries) {
    if (name.empty() || name.starts_with("/") || name.find("..") != std::string::npos ||
        name.find('\\') != std::string::npos || name.size() > 0xFFFFU ||
        bytes.size() > 0xFFFFFFFFULL)
      throw std::runtime_error("unsafe or unsupported proof ZIP entry");
    const auto offset = zip.tellp();
    if (offset < 0 || static_cast<std::uint64_t>(offset) > 0xFFFFFFFFULL)
      throw std::runtime_error("proof ZIP exceeds ZIP32 limits");
    const auto crc = Crc32(bytes);
    U32(zip, 0x04034B50U);
    U16(zip, 20);
    U16(zip, 0);
    U16(zip, 0);
    U16(zip, 0);
    U16(zip, 0x0021);
    U32(zip, crc);
    U32(zip, static_cast<std::uint32_t>(bytes.size()));
    U32(zip, static_cast<std::uint32_t>(bytes.size()));
    U16(zip, static_cast<std::uint16_t>(name.size()));
    U16(zip, 0);
    zip.write(name.data(), static_cast<std::streamsize>(name.size()));
    zip.write(reinterpret_cast<const char *>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    central.push_back({name, crc, static_cast<std::uint32_t>(bytes.size()),
                       static_cast<std::uint32_t>(offset)});
  }
  const auto centralOffset = zip.tellp();
  for (const auto &entry : central) {
    U32(zip, 0x02014B50U);
    U16(zip, 20);
    U16(zip, 20);
    U16(zip, 0);
    U16(zip, 0);
    U16(zip, 0);
    U16(zip, 0x0021);
    U32(zip, entry.crc);
    U32(zip, entry.size);
    U32(zip, entry.size);
    U16(zip, static_cast<std::uint16_t>(entry.name.size()));
    U16(zip, 0);
    U16(zip, 0);
    U16(zip, 0);
    U16(zip, 0);
    U32(zip, 0);
    U32(zip, entry.offset);
    zip.write(entry.name.data(),
              static_cast<std::streamsize>(entry.name.size()));
  }
  const auto centralEnd = zip.tellp();
  if (centralOffset < 0 || centralEnd < centralOffset ||
      static_cast<std::uint64_t>(centralEnd) > 0xFFFFFFFFULL)
    throw std::runtime_error("proof ZIP exceeds ZIP32 limits");
  U32(zip, 0x06054B50U);
  U16(zip, 0);
  U16(zip, 0);
  U16(zip, static_cast<std::uint16_t>(central.size()));
  U16(zip, static_cast<std::uint16_t>(central.size()));
  U32(zip, static_cast<std::uint32_t>(centralEnd - centralOffset));
  U32(zip, static_cast<std::uint32_t>(centralOffset));
  U16(zip, 0);
  if (!zip)
    throw std::runtime_error("could not write complete proof ZIP");
}

} // namespace pkmn::cli::util
