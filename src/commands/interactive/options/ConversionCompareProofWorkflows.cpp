#include "commands/interactive/options/Workflow.hpp"

#include <stdexcept>
#include <utility>

namespace pkmn::cli::commands::interactive::options {
namespace {

Field MakeField(std::string key, std::string question, InputKind kind,
                EmitKind emit = EmitKind::Positional, std::string option = {},
                bool optional = false, bool advanced = false,
                std::string help = {}) {
  Field field;
  field.key = std::move(key);
  field.question = std::move(question);
  field.kind = kind;
  field.emit = emit;
  field.option = std::move(option);
  field.optional = optional;
  field.advanced = advanced;
  field.help = std::move(help);
  return field;
}

Field File(std::string key, std::string question, bool optional = false,
           bool advanced = false, std::string option = {},
           std::string help = {}) {
  return MakeField(std::move(key), std::move(question), InputKind::ExistingFile,
                   option.empty() ? EmitKind::Positional : EmitKind::Option,
                   std::move(option), optional, advanced, std::move(help));
}

Field Output(std::string key, std::string question, std::string option = {},
             bool optional = true, bool advanced = false,
             std::string help = {}) {
  return MakeField(std::move(key), std::move(question), InputKind::OutputFile,
                   option.empty() ? EmitKind::Positional : EmitKind::Option,
                   std::move(option), optional, advanced, std::move(help));
}

Field Directory(std::string key, std::string question, std::string option,
                bool optional = false, bool advanced = false,
                std::string help = {}) {
  return MakeField(std::move(key), std::move(question), InputKind::OutputDirectory,
                   EmitKind::Option, std::move(option), optional, advanced,
                   std::move(help));
}

Field Text(std::string key, std::string question, bool optional = false,
           bool advanced = false, std::string option = {},
           std::string help = {}) {
  return MakeField(std::move(key), std::move(question), InputKind::Text,
                   option.empty() ? EmitKind::Positional : EmitKind::Option,
                   std::move(option), optional, advanced, std::move(help));
}

Field Choice(std::string key, std::string question,
             std::vector<std::string> labels, std::vector<std::string> values,
             std::string option = {}, bool advanced = false,
             std::string help = {}) {
  auto field = MakeField(std::move(key), std::move(question), InputKind::Choice,
                         option.empty() ? EmitKind::Positional : EmitKind::Option,
                         std::move(option), false, advanced, std::move(help));
  field.choices = std::move(labels);
  field.values = std::move(values);
  return field;
}

Field Switch(std::string key, std::string question, std::string option,
             bool advanced = false, std::string help = {}) {
  auto field = MakeField(std::move(key), std::move(question), InputKind::Choice,
                         EmitKind::Switch, std::move(option), true, advanced,
                         std::move(help));
  field.choices = {"No", "Yes"};
  field.values = {"no", "yes"};
  return field;
}

Workflow MakeWorkflow(std::string path, std::vector<Field> fields,
                      bool writes = false, std::string note = {}) {
  Workflow workflow;
  workflow.path = std::move(path);
  workflow.fields = std::move(fields);
  workflow.writes = writes;
  workflow.note = std::move(note);
  return workflow;
}

std::vector<Field> ConversionFields(std::string sourceQuestion,
                                    std::string outputQuestion,
                                    bool physicalSource, bool plan = false,
                                    bool pairedRoute = false) {
  std::vector<Field> fields;
  fields.push_back(File("source", std::move(sourceQuestion)));
  if (pairedRoute) {
    auto planOnly = Switch(
        "planOnly", "Create a proposed JSON plan instead of a save?",
        "--plan-only", false,
        "Choose Yes to write proposed target JSON and audit reports; choose "
        "No to write a generated remake save.");
    planOnly.optional = false;
    fields.push_back(std::move(planOnly));
  }
  fields.push_back(Output(
      "output", std::move(outputQuestion), {}, false, false,
      "Choose an explicit .sav destination for a generated save or a "
      ".fred.json/.lg.json destination for a proposed JSON plan. Audit "
      "artifacts are reviewed before writing."));
  auto saveOnly = [pairedRoute](Field field) {
    if (pairedRoute) {
      field.visibleWhenKey = "planOnly";
      field.visibleWhenValue = "no";
    }
    return field;
  };
  if (!plan)
    fields.push_back(saveOnly(File("template", "Use a clean remake save template?",
                                   true, true, "--template")));
  fields.push_back(Text("salt", "Custom deterministic conversion salt?", true,
                        true, "--salt"));
  fields.push_back(Output("manifest", "Separate manifest output path?",
                          "--manifest", true, true));
  fields.push_back(Output("report", "Separate Markdown report path?",
                          "--report", true, true));
  if (!plan)
    fields.push_back(saveOnly(Switch("keepIntermediate", "Keep intermediate files?",
                                     "--keep-intermediate", true)));
  if (physicalSource) {
    fields.push_back(saveOnly(Switch(
        "repair", "Repair invalid source checksums in memory if needed?",
        "--auto-repair-checksum", true,
        "This changes only the in-memory conversion source. The selected save "
        "is not modified.")));
    fields.push_back(saveOnly(Output(
        "repairedSource", "Also write a repaired source copy?",
        "--write-repaired-source", true, true,
        "A separate repaired copy is written only if checksum repair is needed.")));
  }
  fields.push_back(Switch("autoSuffix", "Use a numbered name if output exists?",
                          "--auto-suffix", true));
  auto policy = Choice("policy", "Conversion policy?", {"Pinned original"},
                       {"original"}, "--policy", true,
                       "Only the pinned original policy is supported.");
  policy.optional = true;
  fields.push_back(std::move(policy));
  return fields;
}

Workflow Conversion(std::string path, std::string sourceQuestion,
                    std::string outputQuestion, std::string evidence,
                    bool physicalSource, bool plan = false,
                    bool pairedRoute = false) {
  auto workflow = MakeWorkflow(
      std::move(path),
      ConversionFields(std::move(sourceQuestion), std::move(outputQuestion),
                       physicalSource, plan, pairedRoute),
      true,
      "Route evidence: " + std::move(evidence) +
          ". A produced save still needs manual emulator testing. Conversion "
          "writes a target plus audit artifacts and never changes the source.");
  if (pairedRoute) {
    const auto route = workflow.path.substr(std::string("convert ").size());
    workflow.buildArguments = [route](const Answers &answers) {
      const auto value = [&answers](const char *key) -> std::string {
        const auto it = answers.find(key);
        return it == answers.end() ? std::string{} : it->second;
      };
      const auto source = value("source");
      const auto output = value("output");
      if (source.empty() || output.empty())
        throw std::invalid_argument("Choose both a source and an output path.");
      const bool planOnly = value("planOnly") == "yes";
      const auto expected = planOnly
          ? (route.ends_with("leafgreen") ? ".lg.json" : ".fred.json")
          : ".sav";
      if (!output.ends_with(expected))
        throw std::invalid_argument(
            "The selected mode needs an output ending in " +
            std::string(expected) + ". Go back and change the output path.");
      std::vector<std::string> argv{"convert", route, source, output};
      if (planOnly) argv.push_back("--plan-only");
      const auto option = [&](const char *key, const char *flag) {
        const auto answer = value(key);
        if (!answer.empty()) {
          argv.push_back(flag);
          argv.push_back(answer);
        }
      };
      const auto selected = [&](const char *key, const char *flag) {
        if (value(key) == "yes") argv.push_back(flag);
      };
      if (!planOnly) option("template", "--template");
      option("salt", "--salt");
      option("manifest", "--manifest");
      option("report", "--report");
      if (!planOnly) {
        selected("keepIntermediate", "--keep-intermediate");
        selected("repair", "--auto-repair-checksum");
        option("repairedSource", "--write-repaired-source");
      }
      selected("autoSuffix", "--auto-suffix");
      option("policy", "--policy");
      return argv;
    };
  }
  return workflow;
}

std::vector<Field> RedComparisonFields(std::string first, std::string second) {
  return {File("first", std::move(first)), File("second", std::move(second)),
          Choice("format", "Show readable Markdown or JSON?",
                 {"Readable Markdown", "JSON"}, {"markdown", "json"},
                 "--format"),
          Output("outputJson", "Also save a JSON report?", "--output-json"),
          Output("outputMarkdown", "Also save a Markdown report?",
                 "--output-markdown")};
}

Workflow FireRedComparison(std::string path, std::string first,
                           std::string second, std::string note = {}) {
  return MakeWorkflow(
      std::move(path),
      {File("first", std::move(first)), File("second", std::move(second)),
       Output("outputJson", "Save the JSON comparison report?",
              "--output-json")},
      true, std::move(note));
}

const std::vector<std::string> kRouteLabels = {
    "Red to FireRed", "Red to LeafGreen", "Blue to FireRed",
    "Blue to LeafGreen"};
const std::vector<std::string> kRouteValues = {
    "red-firered", "red-leafgreen", "blue-firered", "blue-leafgreen"};

} // namespace

std::vector<Workflow> ConversionCompareProofWorkflows() {
  std::vector<Workflow> workflows;
  workflows.reserve(36);

  workflows.push_back(Conversion(
      "red convert", "Which Red save should be converted?",
      "Output FireRed save path?", "EMULATOR_VERIFIED", true));
  {
    auto redJson = Conversion(
        "rjson convert", "Which canonical Red JSON should be converted?",
        "Output remake save path?", "EMULATOR_VERIFIED for FireRed; "
        "STATICALLY_VALIDATED_COMMUNITY_TESTING for LeafGreen", false);
    redJson.fields.insert(redJson.fields.begin(),
                          Choice("target", "Which remake target?",
                                 {"FireRed", "LeafGreen"},
                                 {"firered", "leafgreen"}, "--target"));
    workflows.push_back(std::move(redJson));
  }
  workflows.push_back(Conversion(
      "rjson convert_to_frjson", "Which canonical Red JSON should be planned?",
      "Output proposed FireRed JSON path?", "EMULATOR_VERIFIED", false,
      true));
  workflows.push_back(Conversion(
      "rjson convert_to_lgjson", "Which canonical Red JSON should be planned?",
      "Output proposed LeafGreen JSON path?",
      "STATICALLY_VALIDATED_COMMUNITY_TESTING", false, true));

  workflows.push_back(Conversion(
      "convert red-firered", "Red save or canonical Red JSON source?",
      "Output FireRed save, or proposed JSON if planning?", "EMULATOR_VERIFIED",
      true, false, true));
  workflows.push_back(Conversion(
      "convert red-leafgreen", "Red save or canonical Red JSON source?",
      "Output LeafGreen save, or proposed JSON if planning?",
      "STATICALLY_VALIDATED_COMMUNITY_TESTING", true, false, true));
  workflows.push_back(Conversion(
      "convert blue-firered", "Blue save or canonical Blue JSON source?",
      "Output FireRed save, or proposed JSON if planning?",
      "STATICALLY_VALIDATED_COMMUNITY_TESTING", true, false, true));
  workflows.push_back(Conversion(
      "convert blue-leafgreen", "Blue save or canonical Blue JSON source?",
      "Output LeafGreen save, or proposed JSON if planning?",
      "STATICALLY_VALIDATED_COMMUNITY_TESTING", true, false, true));
  {
    auto blue = Conversion(
        "blue convert", "Which Blue save should be converted?",
        "Output remake save path?", "STATICALLY_VALIDATED_COMMUNITY_TESTING",
        true);
    blue.fields.insert(blue.fields.begin(),
                       Choice("target", "Which remake target?",
                              {"LeafGreen", "FireRed"},
                              {"leafgreen", "firered"}, "--target"));
    workflows.push_back(std::move(blue));
  }
  workflows.push_back(Conversion(
      "bjson convert", "Which canonical Blue JSON should be converted?",
      "Output LeafGreen save path?", "STATICALLY_VALIDATED_COMMUNITY_TESTING",
      false));
  workflows.push_back(Conversion(
      "bjson convert_to_lgjson", "Which canonical Blue JSON should be planned?",
      "Output proposed LeafGreen JSON path?",
      "STATICALLY_VALIDATED_COMMUNITY_TESTING", false, true));
  workflows.push_back(Conversion(
      "bjson convert_to_frjson", "Which canonical Blue JSON should be planned?",
      "Output proposed FireRed JSON path?",
      "STATICALLY_VALIDATED_COMMUNITY_TESTING", false, true));

  {
    auto format = Choice("format", "Show routes as text or JSON?",
                         {"Text", "JSON"}, {"text", "json"});
    format.emit = EmitKind::None;
    auto workflow = MakeWorkflow(
        "convert routes", {std::move(format)}, false,
        "The route list includes current capability and evidence labels. "
        "The direct command lists all routes; it has no game filter.");
    workflow.buildArguments = [](const Answers &answers) {
      std::vector<std::string> argv{"convert", "routes"};
      if (const auto it = answers.find("format");
          it != answers.end() && it->second == "json")
        argv.insert(argv.end(), {"--format", "json"});
      return argv;
    };
    workflows.push_back(std::move(workflow));
  }
  workflows.push_back(MakeWorkflow(
      "convert inspect",
      {Choice("kind", "Inspect event, trainer, or item mappings?",
              {"Event", "Trainer", "Item"}, {"event", "trainer", "item"}),
       Text("query", "Optional mapping name or ID?", true)},
      false, "Inspect pinned bridge authority records."));
  workflows.push_back(MakeWorkflow(
      "convert explain",
      {Choice("kind", "Explain an event, trainer, or item mapping?",
              {"Event", "Trainer", "Item"}, {"event", "trainer", "item"}),
       Text("query", "Which mapping name or ID?", false)},
      false, "The explanation includes the pinned conversion rule and evidence."));
  workflows.push_back(MakeWorkflow(
      "convert validate-manifest",
      {File("manifest", "Which conversion manifest should be checked?")},
      false, "Validate the manifest's structure and audit data."));
  {
    auto sources = MakeField(
        "sources", "Add source saves or canonical JSON files, one per line?",
        InputKind::FileList, EmitKind::Positional, {}, false, false,
        "Choose at least one file. All sources must match the selected route.");
    workflows.push_back(MakeWorkflow(
        "convert batch",
        {Choice("route", "Which conversion route?", kRouteLabels,
                kRouteValues, "--route"),
         std::move(sources),
         Directory("outputDir", "New batch output folder?", "--output-dir"),
         File("template", "Use a clean remake save template?", true, true,
              "--template"),
         Text("salt", "Custom deterministic conversion salt?", true, true,
              "--salt")},
        true,
        "Batch conversion creates a new folder with outputs and audit "
        "artifacts. The route evidence is EMULATOR_VERIFIED only for "
        "Red to FireRed; other paired routes need community testing."));
  }
  workflows.push_back(MakeWorkflow(
      "proof convert",
      {Choice("route", "Which conversion route should be proved?",
              kRouteLabels, kRouteValues),
       File("source", "Which canonical source JSON should be proved?"),
       Directory("outputDir", "New proof folder?", "--output-dir"),
       File("template", "Use a clean remake save template?", true, true,
            "--template"),
       Text("salt", "Custom deterministic conversion salt?", true, true,
            "--salt")},
      true,
      "This builds a static proof package. Emulator acceptance remains a "
      "separate manual step; route evidence is shown in the route list."));

  workflows.push_back(MakeWorkflow(
      "compare progress",
      RedComparisonFields("Older Red save?", "Newer Red save?"), true,
      "Inputs must have matching trainer name and ID from one playthrough."));
  workflows.push_back(MakeWorkflow(
      "compare physical",
      RedComparisonFields("First Red save?", "Second Red save?"), true,
      "Reports byte ranges, counts, and hashes; it does not infer gameplay "
      "meaning from byte differences."));
  workflows.push_back(MakeWorkflow(
      "compare semantic",
      RedComparisonFields("First canonical Red JSON?",
                          "Second canonical Red JSON?"),
      true, "Compares semantic fields under the pinned Red comparison policy."));
  {
    auto candidates = MakeField(
        "candidates", "Add candidate Red JSON files, one per line?",
        InputKind::FileList, EmitKind::Positional, {}, false, false,
        "Add at least one candidate besides the baseline.");
    auto format = Choice("format", "Show readable text or JSON?",
                         {"Text", "JSON"}, {"text", "json"});
    format.emit = EmitKind::None;
    auto workflow = MakeWorkflow(
        "compare semantic-batch",
        {File("baseline", "Baseline canonical Red JSON?"),
         std::move(candidates), std::move(format)},
        false,
        "Each candidate is compared independently with the baseline; "
        "this command prints results and does not save a report.");
    workflow.buildArguments = [](const Answers &answers) {
      std::vector<std::string> argv{"compare", "semantic-batch"};
      if (const auto it = answers.find("baseline"); it != answers.end())
        argv.push_back(it->second);
      if (const auto it = answers.find("candidates"); it != answers.end()) {
        const auto &joined = it->second;
        std::size_t begin = 0;
        while (begin < joined.size()) {
          const auto end = joined.find('\n', begin);
          const auto value = joined.substr(begin, end == std::string::npos
                                               ? std::string::npos
                                               : end - begin);
          if (!value.empty()) argv.push_back(value);
          if (end == std::string::npos) break;
          begin = end + 1;
        }
      }
      if (const auto it = answers.find("format");
          it != answers.end() && it->second == "json")
        argv.insert(argv.end(), {"--format", "json"});
      return argv;
    };
    workflows.push_back(std::move(workflow));
  }
  workflows.push_back(FireRedComparison(
      "compare firered-semantic", "First FireRed JSON?",
      "Second FireRed JSON?", "Excludes archival physical-image bytes."));
  workflows.push_back(FireRedComparison(
      "compare firered-progress", "Older FireRed save?",
      "Newer FireRed save?", "Decodes both saves before semantic comparison."));
  workflows.push_back(FireRedComparison(
      "compare firered-pokemon", "First FireRed JSON?",
      "Second FireRed JSON?", "Compares party, PC, and Daycare state."));
  workflows.push_back(FireRedComparison(
      "compare firered-events", "First FireRed JSON?",
      "Second FireRed JSON?", "Compares events and variables."));
  workflows.push_back(FireRedComparison(
      "compare firered-trainers", "First FireRed JSON?",
      "Second FireRed JSON?", "Compares trainer defeat state."));
  workflows.push_back(FireRedComparison(
      "compare firered-items", "First FireRed JSON?",
      "Second FireRed JSON?", "Compares inventory and obtained-item history."));
  workflows.push_back(FireRedComparison(
      "compare firered-fly", "First FireRed JSON?",
      "Second FireRed JSON?", "Compares Fly destinations."));
  workflows.push_back(FireRedComparison(
      "compare firered-hall-of-fame", "First FireRed JSON?",
      "Second FireRed JSON?", "Compares Hall of Fame entries."));
  workflows.push_back(MakeWorkflow(
      "compare bridge",
      {File("red", "Canonical Red source JSON?"),
       File("firered", "Proposed FireRed target JSON?"),
       File("manifest", "Optional conversion manifest?", true, false,
            "--manifest"),
       Output("outputJson", "Save a JSON bridge audit report?",
              "--output-json")},
      true,
      "Audits the source-to-target mapping and optionally cross-checks a "
      "conversion manifest."));

  workflows.push_back(MakeWorkflow(
      "proof red",
      {File("source", "Which Red save should be proved?"),
       Directory("outputDir", "New proof folder?", "--output-dir"),
       Switch("zip", "Also create a ZIP archive?", "--zip"),
       Output("zipOutput", "Use a specific ZIP path?", "--zip-output", true,
              true),
       Switch("autoSuffix", "Use numbered names for existing outputs?",
              "--auto-suffix", true)},
      true,
      "Creates decode, generation, comparison, determinism, and isolation "
      "artifacts. Emulator acceptance must be checked manually."));
  {
    auto mode = Choice("mode", "Create reports in a new folder or update an "
                             "existing proof folder?",
                       {"New report folder", "Existing proof folder"},
                       {"new", "existing"});
    mode.emit = EmitKind::None;
    auto destination = Text(
        "directory", "Report folder or existing proof folder path?", false,
        false, {},
        "For an existing proof folder, choose its directory containing "
        "proof-manifest.json. A new report folder must not exist yet.");
    destination.emit = EmitKind::None;
    auto workflow = MakeWorkflow(
        "proof post-emulator",
        {File("before", "Generated save before emulator testing?"),
         File("after", "Save after emulator save, close, and reload?"),
         std::move(mode), std::move(destination)},
        true,
        "An existing proof folder is updated with post-emulator reports and "
        "manifest hashes. A new folder contains standalone validation reports.");
    workflow.buildArguments = [](const Answers &answers) {
      std::vector<std::string> argv{"proof", "post-emulator"};
      if (const auto it = answers.find("before"); it != answers.end())
        argv.insert(argv.end(), {"--before", it->second});
      if (const auto it = answers.find("after"); it != answers.end())
        argv.insert(argv.end(), {"--after", it->second});
      if (const auto it = answers.find("directory");
          it != answers.end() && !it->second.empty()) {
        const auto mode = answers.find("mode");
        argv.push_back(mode != answers.end() && mode->second == "existing"
                           ? "--proof-dir"
                           : "--output-dir");
        argv.push_back(it->second);
      }
      return argv;
    };
    workflows.push_back(std::move(workflow));
  }
  {
    auto source = Text("package", "Proof directory or proof ZIP to verify?",
                       false, false, {},
                       "Paste an existing proof directory or .zip path.");
    auto format = Choice("format", "Show text or JSON verification?",
                         {"Text", "JSON"}, {"text", "json"});
    format.emit = EmitKind::None;
    auto workflow = MakeWorkflow(
        "proof verify", {std::move(source), std::move(format)}, false,
        "Checks package hashes, schemas, generated save checksums, and ZIP "
        "safety without modifying the package.");
    workflow.buildArguments = [](const Answers &answers) {
      std::vector<std::string> argv{"proof", "verify"};
      if (const auto it = answers.find("package"); it != answers.end())
        argv.push_back(it->second);
      if (const auto it = answers.find("format");
          it != answers.end() && it->second == "json")
        argv.insert(argv.end(), {"--format", "json"});
      return argv;
    };
    workflows.push_back(std::move(workflow));
  }
  workflows.push_back(MakeWorkflow(
      "proof fred",
      {File("source", "Complete FireRed JSON to prove?"),
       Directory("outputDir", "New FireRed proof folder?", "--output-dir"),
       File("template", "Use a clean FireRed save template?", true, true,
            "--template")},
      true,
      "Builds a native FireRed generation proof package. Emulator testing "
      "is a separate manual acceptance step."));
  workflows.push_back(MakeWorkflow(
      "proof red-to-firered",
      {File("source", "Canonical Red JSON to prove?"),
       Directory("outputDir", "New conversion proof folder?", "--output-dir"),
       File("template", "Use a clean FireRed save template?", true, true,
            "--template"),
       Text("salt", "Custom deterministic conversion salt?", true, true,
            "--salt")},
      true,
      "Builds a static Red-to-FireRed conversion proof and manual emulator "
      "checklist. Route evidence: EMULATOR_VERIFIED."));

  return workflows;
}

} // namespace pkmn::cli::commands::interactive::options
