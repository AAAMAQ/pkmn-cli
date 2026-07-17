#include "util/AtomicOutput.hpp"

#include <atomic>
#include <chrono>
#include <fstream>
#include <set>
#include <stdexcept>

namespace pkmn::cli::util {
namespace {

std::filesystem::path TemporaryPath(const std::filesystem::path &target) {
  static std::atomic<std::uint64_t> sequence{0};
  const auto clock = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  for (std::uint64_t attempt = 0; attempt < 1000; ++attempt) {
    auto candidate = target;
    candidate += ".pkmn-tmp-" + std::to_string(clock) + "-" +
                 std::to_string(sequence.fetch_add(1)) + "-" +
                 std::to_string(attempt);
    if (!std::filesystem::exists(candidate))
      return candidate;
  }
  throw std::runtime_error("could not allocate a transactional output path");
}

void Write(const std::filesystem::path &path,
           std::span<const std::uint8_t> contents) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output)
    throw std::runtime_error("could not stage output: " + path.string());
  output.write(reinterpret_cast<const char *>(contents.data()),
               static_cast<std::streamsize>(contents.size()));
  output.close();
  if (!output)
    throw std::runtime_error("could not complete staged output: " +
                             path.string());
}

} // namespace

OutputTransaction::~OutputTransaction() {
  if (!committed_)
    for (const auto &entry : entries_) {
      std::error_code ignored;
      std::filesystem::remove(entry.temporary, ignored);
    }
}

void OutputTransaction::StageText(const std::filesystem::path &target,
                                  const std::string &contents) {
  const std::span bytes(reinterpret_cast<const std::uint8_t *>(contents.data()),
                        contents.size());
  StageBinary(target, bytes);
}

void OutputTransaction::StageBinary(
    const std::filesystem::path &target,
    std::span<const std::uint8_t> contents) {
  if (committed_)
    throw std::runtime_error("output transaction is already committed");
  if (target.empty() || target.filename().empty())
    throw std::runtime_error("transactional output target is invalid");
  if (std::filesystem::exists(target))
    throw std::runtime_error("refusing to overwrite existing output: " +
                             target.filename().string());
  for (const auto &entry : entries_)
    if (entry.target == target)
      throw std::runtime_error("duplicate output target in transaction");
  const auto temporary = TemporaryPath(target);
  try {
    Write(temporary, contents);
    entries_.push_back({target, temporary});
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    throw;
  }
}

void OutputTransaction::Commit() {
  if (committed_)
    throw std::runtime_error("output transaction is already committed");
  try {
    for (const auto &entry : entries_) {
      std::error_code error;
      std::filesystem::create_hard_link(entry.temporary, entry.target, error);
      if (error)
        throw std::runtime_error(
            "could not atomically publish output without overwrite: " +
            entry.target.filename().string() + " (" + error.message() + ")");
      published_.push_back(entry.target);
    }
    for (const auto &entry : entries_)
      std::filesystem::remove(entry.temporary);
    committed_ = true;
    published_.clear();
  } catch (...) {
    for (const auto &path : published_) {
      std::error_code ignored;
      std::filesystem::remove(path, ignored);
    }
    for (const auto &entry : entries_) {
      std::error_code ignored;
      std::filesystem::remove(entry.temporary, ignored);
    }
    throw;
  }
}

DirectoryTransaction::DirectoryTransaction(
    const std::filesystem::path &target)
    : target_(target), temporary_(TemporaryPath(target)) {
  if (std::filesystem::exists(target_))
    throw std::runtime_error("refusing to overwrite existing directory: " +
                             target_.filename().string());
  if (!std::filesystem::create_directory(temporary_))
    throw std::runtime_error("could not create transactional directory");
}

DirectoryTransaction::~DirectoryTransaction() {
  if (!committed_) {
    std::error_code ignored;
    std::filesystem::remove_all(temporary_, ignored);
  }
}

void DirectoryTransaction::Commit() {
  if (committed_)
    throw std::runtime_error("directory transaction is already committed");
  std::error_code error;
  if (!std::filesystem::create_directory(target_, error))
    throw std::runtime_error("could not reserve output directory without "
                             "overwrite: " + error.message());
  try {
    for (const auto &item : std::filesystem::directory_iterator(temporary_))
      std::filesystem::rename(item.path(), target_ / item.path().filename());
    std::filesystem::remove(temporary_);
    committed_ = true;
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove_all(target_, ignored);
    std::filesystem::remove_all(temporary_, ignored);
    throw;
  }
}

void WriteTextAtomic(const std::filesystem::path &target,
                     const std::string &contents) {
  OutputTransaction transaction;
  transaction.StageText(target, contents);
  transaction.Commit();
}

void ReplaceTextAtomic(const std::filesystem::path &target,
                       const std::string &contents) {
  if (!std::filesystem::is_regular_file(target))
    throw std::runtime_error("replacement target is not an existing file");
  auto replacement = TemporaryPath(target);
  WriteTextAtomic(replacement, contents);
  std::error_code error;
  std::filesystem::rename(replacement, target, error);
  if (!error)
    return;
  auto backup = TemporaryPath(target);
  try {
    std::filesystem::rename(target, backup);
    std::filesystem::rename(replacement, target);
    std::filesystem::remove(backup);
  } catch (...) {
    std::error_code ignored;
    if (!std::filesystem::exists(target) && std::filesystem::exists(backup))
      std::filesystem::rename(backup, target, ignored);
    std::filesystem::remove(replacement, ignored);
    throw;
  }
}

void WriteBinaryAtomic(const std::filesystem::path &target,
                       std::span<const std::uint8_t> contents) {
  OutputTransaction transaction;
  transaction.StageBinary(target, contents);
  transaction.Commit();
}

} // namespace pkmn::cli::util
