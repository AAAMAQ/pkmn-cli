# Command Reference

All currently implemented workflows are internal to `pkmn`. Save Genie and Save Generator executables are not runtime dependencies.

## General

- `pkmn --help` prints the supported and reserved command domains.
- `pkmn --version` prints the stable tool version.
- `pkmn doctor` checks internal readiness; `doctor --deep` runs an internal deterministic generation round trip.
- `pkmn completion <bash|zsh|fish>` prints shell completion source.
- `pkmn config show [--format text|json]` prints immutable compiled safety and default policy; no external engine paths are configured.
- `pkmn get-all-cmds [--format text|json|markdown] [--output <file>]` renders the exhaustive catalog compiled into the installed executable. It automatically follows Homebrew upgrades; the checked-in overview is `docs/ALL_COMMANDS.md`.

Doctor, raw Red inspect/validate, canonical JSON inspect/validate, and compare commands support machine-readable JSON output.

Existing outputs are refused by default. Decode, generation, reconstruction, editing, and proof commands support explicit `--auto-suffix` where output paths may collide, producing `_2`, `_3`, and later names while preserving the no-overwrite rule.

## Pokemon Red saves

- `pkmn red summary <save.sav> [--format text|json|markdown] [--output <file>]` produces a readable trainer/progress report plus party, inventory, storage, Daycare, Hall of Fame, and supported world-state statistics.
- `pkmn red inspect <save.sav>` prints size and checksum status.
- `pkmn red validate <save.sav>` validates the main, bank, and all 12 box checksums.
- `pkmn red repair-checksums <save.sav>` writes a validated repaired copy and JSON report without modifying the source.
- `pkmn red events list|search|show` discovers verified named flags.
- `pkmn red validate-batch` and `decode-batch` provide transactional multi-save automation.
- `pkmn red decode <save.sav> [--output <file.red.json>] [--include-physical-image|--no-physical-image]` writes deterministic canonical JSON. Physical image inclusion is the default. Existing outputs are refused.
- `pkmn red edit <save.sav>` starts the looped interactive copy-first editor, including complex file-backed fields, pending preview, validation, save, and discard actions.
- `pkmn red begin-edit <save.sav> [--output <session.json>]` creates a semantic-only edit session.
- `pkmn red edit-session <session.json> <edits...>` accumulates edits. Named scalar options are `--trainer-name`, `--rival-name`, `--trainer-id`, `--money`, `--coins`, `--badges <0..255|all>`, `--selected-box`, and `--event EVENT_NAME on|off` (with story/trainer/static aliases). Complex `--*-file` options cover inventory, party, boxes/cache, Daycare, Hall of Fame, Pokédex, options, playtime, and verified world state. `--set` and `--set-file` expose supported existing semantic fields.
- `pkmn red pending-edits <session.json>` lists staged changes.
- `pkmn red undo-edit`, `edit-history`, and `annotate-edit` manage session history.
- `pkmn red validate-edit <session.json>` verifies source identity, generation policy, and output checksums without writing a save.
- `pkmn red end-edit <session.json> [--output <output.sav>]` writes a new save and JSON/Markdown reports. It never overwrites an existing artifact.
- `edit-session` and `end-edit` accept `--dry-run`; candidates are generated, re-decoded, and semantically compared without persistence.

Arbitrary location edits are rejected. Generated and edited semantic saves use the verified Red's-house second-floor preset.

See `docs/EDIT_MODE.md` for the complete editor and validation contract.

## Canonical Red JSON

- `pkmn rjson inspect <file.red.json>` reports schema, semantic, and physical-image status.
- `pkmn rjson validate <file.red.json>` validates supported schema and semantic constraints; when physical bytes exist it also verifies source SHA-256 and Red checksums.
- `pkmn rjson validate ... --profile standard|strict|generation|archival` selects an explicit policy.
- `pkmn rjson migrate` enriches compatible schema `0.1.0` documents without changing semantic values.
- `pkmn rjson schema [--format json]` reports the supported schema contract, profiles, and authority boundary.
- `pkmn rjson generate-batch` generates several inputs transactionally.
- `pkmn rjson reconstruct <file.red.json> [--output <output.sav>]` performs archival byte reconstruction and requires `physicalImage`.
- `pkmn rjson generate <file.red.json> [output.sav]` performs deterministic semantic generation. It never uses `physicalImage` as authority and emits generation reports.

## Comparison and proof

- `pkmn compare progress <older.sav> <newer.sav> [report options]` explains gameplay changes between two valid backups with the same trainer name and ID: elapsed time, currency, badges, Pokedex, location, party, items, storage, Hall of Fame, verified events, battles, encounters, and story flags. Older input comes first.
- `pkmn compare physical <a.sav> <b.sav> [report options]` reports SHA-256, byte counts and percentages, first/last difference, and contiguous differing ranges.
- `pkmn compare semantic <a.red.json> <b.red.json> [report options]` reports field-aware differences classified as exact, normalized, derived, synchronized mirror, permitted canonical, runtime drift, cache drift, deferred, or unexpected.
- `pkmn compare semantic-batch <baseline> <candidates...>` summarizes several semantic comparisons.
- Comparison report options are `--format markdown|json`, `--output-json <file>`, and `--output-markdown <file>`.
- `pkmn proof red <source.sav> [--output-dir <directory>] [--zip|--zip-output <archive.zip>]` decodes, generates, re-decodes, compares, proves determinism and physical-image isolation, and writes the complete machine/human report set plus a manual emulator checklist.
- `pkmn red validate-post-emulator <before.sav> <after.sav> [--output-dir <directory>]` validates and classifies an emulator round trip without modifying either save.
- `pkmn proof post-emulator --before <save> --after <save> [--output-dir <directory>|--proof-dir <proof-package>]` performs independent validation or explicitly advances an existing proof manifest.
- `pkmn proof verify <proof-directory|proof.zip>` verifies artifact hashes, ZIP safety, JSON schemas, and generated-save checksums.

Proof output is evidence, not source material. Do not commit it. Automated checks do not claim that the manual emulator gate passed.

See `docs/PROOF_WORKFLOW.md` for artifact, privacy, ZIP, and emulator-gate details.

New users can follow `docs/BEGINNERS_GUIDE.md`; the exhaustive endpoint table is `docs/ALL_COMMANDS.md`.

## Exit codes

The stable categories are: `0` success, `1` general failure, `2` invalid arguments, `3` invalid input, `4` checksum failure, `5` generation failure, `6` physical-image isolation failure, `7` determinism failure, `8` semantic mismatch, `9` output/collision failure, `10` post-emulator validation failure, `11` edit validation failure, and `12` unsupported operation.

## Reserved commands

`fred`, `frjson`, and `convert` are honest placeholders. This release does not claim FireRed parsing, generation, editing, or conversion support.
