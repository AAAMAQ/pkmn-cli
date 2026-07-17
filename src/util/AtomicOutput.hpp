#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace pkmn::cli::util {

class OutputTransaction {
public:
  OutputTransaction() = default;
  OutputTransaction(const OutputTransaction &) = delete;
  OutputTransaction &operator=(const OutputTransaction &) = delete;
  ~OutputTransaction();

  void StageText(const std::filesystem::path &target,
                 const std::string &contents);
  void StageBinary(const std::filesystem::path &target,
                   std::span<const std::uint8_t> contents);
  void Commit();

private:
  struct Entry {
    std::filesystem::path target;
    std::filesystem::path temporary;
  };
  std::vector<Entry> entries_;
  std::vector<std::filesystem::path> published_;
  bool committed_ = false;
};

class DirectoryTransaction {
public:
  explicit DirectoryTransaction(const std::filesystem::path &target);
  DirectoryTransaction(const DirectoryTransaction &) = delete;
  DirectoryTransaction &operator=(const DirectoryTransaction &) = delete;
  ~DirectoryTransaction();

  const std::filesystem::path &Path() const { return temporary_; }
  void Commit();

private:
  std::filesystem::path target_;
  std::filesystem::path temporary_;
  bool committed_ = false;
};

void WriteTextAtomic(const std::filesystem::path &target,
                     const std::string &contents);
void ReplaceTextAtomic(const std::filesystem::path &target,
                       const std::string &contents);
void WriteBinaryAtomic(const std::filesystem::path &target,
                       std::span<const std::uint8_t> contents);

} // namespace pkmn::cli::util
