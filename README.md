# pkmn-cli

`pkmn-cli` is the standalone home of `pkmn`, a unified command-line interface for Pokemon save research, preservation, validation, editing, generation, reconstruction, comparison, and proof workflows.

The project currently provides its C++20/CMake foundation, command router, and environment doctor. Pokemon Red workflow wrappers are the next integration milestone. FireRed and Red-to-FireRed features are future work and will not be advertised as supported until their separate engines are verified and emulator-tested.

## Relationship to the verified Red projects

`pkmn` begins as a wrapper/orchestration layer around two completed engines:

- **Pokemon Red Save Genie** decodes, inspects, validates, safely edits, and exports canonical `.red.json` data.
- **Pokemon Red Save Generator** creates deterministic gameplay-semantic `.sav` files without using target `physicalImage` as generation authority.

The engines remain independent source-of-truth projects. This repository will coordinate them, enforce shared safety policies, and produce consistent output layouts and reports.

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

## Foundation commands

```text
pkmn --help
pkmn --version
pkmn doctor
```

`pkmn doctor` searches for the two Red helper executables in this order:

1. `PKMN_RED_SAVE_GENIE` or `PKMN_RED_SAVE_GENERATOR`;
2. the current `PATH`;
3. nearby sibling development builds.

Missing tools are reported with setup guidance. No save, ROM, or evidence file is read by the doctor.

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

These workflow commands are reserved by the router but intentionally return an unsupported-operation exit code until implemented.

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

