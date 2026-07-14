# Architecture

`pkmn` is the public orchestration layer for independently verified save engines. It does not duplicate their parsing and generation logic during the first integration phase.

```text
pkmn command router
  -> workflow command
  -> helper discovery/configuration
  -> Pokemon Red Save Genie or Save Generator process
  -> output validation, reporting, and packaging
```

## Current foundation

- `src/app`: stable process entry, version, exit codes, and command routing.
- `src/commands`: domain command implementations; currently `doctor`.
- `src/integration`: external helper discovery. Process execution will be added with the first Red wrapper commands.
- `tests`: synthetic command parsing tests with no private saves or ROM dependencies.

## Non-negotiable workflow boundaries

- Decode reads source `.sav` bytes and produces semantic/archival outputs.
- Generate uses semantic `.red.json` data and must never use its `physicalImage` as authority.
- Reconstruct requires `physicalImage` and is explicitly archival, not semantic generation.
- Edit works on a copy, validates before final output, and does not overwrite the source by default.
- FireRed and Red-to-FireRed commands remain unavailable until separate engines are verified and emulator-tested.

## Integration status

The Save Generator already provides a scriptable `pkmn-red-save-generator` executable. Save Genie currently has a verified interactive executable but needs a stable automation-facing CLI contract before decode/inspect/validate wrapper commands can be completed safely.

