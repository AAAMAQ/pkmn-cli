#include "FileManipulation.hpp"

#include <fstream>
#include <stdexcept>
#include <string_view>

namespace firered {

Bytes ReadBinaryFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("cannot open input file: " + path.string());
    const auto end = input.tellg();
    if (end < 0) throw std::runtime_error("cannot determine file size: " + path.string());
    Bytes bytes(static_cast<std::size_t>(end));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    if (!input) throw std::runtime_error("failed while reading: " + path.string());
    return bytes;
}

void WriteNewBinaryFile(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
    if (std::filesystem::exists(path)) {
        throw std::runtime_error("refusing to overwrite existing output: " + path.string());
    }
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("cannot create output: " + path.string());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    if (!output) throw std::runtime_error("failed while writing: " + path.string());
}

void WriteNewTextFile(const std::filesystem::path& path, const std::string& text) {
    if (std::filesystem::exists(path)) {
        throw std::runtime_error("refusing to overwrite existing output: " + path.string());
    }
    std::ofstream output(path);
    if (!output) throw std::runtime_error("cannot create output: " + path.string());
    output << text;
    if (!output) throw std::runtime_error("failed while writing: " + path.string());
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open input file: " + path.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::filesystem::path DefaultJsonPath(const std::filesystem::path& savePath) {
    auto output = savePath;
    output.replace_extension(".fred.json");
    return output;
}

std::filesystem::path DefaultReconstructedPath(
    const std::filesystem::path& jsonPath,
    const std::string& sourceFilename) {
    return jsonPath.parent_path() / ("[RECONSTRUCTED] " + sourceFilename);
}

std::filesystem::path CollisionSafePath(const std::filesystem::path& preferred) {
    if (!std::filesystem::exists(preferred)) return preferred;
    const auto parent = preferred.parent_path();
    const auto filename = preferred.filename().string();
    std::string stem;
    std::string extension;
    constexpr std::string_view kMasterJsonExtension = ".fred.json";
    if (filename.ends_with(kMasterJsonExtension)) {
        stem = filename.substr(0, filename.size() - kMasterJsonExtension.size());
        extension = kMasterJsonExtension;
    } else {
        stem = preferred.stem().string();
        extension = preferred.extension().string();
    }
    for (std::size_t suffix = 2; suffix < 10000; ++suffix) {
        const auto candidate = parent / (stem + "_" + std::to_string(suffix) + extension);
        if (!std::filesystem::exists(candidate)) return candidate;
    }
    throw std::runtime_error("could not find a collision-safe output path");
}

} // namespace firered
