#include "commands/interactive/InteractiveCommand.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

#include "app/ExitCode.hpp"
#include "commands/conversion/ConversionCommand.hpp"
#include "red/save/RedSave.hpp"
#include "red/validation/SaveValidator.hpp"

namespace pkmn::cli::commands::interactive {
namespace {

std::string Trim(std::string value) {
  const auto whitespace = [](unsigned char character) {
    return std::isspace(character) != 0;
  };
  value.erase(value.begin(),
              std::find_if_not(value.begin(), value.end(), whitespace));
  value.erase(std::find_if_not(value.rbegin(), value.rend(), whitespace).base(),
              value.end());
  if (value.size() >= 2 &&
      ((value.front() == '"' && value.back() == '"') ||
       (value.front() == '\'' && value.back() == '\'')))
    value = value.substr(1, value.size() - 2);
  return value;
}

std::string Lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  return value;
}

bool Read(std::istream &input, std::ostream &output, std::string_view prompt,
          std::string &answer) {
  output << prompt;
  output.flush();
  if (!std::getline(input, answer))
    return false;
  answer = Trim(answer);
  return true;
}

bool IsQuit(const std::string &answer) {
  const auto lower = Lower(answer);
  return lower == "q" || lower == "quit" || lower == "exit";
}

void Help(std::ostream &output) {
  output << "Interactive controls: enter the displayed choice; type back to "
            "return, help for guidance, or quit to exit.\n"
            "All four Red/Blue -> FireRed/LeafGreen routes are available.\n"
            "Red -> FireRed is emulator verified; the other routes are statically validated and open for community testing.\n";
  output << "Report problems: https://github.com/AAAMAQ/pkmn-cli/issues\n";
}

std::filesystem::path DefaultOutput(const std::filesystem::path &input,
                                    const std::string &target) {
  return input.parent_path() /
         (input.stem().string() + (target == "leafgreen" ? "_lg.sav" : "_fr.sav"));
}

bool OutputFamilyExists(const std::filesystem::path &output) {
  auto stem = output;
  stem.replace_extension();
  return std::filesystem::exists(output) ||
         std::filesystem::exists(stem.string() +
                                 ".conversion-manifest.json") ||
         std::filesystem::exists(stem.string() + ".conversion-report.md");
}

} // namespace

int Run(const std::vector<std::string> &arguments, std::istream &input,
        std::ostream &output, std::ostream &error) {
  if (!arguments.empty()) {
    if (arguments.size() == 1 &&
        (arguments[0] == "--help" || arguments[0] == "help")) {
      Help(output);
      return 0;
    }
    error << "pkmn interactive: no command-line options are required\n";
    return ToInt(ExitCode::InvalidArguments);
  }

  output << "pkmn 3.0 interactive\n"
            "1) Convert a save\n"
            "2) Show route status\n"
            "Q) Quit\n";
  std::string answer;
  if (!Read(input, output, "Choose an action: ", answer) || IsQuit(answer))
    return 0;
  if (Lower(answer) == "help") {
    Help(output);
    return 0;
  }
  if (answer == "2")
    return commands::conversion::RunConvert({"routes"}, output, error);
  if (answer != "1") {
    error << "pkmn interactive: unknown menu choice\n";
    return ToInt(ExitCode::InvalidArguments);
  }

  if (!Read(input, output, "Source game [R=Red, B=Blue, back, quit]: ",
            answer) || IsQuit(answer) || Lower(answer) == "back")
    return 0;
  const auto sourceAnswer = Lower(answer);
  if (sourceAnswer != "r" && sourceAnswer != "red" &&
      sourceAnswer != "b" && sourceAnswer != "blue") {
    error << "pkmn interactive: choose Red or Blue\n";
    return ToInt(ExitCode::UnsupportedOperation);
  }
  const std::string sourceGame = (sourceAnswer == "b" || sourceAnswer == "blue") ? "blue" : "red";
  if (!Read(input, output,
            "Target game [FR=FireRed, LG=LeafGreen, back, quit]: ", answer) ||
      IsQuit(answer) || Lower(answer) == "back")
    return 0;
  const auto targetAnswer = Lower(answer);
  if (targetAnswer != "fr" && targetAnswer != "firered" &&
      targetAnswer != "lg" && targetAnswer != "leafgreen") {
    error << "pkmn interactive: choose FireRed or LeafGreen\n";
    return ToInt(ExitCode::UnsupportedOperation);
  }
  const std::string targetGame = (targetAnswer == "lg" || targetAnswer == "leafgreen") ? "leafgreen" : "firered";
  const std::string route = sourceGame + "-" + targetGame;
  output << "Route evidence: "
         << (route == "red-firered" ? "EMULATOR_VERIFIED" : "STATICALLY_VALIDATED_COMMUNITY_TESTING")
         << '\n';
  if (!Read(input, output, "Proceed with this conversion? [Y/N]: ", answer) ||
      Lower(answer) != "y") {
    output << "Conversion cancelled.\n";
    return 0;
  }
  std::string inputPath;
  if (!Read(input, output, "Gen I save path: ", inputPath) || IsQuit(inputPath) ||
      Lower(inputPath) == "back")
    return 0;
  if (!std::filesystem::is_regular_file(inputPath)) {
    error << "pkmn interactive: save file was not found: " << inputPath << '\n';
    return ToInt(ExitCode::InvalidInput);
  }

  bool repair = false;
  try {
    const auto save = red::save::RedSave::Read(inputPath);
    const auto report = red::validation::SaveValidator::Validate(save);
    if (!report.expectedSize) {
      error << "pkmn interactive: input is not a standard Gen I save\n";
      return ToInt(ExitCode::InvalidInput);
    }
    if (!report.Valid()) {
      if (!Read(input, output,
                "Checksums are invalid. Repair in memory and convert? [Y/N]: ",
                answer) || Lower(answer) != "y") {
        output << "Conversion cancelled; source was not modified.\n";
        return 0;
      }
      repair = true;
    }
  } catch (const std::exception &exception) {
    error << "pkmn interactive: " << exception.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }

  std::string outputPath;
  if (!Read(input, output,
            "Output path [Enter for automatic target suffix]: ", outputPath))
    return 0;
  std::vector<std::string> forwarded{route, inputPath};
  if (!outputPath.empty())
    forwarded.push_back(outputPath);
  if (repair)
    forwarded.push_back("--auto-repair-checksum");
  const auto selectedOutput = outputPath.empty()
                                  ? DefaultOutput(inputPath, targetGame)
                                  : std::filesystem::path(outputPath);
  if (OutputFamilyExists(selectedOutput)) {
    if (!Read(input, output,
              "Output already exists. Use an automatic numbered suffix? "
              "[Y/N]: ",
              answer) ||
        Lower(answer) != "y") {
      output << "Conversion cancelled; existing files were not changed.\n";
      return 0;
    }
    forwarded.push_back("--auto-suffix");
  }
  output << "Running the same conversion engine used by the direct command.\n";
  return commands::conversion::RunConvert(forwarded, output, error);
}

} // namespace pkmn::cli::commands::interactive
