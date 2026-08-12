# Complete pkmn Command Catalog

Generated from the command catalog compiled into `pkmn 2.0.0`. Available command endpoints: **92**.

## General

### `doctor`

```sh
pkmn doctor [--deep] [--format json]
```

Check internal readiness; --deep runs a deterministic generation self-test.

### `completion`

```sh
pkmn completion <bash|zsh|fish>
```

Generate shell completion source.

### `config show`

```sh
pkmn config show [--format text|json]
```

Show compiled safety and default policy.

### `get-all-cmds`

```sh
pkmn get-all-cmds [--format text|json|markdown] [--output <file>]
```

Print the complete command catalog compiled into this executable.

## Pokemon Red saves

### `red summary`

```sh
pkmn red summary <save.sav> [--format text|json|markdown] [--output <file>]
```

Create a readable trainer, progress, party, inventory, and storage summary.

### `red inspect`

```sh
pkmn red inspect <save.sav> [--format json]
```

Inspect save size and checksum integrity.

### `red validate`

```sh
pkmn red validate <save.sav> [--format json]
```

Validate all known Red checksums.

### `red repair-checksums`

```sh
pkmn red repair-checksums <save.sav> [--output <copy.sav>] [--auto-suffix]
```

Write a checksum-repaired copy without modifying the source.

### `red decode`

```sh
pkmn red decode <save.sav> [--output <file.red.json>|-] [--include-physical-image|--no-physical-image] [--auto-suffix]
```

Export deterministic canonical Red JSON.

### `red events list`

```sh
pkmn red events list [--category <category>] [--format json]
```

List verified named event flags.

### `red events search`

```sh
pkmn red events search <query> [--format json]
```

Search verified named event flags.

### `red events show`

```sh
pkmn red events show <EVENT_NAME> [--format json]
```

Show one verified event definition.

### `red validate-batch`

```sh
pkmn red validate-batch <save.sav>... [--format json]
```

Validate several saves in one command.

### `red decode-batch`

```sh
pkmn red decode-batch <save.sav>... --output-dir <directory> [--no-physical-image]
```

Decode several saves into a transactional output directory.

### `red validate-post-emulator`

```sh
pkmn red validate-post-emulator <before.sav> <after.sav> [--output-dir <directory>]
```

Analyze and classify an emulator round trip.

## Pokemon Red to FireRed conversion

### `red convert`

```sh
pkmn red convert <save.sav> [output.sav] [--template <clean-fire-red.sav>] [--salt <value>] [--keep-intermediate] [--auto-suffix]
```

Validate and convert a Pokemon Red save into an auditable FireRed save.

## Pokemon Red editing

### `red edit`

```sh
pkmn red edit <save.sav>
```

Open the interactive copy-first editor.

### `red begin-edit`

```sh
pkmn red begin-edit <save.sav> [--output <session.json>]
```

Start a scriptable semantic edit session.

### `red edit-session`

```sh
pkmn red edit-session <session.json> <edits...> [--dry-run] [--format json] [--explain-error]
```

Stage and validate one or more edits.

### `red pokemon`

```sh
pkmn red pokemon <session.json> <party|species|nickname> <value> <rename|level|move> ... [--dry-run]
```

Apply coherent party-Pokemon name, level/stat, or move/PP edits.

### `red bag`

```sh
pkmn red bag <session.json> <add <item> <quantity>|remove <item>> [--dry-run]
```

Add, merge, or remove bag stacks with synchronized slots and counts.

### `red progress`

```sh
pkmn red progress <session.json> fly-destinations all [--dry-run]
```

Apply verified progress presets without arbitrary map-state mutation.

### `red pending-edits`

```sh
pkmn red pending-edits <session.json> [--format json]
```

Show staged edits.

### `red undo-edit`

```sh
pkmn red undo-edit <session.json> [--count <number>]
```

Undo the most recent staged edits.

### `red edit-history`

```sh
pkmn red edit-history <session.json> [--format json]
```

Show edit history and annotations.

### `red annotate-edit`

```sh
pkmn red annotate-edit <session.json> <note>
```

Attach a note to an edit session.

### `red validate-edit`

```sh
pkmn red validate-edit <session.json>
```

Run generation, checksum, re-decode, and semantic validation without writing a save.

### `red end-edit`

```sh
pkmn red end-edit <session.json> [--output <save.sav>] [--auto-suffix] [--dry-run] [--format json]
```

Publish a validated edited copy and reports.

## Canonical Red JSON

### `rjson inspect`

```sh
pkmn rjson inspect <file.red.json|-> [--format json]
```

Inspect canonical Red JSON and archival-image status.

### `rjson validate`

```sh
pkmn rjson validate <file.red.json|-> [--format json] [--profile standard|strict|generation|archival]
```

Validate schema, semantics, and optional physical image.

### `rjson generate`

```sh
pkmn rjson generate <file.red.json|-> [output.sav|-] [--auto-suffix]
```

Generate a deterministic save from semantic fields only.

### `rjson reconstruct`

```sh
pkmn rjson reconstruct <file.red.json|-> [--output <save.sav>|-] [--auto-suffix]
```

Reconstruct archived source bytes from physicalImage.

### `rjson migrate`

```sh
pkmn rjson migrate <file.red.json> [--output <migrated.red.json>] [--auto-suffix]
```

Apply compatible deterministic schema enrichment.

### `rjson schema`

```sh
pkmn rjson schema [--format json]
```

Describe the supported canonical schema contract.

### `rjson generate-batch`

```sh
pkmn rjson generate-batch <file.red.json>... --output-dir <directory>
```

Generate several semantic saves transactionally.

## Pokemon Red to FireRed conversion

### `rjson convert`

```sh
pkmn rjson convert <save.red.json> [output.sav] [--template <clean-fire-red.sav>] [--salt <value>] [--keep-intermediate] [--auto-suffix]
```

Convert canonical Red JSON directly into an auditable FireRed save.

### `rjson convert_to_frjson`

```sh
pkmn rjson convert_to_frjson <save.red.json> [output.fred.json] [--salt <value>] [--auto-suffix]
```

Translate canonical Red semantics into a proposed FireRed JSON document.

## Canonical Red JSON

### `rjson update_schema`

```sh
pkmn rjson update_schema <save.red.json> [--output <updated.red.json>] [--auto-suffix]
```

Safely migrate Red JSON to the latest supported schema.

## FireRed JSON

### `frjson inspect`

```sh
pkmn frjson inspect <save.fred.json>
```

Inspect native or planned FireRed JSON.

### `frjson validate`

```sh
pkmn frjson validate <save.fred.json>
```

Validate native or planned FireRed JSON.

### `frjson schema`

```sh
pkmn frjson schema [--format json]
```

Describe native and planned FireRed JSON contracts and release gates.

### `frjson update_schema`

```sh
pkmn frjson update_schema <save.fred.json> [--output <updated.fred.json>] [--auto-suffix]
```

Safely migrate FireRed JSON to the latest supported schema.

### `frjson generate`

```sh
pkmn frjson generate <save.fred.json> [output.sav] [--template <clean-fire-red.sav>]
```

Generate a template-backed FireRed save from accepted FireRed semantics.

### `frjson reconstruct`

```sh
pkmn frjson reconstruct <save.fred.json> [--output <save.sav>]
```

Reconstruct archived FireRed source bytes from physicalImage.

### `frjson migrate`

```sh
pkmn frjson migrate <save.fred.json> [--output <migrated.fred.json>] [--auto-suffix]
```

Apply compatible deterministic FireRed schema enrichment.

### `frjson generate-batch`

```sh
pkmn frjson generate-batch <save.fred.json>... --output-dir <directory> --template <clean.sav>
```

Generate several native or planned FireRed saves.

## Pokemon FireRed saves

### `fred summary`

```sh
pkmn fred summary <save.sav> [--detailed]
```

Create a compact or detailed FireRed save summary.

### `fred inspect`

```sh
pkmn fred inspect <save.sav> [--format json]
```

Inspect FireRed slots, counters, sectors, and checksum state.

### `fred validate`

```sh
pkmn fred validate <save.sav> [--format json]
```

Validate FireRed sectors, active slot, and checksums.

### `fred decode`

```sh
pkmn fred decode <save.sav> [--output <save.fred.json>]
```

Export complete native FireRed schema 0.4.0 JSON.

### `fred repair-checksums`

```sh
pkmn fred repair-checksums <save.sav> [--output <copy.sav>]
```

Repair recognized FireRed main-section checksums in a new copy.

### `fred validate-batch`

```sh
pkmn fred validate-batch <save.sav>... [--format json]
```

Validate several FireRed saves.

### `fred decode-batch`

```sh
pkmn fred decode-batch <save.sav>... --output-dir <directory>
```

Decode several FireRed saves transactionally.

### `fred validate-post-emulator`

```sh
pkmn fred validate-post-emulator <before.sav> <after.sav> [--output-dir <directory>]
```

Validate a FireRed emulator save/close/reload round trip.

## Pokemon FireRed editing

### `fred edit`

```sh
pkmn fred edit <save.sav> [--output <copy.sav>] [safe edits]
```

Apply narrow validated copy-first FireRed edits.

### `fred begin-edit`

```sh
pkmn fred begin-edit <save.sav> [--output <session.json>]
```

Start a safe FireRed edit session.

### `fred edit-session`

```sh
pkmn fred edit-session <session.json> [safe edit options]
```

Stage player, rival, money, coins, or badge edits.

### `fred pokemon`

```sh
pkmn fred pokemon <session.json> party <slot> rename <name>
```

Stage a safe party-Pokemon nickname edit.

### `fred bag`

```sh
pkmn fred bag <session.json> quantity <pocket> <slot> <item-id> <quantity>
```

Change an existing verified item stack quantity.

### `fred progress`

```sh
pkmn fred progress <session.json> badge <1-8> <on|off>
```

Stage the compact badge progress edit supported by Save Genie.

### `fred pending-edits`

```sh
pkmn fred pending-edits <session.json>
```

Show staged FireRed edits.

### `fred undo-edit`

```sh
pkmn fred undo-edit <session.json> [--count <number>]
```

Undo staged FireRed edits.

### `fred edit-history`

```sh
pkmn fred edit-history <session.json>
```

Show FireRed edit history.

### `fred annotate-edit`

```sh
pkmn fred annotate-edit <session.json> <note>
```

Annotate a FireRed edit session.

### `fred validate-edit`

```sh
pkmn fred validate-edit <session.json>
```

Validate staged edits without publishing a save.

### `fred end-edit`

```sh
pkmn fred end-edit <session.json> [--output <copy.sav>]
```

Publish a validated edited FireRed copy.

## Pokemon FireRed saves

### `fred events list`

```sh
pkmn fred events list [--kind flag|variable] [--format json]
```

List pinned pret FireRed flags or variables.

### `fred events search`

```sh
pkmn fred events search <query> [--kind flag|variable] [--format json]
```

Search pinned pret FireRed flags or variables.

### `fred events show`

```sh
pkmn fred events show <name|id> [--kind flag|variable] [--format json]
```

Show one pinned pret FireRed flag or variable.

## Pokemon Red to FireRed conversion

### `convert red-to-firered`

```sh
pkmn convert red-to-firered <red.sav|red.json> [output.sav] [conversion options]
```

Convert either supported Red source form to FireRed.

### `convert inspect`

```sh
pkmn convert inspect <event|trainer|item> [query]
```

Inspect pinned bridge authority records.

### `convert explain`

```sh
pkmn convert explain <event|trainer|item> <query>
```

Explain a bridge decision with source evidence.

### `convert validate-manifest`

```sh
pkmn convert validate-manifest <conversion-manifest.json>
```

Validate an auditable conversion manifest.

### `convert batch`

```sh
pkmn convert batch <red.sav|red.json>... --output-dir <directory> --template <clean.sav>
```

Convert several Red sources to FireRed.

## Comparison

### `compare progress`

```sh
pkmn compare progress <older.sav> <newer.sav> [report options]
```

Explain gameplay progress between two backups from the same playthrough.

### `compare physical`

```sh
pkmn compare physical <a.sav> <b.sav> [report options]
```

Compare physical bytes, ranges, percentages, and hashes.

### `compare semantic`

```sh
pkmn compare semantic <a.red.json> <b.red.json> [report options]
```

Compare canonical semantic fields.

### `compare semantic-batch`

```sh
pkmn compare semantic-batch <baseline.red.json> <candidate.red.json>... [--format json]
```

Compare several canonical documents to one baseline.

### `compare firered-semantic`

```sh
pkmn compare firered-semantic <a.fred.json> <b.fred.json> [--output-json <file>]
```

Compare FireRed semantics while excluding archival physical bytes.

### `compare firered-progress`

```sh
pkmn compare firered-progress <older.sav> <newer.sav> [--output-json <file>]
```

Decode and compare two FireRed progress saves.

### `compare firered-pokemon`

```sh
pkmn compare firered-pokemon <a.fred.json> <b.fred.json>
```

Compare party, PC, and daycare data.

### `compare firered-events`

```sh
pkmn compare firered-events <a.fred.json> <b.fred.json>
```

Compare event and variable state.

### `compare firered-trainers`

```sh
pkmn compare firered-trainers <a.fred.json> <b.fred.json>
```

Compare trainer-defeat state.

### `compare firered-items`

```sh
pkmn compare firered-items <a.fred.json> <b.fred.json>
```

Compare inventory and obtained-history state.

### `compare firered-fly`

```sh
pkmn compare firered-fly <a.fred.json> <b.fred.json>
```

Compare Fly destinations.

### `compare firered-hall-of-fame`

```sh
pkmn compare firered-hall-of-fame <a.fred.json> <b.fred.json>
```

Compare Hall of Fame data.

### `compare bridge`

```sh
pkmn compare bridge <red.json> <fred.json> [--manifest <file>]
```

Audit Red source, proposed FireRed target, and optional manifest.

## Proof

### `proof red`

```sh
pkmn proof red <source.sav> [--output-dir <directory>] [--zip|--zip-output <archive.zip>] [--auto-suffix]
```

Run decode, generation, comparison, determinism, and isolation proofs.

### `proof post-emulator`

```sh
pkmn proof post-emulator --before <save.sav> --after <save.sav> [--output-dir <directory>|--proof-dir <directory>]
```

Continue a proof package after manual emulator testing.

### `proof verify`

```sh
pkmn proof verify <proof-directory|proof.zip> [--format json]
```

Verify proof hashes, schemas, ZIP safety, and generated checksums.

### `proof fred`

```sh
pkmn proof fred <complete.fred.json> --template <clean.sav> [--output-dir <directory>]
```

Run the automated Phase 5 native FireRed generation proof.

### `proof red-to-firered`

```sh
pkmn proof red-to-firered <save.red.json> --template <clean.sav> [--output-dir <directory>]
```

Run the automated Phase 6 conversion proof and prepare MAQ verification.

## Global controls

`--quiet`, `--verbose`, and `--no-color` must appear before the command. FireRed physical generation passed Phase 5; Red-to-FireRed conversion passed Phase 6.
