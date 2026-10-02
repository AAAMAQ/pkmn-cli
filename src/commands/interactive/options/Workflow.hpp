#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace pkmn::cli::commands::interactive::options {

enum class InputKind { ExistingFile, ExistingDirectory, OutputFile,
                       OutputDirectory, Text, Integer, Choice, FileList };
enum class EmitKind { Positional, Option, Switch, TokenChoice, None };

struct Field {
  std::string key;
  std::string question;
  InputKind kind = InputKind::Text;
  EmitKind emit = EmitKind::Positional;
  std::string option;
  std::vector<std::string> choices;
  std::vector<std::string> values;
  bool optional = false;
  bool advanced = false;
  std::string help;
  std::string defaultValue;
  long long minimum = 0;
  long long maximum = 0;
  std::string visibleWhenKey;
  std::string visibleWhenValue;
};

using Answers = std::map<std::string, std::string>;
using ArgumentBuilder =
    std::function<std::vector<std::string>(const Answers &)>;

struct Workflow {
  std::string path;
  std::vector<Field> fields;
  bool writes = false;
  std::string note;
  ArgumentBuilder buildArguments;
};

std::vector<Workflow> SaveAndJsonWorkflows();
std::vector<Workflow> EditAndJapaneseWorkflows();
std::vector<Workflow> ConversionCompareProofWorkflows();
const std::vector<Workflow> &AllWorkflows();

} // namespace pkmn::cli::commands::interactive::options
