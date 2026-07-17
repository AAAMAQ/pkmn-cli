#include "commands/catalog/CatalogCommand.hpp"

#include <filesystem>
#include <ostream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "app/CommandCatalog.hpp"
#include "app/ExitCode.hpp"
#include "app/Version.hpp"
#include "util/AtomicOutput.hpp"

namespace pkmn::cli::commands::catalog {
namespace {

std::string TextCatalog() {
  std::string result = "pkmn complete command catalog\nVersion: ";
  result += kVersion;
  result += "\nCommands: " + std::to_string(AvailableCommands().size()) + "\n";
  std::string_view category;
  for (const auto &command : AvailableCommands()) {
    if (command.category != category) {
      category = command.category;
      result += "\n" + std::string(category) + "\n";
    }
    result += "  " + std::string(command.usage) + "\n    " +
              std::string(command.description) + "\n";
  }
  result += "\nGlobal controls: --quiet, --verbose, --no-color\n";
  result += "Reserved only: fred, frjson, convert (not implemented)\n";
  return result;
}

std::string MarkdownCatalog() {
  std::string result = "# Complete pkmn Command Catalog\n\n";
  result += "Generated from the command catalog compiled into `pkmn ";
  result += kVersion;
  result += "`. Available command endpoints: **" +
            std::to_string(AvailableCommands().size()) + "**.\n\n";
  std::string_view category;
  for (const auto &command : AvailableCommands()) {
    if (command.category != category) {
      category = command.category;
      result += "## " + std::string(category) + "\n\n";
    }
    result += "### `" + std::string(command.path) + "`\n\n```sh\n" +
              std::string(command.usage) + "\n```\n\n" +
              std::string(command.description) + "\n\n";
  }
  result += "## Global controls\n\n`--quiet`, `--verbose`, and `--no-color` "
            "must appear before the command. The `fred`, `frjson`, and "
            "`convert` domains are reserved but not implemented.\n";
  return result;
}

std::string JsonCatalog() {
  nlohmann::ordered_json commands = nlohmann::ordered_json::array();
  for (const auto &command : AvailableCommands())
    commands.push_back({{"category", command.category},
                        {"path", command.path},
                        {"usage", command.usage},
                        {"description", command.description},
                        {"available", true}});
  return nlohmann::ordered_json(
             {{"format", "pkmn-command-catalog"},
              {"reportVersion", "1.0.0"},
              {"toolVersion", std::string(kVersion)},
              {"commandCount", commands.size()},
              {"globalControls", {"--quiet", "--verbose", "--no-color"}},
              {"reservedDomains", {"fred", "frjson", "convert"}},
              {"commands", commands}})
             .dump(2) +
         '\n';
}

} // namespace

int Run(const std::vector<std::string> &arguments, std::ostream &output,
        std::ostream &error) {
  std::string format = "text";
  std::filesystem::path outputPath;
  for (std::size_t index = 0; index < arguments.size(); ++index) {
    if (arguments[index] == "--format" && index + 1 < arguments.size())
      format = arguments[++index];
    else if (arguments[index] == "--output" && index + 1 < arguments.size())
      outputPath = arguments[++index];
    else if (arguments[index] == "help" || arguments[index] == "--help" ||
             arguments[index] == "-h") {
      output << "Usage: pkmn get-all-cmds [--format text|json|markdown] "
                "[--output <file>]\n";
      return 0;
    } else {
      error << "pkmn get-all-cmds: invalid arguments\n";
      return ToInt(ExitCode::InvalidArguments);
    }
  }
  if (format != "text" && format != "json" && format != "markdown") {
    error << "pkmn get-all-cmds: format must be text, json, or markdown\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  const auto contents = format == "json"     ? JsonCatalog()
                        : format == "markdown" ? MarkdownCatalog()
                                                : TextCatalog();
  try {
    if (outputPath.empty() || outputPath == "-")
      output << contents;
    else {
      util::WriteTextAtomic(outputPath, contents);
      output << "Command catalog written: " << outputPath.filename().string()
             << '\n';
    }
    return 0;
  } catch (const std::exception &exception) {
    error << "pkmn get-all-cmds: " << exception.what() << '\n';
    return ToInt(ExitCode::OutputFailure);
  }
}

} // namespace pkmn::cli::commands::catalog
