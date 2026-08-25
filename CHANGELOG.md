# Changelog

## 3.0.0 — Phase 3 and public release

- Added a private PyInstaller one-directory conversion runtime so downloaded
  packages require no separate Python installation.
- Added executable-relative bundled-runtime discovery with an explicit
  developer Python fallback and doctor/config reporting.
- Added Windows ZIP/installer, macOS arm64/x86-64 archive/package, and Linux
  archive/DEB/AppImage release automation.
- Added clean-prefix installed smoke tests, SHA-256 release sums, SPDX release
  SBOM generation, privacy scanning, licenses, and tagged release publication.
- Added Windows, macOS, and Linux interactive launchers.
- Rewrote the README, installation, Homebrew, troubleshooting, release, and
  beginner-facing distribution documentation for self-contained downloads.
- Preserved Red → FireRed's MAQ emulator evidence and the three new routes'
  statically validated community-testing labels.

## 3.0.0-alpha.2 — Phase 2

- Enabled all four typed routes: Red/Blue to FireRed/LeafGreen.
- Added explicit `GEN1_BLUE`/`GEN3_LEAFGREEN` profiles, `.blue.json` and
  `.lg.json` command surfaces, and conflict-safe source declarations.
- Added `blue`, `bjson`, `leafgreen`/`lg`, and `lgjson` domains over the two
  shared paired-game engines.
- Made planner output, Manifest 3.0, default filenames, intermediate JSON,
  generation reports, and Pokémon origin metadata route-aware.
- Added beginner interactive selection for all four routes with evidence labels.
- Added pinned paired-version overlay data and a Phase 2 evidence record.
- Added four-route output/checksum/manifest regression coverage while retaining
  `EMULATOR_VERIFIED` only for Red → FireRed.

## 3.0.0-alpha.1 — Phase 1

- Added typed Red, Blue, FireRed, and LeafGreen game profiles.
- Added a four-route registry with capability and evidence discovery through
  `pkmn convert routes`.
- Added the canonical `pkmn convert red-firered` spelling while retaining the
  v2 `red-to-firered` alias.
- Added safe `--auto-repair-checksum`/`--auto_repair_checksum` conversion and
  explicit `--write-repaired-source` output.
- Added Manifest 3.0 route, evidence, integrity, repair, template, and policy
  fields.
- Added `pkmn interactive` with the emulator-verified Red → FireRed workflow.
- Added deterministic regression, semantic-corruption rejection,
  source-preservation, and direct/interactive equivalence coverage.

## 2.0.0 - Unreleased

- Put Red → FireRed conversion at the top of the product and help experience.
- Added `pkmn red convert` with Pokémon Red identity/checksum validation and
  default `<stem>_fr.sav` naming.
- Added `pkmn rjson convert`, `convert_to_frjson`, and `update_schema`.
- Added `pkmn frjson inspect`, `validate`, `update_schema`, and template-backed
  `generate`, archival `reconstruct`, and schema discovery.
- Added native FireRed save summary, inspection, checksum validation, complete
  schema 0.4.0 decoding, narrow copy-first editing, and pinned event discovery.
- Bundled the pinned deterministic bridge planner, Pokémon policy, event/item/
  trainer authorities, and FireRed generator runtime.
- Added MAQ's clean pre-starter FireRed template as the validated automatic
  default, strict support for equivalent user-dumped templates, installed
  resource self-tests, and a complete cartridge/emulator conversion guide.
- Added copy-first output collision protection, conversion manifests, reports,
  physical-image isolation, and explicit Phase 5/6 verification gating.
- Expanded the compiled v2 catalog to 92 endpoints and preserved original
  source-save filename/SHA-256 provenance in direct-conversion manifests.
- Added native schema 0.4.0 FireRed generation from complete logical-block and
  special-sector JSON authority without reading `physicalImage`.
- Added FireRed checksum repair, transactional batch decode/validation,
  post-emulator validation, and copy-first safe edit sessions.
- Added top-level conversion preview/planning, bridge inspection/explanation,
  manifest validation, batch conversion, and explicit PCCS policy selection.
- Added FireRed semantic/domain comparisons, Red-to-FireRed bridge auditing,
  and Phase 5/6 proof packages with deterministic and isolation checks.
- Recorded MAQ's Phase 5 FireRed generator acceptance: complete native
  `.fred.json` generated an emulator-verified equivalent save without using
  the original physical image.
- Added source-backed Red `wPlayerStarter` and `wRivalStarter` decoding so
  Phase 6 preserves Charmander/Bulbasaur/Squirtle choice and its rival branch
  without guessing from the current party.
- Recorded MAQ's Phase 6 Red → FireRed converter acceptance after deterministic
  proof, checksum validation, bridge/manifest audit, and detailed emulator
  equivalence testing.

## Unreleased

- Adopted a self-contained architecture: completed workflows use internal Red modules rather than required helper executables.
- Added the first internal Red engine slice with immutable 32 KiB loading and main, bank, and all-box checksum validation.
- Implemented standalone `red inspect` and `red validate`; kept `red decode` gated on internal canonical JSON export.
- Added migration planning, acceptance gates, licensing attribution, and standalone tests.
- Removed the initial helper-executable discovery scaffold so the product boundary cannot regress into a sidecar dependency.
- Updated `doctor` to report compiled-in module readiness only.
- Implemented deterministic internal `red decode` with canonical `.red.json`, optional archival physical image, SHA-256 identities, collision refusal, and broad Red semantic sections.
- Added internal `rjson inspect` and `rjson validate` with schema, required-field, semantic, physical-image, checksum, and SHA-256 validation.
- Added explicit collision-safe `rjson reconstruct` archival reconstruction with mandatory `physicalImage` authority.
- Added internal deterministic `rjson generate`, the validated bundled Red's-house template, full supported semantic serialization, physical-image isolation, checksum regeneration, write-range overlap validation, and JSON/Markdown generation reports.
- Added physical and field-aware semantic comparison plus the complete internal `proof red` decode/generate/redecode/determinism/isolation/report/checklist workflow.
- Added interactive and scriptable copy-first Red edit sessions with multiple pending edits, source identity checks, safe-location enforcement, checksum validation, collision protection, and edit reports.
- Added macOS/Linux CI, installed documentation, a clean release verification script, Homebrew formula draft, command/install/release documentation, and public-data-only examples.
- Added complete semantic-difference classifications, machine-readable comparison output, post-emulator validation/proof continuation, expanded proof reports, and deterministic privacy-marked ZIP proof packages.
- Expanded editing into a validated looped terminal application with broad file-backed named fields, advanced value-file support, derived-field synchronization, and detailed in-memory semantic validation.
- Added JSON output for diagnostic commands, generated bash/zsh/fish completion, an installed `pkmn(1)` manual, and dedicated schema, reconstruction, editing, proof, and future-FireRed documentation.
- Added immutable `config show` policy reporting and explicit collision-safe `_2`/`_3` output suffixing across decode, generation, reconstruction, editing, and proof workflows.
- Completed the 507-entry named-event layer; transactional output publication; proof self-verification; deep doctor; event discovery; copy-first checksum repair; edit dry-run/history/undo; schema enrichment; batch and pipeline workflows; verified Gen I name databases; validation profiles; Windows/release CI; CPack, universal-macOS, and SPDX packaging scaffolds.
- Added `red summary` for readable and structured save statistics, `compare progress` for explaining gameplay changes between ordered backups, and `get-all-cmds` backed by the executable's authoritative compiled catalog.
- Added an exhaustive command list and a non-technical installation and usage guide with safe output examples.
- Added a complete copy-and-paste manual explaining all 38 endpoints with `backup.sav` examples, expected terminal output, generated filenames, and safety behavior.
- Normalized erased `0xFF` PC boxes and other uninitialized Red collections as empty state instead of fake records.
- Added semantic party-Pokémon selectors and synchronized rename, level/experience/stat/HP, and move/PP editing.
- Added safe bag add/merge/remove commands, the verified all-Fly-destinations progress preset, 24-bit/growth-rate experience validation, move-specific PP validation, and actionable edit-error hints.
- Expanded the compiled catalog to 41 endpoints and added synthetic end-to-end coverage for the new edit workflows.

All notable changes to `pkmn-cli` will be documented here.

## 0.1.0 - Unreleased

- Create the standalone C++20/CMake project foundation.
- Add the `pkmn` command router, help, and version output.
- Add the initial `pkmn doctor` readiness scaffold.
- Reserve Red, RJSON, proof, compare, config, and future FireRed domains.
- Establish privacy exclusions for saves, ROMs, screenshots, and proof evidence.
