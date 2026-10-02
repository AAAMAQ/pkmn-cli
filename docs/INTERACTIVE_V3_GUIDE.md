# Version 3.0 Interactive Command Experience

For a prompt-by-prompt example of each current endpoint and future proposal,
see [INTERACTIVE_COMMAND_EXAMPLES.md](INTERACTIVE_COMMAND_EXAMPLES.md).

## Start guided mode

Version 3.0 has one guided entry point:

```sh
pkmn interactive
```

From there, users choose a task, answer a sequence of clear questions, review
what the program is about to do, and then run it. The experience is a
back-and-forth conversation with the terminal—not a requirement to know and
type a full command line. Direct commands remain available for experienced
users, scripts, and automation.

The interaction pattern is:

```text
Choose a task
  → choose a game, route, or operation
  → answer guided questions
  → correct answers or go back at any time
  → review inputs, options, evidence, and outputs
  → confirm before writing or making a change
  → see results and choose a useful next task
```

The opening choice is simple save conversion, including experimental Japanese
Red. Advanced conversion is the second choice. Other tasks are grouped by
purpose and available through a searchable, paginated command browser.

## Current coverage and evidence

The compiled `pkmn get-all-cmds` catalog reports 124 endpoints. Guided mode
provides a typed path for every current endpoint; `interactive` is the entry
point itself. Catalog presence does not mean identical maturity or evidence.
Japanese Red remains experimental. Red → FireRed is emulator verified; the
other paired remake routes are statically validated and open for community
testing. Future proposals in [FUTURE_COMMANDS.md](FUTURE_COMMANDS.md) are not
current executable commands.

## Main menu

```text
pkmn 3.1 interactive - guided mode
1 Convert a save (simple, including Japanese Red/Green)
2 Advanced conversion and Japanese tools
3 Inspect, validate, summarize, or repair a save
4 Decode, generate, reconstruct, or migrate save data
5 Edit a save safely
6 Compare saves or playthrough progress
7 Run proof or emulator checks
8 Browse every current command and option
9 Doctor, settings, and beginner help
0 Exit
? Help  B Back  Q Quit
Choose a task:
```

The menu exposes runnable routes with their evidence labels. Japanese Red
commands ask for the required explicit revision. Planned commands remain in
the roadmap and cannot be launched as current commands.

## Conversational rules shared by every workflow

- Read each answer as a complete line. Paths are not split at spaces.
- Accept paths with no quotes, matching single/double quotes, pasted spaces,
  Unicode, relative paths, and supported `~` expansion. Show the resolved
  path before use, and let the user correct a missing file rather than
  silently choosing a similar one.
- Every screen offers `?` for context help, `B`/`back` to return, and
  `Q`/`quit` to cancel safely. Preserve prior answers when returning unless
  changed.
- Main-menu tasks use numbers; listed answers and command-browser entries also
  accept their displayed names where supported. No arrow-key terminal support
  is required.
- Explain technical terms (checksum, semantic generation, physical
  reconstruction, evidence level) in context.
- Before any write, display source(s), target/profile, selected options,
  warnings/evidence, and every output path. Require an affirmative confirmation.
- Never overwrite an input. Refuse output collisions by default; present
  auto-suffix only where that command supports it.
- Checksum “auto-fix” repairs only recognized checksums, validates the result,
  and writes a separate copy. Conversion-only in-memory repair is a different
  choice and must be described separately.
- After execution, show the command result and return to the task menu for
  another operation.
- Interactive dispatch uses the same in-process command handlers as direct
  invocation, not shell strings or helper executables.

## Main user journeys

### Convert an international Red/Blue save to FireRed/LeafGreen

Ask, one prompt at a time:

1. Source game: English Red, English Blue, or Japanese Red.
2. Save file: accept a full-line path or type Browse.
3. Target game: FireRed or LeafGreen for English saves.
4. Confirm the automatically selected new save path with YES.

Simple conversion names results beside the source with `_fr.sav` or `_lg.sav`,
adding a number if a result or its report already exists. It enables in-memory
checksum repair for English saves and explains this before confirmation.
The original stays unchanged. JSON, custom output paths, templates, and other
specialist choices remain in menu 2 and the complete command browser (menu 8).

Example: menu 1 → English Red → `Pkmn Red Eng.sav` → LeafGreen → YES
creates `Pkmn Red Eng_lg.sav` plus conversion reports.

### Convert Japanese Red

In simple mode, select Japanese Red, provide the save, then choose original
release (1.0) or revised release (1.1). FireRed is the only available target.
If the revision is unknown, ask the user to check it rather than guess. For
the current experimental route, describe Japanese-name
handling and evidence limitations. The planned Japanese Gen I route is distinct
and remains unavailable until implemented:

- Japanese original names/bytes are retained in the translated English-shaped
  JSON's provenance fields.
- English semantic name fields are separately populated for the destination.
- English Gen I `.sav` generation reads only those English semantic fields;
  original Japanese text remains in JSON and is not claimed to display in the
  English game.

Never show planned Japanese Green → Red/Blue conversion as available before
the Green-to-Gen-I route is implemented and tested. Green 1.0-to-FireRed now exists experimentally; see [Green support](JAPANESE_GREEN_IMPLEMENTATION.md).

### Inspect, validate, summarize, repair

Ask for game/profile (or a safe user-confirmed identification if a future
identifier exists), then operation and save file(s). Summaries show readable
trainer, progress, inventory, party, storage, and world information where
supported. Inspect explains file structure and checksum state. Validate reports
all relevant integrity checks. Repair writes a new validated copy. Batch mode
collects multiple files, previews the output directory, and reports outcomes
per file.

### Decode and work with JSON

Ask whether the user wants to decode a save, inspect/validate a JSON document,
generate a semantic save, reconstruct archived physical bytes, migrate a
schema, or batch a supported operation. Explain the key distinction before
running:

- **Generate:** build a save from semantic fields; archival `physicalImage` is
  not generation authority.
- **Reconstruct:** restore bytes from `physicalImage`; this is archival, not
  semantic generation.

Ask physical-image inclusion only on decode flows that support it. Preview
output names and report files before writing.

### Edit a save

Ask whether to start a new session or resume one. Guide the user to a supported
field editor, describe valid ranges/options, show staged edits and validation
warnings, and offer undo/history/annotations. On finalization, revalidate,
re-decode, show the edit summary, then write a new save and reports. Unsupported
fields remain unavailable with an explanation; never offer arbitrary byte
editing as a normal path.

### Compare saves or progress

Ask which comparison goal fits:

- Progress since an older backup: two saves from the same playthrough; explain
  changed badges, events, party, inventory, location, and other supported
  fields.
- Physical comparison: byte changes, ranges, and hashes.
- Semantic comparison: meaningful JSON field differences.
- FireRed domain comparison: Pokémon, events, trainers, items, Fly, Hall of
  Fame, or progress where supported.
- Bridge comparison: source Red JSON against target FireRed JSON and optional
  manifest.

Ask for the relevant paths, explain same-playthrough assumptions, and preview
report destinations. Never present byte equality as semantic equality.

### Proof and emulator checks

Guide proof creation, continuation after manual emulator testing, and proof
verification. State which checks are automated and which require a human to run
the save in an emulator. Never claim the emulator gate passed merely because a
proof command completed.

### Browse every command

The command browser is the universal fallback and advanced-user bridge:

1. Search/filter by game, task, keyword, or command name.
2. Open a result to see its purpose, full direct syntax, and current status.
3. Start its typed questions, including relevant advanced options.
4. Review the exact command and output destinations before a write.
5. Return to the browser or main menu when finished.

Do not derive argument prompts by parsing help strings. Each command needs typed
argument/option metadata and a dispatch route. The browser is generated from
the live compiled catalog; planned features are shown in a separate roadmap
view and cannot be launched.

## Current command catalog: how each group becomes interactive

The endpoints below are the v3.0 catalog, grouped by their guided flows. For
full direct syntax and descriptions, use [ALL_COMMANDS.md](ALL_COMMANDS.md)
or run `pkmn get-all-cmds`. Common choices appear first; specialist flags
appear under Advanced options.

### General and discovery

| Current endpoints | Interactive treatment |
|---|---|
| `doctor`, `completion`, `config show`, `get-all-cmds`, `interactive` | Offer diagnostics and configuration as read-only tasks; ask which shell before generating completions and explain installation steps; let users display/save/search the catalog; `interactive` is the entry point, not a nested action. |

### Red save inspection, validation, decode, and conversion

| Current endpoints | Interactive treatment |
|---|---|
| `red summary`, `red inspect`, `red validate`, `red repair-checksums`, `red decode` | Ask for one save, then requested report format/image inclusion/output. Explain validation results; repair creates a copy; decode previews JSON path and image policy. |
| `red events list`, `red events search`, `red events show` | Ask to browse by category, search text, or enter/select an event; show definition and known meaning. |
| `red validate-batch`, `red decode-batch` | Pick multiple files or a folder, select output directory/image option, preflight collisions, and show per-file results. |
| `red validate-post-emulator` | Ask for before/after saves and report directory; explain checksum and classified drift results. |
| `red convert`, `convert red-firered`, `convert red-leafgreen`, `convert blue-firered`, `convert blue-leafgreen`, `blue convert`, `rjson convert`, `bjson convert`, `rjson convert_to_frjson`, `rjson convert_to_lgjson`, `bjson convert_to_frjson`, `bjson convert_to_lgjson` | Route through the conversion-first wizard: source/profile, target, file, advanced mapping/template/salt options, checksum policy, preview, confirm. Show evidence status per route. |

### Japanese Red and Japanese archive workflows

| Current endpoints | Interactive treatment |
|---|---|
| `red-jp validate`, `red-jp decode`, `red-jp convert` | Clearly mark experimental; ask for explicit JP revision, path, output, and any optional FireRed template/salt/name policy; summarize which Japanese text is retained, transformed, or unverified. |
| `rjpjson inspect`, `rjpjson validate`, `rjpjson reconstruct`, `rjpjson project`, `rjpjson compare` | Ask for archive/projection JSON; explain archival reconstruction versus semantic projection; preview generated outputs and show source-to-projection field/name mappings. |

### Red edit sessions

| Current endpoints | Interactive treatment |
|---|---|
| `red edit`, `red begin-edit`, `red edit-session`, `red pokemon`, `red bag`, `red progress` | Start or resume an edit session, ask which supported area, collect validated values, and stage changes without writing over the source. |
| `red pending-edits`, `red undo-edit`, `red edit-history`, `red annotate-edit`, `red validate-edit`, `red end-edit` | Display staged changes, ask which/how many to undo, collect notes, run validation, or review and publish a new copy plus reports. |

### Red JSON

| Current endpoints | Interactive treatment |
|---|---|
| `rjson inspect`, `rjson validate`, `rjson schema`, `rjson migrate`, `rjson update_schema` | Ask for JSON/profile/desired output; explain warnings and migration changes; schema displays the current fields and constraints in the selected format. |
| `rjson generate`, `rjson reconstruct` | Ask for JSON and output path; explain semantic generation versus exact archival reconstruction; validate destination and collision state before writing. |
| `rjson generate-batch` | Select JSON inputs and output folder, preview all destinations, then report each generated save. |

### FireRed saves, JSON, edits, and events

| Current endpoints | Interactive treatment |
|---|---|
| `fred summary`, `fred inspect`, `fred validate`, `fred decode`, `fred repair-checksums` | Ask for save and compact/detailed/report options; checksum repair writes a new copy. |
| `fred validate-batch`, `fred decode-batch`, `fred validate-post-emulator` | Collect one or more saves, before/after pair, or output directory; preflight outputs and show results. |
| `frjson inspect`, `frjson validate`, `frjson schema`, `frjson update_schema`, `frjson migrate` | Guide JSON inspection, profile/schema validation, and copy-first migration. Label native versus planned/deferred fields. |
| `frjson generate`, `frjson reconstruct`, `frjson generate-batch` | Explain template-backed semantic generation versus physical reconstruction; ask for template/output/batch options and confirm. |
| `fred edit`, `fred begin-edit`, `fred edit-session`, `fred pokemon`, `fred bag`, `fred progress` | Guide the currently supported narrow FireRed edits, collect typed values, and stage them in a session. |
| `fred pending-edits`, `fred undo-edit`, `fred edit-history`, `fred annotate-edit`, `fred validate-edit`, `fred end-edit` | Review, undo, annotate, validate, then publish a validated copy after a final summary. |
| `fred events list`, `fred events search`, `fred events show` | Ask flags versus variables, then browse/search/select pinned authority records. |

### Blue and LeafGreen

| Current endpoints | Interactive treatment |
|---|---|
| `blue decode`, `blue convert` | Ask for source and output; `blue convert` defaults to its paired route but lets the user choose an available target where supported. |
| `leafgreen decode`, `leafgreen validate` | Ask for save, output, and report format; explain save-container validation. |
| `lgjson generate`, `lgjson validate` | Ask for JSON, optional validated template/output, then generate or report semantic validation. |

### Conversion support and batch

| Current endpoints | Interactive treatment |
|---|---|
| `convert routes`, `convert inspect`, `convert explain`, `convert validate-manifest`, `convert batch` | Browse route/evidence matrix; search mapping authority; explain a selected mapping; select a manifest for validation; or choose route, multiple inputs, template, and output folder for batch conversion. |
| `proof convert` | Ask for route, source JSON, and proof directory; enumerate artifacts and explain the proof's limits before running. |

### Compare and proof

| Current endpoints | Interactive treatment |
|---|---|
| `compare progress`, `compare physical`, `compare semantic`, `compare semantic-batch` | Ask which comparison goal, collect two or more files, choose report outputs, preview, then explain result categories. |
| `compare firered-semantic`, `compare firered-progress`, `compare firered-pokemon`, `compare firered-events`, `compare firered-trainers`, `compare firered-items`, `compare firered-fly`, `compare firered-hall-of-fame`, `compare bridge` | Offer named comparison domain, collect the appropriate JSON/save pair and optional manifest, then show readable differences and report paths. |
| `proof red`, `proof post-emulator`, `proof verify`, `proof fred`, `proof red-to-firered` | Ask for inputs, proof directory/ZIP/template options as supported; clearly separate automated validation from manual emulator acceptance. |

## Future planned commands: interactive treatment

These proposals live in [FUTURE_COMMANDS.md](FUTURE_COMMANDS.md); none should be
presented as a runnable current command until implemented and tested. Once
available, the same interactive registry should add guided prompts for them.

| Planned command(s) | Guided interaction when implemented |
|---|---|
| `red-jp convert-gen1`, `green-jp convert-gen1` | Ask source game/revision, English Red or Blue target, save path, English-name policy, and JSON output. Preserve Japanese original names/bytes in separate JSON provenance fields; generation reads English semantic names only. Explain that the English game does not display Japanese names. |
| Green 1.1 support | Await source-layout verification. Green 1.0 validation, decoding, archive tools, and FireRed conversion are now available experimentally in menus 1, 2, and 8. |
| `convert plan`, conversion `--dry-run`, `verify-conversion`, expanded `convert explain` | Collect route/source or source/target, show mapping/evidence/warnings without writing during planning, then verify generated target against source and optional manifest. |
| `identify`, `profiles` | Inspect a save and display candidate profiles with confidence/evidence; user confirms ambiguous profiles. Browse source/target support and evidence labels. |
| `json names`, `json provenance`, `json export-report` | Ask for JSON and show original versus generation-name fields, source/mapping history, or a preview of retained/translated/omitted fields. |
| unified `validate-batch`, enhanced `convert batch`, `batch report` | Select files/folder, choose confirmed profile/route, preflight all outputs, execute supported jobs, and explain successes/failures by input. |
| `json diff`, `save info` | Collect documents/saves, choose semantic/physical image inclusion behavior, confirm profile if needed, and display a readable report. |
| `outputs list`, `self-test`, `interactive --search` | Browse prior generated artifacts read-only, run a quick local health check, or open the current command browser with a search already applied. The browser itself is available now through menu 8 and `S` search. |

## Implementation and maintenance record

The interactive shell uses typed questions, path handling, Back/Help/Quit,
review, and confirmation around the same in-process handlers used by direct
commands. Its workflow registry is checked against the compiled catalog so a
new endpoint needs a guided descriptor. Prompt metadata is explicit rather
than inferred from display usage strings. Future commands enter the browser
only after their handlers and capability status are implemented.

The acceptance checks below remain the release and maintenance criteria for
new commands and changes to existing workflows.

## Acceptance criteria for full interactive coverage

- Every currently supported command endpoint and meaningful option is
  searchable and usable through `pkmn interactive`.
- The leading conversion journey is unmistakable and asks questions
  conversationally rather than demanding a typed command.
- Japanese Gen I provenance and English save-generation fields are kept
  separate as specified in the roadmap.
- File paths with spaces, quotes, Unicode, relative paths, and pasted paths are
  handled; missing paths lead to recovery prompts.
- Back/help/quit work on all screens; cancellation never writes files.
- Every write has a review, confirmation, collision check, and no-overwrite
  behavior.
- Experimental and community-test evidence is visible at the choice and review
  stages; planned features cannot be accidentally launched.
- Automated tests cover menu transcripts, every command workflow, path entry,
  cancellation, output safety, and direct/interactive parity on supported OSes.
