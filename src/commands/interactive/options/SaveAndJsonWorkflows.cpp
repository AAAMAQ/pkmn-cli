#include "commands/interactive/options/Workflow.hpp"

#include <utility>

namespace pkmn::cli::commands::interactive::options {
namespace {

Field Input(std::string key, std::string question, std::string help = {}) {
  Field field;
  field.key = std::move(key);
  field.question = std::move(question);
  field.kind = InputKind::ExistingFile;
  field.help = std::move(help);
  return field;
}

Field Inputs(std::string key, std::string question) {
  Field field;
  field.key = std::move(key);
  field.question = std::move(question);
  field.kind = InputKind::FileList;
  field.help = "Add at least one file. Each path is read as a whole line, including spaces.";
  return field;
}

Field Output(std::string key, std::string question, EmitKind emit,
             std::string option = {}, bool optional = false) {
  Field field;
  field.key = std::move(key);
  field.question = std::move(question);
  field.kind = InputKind::OutputFile;
  field.emit = emit;
  field.option = std::move(option);
  field.optional = optional;
  field.help = "Choose a new file. Existing outputs are not overwritten.";
  return field;
}

Field OutputFolder(std::string key, std::string question) {
  Field field;
  field.key = std::move(key);
  field.question = std::move(question);
  field.kind = InputKind::OutputDirectory;
  field.emit = EmitKind::Option;
  field.option = "--output-dir";
  field.help = "Choose the destination for all generated files and reports.";
  return field;
}

Field Text(std::string key, std::string question, EmitKind emit = EmitKind::Positional,
           std::string option = {}, bool optional = false) {
  Field field;
  field.key = std::move(key);
  field.question = std::move(question);
  field.kind = InputKind::Text;
  field.emit = emit;
  field.option = std::move(option);
  field.optional = optional;
  return field;
}

Field Choice(std::string key, std::string question,
             std::vector<std::string> labels, std::vector<std::string> values,
             EmitKind emit, std::string option = {}, bool advanced = false) {
  Field field;
  field.key = std::move(key);
  field.question = std::move(question);
  field.kind = InputKind::Choice;
  field.emit = emit;
  field.option = std::move(option);
  field.choices = std::move(labels);
  field.values = std::move(values);
  field.advanced = advanced;
  return field;
}

Field Switch(std::string key, std::string question, std::string option,
             bool advanced = false) {
  Field field;
  field.key = std::move(key);
  field.question = std::move(question);
  field.kind = InputKind::Choice;
  field.emit = EmitKind::Switch;
  field.option = std::move(option);
  field.choices = {"No", "Yes"};
  field.values = {"no", "yes"};
  field.advanced = advanced;
  return field;
}

Field JsonFormat() {
  return Choice("format", "Would you like a readable text result or JSON?",
                {"Text", "JSON"}, {"", "json"}, EmitKind::Option,
                "--format");
}

Field AutoSuffix() {
  auto field = Switch("auto_suffix", "If that output name is taken, may pkmn add a numbered suffix?",
                      "--auto-suffix", true);
  field.help = "A numbered suffix creates another file instead of replacing one.";
  return field;
}

Field Template() {
  auto field = Input("template", "Do you want to provide a verified clean template save?");
  field.emit = EmitKind::Option;
  field.option = "--template";
  field.optional = true;
  field.advanced = true;
  field.help = "Leave this blank to use the bundled template.";
  return field;
}

Workflow Make(std::string path, std::vector<Field> fields, bool writes = false,
              std::string note = {}) {
  Workflow workflow;
  workflow.path = std::move(path);
  workflow.fields = std::move(fields);
  workflow.writes = writes;
  workflow.note = std::move(note);
  return workflow;
}

} // namespace

std::vector<Workflow> SaveAndJsonWorkflows() {
  std::vector<Workflow> workflows;
  workflows.reserve(44);

  // General commands. A catalog output is optional because the user may
  // choose to read it in the terminal instead of saving it.
  workflows.push_back(Make("doctor", {
      Switch("deep", "Would you like the deeper resource and engine self-test?", "--deep"),
      JsonFormat()}, false,
      "The deep check uses bundled resources and does not need a personal save."));
  workflows.push_back(Make("completion", {
      Choice("shell", "Which shell should the completion script support?",
             {"Bash", "Zsh", "Fish"}, {"bash", "zsh", "fish"},
             EmitKind::Positional)}, false,
      "The script is printed for you; shell startup files are not changed."));
  workflows.push_back(Make("config show", {JsonFormat()}, false,
      "Displays the compiled safety and default policy."));
  workflows.push_back(Make("get-all-cmds", {
      Choice("format", "How should the command catalog be formatted?",
             {"Text", "JSON", "Markdown"}, {"text", "json", "markdown"},
             EmitKind::Option, "--format"),
      Output("output", "Where should the catalog be saved? Leave blank to view it here.",
             EmitKind::Option, "--output", true)}, true,
      "When you choose a file, review its name before saving the catalog."));

  // International Red saves and the verified Red event catalog.
  workflows.push_back(Make("red summary", {
      Input("save", "Which Pokemon Red save should I summarize?"),
      Choice("format", "Would you like short text, detailed Markdown, or JSON?",
             {"Short text", "Detailed Markdown", "JSON"},
             {"text", "markdown", "json"}, EmitKind::Option, "--format"),
      Output("output", "Would you like to save the report to a file? Leave blank to view it here.",
             EmitKind::Option, "--output", true)}, true));
  workflows.push_back(Make("red inspect", {
      Input("save", "Which Pokemon Red save should I inspect?"), JsonFormat()}));
  workflows.push_back(Make("red validate", {
      Input("save", "Which Pokemon Red save should I validate?"), JsonFormat()}));
  workflows.push_back(Make("red repair-checksums", {
      Input("save", "Which Pokemon Red save has checksum errors?"),
      Output("output", "Where should I write the repaired copy?", EmitKind::Option,
             "--output"), AutoSuffix()}, true,
      "Only recognized checksum bytes are repaired. The source save stays unchanged."));
  workflows.push_back(Make("red decode", {
      Input("save", "Which Pokemon Red save should I decode?"),
      Choice("physical", "Should the JSON include the exact archival physical image?",
             {"Yes, include it", "No, semantic data only"},
             {"", "--no-physical-image"}, EmitKind::TokenChoice),
      Output("output", "Where should I write the Red JSON?", EmitKind::Option,
             "--output"), AutoSuffix()}, true,
      "The physical image permits archival reconstruction but does not control semantic generation."));

  workflows.push_back(Make("red events list", {
      Choice("category", "Which verified event category would you like to browse?",
             {"All categories", "Gym or badge", "Major story", "Item or gift",
              "Static battle", "Door or unlock", "Environment puzzle",
              "Story progress", "Other events"},
             {"", "gym_or_badge", "major_story", "item_or_gift",
              "static_battle", "door_or_unlock", "environment_puzzle",
              "story_progress", "misc_event"}, EmitKind::Option,
             "--category"), JsonFormat()}, false,
      "Only flags in the verified named event catalog are shown."));
  workflows.push_back(Make("red events search", {
      Text("query", "What word or event fragment should I search for?"),
      JsonFormat()}));
  workflows.push_back(Make("red events show", {
      Text("event", "Which exact verified event name should I show?"),
      JsonFormat()}));
  workflows.push_back(Make("red validate-batch", {
      Inputs("saves", "Which Pokemon Red saves should I validate?"),
      JsonFormat()}, false,
      "Each save is reported separately; this check creates no save files."));
  workflows.push_back(Make("red decode-batch", {
      Inputs("saves", "Which Pokemon Red saves should I decode?"),
      OutputFolder("output_dir", "Which new folder should contain their JSON files?"),
      Choice("physical", "Should each JSON keep its archival physical image?",
             {"Yes, include it", "No, semantic data only"},
             {"", "--no-physical-image"}, EmitKind::TokenChoice)}, true,
      "The output folder must be new. Each result receives a distinct name."));
  workflows.push_back(Make("red validate-post-emulator", {
      Input("before", "Which save was made before the emulator test?"),
      Input("after", "Which save came back from the emulator?"),
      OutputFolder("output_dir", "Which folder should receive the validation reports?")},
      true, "This checks the emulator round trip; it does not change either save."));

  // Canonical Red JSON. Generation uses semantic fields; reconstruction
  // restores archived bytes and requires a physical image.
  workflows.push_back(Make("rjson inspect", {
      Input("json", "Which Red JSON document should I inspect?"), JsonFormat()}));
  workflows.push_back(Make("rjson validate", {
      Input("json", "Which Red JSON document should I validate?"),
      Choice("profile", "Which validation policy fits your task?",
             {"Standard", "Strict", "Generation", "Archival"},
             {"standard", "strict", "generation", "archival"},
             EmitKind::Option, "--profile"), JsonFormat()}, false,
      "Generation validates semantic save creation; archival checks preserved physical bytes."));
  workflows.push_back(Make("rjson generate", {
      Input("json", "Which Red JSON should supply the semantic save fields?"),
      Output("output", "Where should I write the generated Red save?",
             EmitKind::Positional), AutoSuffix()}, true,
      "Generation ignores an archival physical image and creates a new semantic save."));
  workflows.push_back(Make("rjson reconstruct", {
      Input("json", "Which Red JSON contains the physical image to restore?"),
      Output("output", "Where should I write the byte-for-byte restored save?",
             EmitKind::Option, "--output"), AutoSuffix()}, true,
      "Reconstruction requires an archival physical image; it does not regenerate game state."));
  workflows.push_back(Make("rjson migrate", {
      Input("json", "Which compatible Red JSON should I enrich?"),
      Output("output", "Where should I write the migrated JSON copy?",
             EmitKind::Option, "--output"), AutoSuffix()}, true));
  workflows.push_back(Make("rjson schema", {
      JsonFormat()}, false,
      "Shows the complete current Red JSON schema and validation policies."));
  workflows.push_back(Make("rjson generate-batch", {
      Inputs("json_files", "Which Red JSON documents should I generate from?"),
      OutputFolder("output_dir", "Which new folder should contain the generated saves?")},
      true, "A failed input prevents the batch from being presented as complete."));
  workflows.push_back(Make("rjson update_schema", {
      Input("json", "Which Red JSON should I update to the supported schema?"),
      Output("output", "Where should I write the updated JSON copy?",
             EmitKind::Option, "--output"), AutoSuffix()}, true));

  // FireRed JSON has separate native and proposed conversion contracts.
  workflows.push_back(Make("frjson inspect", {
      Input("json", "Which FireRed JSON document should I inspect?")}));
  workflows.push_back(Make("frjson validate", {
      Input("json", "Which FireRed JSON document should I validate?")}));
  workflows.push_back(Make("frjson schema", {JsonFormat()}, false,
      "Shows the complete native and proposed FireRed JSON contracts."));
  workflows.push_back(Make("frjson update_schema", {
      Input("json", "Which FireRed JSON should I update?"),
      Output("output", "Where should I write the updated FireRed JSON copy?",
             EmitKind::Option, "--output"), AutoSuffix()}, true));
  workflows.push_back(Make("frjson generate", {
      Input("json", "Which FireRed JSON should supply semantic save data?"),
      Output("output", "Where should I write the generated FireRed save?",
             EmitKind::Positional), Template()}, true,
      "A complete native document or supported proposed conversion document is required."));
  workflows.push_back(Make("frjson reconstruct", {
      Input("json", "Which FireRed JSON includes an archival physical image?"),
      Output("output", "Where should I write the restored FireRed save?",
             EmitKind::Option, "--output")}, true,
      "Restores physical bytes; this is distinct from semantic generation."));
  workflows.push_back(Make("frjson migrate", {
      Input("json", "Which FireRed JSON should I migrate?"),
      Output("output", "Where should I write the migrated JSON copy?",
             EmitKind::Option, "--output"), AutoSuffix()}, true));
  workflows.push_back(Make("frjson generate-batch", {
      Inputs("json_files", "Which FireRed JSON documents should I generate from?"),
      OutputFolder("output_dir", "Which new folder should contain the generated saves?"),
      Template()}, true));

  // Native FireRed saves.
  workflows.push_back(Make("fred summary", {
      Input("save", "Which FireRed save should I summarize?"),
      Switch("detailed", "Would you like the detailed summary?", "--detailed")}));
  workflows.push_back(Make("fred inspect", {
      Input("save", "Which FireRed save should I inspect?"), JsonFormat()}));
  workflows.push_back(Make("fred validate", {
      Input("save", "Which FireRed save should I validate?"), JsonFormat()}));
  workflows.push_back(Make("fred decode", {
      Input("save", "Which FireRed save should I decode?"),
      Output("output", "Where should I write its native FireRed JSON?",
             EmitKind::Option, "--output")}, true));
  workflows.push_back(Make("fred repair-checksums", {
      Input("save", "Which FireRed save has checksum errors?"),
      Output("output", "Where should I write the repaired copy?",
             EmitKind::Option, "--output")}, true,
      "Only checksum fields are repaired; the source stays unchanged."));
  workflows.push_back(Make("fred validate-batch", {
      Inputs("saves", "Which FireRed saves should I validate?"), JsonFormat()}));
  workflows.push_back(Make("fred decode-batch", {
      Inputs("saves", "Which FireRed saves should I decode?"),
      OutputFolder("output_dir", "Which new folder should contain the JSON files?")},
      true));
  workflows.push_back(Make("fred validate-post-emulator", {
      Input("before", "Which FireRed save was made before the emulator test?"),
      Input("after", "Which save came back from the emulator?"),
      OutputFolder("output_dir", "Which folder should receive the validation reports?")},
      true, "This classifies an emulator round trip without changing either save."));

  // The narrower Blue and LeafGreen endpoints in the 3.0 catalog.
  workflows.push_back(Make("blue decode", {
      Input("save", "Which Pokemon Blue save should I decode?"),
      Output("output", "Where should I write its Blue JSON?",
             EmitKind::Option, "--output")}, true));
  workflows.push_back(Make("leafgreen decode", {
      Input("save", "Which LeafGreen save should I decode?"),
      Output("output", "Where should I write its LeafGreen JSON?",
             EmitKind::Option, "--output")}, true));
  workflows.push_back(Make("leafgreen validate", {
      Input("save", "Which LeafGreen save should I validate?"), JsonFormat()}));
  workflows.push_back(Make("lgjson generate", {
      Input("json", "Which LeafGreen JSON should supply semantic save data?"),
      Output("output", "Where should I write the generated LeafGreen save?",
             EmitKind::Positional), Template()}, true));
  workflows.push_back(Make("lgjson validate", {
      Input("json", "Which LeafGreen JSON should I validate?")}));

  return workflows;
}

} // namespace pkmn::cli::commands::interactive::options
