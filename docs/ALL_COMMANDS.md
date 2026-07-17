# Complete Command List

This is the checked-in overview of every command endpoint in `pkmn 0.1.0`.
The executable is the authority: after any Homebrew upgrade, run
`pkmn get-all-cmds` to see the catalog compiled into that installed version.
Use `--format json` for scripts or `--format markdown --output commands.md` to
save a fresh reference without copying terminal text.

For exact `backup.sav` examples and expected output for every endpoint, read
[COMPLETE_USAGE_GUIDE.md](COMPLETE_USAGE_GUIDE.md).

## General

| Command | Purpose |
|---|---|
| `pkmn doctor [--deep] [--format json]` | Check the standalone internal engine; `--deep` performs a deterministic self-test. |
| `pkmn completion <bash\|zsh\|fish>` | Generate shell completion source. |
| `pkmn config show [--format text\|json]` | Show compiled safety and output defaults. |
| `pkmn get-all-cmds [--format text\|json\|markdown] [--output <file>]` | Print or save the complete compiled command catalog. |

## Pokemon Red saves

| Command | Purpose |
|---|---|
| `pkmn red summary <save.sav> [--format text\|json\|markdown] [--output <file>]` | Readable trainer, progress, party, inventory, and storage statistics. |
| `pkmn red inspect <save.sav> [--format json]` | Inspect size and checksum integrity. |
| `pkmn red validate <save.sav> [--format json]` | Validate every known Red checksum. |
| `pkmn red repair-checksums <save.sav> [--output <copy.sav>] [--auto-suffix]` | Write a checksum-repaired copy. |
| `pkmn red decode <save.sav> [--output <file.red.json>\|-] [--include-physical-image\|--no-physical-image] [--auto-suffix]` | Export canonical Red JSON. |
| `pkmn red events list [--category <category>] [--format json]` | List verified named event flags. |
| `pkmn red events search <query> [--format json]` | Search the event catalog. |
| `pkmn red events show <EVENT_NAME> [--format json]` | Show one event definition. |
| `pkmn red validate-batch <save.sav>... [--format json]` | Validate multiple saves. |
| `pkmn red decode-batch <save.sav>... --output-dir <directory> [--no-physical-image]` | Transactionally decode multiple saves. |
| `pkmn red validate-post-emulator <before.sav> <after.sav> [--output-dir <directory>]` | Analyze an emulator round trip. |

## Pokemon Red editing

| Command | Purpose |
|---|---|
| `pkmn red edit <save.sav>` | Open the interactive copy-first editor. |
| `pkmn red begin-edit <save.sav> [--output <session.json>]` | Start a scriptable semantic edit session. |
| `pkmn red edit-session <session.json> <edits...> [--dry-run] [--format json]` | Stage and validate edits. |
| `pkmn red pending-edits <session.json> [--format json]` | Show staged edits. |
| `pkmn red undo-edit <session.json> [--count <number>]` | Undo recent staged edits. |
| `pkmn red edit-history <session.json> [--format json]` | Show edit history. |
| `pkmn red annotate-edit <session.json> <note>` | Attach a note to a session. |
| `pkmn red validate-edit <session.json>` | Validate the candidate without writing a save. |
| `pkmn red end-edit <session.json> [--output <save.sav>] [--auto-suffix] [--dry-run] [--format json]` | Publish a validated edited copy and reports. |

## Canonical Red JSON

| Command | Purpose |
|---|---|
| `pkmn rjson inspect <file.red.json\|-> [--format json]` | Inspect schema, semantics, and physical-image status. |
| `pkmn rjson validate <file.red.json\|-> [--format json] [--profile standard\|strict\|generation\|archival]` | Validate schema and semantics. |
| `pkmn rjson generate <file.red.json\|-> [output.sav\|-] [--auto-suffix]` | Generate a deterministic semantic save. |
| `pkmn rjson reconstruct <file.red.json\|-> [--output <save.sav>\|-] [--auto-suffix]` | Reconstruct archived source bytes from `physicalImage`. |
| `pkmn rjson migrate <file.red.json> [--output <migrated.red.json>] [--auto-suffix]` | Enrich a compatible schema document. |
| `pkmn rjson schema [--format json]` | Describe the canonical schema contract. |
| `pkmn rjson generate-batch <file.red.json>... --output-dir <directory>` | Transactionally generate multiple saves. |

## Comparison

The `report options` accepted by the individual comparison commands include
`--format markdown|json`, `--output-json <file>`, and
`--output-markdown <file>`.

| Command | Purpose |
|---|---|
| `pkmn compare progress <older.sav> <newer.sav> [report options]` | Explain what changed between two backups from one playthrough. |
| `pkmn compare physical <a.sav> <b.sav> [report options]` | Compare bytes, hashes, ranges, and percentages. |
| `pkmn compare semantic <a.red.json> <b.red.json> [report options]` | Classify field-aware semantic differences. |
| `pkmn compare semantic-batch <baseline.red.json> <candidate.red.json>... [--format json]` | Compare several documents with one baseline. |

## Proof

| Command | Purpose |
|---|---|
| `pkmn proof red <source.sav> [--output-dir <directory>] [--zip\|--zip-output <archive.zip>] [--auto-suffix]` | Run the complete Red proof pipeline. |
| `pkmn proof post-emulator --before <save.sav> --after <save.sav> [--output-dir <directory>\|--proof-dir <directory>]` | Continue proof after manual emulator testing. |
| `pkmn proof verify <proof-directory\|proof.zip> [--format json]` | Verify proof artifacts and checksums. |

## Global controls and future domains

Place `--quiet`, `--verbose`, or `--no-color` before the command. `fred`,
`frjson`, and `convert` are reserved placeholders; this version does not claim
FireRed or Red-to-FireRed support.
