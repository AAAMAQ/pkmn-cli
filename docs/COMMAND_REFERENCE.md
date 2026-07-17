# Command Reference

All currently implemented workflows are internal to `pkmn`. Save Genie and Save Generator executables are not runtime dependencies.

## General

- `pkmn --help` prints the supported and reserved command domains.
- `pkmn --version` prints the stable tool version.
- `pkmn doctor` checks that the internal modules and installed Red generation resource are ready without reading user files.

## Pokemon Red saves

- `pkmn red inspect <save.sav>` prints size and checksum status.
- `pkmn red validate <save.sav>` validates the main, bank, and all 12 box checksums.
- `pkmn red decode <save.sav> [--output <file.red.json>] [--include-physical-image|--no-physical-image]` writes deterministic canonical JSON. Physical image inclusion is the default. Existing outputs are refused.
- `pkmn red edit <save.sav>` starts the interactive copy-first scalar editor.
- `pkmn red begin-edit <save.sav> [--output <session.json>]` creates a semantic-only edit session.
- `pkmn red edit-session <session.json> <edits...>` accumulates edits. Named options are `--trainer-name`, `--rival-name`, `--trainer-id`, `--money`, `--coins`, and `--badges`. `--set </decoded/pointer> <JSON-value>` exposes supported complex semantic structures.
- `pkmn red pending-edits <session.json>` lists staged changes.
- `pkmn red validate-edit <session.json>` verifies source identity, generation policy, and output checksums without writing a save.
- `pkmn red end-edit <session.json> [--output <output.sav>]` writes a new save and JSON/Markdown reports. It never overwrites an existing artifact.

Arbitrary location edits are rejected. Generated and edited semantic saves use the verified Red's-house second-floor preset.

## Canonical Red JSON

- `pkmn rjson inspect <file.red.json>` reports schema, semantic, and physical-image status.
- `pkmn rjson validate <file.red.json>` validates supported schema and semantic constraints; when physical bytes exist it also verifies source SHA-256 and Red checksums.
- `pkmn rjson reconstruct <file.red.json> [--output <output.sav>]` performs archival byte reconstruction and requires `physicalImage`.
- `pkmn rjson generate <file.red.json> [output.sav]` performs deterministic semantic generation. It never uses `physicalImage` as authority and emits generation reports.

## Comparison and proof

- `pkmn compare physical <a.sav> <b.sav> [report options]` reports SHA-256, byte counts and percentages, first/last difference, and contiguous differing ranges.
- `pkmn compare semantic <a.red.json> <b.red.json> [report options]` reports field-aware differences classified as exact, normalized, derived, synchronized mirror, permitted canonical, runtime drift, cache drift, deferred, or unexpected.
- Comparison report options are `--format markdown|json`, `--output-json <file>`, and `--output-markdown <file>`.
- `pkmn proof red <source.sav> [--output-dir <directory>] [--zip|--zip-output <archive.zip>]` decodes, generates, re-decodes, compares, proves determinism and physical-image isolation, and writes the complete machine/human report set plus a manual emulator checklist.
- `pkmn red validate-post-emulator <before.sav> <after.sav> [--output-dir <directory>]` validates and classifies an emulator round trip without modifying either save.
- `pkmn proof post-emulator --before <save> --after <save> [--output-dir <directory>|--proof-dir <proof-package>]` performs independent validation or explicitly advances an existing proof manifest.

Proof output is evidence, not source material. Do not commit it. Automated checks do not claim that the manual emulator gate passed.

See `docs/PROOF_WORKFLOW.md` for artifact, privacy, ZIP, and emulator-gate details.

## Exit codes

The stable categories are: `0` success, `1` general failure, `2` invalid arguments, `3` invalid input, `4` checksum failure, `5` generation failure, `6` physical-image isolation failure, `7` determinism failure, `8` semantic mismatch, `9` output/collision failure, `10` post-emulator validation failure, `11` edit validation failure, and `12` unsupported operation.

## Reserved commands

`fred`, `frjson`, and `convert` are honest placeholders. This release does not claim FireRed parsing, generation, editing, or conversion support.
