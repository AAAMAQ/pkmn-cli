# Complete pkmn Usage Guide

This guide explains every command endpoint available in `pkmn 0.1.0`. It
assumes your Pokemon Red save is named `backup.sav`.

`pkmn` never needs a ROM. Commands that create files refuse to replace an
existing file by default. Keep an untouched copy of every important save.

## Before typing a command

Open Terminal and move into the folder containing `backup.sav`. For example,
if the save is inside a folder named `Pokemon Saves` in your home folder:

```sh
cd "$HOME/Pokemon Saves"
ls
```

You should see `backup.sav` in the `ls` output. You can then type commands such
as:

```sh
pkmn red validate backup.sav
```

If the save is elsewhere, either change into its folder or drag the file from
Finder into Terminal after typing the command. Dragging it inserts the complete
path:

```sh
pkmn red validate "/full/path/to/backup.sav"
```

Quotes are required when a path contains spaces. In the examples below:

- `backup.sav` is the original backup.
- `newer-backup.sav` is a later save from the same playthrough.
- `after-emulator.sav` is a save written by the emulator after a manual test.
- `backup.red.json` is created by decoding `backup.sav`.
- Commands marked **read-only** do not create or modify files.

## Basic program commands

These two program controls are not part of the 38 endpoint count, but are the
first commands to know.

### Help

Type:

```sh
pkmn --help
```

Expected result: the terminal shows all 38 available endpoints, short
descriptions, examples, global controls, and the unimplemented future domains.
No files are created.

### Version

Type:

```sh
pkmn --version
```

Expected result for this release:

```text
pkmn 0.1.0
```

No files are created.

## General commands

### 1. `pkmn doctor` — check whether the CLI is ready

Type:

```sh
pkmn doctor
```

Expected result: each internal Red module is marked `[ok]`, followed by
`Standalone readiness: ready`. This is read-only and does not examine
`backup.sav`.

Run the stronger deterministic self-test with:

```sh
pkmn doctor --deep
```

Expected result: the normal readiness report plus
`[ok] deep deterministic generation self-test`. For structured output:

```sh
pkmn doctor --deep --format json
```

### 2. `pkmn completion` — generate shell completions

For the default macOS zsh shell, type:

```sh
pkmn completion zsh
```

Expected result: completion-script text beginning with `#compdef pkmn` is
printed to the terminal. It does not process a save. Bash and fish users can
instead type:

```sh
pkmn completion bash
pkmn completion fish
```

Homebrew installs the zsh completion automatically. The printed form is useful
for manual shell configuration.

### 3. `pkmn config show` — show built-in safety policy

Type:

```sh
pkmn config show
```

Expected result: a readable list of immutable defaults, including standalone
operation, collision refusal, copy-first editing, and the `physicalImage`
authority boundary. For automation:

```sh
pkmn config show --format json
```

This is read-only and is not a user preference editor.

### 4. `pkmn get-all-cmds` — show the authoritative command catalog

Type:

```sh
pkmn get-all-cmds
```

Expected result: all 38 compiled endpoints, usage lines, and explanations are
printed. This list updates automatically when the Homebrew package is updated.

Save it as a Markdown document:

```sh
pkmn get-all-cmds --format markdown --output pkmn-commands.md
```

Expected files:

```text
pkmn-commands.md
```

The command refuses to replace that file if it already exists. JSON is also
available with `--format json`.

## Pokemon Red save commands

### 5. `pkmn red summary` — readable save analysis

Type:

```sh
pkmn red summary backup.sav
```

Expected result: a terminal summary containing the trainer and rival, Trainer
ID, playtime, location, money, coins, badges, Pokedex counts, party, bag and PC
items, PC box counts, Daycare, Hall of Fame, event totals, battles, encounters,
story progress, and checksum status. The save is not modified.

Create a report for a person to read:

```sh
pkmn red summary backup.sav --format markdown --output backup-summary.md
```

Or structured statistics for software:

```sh
pkmn red summary backup.sav --format json --output backup-summary.json
```

Expected output is the requested report file. Existing reports are not
overwritten.

### 6. `pkmn red inspect` — inspect save integrity

Type:

```sh
pkmn red inspect backup.sav
```

Expected result: the terminal shows the save size, main checksum, Bank 2 and
Bank 3 checksums, and all 12 box checksums. Valid saves show `valid` for every
checksum. This is read-only.

For JSON output:

```sh
pkmn red inspect backup.sav --format json
```

### 7. `pkmn red validate` — pass/fail save validation

Type:

```sh
pkmn red validate backup.sav
```

Expected result for a healthy save: the checksum details followed by
`Validation: passed`. A malformed or corrupt save returns a non-zero exit code
and identifies the failing checksum. No file is written.

For scripts:

```sh
pkmn red validate backup.sav --format json
```

### 8. `pkmn red repair-checksums` — create a repaired copy

Type:

```sh
pkmn red repair-checksums backup.sav
```

Expected files:

```text
backup_repaired-checksums.sav
backup_repaired-checksums.sav.repair-report.json
```

Expected terminal output says the repair copy was created, reports how many
checksum bytes changed, and confirms `Source overwritten: no`. This repairs
known checksum bytes; it cannot prove that unrelated corrupt gameplay data is
correct.

Choose a name explicitly with:

```sh
pkmn red repair-checksums backup.sav --output repaired-backup.sav
```

Use `--auto-suffix` to select a safe `_2`, `_3`, and later name if output
already exists.

### 9. `pkmn red decode` — export canonical `.red.json`

Type:

```sh
pkmn red decode backup.sav
```

Expected file:

```text
backup.red.json
```

Expected result: a deterministic canonical document containing source
metadata, checksum status, semantic save data, all supported party/storage and
world-state fields, and—by default—the archival `physicalImage`. The terminal
reports the output name and physical-image status.

Create a semantic-only export without archival bytes:

```sh
pkmn red decode backup.sav --no-physical-image
```

Choose a different path:

```sh
pkmn red decode backup.sav --output my-backup.red.json
```

Use `--include-physical-image` to state archival inclusion explicitly, or
`--auto-suffix` for a safe numbered output. Treat every `.red.json` as private
save data even when `physicalImage` is absent.

### 10. `pkmn red events list` — list verified event flags

Type:

```sh
pkmn red events list
```

Expected result: the verified 507-entry Pokemon Red event catalog is printed
with flag index, internal `EVENT_NAME`, category, and readable label. This does
not read `backup.sav` and creates no files.

Filter a category or request JSON:

```sh
pkmn red events list --category major_story
pkmn red events list --format json
```

### 11. `pkmn red events search` — find an event name

Type:

```sh
pkmn red events search brock
```

Expected result: matching verified entries such as the Brock battle event are
printed. This is useful before an event edit. For structured results:

```sh
pkmn red events search brock --format json
```

### 12. `pkmn red events show` — inspect one exact event

After finding an event name, type:

```sh
pkmn red events show EVENT_BEAT_BROCK
```

Expected result: one verified definition with its flag index, category,
readable label, and trainer/static/story classifications. The name is
case-sensitive. An unknown name fails safely.

```sh
pkmn red events show EVENT_BEAT_BROCK --format json
```

### 13. `pkmn red validate-batch` — validate several saves

Assume you also have `newer-backup.sav`. Type:

```sh
pkmn red validate-batch backup.sav newer-backup.sav
```

Expected result: one validation status per input and a valid/total summary. No
files are created. Machine-readable output is available with:

```sh
pkmn red validate-batch backup.sav newer-backup.sav --format json
```

The command returns failure if any input is invalid, which makes it useful in
scripts.

### 14. `pkmn red decode-batch` — decode several saves

Type:

```sh
pkmn red decode-batch backup.sav newer-backup.sav --output-dir decoded-saves
```

Expected result: a new `decoded-saves` directory containing one canonical
`.red.json` per save plus the batch manifest produced by the transactional
workflow. The complete directory is published only if every input succeeds.

For semantic-only exports:

```sh
pkmn red decode-batch backup.sav newer-backup.sav \
  --output-dir decoded-saves --no-physical-image
```

The output directory must not already exist.

### 15. `pkmn red validate-post-emulator` — analyze an emulator round trip

Use `backup.sav` as the save before testing and `after-emulator.sav` as the
save written by the emulator afterward:

```sh
pkmn red validate-post-emulator backup.sav after-emulator.sav
```

Expected result: post-emulator validation runs through the same proof engine
and creates the default validation report directory beside the before-save.
The terminal reports whether the round trip passed and confirms neither source
file was modified.

Choose the directory explicitly:

```sh
pkmn red validate-post-emulator backup.sav after-emulator.sav \
  --output-dir emulator-validation
```

Expected files in that directory include
`post-emulator-validation.json` and `post-emulator-validation.md`.

## Pokemon Red editing commands

All edit workflows protect `backup.sav` with its SHA-256 identity and publish a
new validated save. Arbitrary location editing is disabled except for the
verified `reds-house-2f` preset.

### 16. `pkmn red edit` — interactive editor

Type:

```sh
pkmn red edit backup.sav
```

Expected result: a numbered terminal menu opens. Select trainer, money, party,
inventory, boxes, events, and other supported areas; preview pending edits;
validate; then choose **Save validated edited copy**. The default output name
describes the edit, for example:

```text
backup_generated_money.sav
backup_generated_money.sav.edit-report.json
backup_generated_money.sav.edit-report.md
```

Multiple different edits use `backup_generated_manual-edit.sav`. Choosing
discard leaves no edited save and never changes `backup.sav`.

### 17. `pkmn red begin-edit` — begin a scriptable edit session

Type:

```sh
pkmn red begin-edit backup.sav
```

Expected file:

```text
backup.edit-session.json
```

Expected terminal output confirms the source is read-only and `physicalImage`
is excluded. The session contains semantic working data, source identity, edit
history, and pending edits. Choose another session path with:

```sh
pkmn red begin-edit backup.sav --output my-session.json
```

### 18. `pkmn red edit-session` — stage one or more edits

After `begin-edit`, type for example:

```sh
pkmn red edit-session backup.edit-session.json \
  --money 5000 --coins 250 --badges 1
```

Expected result: all edits are validated together, saved into the session, and
the terminal reports the pending-edit count. Supported simple options include
`--trainer-name`, `--rival-name`, `--trainer-id`, `--money`, `--coins`,
`--badges`, `--selected-box`, and `--event EVENT_NAME on|off`.

Test an edit without changing the session:

```sh
pkmn red edit-session backup.edit-session.json --money 5000 --dry-run
```

Add `--format json` for structured validation output. Complex structures can
be supplied with the documented `--bag-file`, `--party-file`, box, Daycare,
Hall-of-Fame, Pokedex, options, playtime, and world-state file options.

For common Pokémon, inventory, and progress edits, prefer the synchronized
semantic commands:

```sh
pkmn red pokemon backup.edit-session.json party 1 rename SUWII
pkmn red pokemon backup.edit-session.json nickname SUWII level 100
pkmn red pokemon backup.edit-session.json party 1 move replace 1 FLY
pkmn red bag backup.edit-session.json add "MASTER BALL" 99
pkmn red bag backup.edit-session.json remove "POKE BALL"
pkmn red progress backup.edit-session.json fly-destinations all
```

Use `--dry-run` with any semantic command. Add `--explain-error` to
`edit-session` for corrective guidance when a low-level JSON-pointer edit is
rejected. The progress preset marks all 11 Fly destinations, not every map.

### 19. `pkmn red pending-edits` — review staged changes

Type:

```sh
pkmn red pending-edits backup.edit-session.json
```

Expected result: the number of pending edits and each semantic path's previous
and proposed value are printed. The session is not changed.

```sh
pkmn red pending-edits backup.edit-session.json --format json
```

### 20. `pkmn red undo-edit` — remove recent staged edits

Undo the most recent edit:

```sh
pkmn red undo-edit backup.edit-session.json
```

Undo the two most recent edits:

```sh
pkmn red undo-edit backup.edit-session.json --count 2
```

Expected result: the session is safely updated and the terminal reports how
many edits were undone and how many remain. No `.sav` is written.

### 21. `pkmn red edit-history` — inspect session history

Type:

```sh
pkmn red edit-history backup.edit-session.json
```

Expected result: command-history and annotation counts are shown. For complete
machine-readable history:

```sh
pkmn red edit-history backup.edit-session.json --format json
```

This is read-only.

### 22. `pkmn red annotate-edit` — attach a note

Type:

```sh
pkmn red annotate-edit backup.edit-session.json "Preparing a Brock backup"
```

Expected result: `Edit-session annotation recorded`. The note is stored in the
session history and does not affect generated gameplay data.

### 23. `pkmn red validate-edit` — validate without publishing

Type:

```sh
pkmn red validate-edit backup.edit-session.json
```

Expected result: generation, checksum regeneration, re-decoding, and semantic
comparison run in memory. A successful result shows `Edit validation: passed`,
the pending-edit count, `Source unchanged: yes`, and
`Generated checksums: valid`. No save is written.

### 24. `pkmn red end-edit` — publish the edited copy

Type:

```sh
pkmn red end-edit backup.edit-session.json
```

Expected result: a new `.sav`, JSON edit report, and Markdown edit report are
written. A single money edit produces `backup_generated_money.sav`; mixed
edits produce `backup_generated_manual-edit.sav`. The terminal confirms
checksum validation and that the source was not overwritten.

Choose the output explicitly:

```sh
pkmn red end-edit backup.edit-session.json --output my-edited-backup.sav
```

Preview without writing:

```sh
pkmn red end-edit backup.edit-session.json --dry-run
```

Use `--auto-suffix` to select a safe numbered name when output exists, and
`--format json` for structured dry-run output.

## Canonical Red JSON commands

Run `pkmn red decode backup.sav` first so `backup.red.json` exists.

### 25. `pkmn rjson inspect` — inspect canonical JSON

Type:

```sh
pkmn rjson inspect backup.red.json
```

Expected result: schema version, semantic validity, physical-image status,
generation isolation policy, trainer, party count, and PC box count are shown.
No file is changed.

```sh
pkmn rjson inspect backup.red.json --format json
```

### 26. `pkmn rjson validate` — validate canonical JSON

Type:

```sh
pkmn rjson validate backup.red.json
```

Expected result: schema `0.1.0`, semantics, optional physical image, named
events, and the standard validation profile are checked. A healthy document
shows `Validation profile: standard (passed)`.

Other policies are explicit:

```sh
pkmn rjson validate backup.red.json --profile strict
pkmn rjson validate backup.red.json --profile generation
pkmn rjson validate backup.red.json --profile archival
pkmn rjson validate backup.red.json --format json --profile generation
```

`generation` proves the semantic fields can generate; `archival` requires a
valid `physicalImage`; `strict` requires the full verified named-event catalog.

### 27. `pkmn rjson generate` — generate from semantic fields

Type:

```sh
pkmn rjson generate backup.red.json
```

Expected files:

```text
backup_generated_semantic.sav
backup_generated_semantic.sav.generation-report.json
backup_generated_semantic.sav.generation-report.md
```

Expected terminal output says `physicalImage` was ignored and integrity is
valid. Generation uses supported semantic fields only, canonicalizes unsafe
location state to the verified Red's-house preset, and regenerates checksums.

Choose another name or permit a safe suffix:

```sh
pkmn rjson generate backup.red.json my-generated.sav
pkmn rjson generate backup.red.json --auto-suffix
```

### 28. `pkmn rjson reconstruct` — restore archived source bytes

Type:

```sh
pkmn rjson reconstruct backup.red.json
```

Expected file:

```text
backup_reconstructed_from-physical-image.sav
```

Expected terminal output explicitly says `physicalImage authority`. The output
must match the source SHA-256 recorded during decode. This fails if the JSON was
created with `--no-physical-image`. Reconstruction is archival restoration, not
semantic generation.

```sh
pkmn rjson reconstruct backup.red.json --output restored-backup.sav
```

### 29. `pkmn rjson migrate` — enrich compatible canonical JSON

Type:

```sh
pkmn rjson migrate backup.red.json
```

Expected file:

```text
backup.migrated.red.json
```

Expected result: compatible schema `0.1.0` data is deterministically enriched
with current tool metadata and the named-event catalog when needed, without
changing semantic values. The terminal reports whether named events were added
or already present.

### 30. `pkmn rjson schema` — describe the schema contract

Type:

```sh
pkmn rjson schema
```

Expected result: format name, schema version, extension, profiles, authority
boundary, and verified event count are printed. This does not read a file.

```sh
pkmn rjson schema --format json
```

### 31. `pkmn rjson generate-batch` — generate several saves

Assume `backup.red.json` and `newer-backup.red.json` both exist. Type:

```sh
pkmn rjson generate-batch backup.red.json newer-backup.red.json \
  --output-dir generated-saves
```

Expected result: a new `generated-saves` directory contains each generated
`.sav`, its JSON/Markdown generation reports, and `batch-manifest.json`. The
directory is published transactionally only if all inputs succeed and must not
already exist.

## Comparison commands

Comparison commands print Markdown by default. Add `--format json` for JSON on
the terminal, or `--output-json report.json` and
`--output-markdown report.md` to save both forms.

### 32. `pkmn compare progress` — explain what happened since a backup

Put the older save first and the newer save second:

```sh
pkmn compare progress backup.sav newer-backup.sav
```

Expected result: a readable playthrough-progress report explains elapsed time,
money and coins gained or spent, badges, Pokedex, location, party, items,
storage, Hall of Fame, verified events, trainer battles such as `Beat Brock`,
static encounters, and story changes. Both saves must have valid checksums and
matching trainer name and ID.

Save the reports:

```sh
pkmn compare progress backup.sav newer-backup.sav \
  --output-markdown progress.md --output-json progress.json
```

Neither save is changed.

### 33. `pkmn compare physical` — compare every byte

Type:

```sh
pkmn compare physical backup.sav newer-backup.sav
```

Expected result: hashes, sizes, identical/different status, equal/differing byte
counts and percentages, first/last difference, contiguous ranges, and mapped
save regions are reported. Normal gameplay usually changes physical bytes, so
`not identical` does not itself mean corruption.

```sh
pkmn compare physical backup.sav newer-backup.sav --format json
```

### 34. `pkmn compare semantic` — compare decoded meaning

Decode both saves first, then type:

```sh
pkmn compare semantic backup.red.json newer-backup.red.json
```

Expected result: field-aware differences are classified as exact, normalized,
derived, synchronized mirror, permitted canonical, runtime/cache drift,
deferred, or unexpected. Archival `physicalImage` is ignored as semantic
authority. An unexpected mismatch produces a non-zero semantic-mismatch exit
code.

### 35. `pkmn compare semantic-batch` — compare many JSON files

Type:

```sh
pkmn compare semantic-batch backup.red.json \
  newer-backup.red.json another-backup.red.json
```

Expected result: `backup.red.json` is the baseline and every later input gets
an `[ok]` or `[diff]` result plus an equivalent/total summary. For full
structured results:

```sh
pkmn compare semantic-batch backup.red.json \
  newer-backup.red.json another-backup.red.json --format json
```

## Proof commands

Proof packages contain private save data. Review them before sharing and never
publish a ROM or emulator binary with them.

### 36. `pkmn proof red` — run the complete internal proof pipeline

Type:

```sh
pkmn proof red backup.sav
```

Expected directory:

```text
backup.pkmn-proof/
```

Expected result: the CLI decodes, semantically generates, re-decodes, compares,
checks generated checksums, proves determinism, proves `physicalImage`
isolation, and writes the proof manifest, original/generated JSON, generated
save, physical/semantic/summary/archival reports, generation reports, and
`emulator-checklist.md`. The terminal still says emulator validation is
required; it is never claimed automatically.

Create a deterministic ZIP too:

```sh
pkmn proof red backup.sav --zip
```

Expected additional file: `backup.pkmn-proof.zip`. Use `--auto-suffix` if a
proof directory already exists.

### 37. `pkmn proof post-emulator` — continue after manual testing

After loading the generated save in an emulator, interacting, saving normally,
and copying that new save as `after-emulator.sav`, type:

```sh
pkmn proof post-emulator --before backup.sav --after after-emulator.sav
```

Expected directory:

```text
backup.post-emulator-validation/
```

It contains JSON and Markdown reports classifying expected runtime drift and
unexpected corruption. To attach the result to an existing proof package:

```sh
pkmn proof post-emulator --before backup.pkmn-proof/generated.sav \
  --after after-emulator.sav --proof-dir backup.pkmn-proof
```

Expected result: post-emulator reports are added and the manifest's manual gate
is updated only if validation passes.

### 38. `pkmn proof verify` — verify a proof directory or ZIP

Type:

```sh
pkmn proof verify backup.pkmn-proof
```

Or verify the ZIP:

```sh
pkmn proof verify backup.pkmn-proof.zip
```

Expected result: the manifest, every artifact SHA-256, JSON documents, ZIP
safety, and generated-save checksums are verified. Success prints
`Proof package verification: passed`, artifact count, and valid generated
checksums.

```sh
pkmn proof verify backup.pkmn-proof --format json
```

## Global controls

These controls go before the command:

```sh
pkmn --quiet red validate backup.sav
pkmn --verbose red validate backup.sav
pkmn --no-color red summary backup.sav
```

- `--quiet` suppresses normal standard output but preserves errors and exit
  status.
- `--verbose` prints build/context information before normal output.
- `--no-color` requests plain output. Current output is already suitable for
  files and scripts.

Do not combine `--quiet` and `--verbose`.

## Reserved commands that do not work yet

The following domains intentionally return an unsupported-operation message:

```sh
pkmn fred --help
pkmn frjson --help
pkmn convert --help
```

They reserve future FireRed and Red-to-FireRed command space. `pkmn 0.1.0` does
not claim FireRed decoding, generation, editing, proof, or conversion support.

## Updating and checking the installed catalog

For the current Homebrew HEAD package:

```sh
brew update
brew upgrade --fetch-HEAD AAAMAQ/pkmn/pkmn-cli
pkmn doctor --deep
pkmn get-all-cmds
```

If Homebrew reports that the package is already up to date, your installed
compiled command catalog is current.
