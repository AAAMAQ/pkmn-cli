#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace pkmn::cli::integration {

struct ToolSpec {
    std::string displayName;
    std::vector<std::string> environmentVariables;
    std::vector<std::string> pathNames;
    std::vector<std::filesystem::path> siblingCandidates;
};

struct ToolLocation {
    std::string displayName;
    std::optional<std::filesystem::path> path;
    std::string source;
    std::string guidance;

    [[nodiscard]] bool Found() const noexcept { return path.has_value(); }
};

ToolLocation DiscoverTool(const ToolSpec& specification);
ToolSpec SaveGenieSpec();
ToolSpec SaveGeneratorSpec();

}  // namespace pkmn::cli::integration

