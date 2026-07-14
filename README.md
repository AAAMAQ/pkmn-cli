# pkmn-cli

`pkmn-cli` is the standalone home of `pkmn`, a unified command-line interface for Pokemon save research, preservation, validation, editing, generation, reconstruction, comparison, and proof workflows.

The project provides its C++20/CMake foundation, command router, and the first internal Pokemon Red save reader/validator. FireRed and Red-to-FireRed features are future work and will not be advertised as supported until their research engines are verified and emulator-tested.

## Relationship to the verified Red projects

`pkmn` incorporates proven logic from two completed research engines:

- **Pokemon Red Save Genie** decodes, inspects, validates, safely edits, and exports canonical `.red.json` data.
- **Pokemon Red Save Generator** creates deterministic gameplay-semantic `.sav` files without using target `physicalImage` as generation authority.

The completed engines remain independent research/source-reference projects. `pkmn-cli` adapts the necessary MIT-licensed logic into clean internal modules and will not require their executables after installation. See [the self-contained Red engine plan](docs/SELF_CONTAINED_RED_ENGINE_PLAN.md) and [third-party notices](THIRD_PARTY_NOTICES.md).

## Build

Requirements:

- CMake 3.20 or newer;
- a C++20 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the development binary:

```sh
build/pkmn --help
build/pkmn --version
build/pkmn doctor
```

Install to a chosen prefix:

```sh
cmake --install build --prefix /your/install/prefix
```

## Implemented commands

```text
pkmn --help
pkmn --version
pkmn doctor
pkmn red decode input.sav
pkmn red inspect input.sav
pkmn red validate input.sav
pkmn rjson inspect input.red.json
pkmn rjson validate input.red.json
pkmn rjson reconstruct input.red.json
pkmn rjson generate input.red.json [output.sav]
pkmn compare physical first.sav second.sav
pkmn compare semantic first.red.json second.red.json
pkmn proof red input.sav
pkmn red begin-edit input.sav
pkmn red edit-session input.edit-session.json --money 999999 --trainer-name RED
pkmn red pending-edits input.edit-session.json
pkmn red validate-edit input.edit-session.json
pkmn red end-edit input.edit-session.json
```

`pkmn doctor` reports the modules compiled into the standalone executable. It does not search for or invoke Save Genie or Save Generator. No save, ROM, or evidence file is read by the doctor.

`pkmn red decode`, `pkmn red inspect`, and `pkmn red validate` use the internal reader and require no Save Genie executable. Decode includes the archival `physicalImage` by default; use `--no-physical-image` for a semantic-only export. Existing output files are never overwritten.

`pkmn rjson inspect` and `validate` verify schema `0.1.0`, required semantics, and—when present—the physical image SHA-256 and Red checksums. `pkmn rjson reconstruct` is a separate archival mode that requires `physicalImage`; it is never semantic generation.

`pkmn rjson generate` uses only decoded semantic fields. It ignores target `physicalImage`, rewrites supported trainer/core, inventory, Pokédex, party, permanent/current storage, Daycare, Hall of Fame, events/scripts/missables/hidden-state fields, canonicalizes unsafe locations to the verified Red's-house baseline, regenerates all checksums, and writes JSON/Markdown reports. The bundled public template is identity-checked before use.

`pkmn compare physical` reports hashes, percentages, first/last differences, and contiguous equal/different ranges mapped to save banks. `compare semantic` classifies safe-location canonicalization separately from unexpected mismatches. `pkmn proof red` runs decode, generation, re-decode, both comparisons, determinism, and physical-image-isolation checks and creates an emulator checklist; automated proof never claims the manual emulator gate has passed.

Red editing is copy-first. `begin-edit` creates a semantic-only session, `edit-session` accumulates named or JSON-pointer edits, `validate-edit` performs an in-memory generation/checksum gate, and `end-edit` writes a new save plus JSON/Markdown reports. The source hash is rechecked at every validation. Arbitrary locations are rejected; generated edits use the verified Red's-house preset. Complex supported structures—including inventory, party, boxes, current-box cache, Daycare, Hall of Fame, and raw supported event state—are scriptable through `--set /decoded/... <JSON-value>`.

## Planned command shape

```sh
pkmn red decode savefile.sav
pkmn red inspect savefile.sav
pkmn red validate savefile.sav
pkmn red edit savefile.sav
pkmn rjson generate savefile.red.json
pkmn rjson reconstruct savefile.red.json
pkmn proof red savefile.sav
pkmn compare physical original.sav generated.sav
```

Internal `red inspect` and `red validate` are implemented. `red decode` and all `rjson` commands are pending internal modules.

## Generation is not reconstruction

This distinction is central to the project:

| Mode | Authority | Purpose |
|---|---|---|
| Decode | source `.sav` bytes | Parse and export a real save |
| Generate | semantic `.red.json` fields only | Create an independent gameplay-equivalent save |
| Reconstruct | `.red.json physicalImage` | Restore the archived byte image |
| Edit | a protected working copy | Apply verified changes without overwriting the source |

Semantic generation must never read `physicalImage` as authority. Reconstruction requires it and must be labeled archival.

## Safety and privacy

- Inputs and existing outputs are never overwritten by default.
- ROM files are neither required nor distributed.
- Saves, ROMs, screenshots, semantic exports, and proof evidence are ignored by default.
- Arbitrary Pokemon Red location generation/editing remains restricted because valid checksums and parser acceptance do not prove runtime map-state safety.
- Manual emulator load, interaction, save-again, and reparse remain release gates for generated-save claims.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/PRIVACY_AND_PUBLICATION.md](docs/PRIVACY_AND_PUBLICATION.md), and the preserved planning material in [Pkmn Unified CLI Plan](Pkmn%20Unified%20CLI%20Plan/).

## License

MIT License, copyright MAQ / BiG MAQ Studios, with a non-binding stewardship note. This is an independent research project and is not affiliated with Nintendo, Game Freak, Creatures, or The Pokemon Company.
