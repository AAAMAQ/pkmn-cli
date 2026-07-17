#include "commands/compare/CompareCommand.hpp"
#include "app/ExitCode.hpp"
#include "red/comparison/Comparison.hpp"
#include "red/json/RedJsonDocument.hpp"
#include "red/save/RedSave.hpp"
#include <filesystem>
#include <fstream>
#include <ostream>

namespace pkmn::cli::commands::compare {
namespace {
struct OutputOptions {
  bool jsonToStdout = false;
  std::filesystem::path jsonPath;
  std::filesystem::path markdownPath;
};

OutputOptions ParseOutputOptions(const std::vector<std::string> &arguments) {
  OutputOptions options;
  for (std::size_t index = 3; index < arguments.size(); ++index) {
    if (arguments[index] == "--format" && index + 1 < arguments.size()) {
      const auto &format = arguments[++index];
      if (format == "json")
        options.jsonToStdout = true;
      else if (format != "markdown")
        throw std::runtime_error("--format must be json or markdown");
    } else if (arguments[index] == "--output-json" &&
               index + 1 < arguments.size()) {
      options.jsonPath = arguments[++index];
    } else if (arguments[index] == "--output-markdown" &&
               index + 1 < arguments.size()) {
      options.markdownPath = arguments[++index];
    } else {
      throw std::runtime_error("invalid comparison output option");
    }
  }
  for (const auto &path : {options.jsonPath, options.markdownPath})
    if (!path.empty() && std::filesystem::exists(path))
      throw std::runtime_error("refusing to overwrite comparison report");
  return options;
}

void Emit(const red::json::OrderedJson &report, const std::string &markdown,
          const OutputOptions &options, std::ostream &output) {
  if (!options.jsonPath.empty()) {
    std::ofstream file(options.jsonPath, std::ios::binary);
    file << report.dump(2) << '\n';
    if (!file)
      throw std::runtime_error("could not write JSON comparison report");
  }
  if (!options.markdownPath.empty()) {
    std::ofstream file(options.markdownPath, std::ios::binary);
    file << markdown;
    if (!file)
      throw std::runtime_error("could not write Markdown comparison report");
  }
  output << (options.jsonToStdout ? report.dump(2) + "\n" : markdown);
}
} // namespace

int Run(const std::vector<std::string> &a, std::ostream &out,
        std::ostream &err) {
  if (a.empty() || a[0] == "help" || a[0] == "--help") {
    out << "Usage:\n"
           "  pkmn compare physical <a.sav> <b.sav> [report options]\n"
           "  pkmn compare semantic <a.red.json> <b.red.json> [report options]\n"
           "Report options:\n"
           "  --format markdown|json\n"
           "  --output-json <report.json>\n"
           "  --output-markdown <report.md>\n";
    return 0;
  }
  if (a.size() < 3 || (a[0] != "physical" && a[0] != "semantic")) {
    err << "pkmn compare: invalid arguments\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  try {
    const auto options = ParseOutputOptions(a);
    if (a[0] == "physical") {
      const auto left = red::save::RedSave::Read(a[1]);
      const auto right = red::save::RedSave::Read(a[2]);
      const auto report =
          red::comparison::ComparePhysical(left.BytesView(), right.BytesView());
      Emit(report, red::comparison::PhysicalMarkdown(report), options, out);
      return 0;
    }
    const auto left = red::json::LoadAndValidate(a[1]);
    const auto right = red::json::LoadAndValidate(a[2]);
    if (!left.validation.Valid() || !right.validation.Valid())
      throw std::runtime_error("both semantic inputs must validate");
    const auto report = red::comparison::CompareSemantic(left.root, right.root);
    Emit(report, red::comparison::SemanticMarkdown(report), options, out);
    return report.at("equivalent").get<bool>()
               ? 0
               : ToInt(ExitCode::SemanticMismatch);
  } catch (const std::exception &e) {
    err << "pkmn compare: " << e.what() << '\n';
    return std::string(e.what()).find("overwrite") != std::string::npos ||
                   std::string(e.what()).find("write") != std::string::npos
               ? ToInt(ExitCode::OutputFailure)
               : ToInt(ExitCode::InvalidInput);
  }
}
} // namespace pkmn::cli::commands::compare
