# Unified `pkmn` CLI Project Plan

Prepared for MAQ / BiG MAQ Studios  
Project purpose: education, research, preservation, retro development, and future Pokemon Red to FireRed save-conversion work.

## 1. Executive Summary

The unified `pkmn` command-line tool will be the public-facing interface for the Pokemon save research ecosystem.

The completed Red-side repositories already provide the verified engines:

- `Pkmn Red Save Genie`: reads, analyzes, validates, edits, and exports Pokemon Red save data.
- `Pkmn Red Save Generator`: generates valid Pokemon Red saves from semantic `.red.json` data without using `physicalImage` as generation authority.

The new unified CLI should not replace those projects. It should orchestrate them, package their outputs, expose their functionality through one consistent command, and prepare the project family for future FireRed support.

The installed command should be:

```bash
pkmn
```

Example command style:

```bash
pkmn red decode savefile.sav
pkmn red edit savefile.sav
pkmn red validate savefile.sav
pkmn rjson generate savefile.red.json
pkmn rjson reconstruct savefile.red.json
pkmn proof red originalRed2026.sav
pkmn compare physical original.sav generated.sav
pkmn doctor
pkmn help
```

## 2. Core Philosophy

The unified CLI must make the research tools usable without hiding the difference between the different workflows.

There are four major modes:

| Mode | Input | Output | Uses physical image? | Purpose |
|---|---|---|---|---|
| Decode | `.sav` | `.red.json`, summaries, reports | Reads source bytes as input | Parse and analyze a real save |
| Generate | `.red.json` | new `.sav` | No | Create an independent gameplay-equivalent save |
| Reconstruct | `.red.json` | reconstructed `.sav` | Yes | Archival byte-for-byte reconstruction from Save Genie physical data |
| Edit | `.sav` | edited copy `.sav` | Edits source save semantics/bytes through verified writers | Manual editing, testing, fixture creation |

The tool must always be honest about which mode is being used.

Generation and reconstruction are intentionally different:

- `pkmn rjson generate` must never use `physicalImage`.
- `pkmn rjson reconstruct` requires `physicalImage` and exists for archival/lossless reconstruction.

This distinction is central to the whole project.

## 3. New Repository

Recommended repository name:

```text
pkmn-cli
```

Alternative names:

```text
pkmn-save-tools
pkmn-command
pkmn-toolkit
```

Recommended GitHub repository:

```text
AAAMAQ/pkmn-cli
```

Recommended installed executable:

```text
pkmn
```

The repo should be separate from both Red repos. The Red repos remain focused engines, while this repo becomes the orchestration layer.

## 4. Relationship To Existing Projects

The unified CLI should depend on the existing projects in phases.

### Phase 1: Wrapper Orchestration

The CLI calls the existing built executables:

```text
pkmn
-> pkmn-red-save-genie executable
-> pkmn-red-save-generator executable
-> collects outputs
-> validates outputs
-> writes folders, reports, manifests, zips
```

This is the fastest and safest first version because the Red engines are already verified.

### Phase 2: Library Integration

After the first version works, the Red projects may expose reusable libraries:

```text
pkmn
-> links Save Genie library
-> links Save Generator library
-> calls parser/generator APIs directly
```

This is cleaner long-term but should not block the first CLI framework.

### Phase 3: Multi-Game Ecosystem

Future engines can plug in:

```text
Pkmn FireRed Save Genie
Pkmn FireRed Save Generator
Red-to-FireRed semantic converter
```

The unified CLI should be designed so these can be added without rewriting the Red commands.

## 5. Command Structure

Recommended shape:

```bash
pkmn <domain> <command> [arguments] [options]
```

Domains:

| Domain | Meaning |
|---|---|
| `red` | Raw Pokemon Red `.sav` workflows |
| `rjson` | Pokemon Red semantic/archive `.red.json` workflows |
| `fred` | Future raw Pokemon FireRed `.sav` workflows |
| `frjson` | Future FireRed semantic JSON workflows |
| `proof` | End-to-end semantic sufficiency workflows |
| `compare` | Physical and semantic comparison workflows |
| `doctor` | Environment and dependency checks |
| `config` | Tool configuration |
| `help` | Help and examples |

## 6. Pokemon Red `.sav` Commands

### `pkmn red decode`

Decode a Pokemon Red save using Save Genie.

```bash
pkmn red decode savefile.sav
```

Default outputs:

```text
savefile.red.json
SaveGenieSummary.txt
PokemonSummary.json
PokemonBoxes.json
checksums.json
```

Options:

```bash
--output-dir <dir>
--zip
--json
--markdown
--no-physical-image
--include-physical-image
```

Default output directory:

```text
same directory as input save
```

### `pkmn red inspect`

Print a readable terminal summary.

```bash
pkmn red inspect savefile.sav
```

Should show:

- trainer name;
- rival name;
- trainer ID;
- money;
- coins;
- badges;
- playtime;
- location;
- party count;
- PC box counts;
- selected box;
- Daycare status;
- Hall of Fame count;
- checksum status;
- warnings.

### `pkmn red validate`

Validate save structure and checksums.

```bash
pkmn red validate savefile.sav
```

Must validate:

- file size;
- main checksum;
- Bank 2 checksum;
- Bank 3 checksum;
- all per-box checksums;
- current-box cache coherence;
- permanent box structures;
- party structure;
- Daycare structure;
- Hall of Fame structure;
- text fields;
- known operational invariants.

### `pkmn red edit`

Interactive terminal editor.

```bash
pkmn red edit savefile.sav
```

This is one of the most important features. It should allow broad manual editing, then immediate validation and emulator testing.

The original file must never be overwritten by default.

Default output naming:

```text
savefile_generated_manual-edit.sav
savefile_generated_money.sav
savefile_generated_party.sav
savefile_generated_box-edit.sav
```

If output exists:

```text
savefile_generated_manual-edit_2.sav
```

or require:

```bash
--force
```

## 7. Interactive Edit Mode

The interactive menu should feel practical and direct.

Example:

```text
Pokemon Red Edit Session

Source: savefile.sav
Source SHA-256: ...
Pending edits: 0

1. Trainer info
2. Money and coins
3. Badges
4. Options and playtime
5. Pokedex
6. Bag inventory
7. PC item storage
8. Party Pokemon
9. PC Pokemon boxes
10. Current box
11. Daycare
12. Hall of Fame
13. Story and event flags
14. Trainer battle flags
15. Static encounters
16. Hidden items and hidden coins
17. Missable objects
18. Scripts and visited towns
19. View pending edits
20. Validate pending save
21. Save final edited copy
22. Discard edits and exit
```

Location editing must remain restricted. The Viridian Pokemon Center incident proved that map ID and coordinates alone are not enough. Location editing should initially support only verified safe presets or remain disabled.

## 8. Edit Session Workflow

The CLI should support both human interactive editing and advanced session-based editing.

### Interactive Flow

```bash
pkmn red edit savefile.sav
```

User makes many edits inside the menu.

The session ends by choosing:

```text
Save final edited copy
```

### Scriptable Session Flow

For automation and repeatable testing:

```bash
pkmn red begin-edit savefile.sav
pkmn red edit-session savefile.edit-session.json --money 999999
pkmn red edit-session savefile.edit-session.json --coins 999
pkmn red edit-session savefile.edit-session.json --badges all
pkmn red pending-edits savefile.edit-session.json
pkmn red validate-edit savefile.edit-session.json
pkmn red end-edit savefile.edit-session.json
```

Session file:

```text
savefile.edit-session.json
```

Session file contents:

- source logical name;
- source path, local only;
- source size;
- source SHA-256;
- pending edits;
- validation state;
- output plan;
- tool version;
- command history;
- warnings.

Before `end-edit`, the tool must confirm that the source save has not changed since the session started.

## 9. Editable Red Save Areas

The editor should eventually cover most areas available in advanced Red save editors, while adding stricter validation.

Editable areas:

- trainer name;
- rival name;
- trainer ID;
- money;
- coins;
- badges;
- badge mirror;
- options;
- playtime;
- Pokedex seen;
- Pokedex owned;
- bag inventory;
- PC item storage;
- party Pokemon;
- party moves;
- party PP and PP Ups;
- party HP and status;
- party DVs;
- party stat experience;
- party OT names;
- party nicknames;
- PC box Pokemon;
- all 12 boxes;
- selected box;
- current-box cache;
- Daycare;
- Hall of Fame;
- story flags;
- trainer battle flags;
- static encounter flags;
- legendary encounter state;
- gifts;
- fossils;
- hidden items;
- hidden coins;
- missable objects;
- scripts where verified;
- visited towns;
- Fly destinations where verified;
- duplicate and mirrored values.

Restricted initially:

- arbitrary location editing;
- arbitrary runtime map state;
- raw byte patching;
- unsupported FireRed edits;
- unverified script edits.

## 10. Edit Validation

After every edit or before final save, the tool should validate:

- save size;
- bounds;
- main checksum;
- Bank 2 checksum;
- Bank 3 checksum;
- per-box checksums;
- text encoding;
- party count/species/list structure;
- party stats and HP rules;
- PC box count/species/list structure;
- boxed Pokemon withdrawal viability;
- current-box cache;
- selected permanent box;
- Daycare occupancy;
- Hall of Fame species and record structure;
- Pokedex bitsets;
- inventory capacities;
- item IDs and quantities;
- event flag consistency where known;
- duplicate/mirror synchronization;
- unsupported location policy;
- Save Genie reparse;
- no private path leakage in generated reports.

Validation result should be readable:

```text
Validation passed

Main checksum: valid
Bank 2 checksum: valid
Bank 3 checksum: valid
Per-box checksums: valid
Party: valid
PC boxes: valid
Daycare: valid
Hall of Fame: valid
Save Genie reparse: passed
Original file unchanged
```

## 11. Red JSON Commands

### `pkmn rjson inspect`

```bash
pkmn rjson inspect savefile.red.json
```

Shows semantic summary and warnings.

### `pkmn rjson validate`

```bash
pkmn rjson validate savefile.red.json
```

Validates schema and supported semantic boundaries.

### `pkmn rjson generate`

```bash
pkmn rjson generate savefile.red.json
```

Creates an independent semantic save.

Rules:

- `physicalImage` forbidden as authority;
- deterministic output;
- write provenance required;
- checksums regenerated;
- unsupported non-empty state fails or is explicitly canonicalized by policy;
- original JSON untouched.

Default output:

```text
savefile_generated_semantic.sav
savefile_generated_semantic.report.json
savefile_generated_semantic.report.md
```

### `pkmn rjson reconstruct`

```bash
pkmn rjson reconstruct savefile.red.json
```

Reconstructs the physical save from Save Genie archival data.

Rules:

- `physicalImage` required;
- this is archival reconstruction;
- this is not semantic generation;
- output should match the original physical save if the `.red.json` was exported correctly.

Default output:

```text
savefile_reconstructed_from-physical-image.sav
```

## 12. Proof Commands

### `pkmn proof red`

```bash
pkmn proof red originalRed2026.sav
```

Runs:

```text
original.sav
-> Save Genie decode
-> original.red.json
-> Save Generator semantic generation
-> generated.sav
-> Save Genie reparse
-> physical comparison
-> semantic comparison
-> JSON comparison
-> reports
-> emulator checklist
```

Default output folder:

```text
originalRed2026.pkmn-proof/
```

Default generated save:

```text
generatedRed2026.sav
```

Reports:

```text
comparison.md
comparison.json
physical-comparison.md
physical-comparison.json
summary-comparison.md
summary-comparison.json
semantic-json-comparison.md
semantic-json-comparison.json
archival-json-comparison.md
archival-json-comparison.json
generation-report.md
generation-report.json
emulator-checklist.md
proof-manifest.json
```

The proof must stop before final conclusion until manual emulator verification passes.

## 13. Compare Commands

### Physical Comparison

```bash
pkmn compare physical original.sav generated.sav
```

Must report:

- file sizes;
- SHA-256 hashes;
- identical or not;
- equal byte count;
- differing byte count;
- percentages;
- first differing offset;
- last differing offset;
- contiguous equal regions;
- contiguous differing regions;
- save-region mapping;
- difference classification.

### Semantic Comparison

```bash
pkmn compare semantic original.red.json generated.red.json
```

Must classify differences as:

- exact match;
- normalized match;
- derived match;
- synchronized mirror match;
- permitted canonical difference;
- expected runtime drift;
- expected cache difference;
- unsupported or deferred;
- unexpected mismatch.

## 14. Post-Emulator Validation

```bash
pkmn red validate-post-emulator before.sav after.sav
```

or:

```bash
pkmn proof post-emulator --before generatedRed2026.sav --after generatedRed2026-post-emulator.sav
```

Must:

- preserve post-emulator save;
- record size;
- record SHA-256;
- validate checksums;
- reparse through Save Genie;
- compare pre/post semantics;
- classify gameplay drift;
- detect unexpected corruption;
- update proof reports.

Expected drift may include:

- location changes;
- playtime changes;
- HP/PP changes;
- party changes;
- item changes;
- storage changes;
- event/script changes caused by gameplay;
- cache synchronization.

## 15. Output Folder And ZIP Policy

By default, outputs should be created in the same directory as the input file.

Example:

```bash
pkmn red decode /Saves/savefile.sav
```

Outputs:

```text
/Saves/savefile.red.json
/Saves/savefile.summary/
```

For larger workflows:

```text
/Saves/savefile.pkmn-output/
```

ZIP option:

```bash
pkmn proof red original.sav --zip
```

Creates:

```text
original.pkmn-proof.zip
```

ZIP packages must not include:

- ROMs;
- emulator binaries;
- private screenshots unless explicitly requested;
- private absolute paths in public reports.

## 16. Naming Rules

Generated edited saves:

```text
<base>_generated_<operation>.sav
```

Examples:

```text
savefile_generated_manual-edit.sav
savefile_generated_money.sav
savefile_generated_badges.sav
savefile_generated_party-heal.sav
savefile_generated_box-edit.sav
```

Semantic generated saves:

```text
<base>_generated_semantic.sav
```

Reconstructed saves:

```text
<base>_reconstructed_from-physical-image.sav
```

Collision handling:

```text
savefile_generated_manual-edit_2.sav
savefile_generated_manual-edit_3.sav
```

No overwrite unless:

```bash
--force
```

## 17. Safety Requirements

The CLI must:

- never overwrite inputs by default;
- never overwrite outputs by default;
- never modify source saves in place;
- never require ROM files;
- never commit or publish private saves;
- never use `.red.json physicalImage` for semantic generation;
- clearly separate generation from reconstruction;
- validate before writing final edited output;
- emit meaningful error messages;
- use stable exit codes;
- preserve deterministic outputs where required;
- store private evidence only in ignored folders;
- avoid hardcoded machine paths;
- avoid private absolute paths in public reports.

## 18. Exit Codes

Suggested exit codes:

| Code | Meaning |
|---:|---|
| 0 | success |
| 1 | general failure |
| 2 | invalid command or arguments |
| 3 | invalid input file |
| 4 | checksum failure |
| 5 | generation failure |
| 6 | physical-image isolation failure |
| 7 | deterministic-generation failure |
| 8 | unexpected semantic mismatch |
| 9 | report/output failure |
| 10 | post-emulator validation failure |
| 11 | edit validation failure |
| 12 | unsupported operation |

## 19. Homebrew Packaging

Goal:

```bash
brew tap AAAMAQ/pkmn
brew install pkmn
```

Then:

```bash
pkmn --version
pkmn help
pkmn red decode savefile.sav
```

Homebrew needs:

- stable GitHub release tags;
- source archive;
- build instructions;
- license;
- checksum;
- no private files;
- no ROMs;
- no generated private saves;
- installable executable named `pkmn`.

Potential formula repo:

```text
AAAMAQ/homebrew-pkmn
```

Potential formula:

```ruby
class Pkmn < Formula
  desc "Pokemon save research and conversion command-line tools"
  homepage "https://github.com/AAAMAQ/pkmn-cli"
  license "MIT"
end
```

Do not publish Homebrew formula until the CLI repo has a stable release.

## 20. Future FireRed Namespaces

FireRed commands should exist only as honest placeholders until the FireRed engines are implemented.

Example:

```bash
pkmn fred decode savefile.sav
```

Output:

```text
FireRed support is planned but not implemented yet.
Current supported systems: Pokemon Red Save Genie and Pokemon Red Save Generator.
```

Future commands:

```bash
pkmn fred decode
pkmn fred inspect
pkmn fred validate
pkmn frjson generate
pkmn frjson reconstruct
pkmn convert red-to-firered
```

## 21. Future Red To FireRed Conversion

Long-term command:

```bash
pkmn convert red-to-firered original.red.json
```

Pipeline:

```text
Pokemon Red .sav
-> .red.json
-> Red-to-FireRed semantic conversion
-> .frjson
-> FireRed generator
-> FireRed .sav
```

Must report:

- mapped fields;
- unmapped fields;
- canonical differences;
- unsupported state;
- conversion warnings;
- FireRed validation status.

Do not imply FireRed conversion exists before it is implemented and emulator-tested.

## 22. Suggested Repository Layout

```text
pkmn-cli/
├── README.md
├── LICENSE
├── CHANGELOG.md
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── app/
│   ├── commands/
│   │   ├── red/
│   │   ├── rjson/
│   │   ├── proof/
│   │   ├── compare/
│   │   ├── doctor/
│   │   └── config/
│   ├── integration/
│   │   ├── SaveGenieRunner.hpp
│   │   └── SaveGeneratorRunner.hpp
│   ├── editing/
│   ├── reporting/
│   ├── packaging/
│   └── util/
├── tests/
├── docs/
│   ├── CLI_REFERENCE.md
│   ├── EDIT_MODE.md
│   ├── RECONSTRUCT_MODE.md
│   ├── PROOF_WORKFLOW.md
│   ├── HOMEBREW_INSTALL.md
│   ├── PRIVACY_AND_PUBLICATION.md
│   └── FUTURE_FIRERED_PLAN.md
├── examples/
├── scripts/
└── packaging/
    └── homebrew/
```

## 23. First Implementation Milestones

### Milestone A: Repo Foundation

- Create `pkmn-cli` repo.
- Add MIT license with MAQ / BiG MAQ Studios stewardship note.
- Add CMake or chosen build system.
- Add basic `pkmn --help`.
- Add `pkmn --version`.
- Add command router.
- Add tests for argument parsing.

### Milestone B: Red Tool Discovery

- Implement `pkmn doctor`.
- Locate Save Genie executable.
- Locate Save Generator executable.
- Allow config paths.
- Validate helper tool versions.
- Report missing tools clearly.

### Milestone C: Red Decode And Inspect

- Implement `pkmn red decode`.
- Implement `pkmn red inspect`.
- Implement `pkmn red validate`.
- Package Save Genie outputs.

### Milestone D: RJSON Generate And Reconstruct

- Implement `pkmn rjson generate`.
- Implement `pkmn rjson reconstruct`.
- Enforce physical-image rules.
- Add output naming and collision protection.

### Milestone E: Edit MVP

- Implement `pkmn red edit`.
- Start with trainer, money, coins, badges, inventory, and validation.
- Add pending edit preview.
- Add final output writing.
- Add checksum regeneration.

### Milestone F: Broad Edit Coverage

- Add party editing.
- Add PC box editing.
- Add Daycare editing.
- Add Hall of Fame editing.
- Add event/story flag editing.
- Keep location editing restricted.

### Milestone G: Proof Workflow

- Implement `pkmn proof red`.
- Generate comparison reports.
- Generate emulator checklist.
- Create zipped proof package.

### Milestone H: Homebrew Preparation

- Add release build.
- Add install docs.
- Add Homebrew formula draft.
- Add CI.
- Tag stable release after validation.

## 24. Testing Plan

Test categories:

- command parsing;
- missing arguments;
- invalid commands;
- missing files;
- output collision;
- safe overwrite;
- Red decode;
- Red inspect;
- Red validate;
- RJSON validate;
- RJSON generate;
- RJSON reconstruct;
- physical-image isolation;
- deterministic generation;
- edit session creation;
- multiple pending edits;
- edit validation;
- edit finalization;
- checksum regeneration;
- ZIP packaging;
- report generation;
- privacy scan;
- Homebrew build smoke test.

No committed tests should require private saves, ROMs, screenshots, or emulator binaries.

## 25. Documentation Plan

Required docs:

- `README.md`
- `docs/CLI_REFERENCE.md`
- `docs/EDIT_MODE.md`
- `docs/RECONSTRUCT_MODE.md`
- `docs/PROOF_WORKFLOW.md`
- `docs/HOMEBREW_INSTALL.md`
- `docs/PRIVACY_AND_PUBLICATION.md`
- `docs/FUTURE_FIRERED_PLAN.md`
- `docs/RELEASE_CHECKLIST.md`

README should explain:

- what `pkmn` is;
- relationship to Save Genie and Save Generator;
- Red support status;
- FireRed future status;
- generation vs reconstruction;
- edit mode safety;
- install instructions;
- Homebrew plan;
- examples;
- privacy rules;
- no ROM distribution.

## 26. Release Readiness Criteria

The first public `pkmn` CLI release is ready only when:

- `pkmn --help` works;
- `pkmn --version` works;
- `pkmn doctor` works;
- Red decode works;
- Red inspect works;
- Red validate works;
- RJSON generate works;
- RJSON reconstruct works;
- edit MVP works;
- outputs are collision-safe;
- reports are useful;
- private paths are not leaked;
- no ROMs or private saves are committed;
- CI passes;
- README examples work from a fresh checkout;
- Homebrew formula draft builds locally.

## 27. Important Open Decisions

Before implementation, decide:

- repository name: `pkmn-cli` or another name;
- implementation language: C++20, Swift, Rust, Go, or Node;
- initial integration mode: executable wrapper or library linkage;
- whether Save Genie should expose a stable CLI binary first;
- where helper tool paths are configured;
- whether `pkmn red edit` should use full-screen terminal UI or simple numbered prompts;
- whether output ZIP is default or optional;
- whether Homebrew formula lives in the same repo or a separate tap repo.

Recommended initial choices:

- repo name: `pkmn-cli`;
- executable name: `pkmn`;
- first implementation: C++20 or Rust;
- integration mode: wrapper orchestration first;
- UI mode: numbered terminal prompts first;
- ZIP mode: optional `--zip`;
- Homebrew: separate tap repo later.

## 28. Recommended Next Step

Create the new Git repo:

```bash
mkdir pkmn-cli
cd pkmn-cli
git init
```

Then build the foundation:

```bash
pkmn --help
pkmn --version
pkmn doctor
```

After that, wire in the completed Red tools:

```bash
pkmn red decode
pkmn rjson generate
pkmn red edit
pkmn rjson reconstruct
pkmn proof red
```

The goal is to turn the research system into a usable public tool while preserving the strict validation philosophy that made the Red projects trustworthy.

