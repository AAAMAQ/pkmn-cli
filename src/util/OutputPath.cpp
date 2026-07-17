#include "util/OutputPath.hpp"

#include <stdexcept>

namespace pkmn::cli::util {

std::filesystem::path NumberedOutputPath(
    const std::filesystem::path &preferred, std::size_t number,
    std::string_view compoundExtension) {
  if (number < 1)
    throw std::runtime_error("output sequence number must be positive");
  if (number == 1)
    return preferred;
  const auto filename = preferred.filename().string();
  std::string base;
  std::string extension;
  if (!compoundExtension.empty() && filename.ends_with(compoundExtension)) {
    base = filename.substr(0, filename.size() - compoundExtension.size());
    extension = compoundExtension;
  } else {
    base = preferred.stem().string();
    extension = preferred.extension().string();
  }
  return preferred.parent_path() /
         (base + "_" + std::to_string(number) + extension);
}

} // namespace pkmn::cli::util
