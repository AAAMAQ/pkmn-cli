# Self-Contained Red Engine Plan

## Decision

`pkmn-cli` is the final unified public command-line product. Installed Red workflows must run without locating or launching Pokemon Red Save Genie or Pokemon Red Save Generator. Those completed repositories remain verified research engines, prototypes, and read-only source references.

No executable adapter is part of the product. Internal command coverage is built directly from the verified source references, and acceptance tests run without either helper executable.

Future FireRed support follows the same rule: one `pkmn` executable and one coherent CLI architecture, without required Genie/Generator sidecar executables.

## Attribution and licensing

The incorporated Red parser, serializer, validator, generator, and policy logic originates in the MIT-licensed **Pkmn Red Save Genie** and **Pkmn Red Save Generator** projects, copyright 2026 MAQ / BiG MAQ Studios.

The repository-level MIT license remains in force. `THIRD_PARTY_NOTICES.md` will identify the source projects, source commits used for each import, imported concepts/files, and any substantial adapted portions. New or substantially adapted source files will carry concise provenance comments where useful. No ROM, private save, screenshot, emulator output, or proof evidence will be copied.

## Target module layout

```text
src/red/save/          bounded save-buffer access, Gen I layout, file reader/writer
src/red/json/          canonical .red.json model, schema validation, import/export
src/red/generation/    semantic generation and canonical template policy
src/red/editing/       copy-first validated mutations and checksum repair
src/red/validation/    structural, checksum, semantic, and policy validation
src/red/comparison/    physical and semantic comparison
src/red/reporting/     stable human and machine-readable reports
```

Command implementations depend on these modules through narrow request/result interfaces. They do not depend on source-project directory layouts, UI code, or executable adapters.

## What will be imported or adapted

### From Pkmn Red Save Genie

- Gen I SRAM size, bank layout, field offsets, and bounds-checked byte access.
- Main checksum, bank all-checksum, and per-box checksum algorithms.
- Defensive parsing and validation invariants.
- Gen I text and BCD codecs.
- Party, box, Daycare, Hall of Fame, inventory, Pokedex, location, event, and world-state readers needed by canonical JSON export.
- Canonical `.red.json` model, schema checks, coverage metadata, physical-image archival representation, and reconstruction rules.
- Safe editing primitives only after the read/export path is stable.

### From Pkmn Red Save Generator

- Canonical JSON loader/model validation shared with the internal JSON module.
- Semantic generation pipeline, declared write ranges, deterministic output, and integrity validation.
- Canonical dummy-template policy and provenance reporting, with installation-safe resource lookup.
- Permanent/current-box serialization, stored-Pokemon validation, Hall of Fame handling, text validation, semantic comparison, and post-emulator analysis logic needed by public commands.
- Unsupported-location fail-closed canonicalization and physical-image isolation checks.

## What will be left behind

- Save Genie's interactive application entry point, prompts, Xcode project layout, personal resources, private saves, ROMs, screenshots, generated examples, and publication build artifacts.
- Generator's standalone CLI router, CMake target layout, repository-relative development paths, milestone scratch code, private fixtures, and duplicated third-party trees when a clean shared dependency is available.
- Compatibility code that shells out to the old executables.
- Any feature whose proof or policy boundary cannot be reproduced in `pkmn-cli` tests.

## Migration order

1. **Internal save reader and validator**
   - Load a save into an immutable, bounds-checked buffer.
   - Validate exact SRAM size, main checksum, bank checksums, and all box checksums.
   - Wire `pkmn red inspect` and `pkmn red validate` to internal code with stable exit codes.
2. **Canonical JSON export and inspection**
   - Port the semantic model and readers by subsystem.
   - Implement `pkmn red decode`, internal `pkmn rjson inspect`, and internal `pkmn rjson validate`.
   - Cross-check exports against Save Genie public/synthetic fixtures during migration only.
3. **Internal semantic generator**
   - Port the verified Generator pipeline into `src/red/generation`.
   - Package canonical resources with `pkmn`; remove runtime repository assumptions.
   - Switch `pkmn rjson generate` from the adapter to the internal engine.
4. **Explicit reconstruction**
   - Implement `pkmn rjson reconstruct` as a separately named archival operation.
   - Require a valid `physicalImage`; never share authority paths with semantic generation.
5. **Comparison and proof**
   - Port physical/semantic comparisons, determinism, physical-image isolation, and proof reporting.
6. **Edit mode**
   - Add copy-first mutations, field validation, checksum regeneration, reparse validation, and collision-safe output.
7. **Standalone completion**
   - Verify every installed workflow from outside the source tree with no sibling tools available.

## Safety invariants

- Inputs and existing outputs are never overwritten by default.
- Semantic generation cannot read or branch on target `physicalImage` data.
- Reconstruction is explicit, requires `physicalImage`, and is labeled archival.
- Unsupported runtime locations fail closed or canonicalize only according to the verified safe-location policy.
- Parser acceptance and checksum validity do not imply emulator/runtime safety.
- Output reports use portable/redacted paths and contain no private absolute paths.
- ROMs, private saves, screenshots, emulator artifacts, and evidence files remain excluded.

## Risks and controls

| Risk | Control |
|---|---|
| Silent divergence while adapting proven code | Differential tests against the completed tools using public/synthetic fixtures, followed by internal golden tests. |
| Old repository assumptions leak into installation | Resource locator tests from arbitrary working directories and package-layout smoke tests. |
| Generator accidentally consumes `physicalImage` | Mutation/isolation tests proving identical output when only `physicalImage` changes or is removed. |
| Reconstruction and generation authority become confused | Separate request types, modules, commands, help text, and tests. |
| Partial parsers overclaim support | Coverage metadata, fail-closed errors, subsystem acceptance gates, and documented unsupported fields. |
| Checksums pass while runtime state is unsafe | Preserve location canonicalization and emulator validation gates. |
| Imported licensing/provenance is lost | Repository notice plus per-import source/commit manifest updates. |
| Compatibility adapters remain accidentally required | Tests run with helper environment variables pointing to missing files and an empty helper search path. |

## Test strategy

- Unit tests for buffer bounds, file errors, size rules, every checksum algorithm, codecs, serializers, and collision handling.
- Synthetic fixtures built in memory; no private saves committed.
- Corruption tests for each checksum region and stored checksum byte.
- Cross-project differential tests during porting, never required by installed/runtime tests.
- Canonical JSON schema, round-trip, deterministic serialization, and subsystem coverage tests.
- Generator determinism, declared-range, checksum, semantic, and physical-image-isolation tests.
- Reconstruction byte-identity tests that require `physicalImage` and reject its absence.
- CLI tests for arguments, stdout/stderr, exit mapping, missing files, collision refusal, and unsupported operations.
- Standalone tests with Save Genie and Save Generator executables absent.
- CMake install and Homebrew/package smoke tests from outside the source tree.

## Acceptance gates

Each migration stage must satisfy all applicable gates before becoming the default:

1. Builds with only declared `pkmn-cli` dependencies.
2. Tests pass without either reference repository or executable on `PATH`.
3. Public/synthetic differential results match the verified reference behavior for the imported scope.
4. Safety and privacy scans pass.
5. Documentation states exact support and proof boundaries.
6. No source-project file is modified.
7. `git diff --check`, configure, build, tests, install smoke test, and representative command smoke tests pass.

## First implementation slice

The first slice imports only the verified low-level read/validation boundary:

- immutable save bytes loaded from a caller-supplied path;
- exact 0x8000-byte SRAM requirement;
- main checksum over 0x2598..0x3522 stored at 0x3523;
- Bank 2/3 all-checksums and all 12 per-box checksums;
- internal `red inspect` and `red validate` command paths;
- synthetic tests, including execution with helper paths deliberately missing.

Full semantic decode/export is intentionally deferred to the next slice so that the larger model can be adapted subsystem-by-subsystem with differential coverage.
