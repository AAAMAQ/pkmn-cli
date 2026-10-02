# Interactive examples for every pkmn command

The first menu option now offers **simple save conversion**. For example:

```text
Choose a task: 1
Which game is your save from? 1 (Red, English)
Choose your save file: Pkmn Red Eng.sav
Which game would you like to play it in? 2 (LeafGreen)
New save: Pkmn Red Eng_lg.sav
Type YES to convert: YES
```

It chooses the output name and repairs English save checksums for conversion
when needed, leaving the original untouched. Japanese Red is source choice 3;
it currently targets FireRed only and asks for release 1.0 or 1.1. This route
remains experimental. The detailed command conversations below are available
through menu 8; advanced conversion and Japanese tools are also in menu 2.

This document gives example conversations for every endpoint reported by the
`pkmn 3.1.0 get-all-cmds` catalog, plus a separate future roadmap. The 124
current endpoints have guided paths in `pkmn interactive`; `interactive`
itself is the session entry point. These are illustrative choices and file
names, not literal transcripts or a promise that every result has the same
evidence level. Command maturity and route evidence follow the current
documentation.

Every filename below is fictional. Names deliberately include spaces:

| Example | Meaning |
|---|---|
| `Pkmn Red Eng.sav`, `Pkmn Blue Eng.sav` | International Gen I saves |
| `Pkmn FR Eng.sav`, `Pkmn LG Eng.sav` | FireRed and LeafGreen saves |
| `Pkmn Red JP.sav`, `Pkmn Green JP.sav` | Japanese Gen I saves |
| `Pkmn Red Eng.red.json`, `Pkmn Blue Eng.blue.json` | Decoded Gen I JSON |
| `Pkmn FR Eng.fred.json`, `Pkmn LG Eng.lg.json` | Decoded remake JSON |
| `Pkmn Red JP.red.jp.json`, `Pkmn Green JP.green.jp.json` | Japanese source archives |
| `Pkmn Red Edit.session.json`, `Pkmn FR Edit.session.json` | Edit sessions |

At an interactive path question, the user can paste `Pkmn Red Eng.sav`,
`"Pkmn Red Eng.sav"`, or a full path with spaces. The program reads the whole
line as one path and verifies that the file exists. In this document, arrows
show a short prompt and sample answer; the text after `⇒` is the expected
kind of result. Exact output names should be previewed before writing.

## What one complete conversation looks like

```text
$ pkmn interactive
Choose a task: 1
Source game? 1 Red  2 Blue
Choice: 1
Target game? 1 FireRed  2 LeafGreen
Choice: 1
Route evidence: EMULATOR_VERIFIED
Red save or canonical Red JSON source?
Answer: "Pkmn Red Eng.sav"
Create a proposed JSON plan instead of a save?
Answer: No
Output FireRed save, or proposed JSON if planning?
Answer: Pkmn Red Eng to FR.sav
Review advanced options? No
Review: convert red-firered
Proposed outputs: save, conversion manifest, Markdown report
Run and write these outputs? Type YES: YES
Task complete. Next: main menu / browse / repeat / inspect / compare.
```

Every workflow should follow the same pattern: choose → answer one question
at a time → review → confirm → see results. `?` explains the current prompt,
`B` goes back, and `Q` cancels. The current guided shell asks for `YES` even
for a read-only command, so an empty answer never starts work.

## Current catalog: general commands (5)

- **`pkmn doctor`** — “Standard or deep check?” → `Deep`; “Text or JSON?” → `Text` ⇒ internal readiness and self test result, with no save input.
- **`pkmn completion`** — “Which shell?” → `zsh` ⇒ completion source and instructions for installing it; do not run shell setup automatically.
- **`pkmn config show`** — “Text or JSON?” → `Text` ⇒ current compiled safety/default policy.
- **`pkmn get-all-cmds`** — “Format?” → `Markdown`; “Output?” → `Pkmn Commands.md` ⇒ a catalog file after collision review. Leave output blank to display it in the terminal.
- **`pkmn interactive`** — Launching it opens the main task menu shown above. It is the session entry point; selecting “Main menu” later returns there.

## Current catalog: Red saves, event lookup, and conversion (12)

- **`pkmn red summary`** — “Save?” → `Pkmn Red Eng.sav`; “Short text, detailed Markdown, or JSON?” → `Markdown`; “Save report?” → `Pkmn Red Eng Summary.md` ⇒ readable trainer, progress, party, and inventory summary.
- **`pkmn red inspect`** — “Save?” → `Pkmn Red Eng.sav`; “Text or JSON?” → `Text` ⇒ file size and checksum overview.
- **`pkmn red validate`** — “Save?” → `Pkmn Red Eng.sav`; “Text or JSON?” → `Text` ⇒ main, bank, and box validation results.
- **`pkmn red repair-checksums`** — “Save?” → `Pkmn Red Eng.sav`; “Repaired copy?” → `Pkmn Red Eng Repaired.sav`; “Write copy?” → `Yes` ⇒ validated repaired copy and repair report, source unchanged.
- **`pkmn red decode`** — “Save?” → `Pkmn Red Eng.sav`; “Keep archival physical image?” → `No`; “JSON output?” → `Pkmn Red Eng.red.json`; “Write?” → `Yes` ⇒ canonical Red JSON.
- **`pkmn red events list`** — “Which category?” → `Gym`; “Text or JSON?” → `Text` ⇒ list of verified named gym event flags.
- **`pkmn red events search`** — “Search term?” → `VIRIDIAN_GYM`; “Text or JSON?” → `Text` ⇒ matching named event flags.
- **`pkmn red events show`** — “Event name?” → `EVENT_BEAT_VIRIDIAN_GYM_GIOVANNI` ⇒ verified event definition.
- **`pkmn red validate-batch`** — “Add saves?” → `Pkmn Red Eng.sav`, `Pkmn Red Eng Later.sav`; “Done adding?” → `Yes` ⇒ validation result per save.
- **`pkmn red decode-batch`** — “Add saves?” → `Pkmn Red Eng.sav`, `Pkmn Red Eng Later.sav`; “Output folder?” → `Red Decoded`; “Include physical images?” → `No`; “Run?” → `Yes` ⇒ two Red JSON files in the selected folder.
- **`pkmn red validate-post-emulator`** — “Before save?” → `Pkmn Red Eng.sav`; “After save?” → `Pkmn Red Eng After Emulator.sav`; “Report folder?” → `Red Emulator Check` ⇒ checksum and expected/unexpected drift report.
- **`pkmn red convert`** — “Red save?” → `Pkmn Red Eng.sav`; “Output FireRed save?” → `Pkmn Red Eng to FR.sav`; “Repair checksums in memory if needed?” → `Ask me`; “Run?” → `Yes` ⇒ FireRed save, manifest, and report.

## Current catalog: Japanese Red and Japanese archive (8)

These current endpoints are experimental. Every wizard must show its evidence
status and ask for a Japanese revision/profile when the command requires one.

- **`pkmn red-jp validate`** — “Japanese Red save?” → `Pkmn Red JP.sav`; “Revision?” → `JP_RED_REV0`; “Text or JSON?” → `Text` ⇒ Japanese layout and main checksum results.
- **`pkmn red-jp decode`** — “Japanese Red save?” → `Pkmn Red JP.sav`; “Revision?” → `JP_RED_REV0`; “Archive output?” → `Pkmn Red JP.red.jp.json`; “Include physical image?” → `Yes` ⇒ archive with exact Japanese name bytes.
- **`pkmn red-jp convert`** — “Japanese Red save?” → `Pkmn Red JP.sav`; “Revision?” → `JP_RED_REV0`; “Target?” → `International FireRed`; “Retain experimental player name?” → `No`; “Output?” → `Pkmn Red JP to FR.sav`; “Run?” → `Yes` ⇒ FireRed output with archive/projection and evidence report.
- **`pkmn rjpjson inspect`** — “Japanese archive?” → `Pkmn Red JP.red.jp.json` ⇒ source profile, sections, and physical image presence.
- **`pkmn rjpjson validate`** — “Japanese archive?” → `Pkmn Red JP.red.jp.json` ⇒ schema, source byte provenance, and checksum diagnostics.
- **`pkmn rjpjson reconstruct`** — “Japanese archive?” → `Pkmn Red JP.red.jp.json`; “Restored save?” → `Pkmn Red JP Restored.sav`; “Restore?” → `Yes` ⇒ exact archival save bytes when the physical image is present.
- **`pkmn rjpjson project`** — “Japanese archive?” → `Pkmn Red JP.red.jp.json`; “Use experimental player name policy?” → `No`; “Projection output?” → `Pkmn Red JP Projected.red.json` ⇒ English shaped Red JSON with Japanese name provenance.
- **`pkmn rjpjson compare`** — “Japanese archive?” → `Pkmn Red JP.red.jp.json`; “Projection?” → `Pkmn Red JP Projected.red.json` ⇒ mapping and provenance comparison.

## Current catalog: Red editing (12)

- **`pkmn red edit`** — “Save to edit?” → `Pkmn Red Eng.sav`; “What would you change?” → `Money`; “New amount?” → `5000`; “Output copy?” → `Pkmn Red Eng Edited.sav` ⇒ validated edited copy.
- **`pkmn red begin-edit`** — “Source save?” → `Pkmn Red Eng.sav`; “Session file?” → `Pkmn Red Edit.session.json` ⇒ a new edit session linked to the source hash.
- **`pkmn red edit-session`** — “Session?” → `Pkmn Red Edit.session.json`; “Choose field and value” → `Money: 5000`; “Preview or stage?” → `Stage` ⇒ pending semantic edit.
- **`pkmn red pokemon`** — “Session?” → `Pkmn Red Edit.session.json`; “Party, species, or nickname?” → `Party`; “Slot?” → `1`; “Action?” → `Rename`; “Name?” → `SPARK` ⇒ staged Pokémon change.
- **`pkmn red bag`** — “Session?” → `Pkmn Red Edit.session.json`; “Add or remove?” → `Add`; “Item?” → `POTION`; “Quantity?” → `5` ⇒ staged bag change.
- **`pkmn red progress`** — “Session?” → `Pkmn Red Edit.session.json`; “Verified preset?” → `All Fly destinations`; “Preview?” → `Yes` ⇒ preview/staged supported progress preset.
- **`pkmn red pending-edits`** — “Session?” → `Pkmn Red Edit.session.json`; “Text or JSON?” → `Text` ⇒ list of staged edits.
- **`pkmn red undo-edit`** — “Session?” → `Pkmn Red Edit.session.json`; “How many recent edits?” → `1`; “Undo?” → `Yes` ⇒ latest staged edit removed.
- **`pkmn red edit-history`** — “Session?” → `Pkmn Red Edit.session.json`; “Text or JSON?” → `Text` ⇒ edit history and annotations.
- **`pkmn red annotate-edit`** — “Session?” → `Pkmn Red Edit.session.json`; “Note?” → `Preparing Brock comparison` ⇒ annotation attached to session.
- **`pkmn red validate-edit`** — “Session?” → `Pkmn Red Edit.session.json` ⇒ generation/checksum/redecode validation without publishing a save.
- **`pkmn red end-edit`** — “Session?” → `Pkmn Red Edit.session.json`; “Output?” → `Pkmn Red Eng Edited.sav`; “Dry run?” → `No`; “Run?” → `Yes` ⇒ validated save copy and reports. Choose dry run for a validation preview without publishing.

## Current catalog: Red JSON (11)

- **`pkmn rjson inspect`** — “Red JSON?” → `Pkmn Red Eng.red.json`; “Text or JSON report?” → `Text` ⇒ schema, trainer summary, and physical image presence.
- **`pkmn rjson validate`** — “Red JSON?” → `Pkmn Red Eng.red.json`; “Validation profile?” → `Generation`; “Text or JSON?” → `Text` ⇒ schema and semantic validation results.
- **`pkmn rjson generate`** — “Red JSON?” → `Pkmn Red Eng.red.json`; “Generated save?” → `Pkmn Red Eng Generated.sav`; “Generate?” → `Yes` ⇒ semantic Red save and generation reports.
- **`pkmn rjson reconstruct`** — “Red JSON with physical image?” → `Pkmn Red Eng Archive.red.json`; “Restored save?” → `Pkmn Red Eng Restored.sav`; “Restore?” → `Yes` ⇒ archival source byte reconstruction.
- **`pkmn rjson migrate`** — “Red JSON?” → `Pkmn Red Eng.red.json`; “Migrated JSON?” → `Pkmn Red Eng Migrated.red.json`; “Write?” → `Yes` ⇒ compatible enriched JSON copy.
- **`pkmn rjson schema`** — “Text or JSON schema?” → `Text` ⇒ schema fields and constraints.
- **`pkmn rjson generate-batch`** — “Add JSON files?” → `Pkmn Red Eng.red.json`, `Pkmn Red Eng Later.red.json`; “Output folder?” → `Red Generated`; “Run?” → `Yes` ⇒ generated save for each input.
- **`pkmn rjson convert`** — “Red JSON?” → `Pkmn Red Eng.red.json`; “Output FireRed save?” → `Pkmn Red JSON to FR.sav`; “Advanced template/salt options?” → `Defaults`; “Convert?” → `Yes` ⇒ FireRed save, manifest, and report.
- **`pkmn rjson convert_to_frjson`** — “Red JSON?” → `Pkmn Red Eng.red.json`; “FireRed JSON output?” → `Pkmn Red Eng Proposed.fred.json`; “Translate?” → `Yes` ⇒ proposed FireRed semantic JSON.
- **`pkmn rjson convert_to_lgjson`** — “Red JSON?” → `Pkmn Red Eng.red.json`; “LeafGreen JSON output?” → `Pkmn Red Eng Proposed.lg.json`; “Translate?” → `Yes` ⇒ proposed LeafGreen semantic JSON.
- **`pkmn rjson update_schema`** — “Red JSON?” → `Pkmn Red Eng.red.json`; “Updated copy?” → `Pkmn Red Eng Updated.red.json`; “Migrate?” → `Yes` ⇒ latest supported schema copy.

## Current catalog: FireRed JSON (8)

- **`pkmn frjson inspect`** — “FireRed JSON?” → `Pkmn FR Eng.fred.json` ⇒ metadata and supported content summary.
- **`pkmn frjson validate`** — “FireRed JSON?” → `Pkmn FR Eng.fred.json` ⇒ schema and semantic validation results.
- **`pkmn frjson schema`** — “Text or JSON schema?” → `Text`; “Section?” → `Pokémon` ⇒ current native/deferred schema details.
- **`pkmn frjson update_schema`** — “FireRed JSON?” → `Pkmn FR Eng.fred.json`; “Updated copy?” → `Pkmn FR Eng Updated.fred.json` ⇒ latest supported schema copy.
- **`pkmn frjson generate`** — “FireRed JSON?” → `Pkmn FR Eng.fred.json`; “Use bundled template?” → `Yes`; “Output?” → `Pkmn FR Eng Generated.sav`; “Generate?” → `Yes` ⇒ template backed FireRed save.
- **`pkmn frjson reconstruct`** — “FireRed JSON with physical image?” → `Pkmn FR Eng Archive.fred.json`; “Restored save?” → `Pkmn FR Eng Restored.sav`; “Restore?” → `Yes` ⇒ archived byte reconstruction.
- **`pkmn frjson migrate`** — “FireRed JSON?” → `Pkmn FR Eng.fred.json`; “Migrated copy?” → `Pkmn FR Eng Migrated.fred.json` ⇒ enriched JSON copy.
- **`pkmn frjson generate-batch`** — “Add FireRed JSON files?” → `Pkmn FR Eng.fred.json`, `Pkmn FR Eng Later.fred.json`; “Output folder?” → `FR Generated`; “Template?” → `Bundled`; “Run?” → `Yes` ⇒ multiple generated saves.

## Current catalog: FireRed saves and events (11)

- **`pkmn fred summary`** — “FireRed save?” → `Pkmn FR Eng.sav`; “Compact or detailed?” → `Detailed` ⇒ readable FireRed state summary.
- **`pkmn fred inspect`** — “FireRed save?” → `Pkmn FR Eng.sav`; “Text or JSON?” → `Text` ⇒ active slot, sector, and checksum overview.
- **`pkmn fred validate`** — “FireRed save?” → `Pkmn FR Eng.sav`; “Text or JSON?” → `Text` ⇒ sector and checksum validation.
- **`pkmn fred decode`** — “FireRed save?” → `Pkmn FR Eng.sav`; “JSON output?” → `Pkmn FR Eng.fred.json`; “Decode?” → `Yes` ⇒ native FireRed JSON.
- **`pkmn fred repair-checksums`** — “FireRed save?” → `Pkmn FR Eng.sav`; “Repaired copy?” → `Pkmn FR Eng Repaired.sav`; “Write?” → `Yes` ⇒ validated repaired copy.
- **`pkmn fred validate-batch`** — “Add saves?” → `Pkmn FR Eng.sav`, `Pkmn FR Eng Later.sav`; “Text or JSON?” → `Text` ⇒ per save validation.
- **`pkmn fred decode-batch`** — “Add saves?” → `Pkmn FR Eng.sav`, `Pkmn FR Eng Later.sav`; “Output folder?” → `FR Decoded`; “Run?” → `Yes` ⇒ FireRed JSON files in the folder.
- **`pkmn fred validate-post-emulator`** — “Before?” → `Pkmn FR Eng.sav`; “After?” → `Pkmn FR Eng After Emulator.sav`; “Report folder?” → `FR Emulator Check` ⇒ save/reload drift analysis.
- **`pkmn fred events list`** — “Flags or variables?” → `Flags`; “Text or JSON?” → `Text` ⇒ pinned authority list.
- **`pkmn fred events search`** — “Flags or variables?” → `Flags`; “Search?” → `VIRIDIAN` ⇒ matching FireRed records.
- **`pkmn fred events show`** — “Flag or variable?” → `Flag`; “Name or ID?” → `FLAG_BADGE01_GET` ⇒ one pinned definition when present.

## Current catalog: FireRed editing (12)

- **`pkmn fred edit`** — “FireRed save?” → `Pkmn FR Eng.sav`; “Supported edit?” → `Money`; “New value?” → `5000`; “Output?” → `Pkmn FR Eng Edited.sav` ⇒ validated copy.
- **`pkmn fred begin-edit`** — “FireRed save?” → `Pkmn FR Eng.sav`; “Session?” → `Pkmn FR Edit.session.json` ⇒ new edit session.
- **`pkmn fred edit-session`** — “Session?” → `Pkmn FR Edit.session.json`; “Field?” → `Coins`; “New value?” → `100`; “Stage?” → `Yes` ⇒ pending edit.
- **`pkmn fred pokemon`** — “Session?” → `Pkmn FR Edit.session.json`; “Party slot?” → `1`; “New nickname?” → `SPARK` ⇒ staged rename.
- **`pkmn fred bag`** — “Session?” → `Pkmn FR Edit.session.json`; “Pocket/slot?” → `Items / 1`; “Existing item ID?” → `20`; “New quantity?” → `5` ⇒ staged verified stack quantity.
- **`pkmn fred progress`** — “Session?” → `Pkmn FR Edit.session.json`; “Badge?” → `1`; “Set on or off?” → `On` ⇒ staged badge edit.
- **`pkmn fred pending-edits`** — “Session?” → `Pkmn FR Edit.session.json` ⇒ current staged changes.
- **`pkmn fred undo-edit`** — “Session?” → `Pkmn FR Edit.session.json`; “How many recent edits?” → `1`; “Undo?” → `Yes` ⇒ newest staged edit removed.
- **`pkmn fred edit-history`** — “Session?” → `Pkmn FR Edit.session.json` ⇒ edit history.
- **`pkmn fred annotate-edit`** — “Session?” → `Pkmn FR Edit.session.json`; “Note?” → `Before Elite Four` ⇒ note attached.
- **`pkmn fred validate-edit`** — “Session?” → `Pkmn FR Edit.session.json` ⇒ validation without publishing.
- **`pkmn fred end-edit`** — “Session?” → `Pkmn FR Edit.session.json`; “Output?” → `Pkmn FR Eng Edited.sav`; “Publish?” → `Yes` ⇒ validated edited save copy.

## Current catalog: paired conversion routes and Blue/LeafGreen (13)

- **`pkmn convert red-firered`** — “Source?” → `Pkmn Red Eng.sav`; “Target?” → `FireRed`; “Output?” → `Pkmn Red Eng to FR.sav`; “Run?” → `Yes` ⇒ converted save and audit artifacts.
- **`pkmn convert red-leafgreen`** — “Source?” → `Pkmn Red Eng.sav`; “Target?” → `LeafGreen`; “Output?” → `Pkmn Red Eng to LG.sav`; “Run?” → `Yes` ⇒ converted save with community testing status shown.
- **`pkmn convert blue-firered`** — “Source?” → `Pkmn Blue Eng.sav`; “Target?” → `FireRed`; “Output?” → `Pkmn Blue Eng to FR.sav`; “Run?” → `Yes` ⇒ converted save with evidence status shown.
- **`pkmn convert blue-leafgreen`** — “Source?” → `Pkmn Blue Eng.sav`; “Target?” → `LeafGreen`; “Output?” → `Pkmn Blue Eng to LG.sav`; “Run?” → `Yes` ⇒ converted save with evidence status shown.
- **`pkmn blue decode`** — “Blue save?” → `Pkmn Blue Eng.sav`; “JSON output?” → `Pkmn Blue Eng.blue.json`; “Decode?” → `Yes` ⇒ canonical Blue JSON.
- **`pkmn blue convert`** — “Blue save?” → `Pkmn Blue Eng.sav`; “Paired LeafGreen destination?” → `Yes`; “Output?” → `Pkmn Blue Eng to LG.sav` ⇒ LeafGreen save and reports.
- **`pkmn bjson convert`** — “Blue JSON?” → `Pkmn Blue Eng.blue.json`; “Output LeafGreen save?” → `Pkmn Blue JSON to LG.sav` ⇒ generated target save and report.
- **`pkmn bjson convert_to_lgjson`** — “Blue JSON?” → `Pkmn Blue Eng.blue.json`; “Projected JSON?” → `Pkmn Blue Eng Proposed.lg.json` ⇒ LeafGreen planning document.
- **`pkmn bjson convert_to_frjson`** — “Blue JSON?” → `Pkmn Blue Eng.blue.json`; “Projected JSON?” → `Pkmn Blue Eng Proposed.fred.json` ⇒ FireRed planning document.
- **`pkmn leafgreen decode`** — “LeafGreen save?” → `Pkmn LG Eng.sav`; “JSON output?” → `Pkmn LG Eng.lg.json` ⇒ LeafGreen JSON.
- **`pkmn leafgreen validate`** — “LeafGreen save?” → `Pkmn LG Eng.sav`; “Text or JSON?” → `Text` ⇒ container and checksum validation.
- **`pkmn lgjson generate`** — “LeafGreen JSON?” → `Pkmn LG Eng.lg.json`; “Template?” → `Bundled`; “Output?” → `Pkmn LG Eng Generated.sav`; “Generate?” → `Yes` ⇒ LeafGreen save.
- **`pkmn lgjson validate`** — “LeafGreen JSON?” → `Pkmn LG Eng.lg.json` ⇒ schema and semantic validation.

## Current catalog: route information and batch conversion (6)

- **`pkmn convert routes`** — “List all routes or filter by game?” → `All`; “Text or JSON?” → `Text` ⇒ capabilities and evidence labels.
- **`pkmn convert inspect`** — “Inspect event, trainer, or item?” → `Item`; “Optional query?” → `POTION` ⇒ matching pinned mapping records.
- **`pkmn convert explain`** — “Explain event, trainer, or item?” → `Item`; “Which?” → `POTION` ⇒ reason and source evidence for its conversion rule.
- **`pkmn convert validate-manifest`** — “Manifest?” → `Pkmn Red Eng to FR.conversion-manifest.json` ⇒ manifest structure and audit validation.
- **`pkmn convert batch`** — “Route?” → `red-firered`; “Add sources?” → `Pkmn Red Eng.sav`, `Pkmn Red Eng Later.sav`; “Output folder?” → `Red Batch to FR`; “Run?” → `Yes` ⇒ converted saves and per source audit artifacts.
- **`pkmn proof convert`** — “Route?” → `blue-leafgreen`; “Source JSON?” → `Pkmn Blue Eng.blue.json`; “Proof folder?” → `Blue to LG Proof` ⇒ static proof package with stated limits.

## Current catalog: comparison (13)

- **`pkmn compare progress`** — “Older save?” → `Pkmn Red Eng.sav`; “Newer save?” → `Pkmn Red Eng After Brock.sav`; “Report?” → `Red Progress.md` ⇒ readable changes in badges, inventory, party, events, and other supported progress.
- **`pkmn compare physical`** — “First save?” → `Pkmn Red Eng.sav`; “Second save?” → `Pkmn Red Eng After Brock.sav`; “Report?” → `Red Physical Compare.json` ⇒ byte counts, ranges, and hashes.
- **`pkmn compare semantic`** — “First Red JSON?” → `Pkmn Red Eng.red.json`; “Second?” → `Pkmn Red Eng After Brock.red.json`; “Report?” → `Red Semantic Compare.md` ⇒ field aware differences.
- **`pkmn compare semantic-batch`** — “Baseline?” → `Pkmn Red Eng.red.json`; “Add candidates?” → `Pkmn Red Eng After Brock.red.json`, `Pkmn Red Eng Later.red.json` ⇒ each candidate compared with the baseline.
- **`pkmn compare firered-semantic`** — “First FireRed JSON?” → `Pkmn FR Eng.fred.json`; “Second?” → `Pkmn FR Eng Later.fred.json`; “JSON report?” → `FR Semantic Compare.json` ⇒ semantic differences excluding archival bytes.
- **`pkmn compare firered-progress`** — “Older FireRed save?” → `Pkmn FR Eng.sav`; “Newer?” → `Pkmn FR Eng Later.sav`; “JSON report?” → `FR Progress.json` ⇒ decoded progress changes.
- **`pkmn compare firered-pokemon`** — “First FireRed JSON?” → `Pkmn FR Eng.fred.json`; “Second?” → `Pkmn FR Eng Later.fred.json` ⇒ party, PC, and Daycare differences.
- **`pkmn compare firered-events`** — “First FireRed JSON?” → `Pkmn FR Eng.fred.json`; “Second?” → `Pkmn FR Eng Later.fred.json` ⇒ event/variable differences.
- **`pkmn compare firered-trainers`** — “First FireRed JSON?” → `Pkmn FR Eng.fred.json`; “Second?” → `Pkmn FR Eng Later.fred.json` ⇒ trainer defeat differences.
- **`pkmn compare firered-items`** — “First FireRed JSON?” → `Pkmn FR Eng.fred.json`; “Second?” → `Pkmn FR Eng Later.fred.json` ⇒ inventory and obtained item differences.
- **`pkmn compare firered-fly`** — “First FireRed JSON?” → `Pkmn FR Eng.fred.json`; “Second?” → `Pkmn FR Eng Later.fred.json` ⇒ Fly destination changes.
- **`pkmn compare firered-hall-of-fame`** — “First FireRed JSON?” → `Pkmn FR Eng.fred.json`; “Second?” → `Pkmn FR Eng Later.fred.json` ⇒ Hall of Fame changes.
- **`pkmn compare bridge`** — “Red source JSON?” → `Pkmn Red Eng.red.json`; “FireRed target JSON?” → `Pkmn Red Eng Proposed.fred.json`; “Manifest?” → `Pkmn Red Eng to FR.conversion-manifest.json` ⇒ bridge mapping audit.

## Current catalog: proof (5)

- **`pkmn proof red`** — “Source Red save?” → `Pkmn Red Eng.sav`; “Proof folder?” → `Red Proof`; “Also make ZIP?” → `Yes`; “Run?” → `Yes` ⇒ decode/generate/compare proof artifacts and ZIP.
- **`pkmn proof post-emulator`** — “Before save?” → `Pkmn Red Eng Generated.sav`; “After emulator save?” → `Pkmn Red Eng After Emulator.sav`; “Existing proof folder?” → `Red Proof` ⇒ post emulator classification and updated reports.
- **`pkmn proof verify`** — “Proof folder or ZIP?” → `Red Proof.zip`; “Text or JSON?” → `Text` ⇒ hashes, schema, archive, and generated checksum verification.
- **`pkmn proof fred`** — “Complete FireRed JSON?” → `Pkmn FR Eng.fred.json`; “Template?” → `Bundled`; “Proof folder?” → `FR Proof`; “Run?” → `Yes` ⇒ native generation proof package.
- **`pkmn proof red-to-firered`** — “Red JSON?” → `Pkmn Red Eng.red.json`; “Template?” → `Bundled`; “Proof folder?” → `Red to FR Proof`; “Run?” → `Yes` ⇒ conversion proof and manual emulator checklist.

## Global direct forms

`pkmn --help` and `pkmn --version` are global forms outside the 124 endpoint
catalog. They remain direct terminal commands. The guided shell has contextual
`?` help; menu 9 offers `doctor`, configuration, completion, and command
catalog tasks. Global `--quiet`, `--verbose`, and `--no-color` are direct CLI
options, not session preferences. Guided safety prompts remain visible if
`pkmn --quiet interactive` is used.

## Planned commands: future interactive examples

These are proposals from [FUTURE_COMMANDS.md](FUTURE_COMMANDS.md). The exact
syntax and supported profiles may change. The current browser's roadmap labels
them **Planned**; they cannot be launched until handlers, tests, and evidence
exist.

### Japanese Gen I to international Gen I

- **`pkmn red-jp convert-gen1`** — “Japanese Red save?” → `Pkmn Red JP.sav`; “Revision?” → `JP_RED_REV0`; “English target?” → `Red`; “English name policy?” → `Use compatible aliases`; “Target JSON?” → `Pkmn Red JP to Eng.red.json`; “Save?” → `Pkmn Red JP to Eng.sav` ⇒ JSON keeps original Japanese names separately; generated save uses English names.
- **`pkmn green-jp convert-gen1`** — “Japanese Green save?” → `Pkmn Green JP.sav`; “Verified profile?” → `JP_GREEN_REV0`; “English target?” → `Blue`; “Target JSON?” → `Pkmn Green JP to Blue.blue.json`; “Save?” → `Pkmn Green JP to Blue.sav` ⇒ the same separate Japanese provenance and English generation fields.
- **Japanese Green 1.0 tools are now experimental runtime commands**, including `green-jp validate/decode/convert` and `gjpjson inspect/validate/reconstruct/project/compare`. See [the current Green guide](JAPANESE_GREEN_IMPLEMENTATION.md). Green 1.1 remains planned.
- **`pkmn green-jp decode`** — “Japanese Green save?” → `Pkmn Green JP.sav`; “Profile?” → `JP_GREEN_REV0`; “Archive?” → `Pkmn Green JP.green.jp.json` ⇒ lossless Japanese source archive.
- **`pkmn gjpjson inspect`** — “Green archive?” → `Pkmn Green JP.green.jp.json` ⇒ provenance and structure summary.
- **`pkmn gjpjson validate`** — “Green archive?” → `Pkmn Green JP.green.jp.json` ⇒ schema and byte provenance checks.
- **`pkmn gjpjson reconstruct`** — “Green archive?” → `Pkmn Green JP.green.jp.json`; “Restored save?” → `Pkmn Green JP Restored.sav` ⇒ exact archival bytes when present.

### Planning, profile discovery, and explanation

- **`pkmn convert plan`** — “Route?” → `red-firered`; “Source?” → `Pkmn Red Eng.sav`; “Show mapping preview?” → `Yes` ⇒ source/target profiles, warnings, and proposed outputs without writing.
- **Conversion `--dry-run`** — “Route?” → `blue-leafgreen`; “Source?” → `Pkmn Blue Eng.sav`; “Preview only?” → `Yes` ⇒ conversion plan without save output.
- **`pkmn verify-conversion`** — “Source?” → `Pkmn Red Eng.sav`; “Target?” → `Pkmn Red Eng to FR.sav`; “Manifest?” → `Pkmn Red Eng to FR.conversion-manifest.json` ⇒ checksums and mapped semantic verification.
- **Expanded `pkmn convert explain`** — “Which mapping field?” → `Badge`; “Which badge?” → `Boulder Badge` ⇒ source, target, and policy explanation; extends the existing event/trainer/item command.
- **`pkmn identify`** — “Save?” → `Pkmn Blue Eng.sav`; “Show confidence?” → `Yes` ⇒ candidate game, region, and revision profiles with evidence.
- **`pkmn profiles`** — “Filter by generation?” → `Gen I`; “Text or JSON?” → `Text` ⇒ supported and planned profiles/evidence labels.

### JSON understanding and batch work

- **`pkmn json names`** — “JSON?” → `Pkmn Red JP to Eng.red.json`; “Which names?” → `All` ⇒ original Japanese glyphs/bytes beside English semantic names.
- **`pkmn json provenance`** — “JSON?” → `Pkmn Red JP to Eng.red.json` ⇒ source profile and translation decisions.
- **`pkmn json export-report`** — “JSON?” → `Pkmn Red JP to Eng.red.json`; “Target?” → `English Red`; “Report?” → `Japanese to English Report.md` ⇒ retained/translated/omitted field preview.
- **Unified `pkmn validate-batch`** — “Add saves?” → `Pkmn Red Eng.sav`, `Pkmn FR Eng.sav`; “Confirm detected profiles?” → `Yes` ⇒ cross-game validation per file.
- **Enhanced `pkmn convert batch --dry-run`** — “Route?” → `red-firered`; “Sources?” → `Pkmn Red Eng.sav`, `Pkmn Red Eng Later.sav`; “Preview only?” → `Yes` ⇒ per input output/warning plan.
- **`pkmn batch report`** — “Batch folder?” → `Red Batch to FR`; “Format?” → `Markdown`; “Report?” → `Red Batch Summary.md` ⇒ success/skip/failure overview.
- **`pkmn json diff`** — “First JSON?” → `Pkmn Red Eng.red.json`; “Second?” → `Pkmn Red Eng After Brock.red.json`; “Include archival image?” → `No` ⇒ semantic JSON differences.
- **`pkmn save info`** — “File?” → `Pkmn LG Eng.sav`; “Confirm profile?” → `LeafGreen`; “Format?” → `Text` ⇒ concise cross-game save overview.

### Output discovery and session shortcuts

- **`pkmn outputs list`** — “Folder?” → `Red Proof`; “Text or JSON?” → `Text` ⇒ recognized save/report/manifest artifacts.
- **`pkmn self-test`** — “Short health check?” → `Yes` ⇒ quick installed engine status.
- **`pkmn interactive --search`** — “Search command or task?” → `repair checksum` ⇒ filtered guided menu; choose a result to begin its prompts.

## Coverage requirement

The active catalog has **124 endpoints**, and every one appears once in the
current sections above. The arrows summarize the choices; the program asks
full questions, explains errors, and lets the user go back to correct an
answer before running. Planned commands and options appear only in the
roadmap section and stay unavailable. Catalog coverage tests require a typed
workflow for each current endpoint, so adding a direct command also requires
adding its guided conversation.
