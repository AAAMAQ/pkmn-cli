# Handoff For New Chat: Unified `pkmn` CLI Project

This handoff gives a new Codex/chat session the important context needed to continue the Pokemon save-conversion toolchain work without rereading the entire project history.

## 1. Current Big Picture

The Pokemon Red side of the save-conversion research system is complete and pushed to GitHub.

There are two completed core repositories:

1. `Pkmn Red Save Genie`
2. `Pkmn Red Save Generator`

The next project is a new unified command-line tool, installed as:

```bash
pkmn
```

This new tool should act as the public-facing command line for the full ecosystem.

The unified CLI should start by wrapping and orchestrating the completed Pokemon Red Save Genie and Save Generator. Later, it can grow to support FireRed and Red-to-FireRed conversion.

## 2. Existing Repository Locations

Local paths:

```text
/Users/abdulqadir/Documents/Pkmn Red Save Genie
/Users/abdulqadir/Documents/Pkmn Red Save Generator
```

Remote repositories:

```text
https://github.com/AAAMAQ/pkmn-red-save-genie
https://github.com/AAAMAQ/pkmn-red-save-generator
```

Both repos are on `main`.

Latest known pushed commits:

```text
Pkmn Red Save Genie:
5bd364c Finalize Red Save Genie release corrections

Pkmn Red Save Generator:
d1e54b3 Align CI with safe-location canonicalization
3751b1e Finalize Red Save Generator release corrections
```

The Save Generator GitHub Actions CI passed after commit `d1e54b3`.

## 3. Completed Red-Side Capabilities

### Pkmn Red Save Genie

Purpose:

- parse Pokemon Red `.sav` files;
- validate checksums;
- decode gameplay structures;
- export `.red.json`;
- preserve physical save image for archival reconstruction;
- produce summaries and diagnostic reports;
- provide safe editing foundations.

Important capabilities:

- trainer/core decode;
- money, coins, badges, playtime;
- party decode;
- PC box decode;
- current-box cache decode;
- Daycare decode;
- Hall of Fame decode;
- Pokédex decode;
- inventory and PC item decode;
- story/event/trainer/static/missable/script/world-state decode;
- checksum validation;
- `.red.json` export;
- physical-image archival reconstruction support.

### Pkmn Red Save Generator

Purpose:

- read semantic `.red.json`;
- ignore `physicalImage` for generation;
- generate a new valid Pokemon Red `.sav`;
- preserve supported gameplay semantics;
- regenerate checksums;
- provide deterministic output;
- produce reports and comparison tooling.

Important capabilities:

- trainer/core generation;
- party generation;
- all PC boxes;
- current-box cache;
- Daycare;
- Hall of Fame;
- story/event/trainer/static flags;
- scripts and missables;
- hidden items and hidden coins;
- visited towns;
- Red’s-house safe-location canonicalization;
- physical-image isolation;
- write provenance;
- overlap validation;
- semantic comparison;
- generation reports.

## 4. Important Validation History

The project had a major Milestone 5-6 emulator corruption incident.

Summary:

- a generated save passed automated checks and Save Genie reparse;
- the emulator showed Continue normally;
- selecting Continue immediately caused severe graphical/text corruption;
- root cause was unsafe location/runtime-state generation assumptions;
- the project adopted a fail-closed location policy.

Current policy:

- unsupported or unsafe source locations are canonicalized to Red’s house second floor;
- the generator warns clearly when this happens;
- arbitrary location editing/generation remains restricted until full runtime contracts are understood.

This history matters because it taught the project that:

- valid checksums are not enough;
- parser acceptance is not enough;
- semantic comparison alone is not enough;
- emulator load/save-again validation is mandatory for release claims.

## 5. Final Manual Validation Lessons

The final semantic-sufficiency work exposed and corrected several important defects:

- text punctuation encoding had to become lossless enough for valid Gen I glyphs;
- current-box cache semantics had to distinguish permanent storage from player-visible cache;
- boxed Pokemon withdrawal viability had to be validated;
- Hall of Fame comparison needed full member-level validation, not just entry count;
- internal species IDs and Pokédex numbers must not be confused.

These lessons must guide the unified CLI design.

The CLI should not merely run shallow checks. It should expose validation commands that catch operational problems before emulator testing.

## 6. Licensing

Both completed Red repos now use MIT License.

Copyright holder:

```text
MAQ / BiG MAQ Studios
```

Each license includes a non-binding stewardship note:

- project is for education, research, preservation, and retro development;
- the creator humbly requests that people not merely repackage it as software for sale;
- this note does not restrict the MIT License.

The unified CLI should use the same license style unless the user explicitly changes direction.

Do not use the user’s personal name in license headers. Use:

```text
MAQ / BiG MAQ Studios
```

## 7. New Unified CLI Plan

A detailed plan was created here:

```text
/Users/abdulqadir/Documents/Pkmn Unified CLI Plan/UNIFIED_PKMN_CLI_PROJECT_PLAN.md
```

The new project should likely be:

```text
/Users/abdulqadir/Documents/pkmn-cli
```

Recommended GitHub repository:

```text
AAAMAQ/pkmn-cli
```

Installed executable:

```bash
pkmn
```

## 8. Command Philosophy

The command style should be:

```bash
pkmn <domain> <command> [arguments] [options]
```

Domains:

```text
red      raw Pokemon Red .sav workflows
rjson    Pokemon Red .red.json workflows
fred     future raw FireRed workflows
frjson   future FireRed semantic JSON workflows
proof    semantic proof workflows
compare  physical and semantic comparison
doctor   dependency/environment checks
config   configuration
help     help and examples
```

Example commands:

```bash
pkmn red decode savefile.sav
pkmn red inspect savefile.sav
pkmn red validate savefile.sav
pkmn red edit savefile.sav
pkmn rjson generate savefile.red.json
pkmn rjson reconstruct savefile.red.json
pkmn proof red originalRed2026.sav
pkmn compare physical original.sav generated.sav
pkmn doctor
```

## 9. Critical Mode Distinctions

The CLI must clearly distinguish these modes:

| Mode | Command example | physicalImage allowed? | Purpose |
|---|---|---|---|
| Decode | `pkmn red decode save.sav` | Reads original save bytes | Parse and export |
| Generate | `pkmn rjson generate save.red.json` | No | Independent semantic generation |
| Reconstruct | `pkmn rjson reconstruct save.red.json` | Yes, required | Byte-for-byte archival reconstruction |
| Edit | `pkmn red edit save.sav` | Edits a working copy of the save | Manual editing and testing |

Generation must never use `physicalImage`.

Reconstruction must use `physicalImage`.

This distinction is central to the research claim.

## 10. Edit Mode Requirements

The unified CLI should add a broad interactive editor:

```bash
pkmn red edit savefile.sav
```

The editor should allow multiple pending edits, validation, and final output writing.

It should resemble Junebug/FairyFox-style save-editing coverage, but with stricter validation.

Editable areas should eventually include:

- trainer name;
- rival name;
- trainer ID;
- money;
- coins;
- badges;
- options;
- playtime;
- Pokédex;
- bag inventory;
- PC item storage;
- party Pokemon;
- PC boxes;
- current selected box;
- Daycare;
- Hall of Fame;
- story flags;
- trainer battle flags;
- static encounter flags;
- hidden items;
- hidden coins;
- missables;
- scripts where verified safe;
- visited towns.

Location editing should remain disabled or limited to verified safe presets.

Output naming:

```text
savefile_generated_manual-edit.sav
savefile_generated_money.sav
savefile_generated_party.sav
savefile_generated_box-edit.sav
```

Never overwrite the original save by default.

## 11. Edit Session Commands

Interactive mode:

```bash
pkmn red edit savefile.sav
```

Advanced scriptable mode:

```bash
pkmn red begin-edit savefile.sav
pkmn red edit-session savefile.edit-session.json --money 999999
pkmn red edit-session savefile.edit-session.json --badges all
pkmn red pending-edits savefile.edit-session.json
pkmn red validate-edit savefile.edit-session.json
pkmn red end-edit savefile.edit-session.json
```

The session file should store:

- source save identity;
- source SHA-256;
- pending edits;
- validation status;
- output naming plan;
- command history.

Before finalizing, `end-edit` must confirm the source save has not changed.

## 12. Output Policy

By default, output files should go next to the input file.

Example input:

```text
/Saves/savefile.sav
```

Generated edited output:

```text
/Saves/savefile_generated_manual-edit.sav
```

Semantic generated output:

```text
/Saves/savefile_generated_semantic.sav
```

Reconstructed output:

```text
/Saves/savefile_reconstructed_from-physical-image.sav
```

Proof folder:

```text
/Saves/savefile.pkmn-proof/
```

ZIP output should be optional:

```bash
--zip
```

## 13. Homebrew Goal

The unified CLI should eventually be installable through Homebrew:

```bash
brew tap AAAMAQ/pkmn
brew install pkmn
```

This likely requires:

- stable `pkmn-cli` GitHub repo;
- release tags;
- source archive;
- reproducible build;
- no private data;
- no ROMs;
- MIT license;
- formula in a tap repo such as `AAAMAQ/homebrew-pkmn`.

Do not publish Homebrew formula until the CLI repo has a stable release.

## 14. Suggested New Repo Layout

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
│   ├── editing/
│   ├── reporting/
│   └── util/
├── tests/
├── docs/
├── examples/
├── scripts/
└── packaging/
    └── homebrew/
```

## 15. Recommended First Implementation Phases

### Phase A: Repo Foundation

- Create new `pkmn-cli` repo.
- Add MIT license.
- Add README.
- Add `pkmn --help`.
- Add `pkmn --version`.
- Add command router.
- Add tests.

### Phase B: Tool Discovery

- Implement `pkmn doctor`.
- Find Save Genie binary.
- Find Save Generator binary.
- Allow config paths.
- Report missing dependencies.

### Phase C: Red Decode/Inspect/Validate

- `pkmn red decode`
- `pkmn red inspect`
- `pkmn red validate`

### Phase D: RJSON Generate/Reconstruct

- `pkmn rjson generate`
- `pkmn rjson reconstruct`
- enforce physical-image rules.

### Phase E: Edit MVP

- `pkmn red edit`
- trainer/money/coins/badges/inventory first.
- validate and write safe copy.

### Phase F: Broad Edit Coverage

- party;
- PC boxes;
- Daycare;
- Hall of Fame;
- event flags;
- hidden objects;
- current box.

### Phase G: Proof Workflow

- `pkmn proof red`
- comparison reports;
- emulator checklist;
- zipped evidence package.

### Phase H: Packaging

- CI;
- release docs;
- Homebrew formula draft;
- stable release.

## 16. Safety Rules For Future Chat

Do not:

- modify completed Red repos unless explicitly requested;
- copy ROMs;
- commit private saves;
- commit screenshots;
- commit generated proof evidence;
- use `.red.json physicalImage` for semantic generation;
- claim FireRed support before it exists;
- allow arbitrary location editing without verified runtime state;
- overwrite source saves by default.

Do:

- keep Red Genie and Red Generator as source-of-truth engines;
- build the unified CLI as a separate repo;
- validate every edited/generated output;
- make output naming collision-safe;
- keep public docs private-path-free;
- keep Homebrew/public packaging clean.

## 17. Important External Inspirations

Relevant inspiration projects:

- `1fairyfox/pokered-save-editor`
- `1fairyfox/pokered-save-editor-2`
- `codemonkey85/PKMDS-Blazor`
- `kwsch/PKHeX
- `Striaton-Lab-Team/Pokemon-Community-Conversion-Standard
- `Striaton-Lab-Team/Poke_Transporter_GB

Important distinction:

- PKHeX/PKMDS are broad editing ecosystems using PKHeX.Core-style models.
- The MAQ project uses independent `.red.json` semantic/archival models.
- This project leans more toward save analysis, preservation, semantic proof, and verified generation, though the unified CLI will add practical edit workflows.
- Note:
- The two Striaton-Lab-Team projects which is done by GearsProgress and his project closely aligns witht he future goal of save conversion from pokemon red to fire red and also the middle brige fred/red.json files
- The pokemon conversion from red to fire red must comply with their community standard method
- Only once the FireRed part of project is verified and proven we will be adding the FireRed features of the CLI tool
- For now knowing that these community standard would be a helpful information rather than an implementation because that implementation will be done on sperate project which is the Fire Red Genie and Fire Red Generator.

Credit inspiration where appropriate, but do not copy code unless license compliance and attribution are handled.

## 18. What To Ask The User First In A New Chat

If starting implementation, ask or infer:

1. Should the new repo be named `pkmn-cli`?
2. Should the first implementation be C++20 for consistency with Red repos?
3. Should Phase 1 wrap existing binaries rather than link libraries?
4. Should the first milestone be only `pkmn --help`, `--version`, and `doctor`?

Recommended default:

```text
Create /Users/abdulqadir/Documents/pkmn-cli as a new Git repo.
Use C++20.
Start with wrapper orchestration.
Implement help/version/doctor first.
```

## 19. Current Planning Artifacts

Planning folder:

```text
/Users/abdulqadir/Documents/Pkmn Unified CLI Plan
```

Main plan:

```text
/Users/abdulqadir/Documents/Pkmn Unified CLI Plan/UNIFIED_PKMN_CLI_PROJECT_PLAN.md
```

This handoff:

```text
/Users/abdulqadir/Documents/Pkmn Unified CLI Plan/HANDOFF_FOR_NEW_CHAT.md
```

## 20. Short Project Summary

The Pokemon Red parser/generator work is complete. The next step is to build `pkmn`, a unified command-line tool that wraps the completed Red engines, adds interactive editing and proof workflows, prepares for Homebrew installation, and establishes the command framework for future FireRed and Red-to-FireRed conversion work.

