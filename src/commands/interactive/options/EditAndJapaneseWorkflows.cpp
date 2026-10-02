#include "commands/interactive/options/Workflow.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace pkmn::cli::commands::interactive::options {
namespace {

Field Prompt(std::string key, std::string question, InputKind kind,
             bool optional = false, bool advanced = false,
             std::string help = {}) {
  Field field;
  field.key = std::move(key);
  field.question = std::move(question);
  field.kind = kind;
  field.optional = optional;
  field.advanced = advanced;
  field.help = std::move(help);
  return field;
}

Field File(std::string key, std::string question, bool optional = false,
           bool advanced = false, std::string help = {}) {
  return Prompt(std::move(key), std::move(question), InputKind::ExistingFile,
                optional, advanced, std::move(help));
}

Field Destination(std::string key, std::string question, bool optional = false,
                  bool advanced = false) {
  return Prompt(std::move(key), std::move(question), InputKind::OutputFile,
                optional, advanced);
}

Field Text(std::string key, std::string question, bool optional = false,
           bool advanced = false, std::string help = {}) {
  return Prompt(std::move(key), std::move(question), InputKind::Text, optional,
                advanced, std::move(help));
}

Field Number(std::string key, std::string question, long long minimum,
             long long maximum, bool optional = false,
             bool advanced = false) {
  auto field = Prompt(std::move(key), std::move(question), InputKind::Integer,
                      optional, advanced);
  field.minimum = minimum;
  field.maximum = maximum;
  return field;
}

Field Select(std::string key, std::string question,
             std::vector<std::string> labels,
             std::vector<std::string> values = {}, bool optional = false,
             bool advanced = false, std::string help = {}) {
  auto field = Prompt(std::move(key), std::move(question), InputKind::Choice,
                      optional, advanced, std::move(help));
  field.choices = std::move(labels);
  field.values = std::move(values);
  return field;
}

Field When(Field field, std::string key, std::string value) {
  field.visibleWhenKey = std::move(key);
  field.visibleWhenValue = std::move(value);
  return field;
}

std::vector<std::string> Command(std::string_view path) {
  std::istringstream words(std::string{path});
  std::vector<std::string> result;
  std::string word;
  while (words >> word)
    result.push_back(word);
  return result;
}

std::string Value(const Answers &answers, const std::string &key) {
  const auto it = answers.find(key);
  return it == answers.end() ? std::string{} : it->second;
}

void AddOption(std::vector<std::string> &arguments, const Answers &answers,
               const std::string &key, std::string_view option) {
  const auto value = Value(answers, key);
  if (!value.empty()) {
    arguments.emplace_back(option);
    arguments.push_back(value);
  }
}

void AddFlag(std::vector<std::string> &arguments, const Answers &answers,
             const std::string &key, std::string_view flag) {
  if (Value(answers, key) == "yes")
    arguments.emplace_back(flag);
}

void Require(std::string_view description, const std::string &value) {
  if (value.empty())
    throw std::invalid_argument(std::string(description) + " is required for this choice");
}

void CheckUnsigned(const std::string &value, unsigned long long maximum,
                   std::string_view label, unsigned long long minimum = 0) {
  Require(label, value);
  if (!std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isdigit(character) != 0;
      }))
    throw std::invalid_argument(std::string(label) + " must be a whole number");
  try {
    const auto number = std::stoull(value);
    if (number < minimum || number > maximum)
      throw std::invalid_argument(std::string(label) + " is outside its supported range");
  } catch (const std::out_of_range &) {
    throw std::invalid_argument(std::string(label) + " is outside its supported range");
  }
}

std::string ExistingJsonFile(const std::string &raw) {
  std::string value = raw;
  if (value.size() >= 2 &&
      ((value.front() == '"' && value.back() == '"') ||
       (value.front() == '\'' && value.back() == '\'')))
    value = value.substr(1, value.size() - 2);
  std::string decoded;
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] == '\\' && index + 1 < value.size() &&
        (value[index + 1] == ' ' || value[index + 1] == '\\' ||
         value[index + 1] == '(' || value[index + 1] == ')' ||
         value[index + 1] == '"' || value[index + 1] == '\''))
      ++index;
    decoded.push_back(value[index]);
  }
  if (decoded == "~" || decoded.starts_with("~/")) {
    const char *home = std::getenv("HOME");
    if (!home)
      throw std::invalid_argument("your home folder is unavailable; use an absolute JSON file path");
    decoded = std::string(home) + decoded.substr(1);
  } else if (decoded.starts_with("~")) {
    throw std::invalid_argument("use ~/ for your home folder or an absolute JSON file path");
  }
  std::error_code error;
  const auto path = std::filesystem::absolute(std::filesystem::path(decoded), error)
                        .lexically_normal();
  if (error || !std::filesystem::is_regular_file(path))
    throw std::invalid_argument("the selected JSON value file does not exist");
  return path.string();
}

Workflow Make(std::string path, std::vector<Field> fields, bool writes,
              std::string note = {}, ArgumentBuilder builder = {}) {
  Workflow workflow;
  workflow.path = std::move(path);
  workflow.fields = std::move(fields);
  workflow.writes = writes;
  workflow.note = std::move(note);
  workflow.buildArguments = std::move(builder);
  return workflow;
}

std::vector<std::string> BasicWithSource(std::string_view path,
                                         const Answers &answers,
                                         std::string_view sourceKey = "source") {
  auto arguments = Command(path);
  arguments.push_back(Value(answers, std::string(sourceKey)));
  return arguments;
}

std::vector<Field> SessionOnly() {
  return {File("session", "Which existing edit session should I open?")};
}

std::vector<std::string> RedNamedEdit(const Answers &answers) {
  auto arguments = BasicWithSource("red edit-session", answers, "session");
  const auto edit = Value(answers, "edit");
  auto value = Value(answers, "value");
  Require("a new value", value);
  if (edit == "trainer-id") CheckUnsigned(value, 65535, "trainer ID");
  else if (edit == "money") CheckUnsigned(value, 999999, "money");
  else if (edit == "coins") CheckUnsigned(value, 9999, "coins");
  else if (edit == "selected-box") CheckUnsigned(value, 12, "selected box", 1);
  else if (edit == "badges" && value != "all") CheckUnsigned(value, 255, "badge bits");
  if (edit == "event") {
    arguments.insert(arguments.end(), {"--event", value,
      Value(answers, "eventState").empty() ? "on" : Value(answers, "eventState")});
  } else if (edit == "json-pointer") {
    Require("JSON value", Value(answers, "jsonValue"));
    arguments.insert(arguments.end(), {"--set", value, Value(answers, "jsonValue")});
  } else if (edit == "json-pointer-file") {
    Require("JSON value file", Value(answers, "jsonFile"));
    arguments.insert(arguments.end(), {"--set-file", value, Value(answers, "jsonFile")});
  } else if (edit == "location-preset") {
    if (value != "reds-house-2f")
      throw std::invalid_argument("the verified location preset is reds-house-2f");
    arguments.insert(arguments.end(), {"--location-preset", value});
  } else {
    Require("an edit type", edit);
    if (edit.ends_with("-file")) value = ExistingJsonFile(value);
    arguments.push_back("--" + edit);
    arguments.push_back(value);
  }
  AddFlag(arguments, answers, "dryRun", "--dry-run");
  if (Value(answers, "format") == "json")
    arguments.insert(arguments.end(), {"--format", "json"});
  AddFlag(arguments, answers, "explainError", "--explain-error");
  return arguments;
}

std::vector<std::string> FireRedEdit(const Answers &answers,
                                     bool session) {
  auto arguments = BasicWithSource(session ? "fred edit-session" : "fred edit",
                                   answers, session ? "session" : "source");
  if (!session) AddOption(arguments, answers, "output", "--output");
  const auto field = Value(answers, "edit");
  const auto value = Value(answers, "value");
  Require("a new value", value);
  if (field == "money") CheckUnsigned(value, 999999, "money");
  else if (field == "coins") CheckUnsigned(value, 9999, "coins");
  else if (field == "badge") {
    CheckUnsigned(value, 8, "badge number", 1);
    const auto state = Value(answers, "badgeState");
    arguments.insert(arguments.end(), {"--badge", value + ":" +
      (state.empty() ? "on" : state)});
    return arguments;
  }
  Require("an edit type", field);
  arguments.push_back("--" + field);
  arguments.push_back(value);
  return arguments;
}

} // namespace

std::vector<Workflow> EditAndJapaneseWorkflows() {
  std::vector<Workflow> workflows;
  workflows.reserve(35);

  workflows.push_back(Make(
      "red-jp validate",
      {File("source", "Where is the Japanese Red save?"),
       Select("profile", "Which Japanese Red revision is this?",
              {"JP_RED_REV0", "JP_RED_REV1"}),
       Select("format", "Would you like a readable or JSON validation result?",
              {"Text", "JSON"}, {"text", "json"})},
      false, "Experimental Japanese Red support. Select the revision explicitly; the program does not guess it.",
      [](const Answers &a) {
        auto args = BasicWithSource("red-jp validate", a);
        AddOption(args, a, "profile", "--profile");
        if (Value(a, "format") == "json") args.insert(args.end(), {"--format", "json"});
        return args;
      }));
  workflows.push_back(Make(
      "red-jp decode",
      {File("source", "Where is the Japanese Red save?"),
       Select("profile", "Which Japanese Red revision is this?",
              {"JP_RED_REV0", "JP_RED_REV1"}),
       Select("physical", "Keep the exact physical save bytes in the archive?",
              {"Yes", "No"}, {"yes", "no"}),
       Destination("output", "Where should I write the Japanese archive?")},
      true, "Experimental archive format. Keeping the physical image permits exact byte reconstruction.",
      [](const Answers &a) {
        auto args = BasicWithSource("red-jp decode", a);
        AddOption(args, a, "profile", "--profile");
        AddOption(args, a, "output", "--output");
        if (Value(a, "physical") == "no") args.push_back("--no-physical-image");
        return args;
      }));
  workflows.push_back(Make(
      "red-jp convert",
      {File("source", "Where is the Japanese Red save?"),
       Select("profile", "Which Japanese Red revision is this?",
              {"JP_RED_REV0", "JP_RED_REV1"}),
       Select("target", "Which supported destination should receive the journey?",
              {"International FireRed"}, {"firered"}),
       Select("retainName", "Retain the experimental Japanese player name in the target?",
              {"No", "Yes"}, {"no", "yes"}),
       Destination("output", "Where should I write the FireRed save?"),
       File("template", "Use a different validated clean FireRed template?", true, true),
       Text("salt", "Optional deterministic conversion salt?", true, true)},
      true, "EXPERIMENTAL. This also writes a Japanese archive and an English-shaped projection beside the source before creating the FireRed save. Japanese name bytes stay in archive provenance.",
      [](const Answers &a) {
        auto args = BasicWithSource("red-jp convert", a);
        AddOption(args, a, "profile", "--profile");
        AddOption(args, a, "output", "--output");
        AddOption(args, a, "template", "--template");
        AddOption(args, a, "salt", "--salt");
        if (Value(a, "retainName") == "yes") args.push_back("--retain-playername");
        return args;
      }));

  for (const std::string action : {"inspect", "validate"})
    workflows.push_back(Make(
        "rjpjson " + action,
        {File("archive", "Which Japanese Red archive should I " + action + "?")},
        false, "This reads the archive and reports its source profile and provenance.",
        [action](const Answers &a) {
          return BasicWithSource("rjpjson " + action, a, "archive");
        }));
  workflows.push_back(Make(
      "rjpjson reconstruct",
      {File("archive", "Which Japanese Red archive contains the physical image?"),
       Destination("output", "Where should I write the restored Japanese save?")},
      true, "Reconstruction restores archived bytes; it does not generate from semantic fields.",
      [](const Answers &a) {
        auto args = BasicWithSource("rjpjson reconstruct", a, "archive");
        AddOption(args, a, "output", "--output");
        return args;
      }));
  workflows.push_back(Make(
      "rjpjson project",
      {File("archive", "Which Japanese Red archive should I project?"),
       Select("retainName", "Use the experimental Japanese player-name policy?",
              {"No", "Yes"}, {"no", "yes"}),
       Destination("output", "Where should I write the English-shaped Red JSON?")},
      true, "The projection preserves original Japanese name bytes in provenance.",
      [](const Answers &a) {
        auto args = BasicWithSource("rjpjson project", a, "archive");
        AddOption(args, a, "output", "--output");
        if (Value(a, "retainName") == "yes") args.push_back("--retain-playername");
        return args;
      }));
  workflows.push_back(Make(
      "rjpjson compare",
      {File("archive", "Which Japanese Red archive is the source?"),
       File("projection", "Which English-shaped Red JSON should I compare with it?")},
      false, "Reports exact projection matching and Japanese-to-English slot mappings.",
      [](const Answers &a) {
        auto args = BasicWithSource("rjpjson compare", a, "archive");
        args.push_back(Value(a, "projection"));
        return args;
      }));

  workflows.push_back(Make(
      "red edit",
      {File("source", "Which Red save should the copy-first editor open?")},
      true, "The direct Red editor opens its own numbered menu after this review. Choose a field, stage edits, validate, then select Save validated edited copy. It never changes the source save.",
      [](const Answers &a) { return BasicWithSource("red edit", a); }));
  workflows.push_back(Make(
      "red begin-edit",
      {File("source", "Which Red save should the new edit session use?"),
       Destination("output", "Where should I create its session JSON?")},
      true, "The session records the source identity and stores pending semantic edits.",
      [](const Answers &a) {
        auto args = BasicWithSource("red begin-edit", a);
        AddOption(args, a, "output", "--output");
        return args;
      }));
  workflows.push_back(Make(
      "red edit-session",
      {File("session", "Which Red edit session should receive the change?"),
       Select("edit", "What would you like to change?",
              {"Trainer name", "Rival name", "Trainer ID", "Money", "Coins",
               "Badge bits", "Selected PC box", "Named event flag",
               "Verified location preset", "Bag from JSON file", "PC items from JSON file",
               "Party from JSON file", "Boxes from JSON file", "Current box from JSON file",
               "Daycare from JSON file", "Hall of Fame from JSON file",
               "Pokédex from JSON file", "Options from JSON file",
               "Playtime from JSON file", "World state from JSON file",
               "Advanced JSON pointer/value", "Advanced JSON pointer/file"},
              {"trainer-name", "rival-name", "trainer-id", "money", "coins",
               "badges", "selected-box", "event", "location-preset", "bag-file",
               "pc-items-file", "party-file", "boxes-file", "current-box-file",
               "daycare-file", "hall-of-fame-file", "pokedex-file", "options-file",
               "playtime-file", "world-state-file", "json-pointer", "json-pointer-file"}),
       Text("value", "Enter the new value, event name, preset, JSON pointer, or JSON file path.",
            false, false, "Money 0–999999; coins 0–9999; trainer ID 0–65535; selected box 1–12; badges 0–255 or all. The only verified location preset is reds-house-2f."),
       When(Select("eventState", "Should the event be on or off?",
                   {"On", "Off"}, {"on", "off"}), "edit", "event"),
       When(Text("jsonValue", "What JSON value should this pointer hold?"),
            "edit", "json-pointer"),
       When(File("jsonFile", "Which JSON file supplies this pointer's value?"),
            "edit", "json-pointer-file"),
       Select("dryRun", "Preview validation without saving the session?",
              {"No", "Yes"}, {"no", "yes"}, true, true),
       Select("format", "Report staged edits as text or JSON?",
              {"Text", "JSON"}, {"text", "json"}, true, true),
       Select("explainError", "Show extra edit error guidance if validation fails?",
              {"No", "Yes"}, {"no", "yes"}, true, true)},
      true, "Stages one supported named edit. For JSON-file edits, enter an existing JSON file path as the value. This updates the session unless dry run is chosen.",
      RedNamedEdit));
  workflows.push_back(Make(
      "red pokemon",
      {File("session", "Which Red edit session should receive the Pokémon change?"),
       Select("selector", "How should I find the party Pokémon?",
              {"Party slot", "Species", "Nickname"}, {"party", "species", "nickname"}),
       Text("target", "Which slot number, species, or existing nickname?",
            false, false, "Party slots are numbered 1 through 6. Species or nickname must match exactly one party Pokémon."),
       Select("action", "What should change about that Pokémon?",
              {"Rename", "Level", "Replace move"}, {"rename", "level", "move"}),
       Text("value", "What new name, level, or move should I use?",
            false, false, "Levels must be 1–100. A move is a supported Gen I move name or ID."),
       When(Number("moveSlot", "Which move slot should be replaced (1–4)?", 1, 4),
            "action", "move"),
       Select("dryRun", "Validate without saving the edit session?",
              {"No", "Yes"}, {"no", "yes"}, true, true)},
      true, "A level edit also updates experience, HP, and stats. A move edit also updates PP and packed move fields.",
      [](const Answers &a) {
        auto args = BasicWithSource("red pokemon", a, "session");
        args.push_back(Value(a, "selector"));
        if (Value(a, "selector") == "party")
          CheckUnsigned(Value(a, "target"), 6, "party slot", 1);
        args.push_back(Value(a, "target"));
        const auto action = Value(a, "action");
        args.push_back(action);
        if (action == "level") CheckUnsigned(Value(a, "value"), 100, "Pokémon level", 1);
        if (action == "move") {
          Require("move slot", Value(a, "moveSlot"));
          args.push_back("replace");
          args.push_back(Value(a, "moveSlot"));
        }
        args.push_back(Value(a, "value"));
        AddFlag(args, a, "dryRun", "--dry-run");
        return args;
      }));
  workflows.push_back(Make(
      "red bag",
      {File("session", "Which Red edit session should receive the bag change?"),
       Select("action", "Add an item or remove one?",
              {"Add", "Remove"}, {"add", "remove"}),
       Text("item", "Which supported Gen I item name or ID?"),
       When(Number("quantity", "How many should be added (1–99)?", 1, 99),
            "action", "add"),
       Select("dryRun", "Validate without saving the edit session?",
              {"No", "Yes"}, {"no", "yes"}, true, true)},
      true, "Adding merges an existing stack and keeps bag count and slots synchronized.",
      [](const Answers &a) {
        auto args = BasicWithSource("red bag", a, "session");
        args.push_back(Value(a, "action"));
        args.push_back(Value(a, "item"));
        if (Value(a, "action") == "add") {
          Require("quantity", Value(a, "quantity"));
          args.push_back(Value(a, "quantity"));
        }
        AddFlag(args, a, "dryRun", "--dry-run");
        return args;
      }));
  workflows.push_back(Make(
      "red progress",
      {File("session", "Which Red edit session should receive the progress preset?"),
       Select("preset", "Which verified progress preset?",
              {"All Fly destinations"}, {"all"}),
       Select("dryRun", "Preview validation without saving the session?",
              {"No", "Yes"}, {"no", "yes"}, true, true)},
      true, "Only the verified Fly-destinations preset is supported; arbitrary map-state editing is disabled.",
      [](const Answers &a) {
        auto args = BasicWithSource("red progress", a, "session");
        args.insert(args.end(), {"fly-destinations", "all"});
        AddFlag(args, a, "dryRun", "--dry-run");
        return args;
      }));
  for (const std::string action : {"pending-edits", "edit-history"})
    workflows.push_back(Make(
        "red " + action,
        {File("session", "Which Red edit session should I read?"),
         Select("format", "Would you like text or JSON?",
                {"Text", "JSON"}, {"text", "json"})},
        false, {}, [action](const Answers &a) {
          auto args = BasicWithSource("red " + action, a, "session");
          if (Value(a, "format") == "json") args.insert(args.end(), {"--format", "json"});
          return args;
        }));
  workflows.push_back(Make(
      "red undo-edit",
      {File("session", "Which Red edit session should I change?"),
       Number("count", "How many recent staged edits should I undo?", 1, 1000000)},
      true, "This removes the newest staged edits from the session; the original save stays unchanged.",
      [](const Answers &a) {
        auto args = BasicWithSource("red undo-edit", a, "session");
        AddOption(args, a, "count", "--count");
        return args;
      }));
  workflows.push_back(Make(
      "red annotate-edit",
      {File("session", "Which Red edit session should receive the note?"),
       Text("note", "What note should I attach to this session?")},
      true, "Annotations change only the session record.",
      [](const Answers &a) {
        auto args = BasicWithSource("red annotate-edit", a, "session");
        args.push_back(Value(a, "note"));
        return args;
      }));
  workflows.push_back(Make(
      "red validate-edit", SessionOnly(), false,
      "Generates and validates the pending result in memory without publishing a save.",
      [](const Answers &a) { return BasicWithSource("red validate-edit", a, "session"); }));
  workflows.push_back(Make(
      "red end-edit",
      {File("session", "Which Red edit session should I finish?"),
       Destination("output", "Where should I write the validated edited save?"),
       Select("autoSuffix", "If the output family exists, use a numbered name?",
              {"No", "Yes"}, {"no", "yes"}, true, true),
       Select("dryRun", "Validate and preview without publishing a save?",
              {"No", "Yes"}, {"no", "yes"}, true, true),
       Select("format", "Would you like the command result in text or JSON?",
              {"Text", "JSON"}, {"text", "json"}, true, true)},
      true, "Publishes a new save plus JSON and Markdown edit reports. The source remains unchanged.",
      [](const Answers &a) {
        auto args = BasicWithSource("red end-edit", a, "session");
        AddOption(args, a, "output", "--output");
        AddFlag(args, a, "autoSuffix", "--auto-suffix");
        AddFlag(args, a, "dryRun", "--dry-run");
        if (Value(a, "format") == "json") args.insert(args.end(), {"--format", "json"});
        return args;
      }));

  workflows.push_back(Make(
      "fred edit",
      {File("source", "Which FireRed save should I edit into a new copy?"),
       Select("edit", "Which supported field should I change?",
              {"Player name", "Rival name", "Money", "Coins", "Badge"},
              {"player-name", "rival-name", "money", "coins", "badge"}),
       Text("value", "What new name, amount, or badge number should I use?",
            false, false, "Money is 0–999999; coins 0–9999; badge number 1–8."),
       When(Select("badgeState", "Set that badge on or off?",
                   {"On", "Off"}, {"on", "off"}), "edit", "badge"),
       Destination("output", "Where should I write the edited FireRed copy?")},
      true, "Supports one safe edit at a time and writes a separate validated save.",
      [](const Answers &a) { return FireRedEdit(a, false); }));
  workflows.push_back(Make(
      "fred begin-edit",
      {File("source", "Which FireRed save should start the edit session?"),
       Destination("output", "Where should I create its session JSON?")},
      true, "Creates an edit session linked to the selected source save.",
      [](const Answers &a) {
        auto args = BasicWithSource("fred begin-edit", a);
        AddOption(args, a, "output", "--output");
        return args;
      }));
  workflows.push_back(Make(
      "fred edit-session",
      {File("session", "Which FireRed edit session should receive the change?"),
       Select("edit", "Which supported field should I stage?",
              {"Player name", "Rival name", "Money", "Coins", "Badge"},
              {"player-name", "rival-name", "money", "coins", "badge"}),
       Text("value", "What new name, amount, or badge number should I use?",
            false, false, "Money is 0–999999; coins 0–9999; badge number 1–8."),
       When(Select("badgeState", "Set that badge on or off?",
                   {"On", "Off"}, {"on", "off"}), "edit", "badge")},
      true, "Stages one field change in the session; no save is published yet.",
      [](const Answers &a) { return FireRedEdit(a, true); }));
  workflows.push_back(Make(
      "fred pokemon",
      {File("session", "Which FireRed edit session should receive the Pokémon rename?"),
       Number("slot", "Which party slot (1–6)?", 1, 6),
       Text("name", "What new nickname should this Pokémon have?")},
      true, "Stages a party nickname change only; unsupported Pokémon edits are not offered.",
      [](const Answers &a) {
        auto args = BasicWithSource("fred pokemon", a, "session");
        args.insert(args.end(), {"party", Value(a, "slot"), "rename", Value(a, "name")});
        return args;
      }));
  workflows.push_back(Make(
      "fred bag",
      {File("session", "Which FireRed edit session should receive the bag change?"),
       Select("pocket", "Which pocket contains the verified existing stack?",
              {"PC Items", "Items", "Key Items", "Poké Balls", "TM Case", "Berry Pouch"},
              {"PC Items", "Items", "Key Items", "Poké Balls", "TM Case", "Berry Pouch"}),
       Number("slot", "Which existing zero-based slot number?", 0, 57),
       Number("itemId", "What is the existing item's numeric ID?", 1, 65535),
       Number("quantity", "What new stack quantity?", 1, 999)},
      true, "Only an existing stack with the specified ID can have its quantity changed.",
      [](const Answers &a) {
        auto args = BasicWithSource("fred bag", a, "session");
        static const std::map<std::string, unsigned long long> capacities{
            {"PC Items", 30}, {"Items", 42}, {"Key Items", 30},
            {"Poké Balls", 13}, {"TM Case", 58}, {"Berry Pouch", 43}};
        const auto pocket = Value(a, "pocket");
        const auto found = capacities.find(pocket);
        if (found == capacities.end())
          throw std::invalid_argument("choose a supported FireRed pocket");
        CheckUnsigned(Value(a, "slot"), found->second - 1, "pocket slot");
        args.insert(args.end(), {"quantity", Value(a, "pocket"), Value(a, "slot"),
                                 Value(a, "itemId"), Value(a, "quantity")});
        return args;
      }));
  workflows.push_back(Make(
      "fred progress",
      {File("session", "Which FireRed edit session should receive the badge change?"),
       Number("badge", "Which badge number (1–8)?", 1, 8),
       Select("state", "Set that badge on or off?",
              {"On", "Off"}, {"on", "off"})},
      true, "Stages a supported badge change; no save is published yet.",
      [](const Answers &a) {
        auto args = BasicWithSource("fred progress", a, "session");
        args.insert(args.end(), {"badge", Value(a, "badge"), Value(a, "state")});
        return args;
      }));
  for (const std::string action : {"pending-edits", "edit-history", "validate-edit"})
    workflows.push_back(Make(
        "fred " + action, SessionOnly(), false,
        action == "validate-edit" ? "Validates pending edits without publishing a save."
                                  : "Reads the selected FireRed edit session.",
        [action](const Answers &a) {
          return BasicWithSource("fred " + action, a, "session");
        }));
  workflows.push_back(Make(
      "fred undo-edit",
      {File("session", "Which FireRed edit session should I change?"),
       Number("count", "How many newest staged edits should I undo?", 1, 1000000)},
      true, "This changes only the session's pending list and history.",
      [](const Answers &a) {
        auto args = BasicWithSource("fred undo-edit", a, "session");
        AddOption(args, a, "count", "--count");
        return args;
      }));
  workflows.push_back(Make(
      "fred annotate-edit",
      {File("session", "Which FireRed edit session should receive the note?"),
       Text("note", "What note should I attach?")},
      true, "Annotations change only the session file.",
      [](const Answers &a) {
        auto args = BasicWithSource("fred annotate-edit", a, "session");
        args.push_back(Value(a, "note"));
        return args;
      }));
  workflows.push_back(Make(
      "fred end-edit",
      {File("session", "Which FireRed edit session should I finish?"),
       Destination("output", "Where should I write the validated FireRed copy?")},
      true, "Publishes a new copy after applying all staged edits.",
      [](const Answers &a) {
        auto args = BasicWithSource("fred end-edit", a, "session");
        AddOption(args, a, "output", "--output");
        return args;
      }));

  for (const std::string action : {"list", "search", "show"}) {
    std::vector<Field> fields;
    if (action != "list")
      fields.push_back(Text("query", action == "search"
          ? "What FireRed event or variable text should I search for?"
          : "Which exact event name or numeric ID should I show?"));
    fields.push_back(Select("kind", "Which pinned authority records should I use?",
                            {"Flags", "Variables", "Trainers", "Semantic events"},
                            {"flag", "variable", "trainer", "semantic"}));
    fields.push_back(Select("format", "Would you like text or JSON output?",
                            {"Text", "JSON"}, {"text", "json"}));
    workflows.push_back(Make(
        "fred events " + action, std::move(fields), false,
        "Records come from the pinned FireRed authority tables or bridge planner.",
        [action](const Answers &a) {
          auto args = Command("fred events " + action);
          if (action != "list") args.push_back(Value(a, "query"));
          AddOption(args, a, "kind", "--kind");
          if (Value(a, "format") == "json") args.insert(args.end(), {"--format", "json"});
          return args;
        }));
  }

  // Green 1.0 shares the source-verified Japanese save layout. Keep a distinct
  // command/profile identity while reusing the same typed prompts and engine.
  const auto existing = workflows;
  for (auto workflow : existing) {
    const bool save = workflow.path.starts_with("red-jp ");
    if (!save && !workflow.path.starts_with("rjpjson ")) continue;
    workflow.path.replace(0, save ? 6 : 7, save ? "green-jp" : "gjpjson");
    workflow.note = "EXPERIMENTAL Japanese Green 1.0. Real-save and emulator acceptance pending. " + workflow.note;
    for (auto &field : workflow.fields) {
      const auto at = field.question.find("Japanese Red");
      if (at != std::string::npos) field.question.replace(at, 12, "Japanese Green");
      if (field.key == "profile") {
        field.choices = {"Original release (1.0)"};
        field.values = {"JP_GREEN_REV0"};
      }
    }
    const auto original = workflow.buildArguments;
    workflow.buildArguments = [original, save](const Answers &answers) {
      auto args = original(answers);
      args[0] = save ? "green-jp" : "gjpjson";
      return args;
    };
    workflows.push_back(std::move(workflow));
  }
  return workflows;
}

} // namespace pkmn::cli::commands::interactive::options
