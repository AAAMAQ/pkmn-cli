#include "util/ResourceLocator.hpp"

#include <stdexcept>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace pkmn::cli::util {
namespace {
std::filesystem::path ExecutablePath() {
#if defined(__APPLE__)
  std::uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::vector<char> buffer(size);
  if (_NSGetExecutablePath(buffer.data(), &size) != 0)
    return {};
  return std::filesystem::weakly_canonical(buffer.data());
#elif defined(_WIN32)
  std::vector<char> buffer(32768);
  const auto size = GetModuleFileNameA(nullptr, buffer.data(),
                                       static_cast<DWORD>(buffer.size()));
  return size == 0 ? std::filesystem::path{}
                   : std::filesystem::path(std::string(buffer.data(), size));
#else
  std::vector<char> buffer(4096);
  const auto size = readlink("/proc/self/exe", buffer.data(), buffer.size());
  return size <= 0 ? std::filesystem::path{}
                   : std::filesystem::path(std::string(buffer.data(), size));
#endif
}
} // namespace

std::filesystem::path RedTemplatePath() {
  constexpr auto name = "pokemon-red-usa-europe-v1.template.bin";
  const auto executable = ExecutablePath();
  const std::vector<std::filesystem::path> candidates = {
      executable.parent_path() / "resources" / name,
      executable.parent_path().parent_path() / "share" / "pkmn" / "resources" /
          name,
      std::filesystem::current_path() / "resources" / name};
  for (const auto &candidate : candidates)
    if (std::filesystem::is_regular_file(candidate))
      return candidate;
  throw std::runtime_error(
      "installed Pokemon Red template resource was not found");
}
} // namespace pkmn::cli::util
