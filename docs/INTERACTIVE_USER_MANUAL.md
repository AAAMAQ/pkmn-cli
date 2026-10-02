# The player's manual to pkmn interactive

**Guide date: 2 October 2026.** Covers the 3.1.0 release, including
simple conversion and experimental Japanese Green 1.0 support.

> **Check your build first.** This manual requires pkmn 3.1.0 or newer. Older
> 3.0.0 installations show a different menu and lack the latest Japanese tools.
> Check `pkmn --version` after installation or upgrade.

## Start here

`pkmn` helps you move a supported Pokémon Red or Blue save journey into FireRed
or LeafGreen. It also lets you inspect saves, make readable summaries, export
their data, edit supported fields, compare backups, and check generated saves.
You can do these tasks by answering questions in `pkmn interactive`.

For your first conversion, read sections 1–5. The later sections explain the
other menu options. The command index at the end includes every command in the
current build; you can open its guided questions through menu 8.

Contents:

1. [Prepare your files](#1-prepare-your-files)
2. [Download, install, and open pkmn](#2-download-install-and-open-pkmn)
3. [Understand the menu and file picker](#3-understand-the-menu-and-file-picker)
4. [Convert English Red or Blue](#4-menu-1-convert-english-red-or-blue)
5. [Convert a Japanese save](#5-menu-1-convert-a-japanese-save)
6. [Use the converted save in an emulator](#6-use-the-converted-save-in-an-emulator)
7. [Advanced conversion and Japanese tools](#7-menu-2-advanced-conversion-and-japanese-tools)
8. [Inspect, validate, summarize, or repair](#8-menu-3-inspect-validate-summarize-or-repair)
9. [Decode, generate, reconstruct, or migrate](#9-menu-4-decode-generate-reconstruct-or-migrate)
10. [Edit a save](#10-menu-5-edit-a-save-safely)
11. [Compare saves](#11-menu-6-compare-saves-or-playthrough-progress)
12. [Proof and emulator checks](#12-menu-7-run-proof-or-emulator-checks)
13. [Find every command](#13-menu-8-browse-every-current-command-and-option)
14. [Doctor and help](#14-menu-9-doctor-settings-and-beginner-help)
15. [Troubleshooting](#15-troubleshooting)
16. [Complete command index](#16-complete-command-index)

## 1. Prepare your files

Before using the converter, make a backup of your save and close the emulator
so that it cannot write to that save while you work.

You need a **game save**, usually ending in `.sav`. Use the game's normal Save
option, then obtain its battery-save file or export it using your emulator's
save-export function. An emulator save state is a different kind of file.
Renaming a save state to `.sav` does not turn it into a game save.

The converter itself does **not** need a ROM. To play the result, you need an
emulator or compatible hardware and your own target-game copy. For example,
a FireRed output must be loaded with the appropriate FireRed game. ROM files
and emulator binaries are not distributed here. You do not select a ROM in
the conversion wizard.

Create a working folder, for example `Pokemon Saves`, with a separate backup
folder. This manual uses the following **fictional** files; they are examples,
not downloads or files included with pkmn:

| Example file | What it represents |
|---|---|
| `Pokemon Red.sav` | English Red source save |
| `Pokemon Blue.sav` | English Blue source save |
| `Pokemon FireRed.sav` | Existing English FireRed save |
| `Pokemon LeafGreen.sav` | Existing English LeafGreen save |
| `Pokemon Red JP.sav` | Japanese Red save, with a known 1.0 or 1.1 game revision |
| `Pokemon Green JP.sav` | Japanese Green save from original release 1.0 |
| `Pokemon Red Before Brock.sav` | Older backup of the same Red playthrough |
| `Pokemon Red After Brock.sav` | Newer backup of that playthrough |

You only need the input file for the task you choose. English Red/Blue
conversion uses a bundled target template; you do not need to create a fresh
FireRed or LeafGreen save first.

Choose the actual source game. Renaming a Blue save to `Pokemon Red.sav` does
not change its origin, and the shared save layout cannot reliably identify
Red versus Blue for you.

## 2. Download, install, and open pkmn

### Published packages

Open the official [GitHub Releases page](https://github.com/AAAMAQ/pkmn-cli/releases).
Read the release notes and expand **Assets**. Select a package for your computer:

| Computer | Package choice | How to start |
|---|---|---|
| Windows x86-64 | Installer `.exe`, or portable `.zip` | Install and open a new PowerShell window; portable users extract the whole ZIP and use its launcher |
| Apple Silicon Mac | `macos-arm64` `.pkg` or archive | Install the package, or extract the whole archive |
| Intel Mac | `macos-x86_64` `.pkg` or archive | Install the package, or extract the whole archive |
| Linux x86-64 | AppImage, `.deb`, or archive offered by that release | Use the package for your distribution or the portable launcher |

Use the published assets, not GitHub's **Source code** ZIP, unless you intend
to compile the program. Keep the full extracted directory together; the
executable needs its accompanying resources and runtime. Package availability
and signing information belong to that release's notes.

Open Terminal on macOS/Linux, or PowerShell on Windows. Type each line and
press Enter:

```sh
pkmn --version
pkmn doctor --deep
pkmn interactive
```

The first line shows the version. Doctor checks the installation. The last
line opens the menu. You normally only need the last line on later visits.

For a portable Windows archive, run its `pkmn-interactive.cmd` launcher, or
open PowerShell in the extracted folder and use:

```powershell
.\bin\pkmn.exe interactive
```

For a portable Unix archive, run its supplied launcher or `./bin/pkmn interactive`
from the extracted folder. An AppImage can be made executable and launched
with the exact filename you downloaded; see [installation details](INSTALL.md).

### Homebrew on macOS

If you already use Homebrew, the existing tap offers a development source build:

```sh
brew tap AAAMAQ/pkmn
brew install --HEAD AAAMAQ/pkmn/pkmn-cli
```

For an existing HEAD installation, check published repository changes with:

```sh
brew update
brew upgrade --fetch-HEAD AAAMAQ/pkmn/pkmn-cli
```

These commands use the tap and published source. They cannot install local,
uncommitted additions. Check the actual menu after an update. See
[Homebrew status and instructions](HOMEBREW_INSTALL.md).

### Using the newest development checkout

If you have the checkout containing the additions described here, open a
terminal in the project folder. With CMake, a C++20 compiler, and Python 3
installed, build and launch it:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/pkmn interactive
```

These are Unix/single-configuration build examples. Windows development builds
may place the executable under `build\Release`; use the build instructions for
your generator. Ordinary packaged releases do not require this development setup.

If `pkmn` opens an older program, use the local build command explicitly.
The version should report `3.1.0`; the catalog contains 124 commands.

## 3. Understand the menu and file picker

The current opening screen is:

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

Enter a number and press Enter. Once inside a task, answer one question at a
time. Do not paste an entire terminal command into a file-path question.

| Input | Meaning |
|---|---|
| `?` | Explain the current question |
| `B` | Go back; in a batch file list, remove the most recently added file first |
| `Q` | Quit the interactive session |
| Enter on an optional field | Skip it; when a previous answer exists, keep that answer |
| `Clear` on an optional field | Remove a previous answer |
| `YES` at the final review | Run the task; use the full word |
| `E` at an advanced task's review | Edit answers from the beginning |

Quitting before confirmation prevents that operation from running. Quitting
after a successful operation does not undo outputs already created. An edit
session can already contain staged edits from earlier confirmations.

### Entering a filename with spaces

If your terminal is already in the folder containing the file, enter:

```text
Pokemon Red.sav
```

Otherwise paste its complete path. These are fictional path examples:

```text
~/Desktop/Pokemon Saves/Pokemon Red.sav
"~/Desktop/Pokemon Saves/Pokemon Red.sav"
"C:\Users\Player\Desktop\Pokemon Saves\Pokemon Red.sav"
```

Interactive mode reads the whole line, so spaces are accepted with or without
surrounding quotes. It also handles escaped spaces pasted by many terminals.
Unlike interactive answers, direct shell commands need quotation marks around
a path containing spaces.

At a save-input prompt, type `Browse` to choose from files. The browser offers
numbered entries, next/previous pages, a parent-folder control, and an option
to show all files. If you open the wrong folder, navigate back or enter another
folder. Missing files produce retry/browse choices. Do not rename a file merely
to make it pass a format check.

### Finding a task inside a menu

Menus 2–9 list related commands. Their numbered positions can change as new
commands are added. This manual identifies commands by name rather than giving
fragile submenu numbers.

For any command in this guide, the universal path is:

```text
Main menu: 8
Choose a command: S
Search command or task: red summary
Choose the displayed red summary entry by its number.
1 Walk me through it  2 Show direct syntax  B Back  Q Quit: 1
```

Then answer the questions. Search results may include several commands; choose
the exact displayed name. `N` and `P` change pages, `C` clears the search, and
`R` shows the future roadmap. Planned commands cannot be run.

## 4. Menu 1: convert English Red or Blue

### Worked example: Red to FireRed

Launch interactive mode and enter the answers shown after `>` below. These
are representative excerpts; repeated help controls and resolved absolute
paths are omitted for readability.

```text
Choose a task:
> 1

Which game is your save from?
 1 Red (English)
 2 Blue (English)
 3 Red (Japanese, experimental)
 4 Green (Japanese 1.0, experimental)
> 1

Choose your save file. Paste its path or type Browse.
> Pokemon Red.sav

Which game would you like to play it in?
 1 FireRed
 2 LeafGreen
> 1

Convert Pokemon Red.sav to FireRed?
New save: .../Pokemon Red_fr.sav
Your original save stays unchanged. Conversion reports are saved alongside the result.
Save checksums will be repaired for conversion if needed.
Type YES to convert, B to go back, or Q to quit:
> YES

Converted save ready: .../Pokemon Red_fr.sav
```

Look beside the input for:

```text
Pokemon Red.sav                          Original, unchanged
Pokemon Red_fr.sav                       Converted FireRed save
Pokemon Red_fr.conversion-manifest.json  Detailed conversion record
Pokemon Red_fr.conversion-report.md      Readable conversion report
```

Open the Markdown report to review what was translated, normalized, omitted,
or warned about. A converted save carries supported journey data; it does not
create remake-only progress that never existed in the source game.

### Other English combinations

| Input | Source choice | Target choice | First default save output |
|---|---:|---:|---|
| `Pokemon Red.sav` | 1 | 1 FireRed | `Pokemon Red_fr.sav` |
| `Pokemon Red.sav` | 1 | 2 LeafGreen | `Pokemon Red_lg.sav` |
| `Pokemon Blue.sav` | 2 | 1 FireRed | `Pokemon Blue_fr.sav` |
| `Pokemon Blue.sav` | 2 | 2 LeafGreen | `Pokemon Blue_lg.sav` |

If a result or its conversion reports already exist, simple mode chooses a
new numbered output, for example `Pokemon Red_fr_2.sav`, and shows it before
confirmation. Read the displayed name rather than assuming it used the first.

Simple mode accepts physical saves. Use menu 2 or 8 to convert an existing JSON
document, select a custom destination, or adjust advanced settings.

### How much has been verified?

| Route | Current evidence |
|---|---|
| English Red → FireRed | Project emulator acceptance recorded |
| English Red → LeafGreen | Automated/static validation; community emulator testing |
| English Blue → either remake | Automated/static validation; community emulator testing |
| Japanese Red/Green → FireRed | Experimental; synthetic tests; real-save/emulator acceptance pending |

Even a verified route should be checked with your own generated save. Valid
checksums only establish part of save integrity. Glitch Pokémon, unsupported
species, levels outside supported ranges, or invalid records can stop conversion.
Checksum repair cannot decide how to replace those records.

## 5. Menu 1: convert a Japanese save

### Japanese Red to FireRed

Choose menu 1, source **3**, and enter `Pokemon Red JP.sav`. Select the actual
game revision:

```text
Japanese Red currently converts to FireRed only. Which game version made this save?
 1 Original release (1.0)
 2 Revised release (1.1)
 3 I don't know
> 1
```

Review the experimental notice and output `Pokemon Red JP_fr.sav`, then type
`YES`. Do not choose revision 1.0 just because it is first. If you do not know,
check the game version before proceeding; the save cannot reliably identify it.

### Japanese Green to FireRed

Choose menu 1, source **4**, and enter `Pokemon Green JP.sav`:

```text
Japanese Green converts to FireRed. Confirm your game version:
 1 Original release (1.0)
 2 Revised release (1.1) or I don't know
> 1
```

After confirmation, the new save is `Pokemon Green JP_fr.sav`. Green 1.1 is
not supported by this implementation. Japanese-to-LeafGreen and the planned
Japanese-to-English Red/Blue workflows are not offered in simple mode.

### Keep the Japanese archive and reports

Japanese conversion also writes these files beside its source:

| Source | Archive | Bridge JSON |
|---|---|---|
| `Pokemon Red JP.sav` | `Pokemon Red JP.red.jp.json` | `Pokemon Red JP.red.json` |
| `Pokemon Green JP.sav` | `Pokemon Green JP.green.jp.json` | `Pokemon Green JP.red.json` |

The Green projection's `.red.json` ending describes the reused semantic
contract; its metadata still identifies Japanese Green. It does not change
the source game's identity.

The archive preserves the original bytes and Japanese text. Supported Pokémon
names are encoded as Japanese in FireRed; player/rival display uses the current
English-compatible fallback by default. The optional player-name retention
setting in advanced mode remains experimental and its display is unverified.

If the archive or projection already exists, a repeated one-shot Japanese
conversion is refused. Use the existing projection with `rjson convert` and a
new output path, or use a separately named source copy. Do not delete your only
archive simply to retry. See [Green details](JAPANESE_GREEN_IMPLEMENTATION.md)
and [Japanese Red archive details](JAPANESE_RED_JSON_SCHEMA.md).

## 6. Use the converted save in an emulator

1. Close the emulator and back up any existing save for the target game.
2. Open your own appropriate target game: FireRed for an `_fr.sav` result,
   LeafGreen for `_lg.sav`.
3. Use the emulator's battery-save/import-save function, if available. Otherwise
   follow its documented save-folder and filename rules. Some require the save
   basename to match the ROM basename; work with a copy if renaming is needed.
4. Start or restart the game and choose Continue. Do not restore an old emulator
   save state over the converted game.
5. Check trainer identity, badges, money, party, PC boxes, inventory, location,
   and relevant story progress. For Japanese routes, inspect names as well.
6. Save using the game's normal menu, close the emulator completely, reopen it,
   and verify that Continue still loads correctly.
7. Export a separate post-test save, such as `Pokemon FireRed After Test.sav`.
   Keep the original generated save for comparison.

Exact emulator buttons and save locations vary. pkmn does not launch an emulator
or mark these manual steps passed merely because conversion succeeded.

## 7. Menu 2: advanced conversion and Japanese tools

Choose this menu when you want a custom output, an existing JSON input, a
conversion plan, or Japanese archive operations. Select a displayed command,
then **Walk me through it**. Menu 8 provides the same commands by search.

### Example: Red JSON to a LeafGreen save

Select `convert red-leafgreen`:

```text
Source: Pokemon Red.red.json
Create a proposed JSON plan instead of a save? No
Output: Pokemon Red Custom LeafGreen.sav
Review advanced options? N
Review the displayed choices and outputs.
Run and write these outputs? YES
```

This uses data from a compatible JSON document. You do not need its archival
`physicalImage` for conversion. The document must match the selected source
game. Use a Blue route for `Pokemon Blue.blue.json`.

### Example: review a proposed conversion before generating a save

Select `convert red-leafgreen`, choose your Red save or JSON, answer **Yes** to
the proposed-plan question, and enter `Pokemon Red Planned.lg.json` as the
output. You receive proposed target JSON and conversion reports, not a playable
save. The output must have the target JSON ending requested by the wizard.
Use `lgjson validate` to check the plan and `lgjson generate` to create a save.

### What do the advanced choices mean?

| Choice | Plain-language purpose | Usual beginner answer |
|---|---|---|
| Template | Supply your own validated target baseline | Leave blank for bundled template |
| Salt | Change a reproducible conversion seed | Leave blank |
| Manifest/report paths | Choose where audit files go | Leave blank for defaults |
| Keep intermediate files | Keep proposed target JSON as well as the save | No unless you want to inspect it |
| Repair checksums in memory | Permit checksum-only repair for a physical English source | Yes if needed; source remains unchanged |
| Write repaired source copy | Also save the repaired Gen I copy when repair is needed | Leave blank unless wanted |
| Numbered output | Choose another name if the output is taken | Yes for a separate result |
| Pinned original policy | Use the implemented conversion rules | Keep the supported policy |

For Japanese archives, use `rjpjson` for Red and `gjpjson` for Green. **Inspect**
explains the archive, **validate** checks it, **reconstruct** restores original
bytes, **project** creates bridge JSON, and **compare** checks that projection
against the archive. For example, reconstruct `Pokemon Green JP.green.jp.json`
to a new `Pokemon Green JP Restored.sav`.

`convert inspect` and `convert explain` describe mapping rules for events,
trainers, or items. `convert validate-manifest` checks a conversion record.
`convert routes` currently lists the four English paired routes; Japanese
commands are separately listed in the complete command browser.

For `convert batch`, select one English route, add source files one per line,
type `Done`, and choose a **new** output folder such as `Red Conversion Batch`.
Do not mix Red and Blue sources in a single declared route.

## 8. Menu 3: inspect, validate, summarize, or repair

These are good first steps when you are unsure about a file.

| Goal | Command to select | Example input | Result |
|---|---|---|---|
| Check Red size/checksums | `red inspect` or `red validate` | `Pokemon Red.sav` | Terminal integrity results |
| Read a Red progress summary | `red summary` | `Pokemon Red.sav` | Trainer, party, inventory, storage, and progress summary |
| Check FireRed | `fred inspect` or `fred validate` | `Pokemon FireRed.sav` | Container/checksum results |
| Read FireRed summary | `fred summary` | `Pokemon FireRed.sav` | Summary, optionally detailed |
| Check LeafGreen | `leafgreen validate` | `Pokemon LeafGreen.sav` | Save validation result |
| Check Japanese Red/Green | `red-jp validate` / `green-jp validate` via menu 2 or 8 | Corresponding Japanese save | Validation using its explicit revision |

### Example: create a readable Red report

Select `red summary`, choose `Pokemon Red.sav`, choose **Detailed Markdown**,
then enter `Pokemon Red Summary.md`. Review and confirm. Open that output in
a Markdown viewer or text editor. To display it only in the terminal, leave
the optional output blank. Choose JSON only when you want structured data for
another tool.

### Example: repair checksum errors

Select `red repair-checksums`:

```text
Source: Pokemon Red.sav
Repaired copy: Pokemon Red Repaired.sav
Review advanced options? N
Confirm: YES
```

The output is a new save and a repair report named
`Pokemon Red Repaired.sav.repair-report.json`. This repairs recognized checksum
bytes, not unsupported Pokémon or damaged story data. `fred repair-checksums`
provides the FireRed counterpart; follow its displayed outputs.

For batch validation or decoding, add one file per line and finish with `Done`.
Validation prints per-file results. Decode batches ask for a new output folder.

## 9. Menu 4: decode, generate, reconstruct, or migrate

These words describe different jobs:

| Operation | What it does |
|---|---|
| Decode | Read a save and export structured JSON data |
| Inspect/validate JSON | Explain or check that document |
| Generate | Build a save from supported game-data fields |
| Reconstruct | Restore the exact archived save bytes from `physicalImage` |
| Migrate/update schema | Produce a document copy compatible with supported schema rules |

### Decode your Red save

Select `red decode`:

```text
Save: Pokemon Red.sav
Include the exact archival physical image? Yes
Output: Pokemon Red.red.json
Review advanced options? N
Confirm: YES
```

This creates JSON that can be inspected and, when its image validates, used for
exact reconstruction. Answer **No** to keep semantic data without the archival
image; that document can still supply supported generation/conversion fields.

Other examples:

| Command | Input | Chosen output |
|---|---|---|
| `blue decode` | `Pokemon Blue.sav` | `Pokemon Blue.blue.json` |
| `fred decode` | `Pokemon FireRed.sav` | `Pokemon FireRed.fred.json` |
| `leafgreen decode` | `Pokemon LeafGreen.sav` | `Pokemon LeafGreen.lg.json` |
| `red-jp decode` | `Pokemon Red JP.sav` plus revision | `Pokemon Red JP.red.jp.json` |
| `green-jp decode` | `Pokemon Green JP.sav` plus 1.0 profile | `Pokemon Green JP.green.jp.json` |

Game-specific tools have different supported options. Do not assume the Red
physical-image toggle is offered by every decoder.

### Inspect and validate the JSON

Use `rjson inspect` or `rjson validate` for `Pokemon Red.red.json`. The validation
wizard offers Standard, Strict, Generation, and Archival policies. Start with
Standard; choose Archival when checking reconstruction material or Generation
when preparing a semantic save. Read warnings as well as the final result.
Use `frjson` for FireRed and `lgjson` for LeafGreen where listed in the catalog.

### Generate a new save

Select `rjson generate`, input `Pokemon Red.red.json`, and choose
`Pokemon Red Generated.sav`. Skip advanced choices unless required, review,
and confirm. It produces the new save plus generation reports. This mode uses
semantic fields and ignores `physicalImage` as an authority; supported location
normalization can make the bytes differ from the original.

For a complete supported FireRed document or conversion plan, use `frjson generate`
with `Pokemon FireRed.fred.json` and `Pokemon FireRed Generated.sav`. LeafGreen
uses `lgjson generate` with `Pokemon LeafGreen.lg.json`.

### Restore an archive exactly

Select `rjson reconstruct`, choose `Pokemon Red.red.json`, then
`Pokemon Red Restored.sav`. It requires a valid `physicalImage`. If that image
was excluded at decode time, reconstruction fails; generating a save cannot
recover bytes that were never archived. Japanese archive tools have their own
`rjpjson reconstruct` and `gjpjson reconstruct` commands.

### Migrate or process several documents

The supported `migrate` and `update_schema` commands write a separate JSON copy,
such as `Pokemon Red Updated.red.json`. They do not translate between games.
The `schema` commands describe expected fields. Generation batches accept
multiple JSON inputs and a new destination directory; review per-file failures.

## 10. Menu 5: edit a save safely

Editing writes a new save. For several changes, use an **edit session**: a JSON
file containing staged changes tied to your original save. Keep that original
in place and unchanged while the session is active.

### Worked example: change money and rename a party Pokémon

Use menu 5 or find each command through menu 8. Confirm each task when asked.

1. Select `red begin-edit`. Input `Pokemon Red.sav`; choose
   `Pokemon Red Edit Session.json`. This creates a session, not an edited save.
2. Select `red edit-session`. Choose that session, select **Money**, and enter
   `50000`. Skip advanced choices and confirm. The session now holds that edit.
3. Select `red pokemon`. Choose the session, select **Party slot**, enter `1`,
   choose **Rename**, and enter `BUDDY`. Skip advanced choices and confirm.
4. Select `red pending-edits` and choose the session to review both changes.
5. Select `red validate-edit` and choose the session. Resolve any errors before
   finalizing. This step does not publish a save.
6. Select `red end-edit`. Choose the session and output
   `Pokemon Red Edited.sav`. Skip advanced choices and type `YES` after review.

The final result includes:

```text
Pokemon Red Edited.sav
Pokemon Red Edited.sav.edit-report.json
Pokemon Red Edited.sav.edit-report.md
```

Check the result with `red summary` and then in game. To undo a staged change,
select `red undo-edit`, choose the session, and enter the number of recent edits
to remove. `red edit-history` shows recorded operations; `red annotate-edit`
adds your note. Undoing a session does not erase an already generated output.

Other Red options include names, trainer ID, coins, badges, selected box,
supported events, and JSON-file collection edits. `red bag` adds/removes
supported items. `red pokemon` also offers level and move changes. The direct
`red edit` command opens its own copy-first editor after the guided review.
Arbitrary map/runtime-state editing is restricted; use only verified presets.

### FireRed editing

For a single change, select `fred edit`, input `Pokemon FireRed.sav`, choose
Money, enter `50000`, and choose `Pokemon FireRed Edited.sav`.

For multiple changes, use the parallel `fred begin-edit`, `fred edit-session`,
`fred pending-edits`, `fred validate-edit`, and `fred end-edit` sequence. Current
FireRed named edits include player/rival names, money, coins, and badges.
`fred pokemon` supports party nicknames; it does not offer all Red Pokémon edits.
`fred bag` adjusts an existing identified stack, with zero-based pocket slots
(0 is the first slot). Read its instructions before supplying an item ID.

There is no equivalent complete Japanese editing suite in this build. Do not
feed a Japanese save into an English editor or assume every game has every
command listed for another game.

## 11. Menu 6: compare saves or playthrough progress

### What changed since my backup?

Select `compare progress`:

```text
Older Red save: Pokemon Red Before Brock.sav
Newer Red save: Pokemon Red After Brock.sav
Display: Readable Markdown
JSON report: Pokemon Red Progress.json
Markdown report: Pokemon Red Progress.md
Confirm: YES
```

The saves must belong to the same playthrough, with matching trainer name and
ID. The report can show changed money, badges, Pokémon, inventory, and supported
progress fields. If the older save had no Boulder Badge and the newer one does,
that difference is evidence of progress. Exact messages depend on the files;
the comparison does not observe the actions between the two backups.

| Compare command | Use it for | Example pair |
|---|---|---|
| `compare physical` | Byte differences, offsets, hashes | `Pokemon Red.sav` / `Pokemon Red Edited.sav` |
| `compare semantic` | Meaningful Red JSON fields | `Pokemon Red.red.json` / `Pokemon Red Updated.red.json` |
| `compare semantic-batch` | Several candidates against one baseline | One Red JSON plus multiple Red JSON backups |
| `compare firered-progress` | Two FireRed saves | `Pokemon FireRed.sav` / `Pokemon FireRed After Test.sav` |
| `compare firered-semantic` | FireRed JSON fields | Two `.fred.json` exports |
| `compare firered-pokemon/events/trainers/items/fly/hall-of-fame` | A focused FireRed JSON area; select the actual full command name | Two `.fred.json` exports |
| `compare bridge` | Source Red JSON against a proposed FireRed plan, optionally with its manifest | `Pokemon Red.red.json` / `Pokemon Red Planned.fred.json` |

A result reporting differences is not automatically a failure. Physical
differences between a generated save and its source can be expected. Inspect
the semantic report to understand them. Use the route's conversion report for
cross-generation meaning rather than treating raw byte equality as the goal.

## 12. Menu 7: run proof or emulator checks

A proof task bundles automated checks and reports. It cannot play the game
for you. These workflows are most useful after you understand normal conversion.

### Red generation proof

Select `proof red`, input `Pokemon Red.sav`, and choose a new folder such as
`Pokemon Red Proof`. Choose whether to make a ZIP, skip advanced paths if not
needed, review, and confirm. The folder contains decoded data, generated data,
comparison reports, a manifest, and an emulator checklist.

Typical report names include `proof-manifest.json`, `comparison.md`,
`physical-comparison.json`, `semantic-comparison.json`, and
`emulator-checklist.md`. Read the actual files produced; proof variants differ.
This proves the Red semantic-generation workflow, not Red-to-FireRed conversion.

### Conversion or FireRed proof

| Task | Command | Input example |
|---|---|---|
| English route conversion proof | `proof convert` | Select route, then `Pokemon Red.red.json` or matching Blue JSON |
| Red-to-FireRed proof | `proof red-to-firered` | `Pokemon Red.red.json` |
| Native FireRed generation proof | `proof fred` | Complete `Pokemon FireRed.fred.json` |
| Verify a Red proof directory/ZIP | `proof verify` | `Pokemon Red Proof` or its ZIP |

Select a new destination folder. These checks establish their implemented
static properties and produce manual acceptance instructions. Do not assume
`proof verify` accepts every kind of conversion package simply because it has
a manifest; use it with the supported Red proof package format.

### After manual emulator testing

For Red, use `red validate-post-emulator` with the pre-test and post-test Red
saves and a report folder. `proof post-emulator` can create standalone reports
or update an existing Red proof folder containing `proof-manifest.json`.

For FireRed output, select `fred validate-post-emulator`. For example, choose
`Pokemon Red_fr.sav` as the before file and `Pokemon FireRed After Test.sav` as
the after file, then a new report folder. This checks the saved result after
you completed the manual emulator steps. Play time and other runtime values
may change; inspect the reported classification.

Proof folders may contain your save data and trainer information. Keep them
private unless you deliberately review and share their contents.

## 13. Menu 8: browse every current command and option

This is the complete command directory. The current build contains **124**
catalog entries, including the interactive entry point itself. Each runnable
entry has guided questions; feature maturity differs across games and routes.

Example: find a named gym flag before editing it.

```text
Choose a task: 8
Choose a command: S
Search command or task: red events search
Select the matching command, then 1 Walk me through it.
Search term: VIRIDIAN_GYM
Result format: Text
Confirm: YES
```

This lists matching event definitions. It does not change the save. Use
`red events show` to understand a specific flag before staging an event edit.
Setting a battle flag alone does not automatically reverse all story consequences.

To save the entire directory, find `get-all-cmds`, choose Markdown, and enter
`Pokemon Command List.md`. Leaving the output blank prints it in the terminal.
After upgrading, this command reads the catalog compiled into that executable.

## 14. Menu 9: doctor, settings, and beginner help

- **doctor:** choose a deep check for bundled resources and engine checks. Text
  is easiest to read. A developer build may report `developer-python-fallback`;
  a bundled package should use its packaged runtime.
- **config show:** view the compiled defaults and safety policy. This command
  does not edit preferences.
- **completion:** select Bash, Zsh, or Fish to print shell-completion source.
  It does not automatically edit your shell's startup files.
- **get-all-cmds:** display or save the catalog as described above.
- **convert inspect/explain:** inspect the available event, trainer, or item
  translation rules when a conversion report needs explanation.

If you are unsure, start with doctor and then validate a copy of your save.

## 15. Troubleshooting

| What you see | What to do |
|---|---|
| `pkmn: command not found` | Open a new terminal after installing; use the portable launcher or the explicit local-build path. Check that installation completed. |
| Only the old conversion menu appears | You are running an older executable. Check that `pkmn --version` reports 3.1.0 or newer. |
| File not found | Paste the full path or use Browse. Check the extension and hidden extensions. |
| Output already exists | Choose a new filename. Simple English conversion numbers output families; advanced commands only do so where offered. |
| Japanese archive/projection already exists | Keep it and convert the existing projection to a new destination, or use a separately named source copy. |
| Unknown Japanese revision | Check the source game's revision; do not guess. Green 1.1 is not implemented. |
| Checksum errors | Use supported repair for a new copy. English simple conversion can repair checksum bytes in memory. |
| Invalid species, level, inventory, or Hall of Fame data | Read the details. Repairing checksums cannot fix the meaning of unsupported data. Decide any corrective edits explicitly. |
| Reconstruction requires physical image | Use an archive decoded with that image. An image-free JSON cannot restore exact original bytes. |
| Source profile conflict | Choose the route matching the real source game and its JSON declaration. Renaming the file does not fix the profile. |
| Generated save does not appear in the emulator | Check target game, battery-save import/location, basename, and emulator restart. Avoid loading an older save state. |
| Session source hash changed | The original changed during editing. Start a new session against the intended source; do not bypass the check. |
| Runtime/resources missing | Keep the complete installation together and run doctor. For source builds, check the documented Python/runtime requirements. |

For a bug report, include the command or interactive choices, version, operating
system, and error text at [GitHub Issues](https://github.com/AAAMAQ/pkmn-cli/issues).
Do not attach ROMs or a private save by default. A failure can happen after a
workflow has created some intermediate artifacts; read the error and review
the output folder before retrying.

## 16. Complete command index

### A short glossary before the index

| Word | Meaning in this program |
|---|---|
| Source | The save or JSON you are starting from |
| Target | The game receiving the converted data |
| Checksum | A small stored value used to detect certain changes to save bytes |
| JSON | A text document containing labeled game-data fields |
| Semantic data | The meaning of the save: Pokémon, items, money, flags, and so on |
| Physical image | An archived copy of the original save bytes inside JSON |
| Manifest | A structured record of how a conversion or proof was made |
| Template | The supported baseline used to assemble a target save |
| Session | A file holding your pending edits before final save generation |
| Profile/revision | The declared game layout and version the reader should use |
| Experimental | Implemented, with important real-save or emulator checks still pending |

You do not need to learn these terms before using simple conversion. They help
when reading advanced prompts and reports.

This index was taken from the current local executable on 2 October 2026.
For any entry, open menu 8, search its name, select it, and choose **Walk me
through it**. The usage line is an optional direct-command reference; the
wizard asks you for those pieces separately. Angle-bracket placeholders are
not literal filenames. Square brackets mark optional direct-command arguments.

### General

**`doctor`** — Check internal readiness; --deep runs a deterministic generation self-test.

```text
pkmn doctor [--deep] [--format json]
```

**`completion`** — Generate shell completion source.

```text
pkmn completion <bash|zsh|fish>
```

**`config show`** — Show compiled safety and default policy.

```text
pkmn config show [--format text|json]
```

**`get-all-cmds`** — Print the complete command catalog compiled into this executable.

```text
pkmn get-all-cmds [--format text|json|markdown] [--output <file>]
```

**`interactive`** — Open guided tasks and a searchable browser for all current commands.

```text
pkmn interactive
```

### Pokemon Red saves

**`red summary`** — Create a readable trainer, progress, party, inventory, and storage summary.

```text
pkmn red summary <save.sav> [--format text|json|markdown] [--output <file>]
```

**`red inspect`** — Inspect save size and checksum integrity.

```text
pkmn red inspect <save.sav> [--format json]
```

**`red validate`** — Validate all known Red checksums.

```text
pkmn red validate <save.sav> [--format json]
```

**`red repair-checksums`** — Write a checksum-repaired copy without modifying the source.

```text
pkmn red repair-checksums <save.sav> [--output <copy.sav>] [--auto-suffix]
```

**`red decode`** — Export deterministic canonical Red JSON.

```text
pkmn red decode <save.sav> [--output <file.red.json>|-] [--include-physical-image|--no-physical-image] [--auto-suffix]
```

**`red events list`** — List verified named event flags.

```text
pkmn red events list [--category <category>] [--format json]
```

**`red events search`** — Search verified named event flags.

```text
pkmn red events search <query> [--format json]
```

**`red events show`** — Show one verified event definition.

```text
pkmn red events show <EVENT_NAME> [--format json]
```

**`red validate-batch`** — Validate several saves in one command.

```text
pkmn red validate-batch <save.sav>... [--format json]
```

**`red decode-batch`** — Decode several saves into a transactional output directory.

```text
pkmn red decode-batch <save.sav>... --output-dir <directory> [--no-physical-image]
```

**`red validate-post-emulator`** — Analyze and classify an emulator round trip.

```text
pkmn red validate-post-emulator <before.sav> <after.sav> [--output-dir <directory>]
```

### Japanese Pokemon Red

**`red-jp validate`** — Validate Japanese Red layout and main checksum.

```text
pkmn red-jp validate <save.sav> --profile JP_RED_REV0|JP_RED_REV1 [--format json]
```

**`red-jp decode`** — Archive a Japanese Red save with exact Japanese name bytes.

```text
pkmn red-jp decode <save.sav> --profile JP_RED_REV0|JP_RED_REV1 [--output <file.red.jp.json>] [--no-physical-image]
```

**`red-jp convert`** — Archive, project, and convert Japanese Red into international FireRed.

```text
pkmn red-jp convert <save.sav> --profile JP_RED_REV0|JP_RED_REV1 [--output <file_fr.sav>] [--template <clean.sav>] [--salt <value>] [--retain-playername]
```

### Japanese Red JSON

**`rjpjson inspect`** — Inspect a Japanese Red archive.

```text
pkmn rjpjson inspect <file.red.jp.json>
```

**`rjpjson validate`** — Validate Japanese archive structure and physical provenance.

```text
pkmn rjpjson validate <file.red.jp.json>
```

**`rjpjson reconstruct`** — Reconstruct the original Japanese Red save bytes.

```text
pkmn rjpjson reconstruct <file.red.jp.json> [--output <save.sav>]
```

**`rjpjson project`** — Make an English-shaped bridge projection retaining Japanese name provenance.

```text
pkmn rjpjson project <file.red.jp.json> [--output <file.red.json>] [--retain-playername]
```

**`rjpjson compare`** — Compare a projection against its Japanese archive and slot mapping.

```text
pkmn rjpjson compare <file.red.jp.json> <file.red.json>
```

### Japanese Pokemon Green

**`green-jp validate`** — Validate Japanese Green layout and main checksum.

```text
pkmn green-jp validate <save.sav> --profile JP_GREEN_REV0 [--format json]
```

**`green-jp decode`** — Archive a Japanese Green save with exact Japanese name bytes.

```text
pkmn green-jp decode <save.sav> --profile JP_GREEN_REV0 [--output <file.green.jp.json>] [--no-physical-image]
```

**`green-jp convert`** — Experimentally archive, project, and convert Japanese Green into international FireRed.

```text
pkmn green-jp convert <save.sav> --profile JP_GREEN_REV0 [--output <file_fr.sav>] [--template <clean.sav>] [--salt <value>] [--retain-playername]
```

### Japanese Green JSON

**`gjpjson inspect`** — Inspect a Japanese Green archive.

```text
pkmn gjpjson inspect <file.green.jp.json>
```

**`gjpjson validate`** — Validate Japanese archive structure and physical provenance.

```text
pkmn gjpjson validate <file.green.jp.json>
```

**`gjpjson reconstruct`** — Reconstruct the original Japanese Green save bytes.

```text
pkmn gjpjson reconstruct <file.green.jp.json> [--output <save.sav>]
```

**`gjpjson project`** — Make an English-shaped bridge projection retaining Japanese name provenance.

```text
pkmn gjpjson project <file.green.jp.json> [--output <file.red.json>] [--retain-playername]
```

**`gjpjson compare`** — Compare a projection against its Japanese archive and slot mapping.

```text
pkmn gjpjson compare <file.green.jp.json> <file.red.json>
```

### Pokemon Red to FireRed conversion

**`red convert`** — Validate and convert a Pokemon Red save into an auditable FireRed save.

```text
pkmn red convert <save.sav> [output.sav] [--auto-repair-checksum] [--write-repaired-source <copy.sav>] [--template <clean-fire-red.sav>] [--salt <value>] [--keep-intermediate] [--auto-suffix]
```

**`rjson convert`** — Convert canonical Red JSON directly into an auditable FireRed save.

```text
pkmn rjson convert <save.red.json> [output.sav] [--template <clean-fire-red.sav>] [--salt <value>] [--keep-intermediate] [--auto-suffix]
```

**`rjson convert_to_frjson`** — Translate canonical Red semantics into a proposed FireRed JSON document.

```text
pkmn rjson convert_to_frjson <save.red.json> [output.fred.json] [--salt <value>] [--auto-suffix]
```

**`convert red-firered`** — Convert either supported Red source form to FireRed.

```text
pkmn convert red-firered <red.sav|red.json> [output.sav] [conversion options]
```

**`convert routes`** — List typed game profiles, conversion capabilities, and evidence labels.

```text
pkmn convert routes [--format json]
```

**`convert inspect`** — Inspect pinned bridge authority records.

```text
pkmn convert inspect <event|trainer|item> [query]
```

**`convert explain`** — Explain a bridge decision with source evidence.

```text
pkmn convert explain <event|trainer|item> <query>
```

**`convert validate-manifest`** — Validate an auditable conversion manifest.

```text
pkmn convert validate-manifest <conversion-manifest.json>
```

**`convert batch`** — Convert several declared sources through one selected available route.

```text
pkmn convert batch <source.sav|source.json>... --route <route> --output-dir <directory> [--template <clean.sav>]
```

### Pokemon Red editing

**`red edit`** — Open the interactive copy-first editor.

```text
pkmn red edit <save.sav>
```

**`red begin-edit`** — Start a scriptable semantic edit session.

```text
pkmn red begin-edit <save.sav> [--output <session.json>]
```

**`red edit-session`** — Stage and validate one or more edits.

```text
pkmn red edit-session <session.json> <edits...> [--dry-run] [--format json] [--explain-error]
```

**`red pokemon`** — Apply coherent party-Pokemon name, level/stat, or move/PP edits.

```text
pkmn red pokemon <session.json> <party|species|nickname> <value> <rename|level|move> ... [--dry-run]
```

**`red bag`** — Add, merge, or remove bag stacks with synchronized slots and counts.

```text
pkmn red bag <session.json> <add <item> <quantity>|remove <item>> [--dry-run]
```

**`red progress`** — Apply verified progress presets without arbitrary map-state mutation.

```text
pkmn red progress <session.json> fly-destinations all [--dry-run]
```

**`red pending-edits`** — Show staged edits.

```text
pkmn red pending-edits <session.json> [--format json]
```

**`red undo-edit`** — Undo the most recent staged edits.

```text
pkmn red undo-edit <session.json> [--count <number>]
```

**`red edit-history`** — Show edit history and annotations.

```text
pkmn red edit-history <session.json> [--format json]
```

**`red annotate-edit`** — Attach a note to an edit session.

```text
pkmn red annotate-edit <session.json> <note>
```

**`red validate-edit`** — Run generation, checksum, re-decode, and semantic validation without writing a save.

```text
pkmn red validate-edit <session.json>
```

**`red end-edit`** — Publish a validated edited copy and reports.

```text
pkmn red end-edit <session.json> [--output <save.sav>] [--auto-suffix] [--dry-run] [--format json]
```

### Canonical Red JSON

**`rjson inspect`** — Inspect canonical Red JSON and archival-image status.

```text
pkmn rjson inspect <file.red.json|-> [--format json]
```

**`rjson validate`** — Validate schema, semantics, and optional physical image.

```text
pkmn rjson validate <file.red.json|-> [--format json] [--profile standard|strict|generation|archival]
```

**`rjson generate`** — Generate a deterministic save from semantic fields only.

```text
pkmn rjson generate <file.red.json|-> [output.sav|-] [--auto-suffix]
```

**`rjson reconstruct`** — Reconstruct archived source bytes from physicalImage.

```text
pkmn rjson reconstruct <file.red.json|-> [--output <save.sav>|-] [--auto-suffix]
```

**`rjson migrate`** — Apply compatible deterministic schema enrichment.

```text
pkmn rjson migrate <file.red.json> [--output <migrated.red.json>] [--auto-suffix]
```

**`rjson schema`** — Describe the supported canonical schema contract.

```text
pkmn rjson schema [--format json]
```

**`rjson generate-batch`** — Generate several semantic saves transactionally.

```text
pkmn rjson generate-batch <file.red.json>... --output-dir <directory>
```

**`rjson update_schema`** — Safely migrate Red JSON to the latest supported schema.

```text
pkmn rjson update_schema <save.red.json> [--output <updated.red.json>] [--auto-suffix]
```

### Kanto remake conversion

**`rjson convert_to_lgjson`** — Translate canonical Red semantics into a proposed LeafGreen JSON document.

```text
pkmn rjson convert_to_lgjson <save.red.json> [output.lg.json] [--salt <value>] [--auto-suffix]
```

**`convert red-leafgreen`** — Convert a declared Pokemon Red source to LeafGreen (static/community-test evidence).

```text
pkmn convert red-leafgreen <red.sav|red.json> [output.sav] [conversion options]
```

**`convert blue-firered`** — Convert a declared Pokemon Blue source to FireRed (static/community-test evidence).

```text
pkmn convert blue-firered <blue.sav|blue.json> [output.sav] [conversion options]
```

**`convert blue-leafgreen`** — Convert a declared Pokemon Blue source to LeafGreen (static/community-test evidence).

```text
pkmn convert blue-leafgreen <blue.sav|blue.json> [output.sav] [conversion options]
```

### FireRed JSON

**`frjson inspect`** — Inspect native or planned FireRed JSON.

```text
pkmn frjson inspect <save.fred.json>
```

**`frjson validate`** — Validate native or planned FireRed JSON.

```text
pkmn frjson validate <save.fred.json>
```

**`frjson schema`** — Describe native and planned FireRed JSON contracts and release gates.

```text
pkmn frjson schema [--format json]
```

**`frjson update_schema`** — Safely migrate FireRed JSON to the latest supported schema.

```text
pkmn frjson update_schema <save.fred.json> [--output <updated.fred.json>] [--auto-suffix]
```

**`frjson generate`** — Generate a template-backed FireRed save from accepted FireRed semantics.

```text
pkmn frjson generate <save.fred.json> [output.sav] [--template <clean-fire-red.sav>]
```

**`frjson reconstruct`** — Reconstruct archived FireRed source bytes from physicalImage.

```text
pkmn frjson reconstruct <save.fred.json> [--output <save.sav>]
```

**`frjson migrate`** — Apply compatible deterministic FireRed schema enrichment.

```text
pkmn frjson migrate <save.fred.json> [--output <migrated.fred.json>] [--auto-suffix]
```

**`frjson generate-batch`** — Generate several native or planned FireRed saves.

```text
pkmn frjson generate-batch <save.fred.json>... --output-dir <directory> [--template <clean.sav>]
```

### Pokemon FireRed saves

**`fred summary`** — Create a compact or detailed FireRed save summary.

```text
pkmn fred summary <save.sav> [--detailed]
```

**`fred inspect`** — Inspect FireRed slots, counters, sectors, and checksum state.

```text
pkmn fred inspect <save.sav> [--format json]
```

**`fred validate`** — Validate FireRed sectors, active slot, and checksums.

```text
pkmn fred validate <save.sav> [--format json]
```

**`fred decode`** — Export complete native FireRed schema 0.4.0 JSON.

```text
pkmn fred decode <save.sav> [--output <save.fred.json>]
```

**`fred repair-checksums`** — Repair recognized FireRed main-section checksums in a new copy.

```text
pkmn fred repair-checksums <save.sav> [--output <copy.sav>]
```

**`fred validate-batch`** — Validate several FireRed saves.

```text
pkmn fred validate-batch <save.sav>... [--format json]
```

**`fred decode-batch`** — Decode several FireRed saves transactionally.

```text
pkmn fred decode-batch <save.sav>... --output-dir <directory>
```

**`fred validate-post-emulator`** — Validate a FireRed emulator save/close/reload round trip.

```text
pkmn fred validate-post-emulator <before.sav> <after.sav> [--output-dir <directory>]
```

**`fred events list`** — List pinned pret FireRed flags or variables.

```text
pkmn fred events list [--kind flag|variable] [--format json]
```

**`fred events search`** — Search pinned pret FireRed flags or variables.

```text
pkmn fred events search <query> [--kind flag|variable] [--format json]
```

**`fred events show`** — Show one pinned pret FireRed flag or variable.

```text
pkmn fred events show <name|id> [--kind flag|variable] [--format json]
```

### Pokemon FireRed editing

**`fred edit`** — Apply narrow validated copy-first FireRed edits.

```text
pkmn fred edit <save.sav> [--output <copy.sav>] [safe edits]
```

**`fred begin-edit`** — Start a safe FireRed edit session.

```text
pkmn fred begin-edit <save.sav> [--output <session.json>]
```

**`fred edit-session`** — Stage player, rival, money, coins, or badge edits.

```text
pkmn fred edit-session <session.json> [safe edit options]
```

**`fred pokemon`** — Stage a safe party-Pokemon nickname edit.

```text
pkmn fred pokemon <session.json> party <slot> rename <name>
```

**`fred bag`** — Change an existing verified item stack quantity.

```text
pkmn fred bag <session.json> quantity <pocket> <slot> <item-id> <quantity>
```

**`fred progress`** — Stage the compact badge progress edit supported by Save Genie.

```text
pkmn fred progress <session.json> badge <1-8> <on|off>
```

**`fred pending-edits`** — Show staged FireRed edits.

```text
pkmn fred pending-edits <session.json>
```

**`fred undo-edit`** — Undo staged FireRed edits.

```text
pkmn fred undo-edit <session.json> [--count <number>]
```

**`fred edit-history`** — Show FireRed edit history.

```text
pkmn fred edit-history <session.json>
```

**`fred annotate-edit`** — Annotate a FireRed edit session.

```text
pkmn fred annotate-edit <session.json> <note>
```

**`fred validate-edit`** — Validate staged edits without publishing a save.

```text
pkmn fred validate-edit <session.json>
```

**`fred end-edit`** — Publish a validated edited FireRed copy.

```text
pkmn fred end-edit <session.json> [--output <copy.sav>]
```

### Pokemon Blue saves

**`blue decode`** — Decode through the shared Gen I engine with an explicit GEN1_BLUE profile.

```text
pkmn blue decode <save.sav> [--output <save.blue.json>]
```

**`blue convert`** — Convenience alias for the paired Blue-to-LeafGreen route.

```text
pkmn blue convert <save.sav> [output.sav]
```

### Pokemon Blue JSON

**`bjson convert`** — Convert canonical Blue JSON to LeafGreen.

```text
pkmn bjson convert <save.blue.json> [output.sav]
```

**`bjson convert_to_lgjson`** — Plan Blue-to-LeafGreen conversion without writing a save.

```text
pkmn bjson convert_to_lgjson <save.blue.json> [output.lg.json]
```

**`bjson convert_to_frjson`** — Plan Blue-to-FireRed conversion without writing a save.

```text
pkmn bjson convert_to_frjson <save.blue.json> [output.fred.json]
```

### Pokemon LeafGreen saves

**`leafgreen decode`** — Decode through the shared Kanto-remake engine with GEN3_LEAFGREEN profile.

```text
pkmn leafgreen decode <save.sav> [--output <save.lg.json>]
```

**`leafgreen validate`** — Validate the shared Gen III Kanto-remake save container.

```text
pkmn leafgreen validate <save.sav> [--format json]
```

### Pokemon LeafGreen JSON

**`lgjson generate`** — Generate a LeafGreen-profile save through the shared remake engine.

```text
pkmn lgjson generate <save.lg.json> [output.sav] [--template <clean.sav>]
```

**`lgjson validate`** — Validate native or planned LeafGreen-profile JSON.

```text
pkmn lgjson validate <save.lg.json>
```

### Proof workflows

**`proof convert`** — Create a deterministic static proof package for any available conversion route.

```text
pkmn proof convert <route> <source.json> [--output-dir <directory>]
```

### Comparison

**`compare progress`** — Explain gameplay progress between two backups from the same playthrough.

```text
pkmn compare progress <older.sav> <newer.sav> [report options]
```

**`compare physical`** — Compare physical bytes, ranges, percentages, and hashes.

```text
pkmn compare physical <a.sav> <b.sav> [report options]
```

**`compare semantic`** — Compare canonical semantic fields.

```text
pkmn compare semantic <a.red.json> <b.red.json> [report options]
```

**`compare semantic-batch`** — Compare several canonical documents to one baseline.

```text
pkmn compare semantic-batch <baseline.red.json> <candidate.red.json>... [--format json]
```

**`compare firered-semantic`** — Compare FireRed semantics while excluding archival physical bytes.

```text
pkmn compare firered-semantic <a.fred.json> <b.fred.json> [--output-json <file>]
```

**`compare firered-progress`** — Decode and compare two FireRed progress saves.

```text
pkmn compare firered-progress <older.sav> <newer.sav> [--output-json <file>]
```

**`compare firered-pokemon`** — Compare party, PC, and daycare data.

```text
pkmn compare firered-pokemon <a.fred.json> <b.fred.json>
```

**`compare firered-events`** — Compare event and variable state.

```text
pkmn compare firered-events <a.fred.json> <b.fred.json>
```

**`compare firered-trainers`** — Compare trainer-defeat state.

```text
pkmn compare firered-trainers <a.fred.json> <b.fred.json>
```

**`compare firered-items`** — Compare inventory and obtained-history state.

```text
pkmn compare firered-items <a.fred.json> <b.fred.json>
```

**`compare firered-fly`** — Compare Fly destinations.

```text
pkmn compare firered-fly <a.fred.json> <b.fred.json>
```

**`compare firered-hall-of-fame`** — Compare Hall of Fame data.

```text
pkmn compare firered-hall-of-fame <a.fred.json> <b.fred.json>
```

**`compare bridge`** — Audit Red source, proposed FireRed target, and optional manifest.

```text
pkmn compare bridge <red.json> <fred.json> [--manifest <file>]
```

### Proof

**`proof red`** — Run decode, generation, comparison, determinism, and isolation proofs.

```text
pkmn proof red <source.sav> [--output-dir <directory>] [--zip|--zip-output <archive.zip>] [--auto-suffix]
```

**`proof post-emulator`** — Continue a proof package after manual emulator testing.

```text
pkmn proof post-emulator --before <save.sav> --after <save.sav> [--output-dir <directory>|--proof-dir <directory>]
```

**`proof verify`** — Verify proof hashes, schemas, ZIP safety, and generated checksums.

```text
pkmn proof verify <proof-directory|proof.zip> [--format json]
```

**`proof fred`** — Run the automated Phase 5 native FireRed generation proof.

```text
pkmn proof fred <complete.fred.json> [--template <clean.sav>] [--output-dir <directory>]
```

**`proof red-to-firered`** — Run the automated Phase 6 conversion proof and prepare MAQ verification.

```text
pkmn proof red-to-firered <save.red.json> [--template <clean.sav>] [--output-dir <directory>]
```

## Further reading

- [Installation details](INSTALL.md)
- [Detailed interactive design and controls](INTERACTIVE_V3_GUIDE.md)
- [Individual command conversation examples](INTERACTIVE_COMMAND_EXAMPLES.md)
- [Japanese Green support and test evidence](JAPANESE_GREEN_IMPLEMENTATION.md)
- [Future command proposals](FUTURE_COMMANDS.md)
- [Privacy and publication guide](PRIVACY_AND_PUBLICATION.md)
