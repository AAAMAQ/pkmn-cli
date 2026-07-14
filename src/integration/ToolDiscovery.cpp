#include "integration/ToolDiscovery.hpp"

#include <cstdlib>
#include <system_error>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace pkmn::cli::integration {
namespace {

bool IsUsableExecutable(const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || error) {
        return false;
    }
#if defined(_WIN32)
    return _access(path.string().c_str(), 0) == 0;
#else
    return access(path.c_str(), X_OK) == 0;
#endif
}

std::vector<std::filesystem::path> SearchRoots() {
    std::vector<std::filesystem::path> roots;
    std::error_code error;
    auto current = std::filesystem::current_path(error);
    if (error) {
        return roots;
    }

    for (int depth = 0; depth < 6; ++depth) {
        roots.push_back(current);
        if (!current.has_parent_path() || current.parent_path() == current) {
            break;
        }
        current = current.parent_path();
    }
    return roots;
}

std::vector<std::filesystem::path> PathDirectories() {
    std::vector<std::filesystem::path> directories;
    const char* rawPath = std::getenv("PATH");
    if (rawPath == nullptr) {
        return directories;
    }

#if defined(_WIN32)
    constexpr char separator = ';';
#else
    constexpr char separator = ':';
#endif
    std::string pathValue(rawPath);
    std::size_t begin = 0;
    while (begin <= pathValue.size()) {
        const std::size_t end = pathValue.find(separator, begin);
        const std::string entry = pathValue.substr(begin, end - begin);
        directories.emplace_back(entry.empty() ? "." : entry);
        if (end == std::string::npos) {
            break;
        }
        begin = end + 1;
    }
    return directories;
}

}  // namespace

ToolLocation DiscoverTool(const ToolSpec& specification) {
    for (const auto& variable : specification.environmentVariables) {
        const char* rawValue = std::getenv(variable.c_str());
        if (rawValue == nullptr || *rawValue == '\0') {
            continue;
        }
        const std::filesystem::path candidate(rawValue);
        if (IsUsableExecutable(candidate)) {
            return {specification.displayName, std::filesystem::absolute(candidate),
                    "environment variable " + variable, {}};
        }
        return {specification.displayName, std::nullopt, "environment variable " + variable,
                variable + " is set, but it does not point to an executable file: " +
                    candidate.string()};
    }

    for (const auto& directory : PathDirectories()) {
        for (const auto& name : specification.pathNames) {
            const auto candidate = directory / name;
            if (IsUsableExecutable(candidate)) {
                return {specification.displayName, std::filesystem::absolute(candidate), "PATH", {}};
            }
        }
    }

    for (const auto& root : SearchRoots()) {
        for (const auto& relative : specification.siblingCandidates) {
            const auto candidate = root / relative;
            if (IsUsableExecutable(candidate)) {
                return {specification.displayName, std::filesystem::absolute(candidate),
                        "local sibling build", {}};
            }
        }
    }

    return {specification.displayName, std::nullopt, "not found",
            "build/install the helper, add it to PATH, or set " +
                specification.environmentVariables.front()};
}

ToolSpec SaveGenieSpec() {
    return {
        "Pokemon Red Save Genie",
        {"PKMN_RED_SAVE_GENIE", "PKMN_SAVE_GENIE"},
        {"pkmn-red-save-genie", "SaveGenie", "Pkmn Red Save Genie"},
        {
            "Pkmn Red Save Genie/.xcode-derived/Build/Products/Release/Pkmn Red Save Genie",
            "Pkmn Red Save Genie/.xcode-derived/Build/Products/Debug/Pkmn Red Save Genie",
            "Pkmn Red Save Genie/build/pkmn-red-save-genie",
            "Pkmn Red Save Genie/build/SaveGenie",
        },
    };
}

ToolSpec SaveGeneratorSpec() {
    return {
        "Pokemon Red Save Generator",
        {"PKMN_RED_SAVE_GENERATOR", "PKMN_SAVE_GENERATOR"},
        {"pkmn-red-save-generator"},
        {
            "Pkmn Red Save Generator/build/pkmn-red-save-generator",
            "Pkmn Red Save Generator/build/Release/pkmn-red-save-generator",
        },
    };
}

}  // namespace pkmn::cli::integration

