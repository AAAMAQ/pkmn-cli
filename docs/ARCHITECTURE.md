# Architecture

`pkmn` is a self-contained unified command-line application. Installed Red workflows use internal libraries adapted from the verified Pokemon Red Save Genie and Save Generator projects; users do not install or locate those executables.

```text
pkmn command router
  -> domain command (red, rjson, future fred/frjson)
  -> internal game engine module
  -> validation and safety policy
  -> output/reporting
```

## Modules

- `src/app`: process entry, version, exit codes, and command routing.
- `src/commands`: stable public command contracts.
- `src/red/save`: bounded Gen I SRAM representation and file I/O.
- `src/red/json`: deterministic canonical JSON decode model and serialization; import validation follows next.
- `src/red/generation`: semantic generation (migration pending).
- `src/red/editing`: copy-first validated editing (migration pending).
- `src/red/validation`: structural, checksum, semantic, and policy checks.
- `src/red/comparison`: physical and semantic comparison (migration pending).
- `src/red/reporting`: portable human/machine reports (migration pending).

## Current internal coverage

`red inspect`, `red validate`, and `red decode` internally load a standard 32 KiB SRAM image (preserving optional trailing bytes), validate the main checksum, both bank all-checksums, and all 12 per-box checksums, then optionally export deterministic archival bytes and supported semantics. JSON import workflows, generation, reconstruction, comparison/proof, and editing migrate in the order defined by [SELF_CONTAINED_RED_ENGINE_PLAN.md](SELF_CONTAINED_RED_ENGINE_PLAN.md).

## Non-negotiable boundaries

- Decode reads source `.sav` bytes and produces semantic/archival outputs.
- Generate uses semantic `.red.json` data and never uses `physicalImage` as authority.
- Reconstruct requires `physicalImage` and is explicitly archival.
- Edit writes a collision-safe copy, validates it, and never overwrites the source by default.
- Unsupported runtime locations fail closed or use only the verified canonical safe-location policy.
- FireRed will use internal modules under the same executable, after its separate research engines are verified.

## Reference projects

Pokemon Red Save Genie and Pokemon Red Save Generator remain read-only research/reference projects. Proven code is adapted into this repository's module boundaries with MIT attribution recorded in `THIRD_PARTY_NOTICES.md`; their executables, project structures, UI/CLI entry points, private resources, and build assumptions are not imported or used at runtime.
