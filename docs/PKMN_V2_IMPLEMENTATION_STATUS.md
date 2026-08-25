# pkmn 2.0 Implementation Status

## Implemented in the CLI

- `pkmn red convert <save.sav>` validates a Pokémon Red save, decodes it
  semantically, plans the bridge, generates FireRed output, and writes an
  auditable manifest and report.
- `pkmn rjson convert` converts canonical Red JSON to a FireRed save.
- `pkmn rjson convert_to_frjson` produces proposed FireRed JSON without using
  or embedding physical save bytes.
- `pkmn rjson update_schema` and `pkmn frjson update_schema` write migrated
  copies without overwriting their inputs.
- `pkmn fred` provides native FireRed summary, inspection, checksum validation,
  schema 0.4.0 decoding, narrow safe editing, and pinned pret event discovery.
- `pkmn frjson` provides inspection, validation, schema discovery, archival
  reconstruction, batch generation, and template-backed generation for both
  planned conversion JSON and complete native schema 0.4.0 JSON.
- `pkmn convert` provides unified conversion, plan-only preview, bridge
  inspection/explanation, manifest validation, and batch conversion.
- `pkmn compare` provides FireRed semantic, Pokémon, event, trainer, item, Fly,
  Hall of Fame, progress, and cross-generation bridge reports.
- `pkmn proof fred` and `pkmn proof red-to-firered` create the automated parts
  of the Phase 5 and Phase 6 master-verification packages.
- Conversion is deterministic for the same source, policy, salt, and template.
- Ambiguous trainer mappings default to undefeated; raw Red event IDs are never
  copied into FireRed.

## FireRed template input

The CLI includes the public bridge rules and generator code. It does not include
a ROM or a progressed private FireRed save. It now includes the clean
pre-starter template supplied by MAQ and selects it automatically. Users may
override it with `--template` or `PKMN_FIRERED_TEMPLATE`; an override must pass
the strict clean-template policy.

In the historical 2.0 source installation, Python 3 was required by the bridge
runtime. Version 3 release packages freeze that runtime privately and need no
separate Python installation. No Save Genie or Save Generator executable is
required in either version.

## Verification status

- **Phase 5 — PASS:** automated native generation, determinism, checksum,
  proof-package, physical-image-isolation, logical-authority comparison, and
  MAQ's emulator equivalence review have passed. See
  [Phase 5 FireRed Generator Acceptance](PHASE_5_GENERATOR_ACCEPTANCE.md).
- **Phase 6 — PASS:** deterministic conversion, manifest, bridge audit,
  proof-package verification, FireRed validation, and MAQ's detailed emulator
  equivalence review passed. See
  [Phase 6 Conversion Acceptance](PHASE_6_CONVERSION_ACCEPTANCE.md).

Both master release-verification phases are accepted. Individual generated or
converted saves still retain manifests, warnings, invariant checks, and
optional per-save emulator verification rather than silently hiding policy
decisions.
