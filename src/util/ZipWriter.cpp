#include "util/ZipWriter.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "util/AtomicOutput.hpp"

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
  std::ostringstream zip(std::ios::binary);
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
  const auto contents = zip.str();
  WriteBinaryAtomic(
      output,
      std::span(reinterpret_cast<const std::uint8_t *>(contents.data()),
                contents.size()));
}

std::vector<ZipEntry> ReadDeterministicZip(
    const std::filesystem::path &input) {
  std::ifstream file(input, std::ios::binary | std::ios::ate);
  if (!file)
    throw std::runtime_error("could not open proof ZIP");
  const auto end = file.tellg();
  if (end < 0)
    throw std::runtime_error("could not determine proof ZIP size");
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
  file.seekg(0);
  file.read(reinterpret_cast<char *>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  if (!file && !bytes.empty())
    throw std::runtime_error("could not read complete proof ZIP");
  auto u16 = [&bytes](std::size_t offset) -> std::uint16_t {
    if (offset + 2 > bytes.size())
      throw std::runtime_error("truncated proof ZIP");
    return static_cast<std::uint16_t>(bytes[offset] |
                                     (bytes[offset + 1] << 8U));
  };
  auto u32 = [&bytes](std::size_t offset) -> std::uint32_t {
    if (offset + 4 > bytes.size())
      throw std::runtime_error("truncated proof ZIP");
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
  };
  std::vector<ZipEntry> entries;
  std::vector<std::size_t> localOffsets;
  std::vector<std::uint32_t> localCrcs;
  std::size_t offset = 0;
  while (offset + 4 <= bytes.size() && u32(offset) == 0x04034B50U) {
    const auto localOffset = offset;
    if (offset + 30 > bytes.size())
      throw std::runtime_error("truncated proof ZIP local header");
    const auto flags = u16(offset + 6);
    const auto method = u16(offset + 8);
    const auto expectedCrc = u32(offset + 14);
    const auto compressed = u32(offset + 18);
    const auto uncompressed = u32(offset + 22);
    const auto nameLength = u16(offset + 26);
    const auto extraLength = u16(offset + 28);
    if (flags != 0 || method != 0 || compressed != uncompressed)
      throw std::runtime_error("proof ZIP uses unsupported flags or compression");
    const auto nameOffset = offset + 30;
    const auto dataOffset = nameOffset + nameLength + extraLength;
    const auto dataEnd = dataOffset + compressed;
    if (dataEnd > bytes.size())
      throw std::runtime_error("truncated proof ZIP entry");
    const std::string name(reinterpret_cast<const char *>(bytes.data() + nameOffset),
                           nameLength);
    if (name.empty() || name.starts_with('/') || name.find("..") != std::string::npos ||
        name.find('\\') != std::string::npos)
      throw std::runtime_error("unsafe proof ZIP entry name");
    std::vector<std::uint8_t> contents(bytes.begin() + dataOffset,
                                       bytes.begin() + dataEnd);
    if (Crc32(contents) != expectedCrc)
      throw std::runtime_error("proof ZIP entry CRC mismatch: " + name);
    entries.emplace_back(name, std::move(contents));
    localOffsets.push_back(localOffset);
    localCrcs.push_back(expectedCrc);
    offset = dataEnd;
  }
  if (entries.empty() || offset + 4 > bytes.size() ||
      u32(offset) != 0x02014B50U)
    throw std::runtime_error("proof ZIP has no valid central directory");
  const auto centralOffset = offset;
  std::size_t centralCount = 0;
  while (offset + 4 <= bytes.size() && u32(offset) == 0x02014B50U) {
    if (offset + 46 > bytes.size() || centralCount >= entries.size())
      throw std::runtime_error("truncated or excessive proof ZIP central entry");
    const auto flags = u16(offset + 8);
    const auto method = u16(offset + 10);
    const auto crc = u32(offset + 16);
    const auto compressed = u32(offset + 20);
    const auto uncompressed = u32(offset + 24);
    const auto nameLength = u16(offset + 28);
    const auto extraLength = u16(offset + 30);
    const auto commentLength = u16(offset + 32);
    const auto disk = u16(offset + 34);
    const auto localOffset = u32(offset + 42);
    const auto entryEnd = offset + 46 + nameLength + extraLength + commentLength;
    if (entryEnd > bytes.size() || flags != 0 || method != 0 || disk != 0 ||
        compressed != uncompressed ||
        uncompressed != entries[centralCount].second.size() ||
        crc != localCrcs[centralCount] ||
        localOffset != localOffsets[centralCount])
      throw std::runtime_error("proof ZIP central directory disagrees with local entries");
    const std::string name(
        reinterpret_cast<const char *>(bytes.data() + offset + 46), nameLength);
    if (name != entries[centralCount].first)
      throw std::runtime_error("proof ZIP central entry name mismatch");
    offset = entryEnd;
    ++centralCount;
  }
  const auto centralSize = offset - centralOffset;
  if (centralCount != entries.size() || offset + 22 != bytes.size() ||
      u32(offset) != 0x06054B50U || u16(offset + 4) != 0 ||
      u16(offset + 6) != 0 || u16(offset + 8) != centralCount ||
      u16(offset + 10) != centralCount || u32(offset + 12) != centralSize ||
      u32(offset + 16) != centralOffset || u16(offset + 20) != 0)
    throw std::runtime_error("proof ZIP end record is invalid or has trailing data");
  return entries;
}

} // namespace pkmn::cli::util
