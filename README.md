# pkmn 3.0

`pkmn` translates complete Pokémon Red and Blue save journeys into Pokémon
FireRed or LeafGreen. It carries forward supported Pokémon, identity, Pokédex,
inventory, badges, trainers, story progression, world state, and other save
semantics instead of moving only individual Pokémon.

The version 3 release packages are self-contained. Ordinary Windows, macOS,
and Linux users do **not** need Python, CMake, Git, a compiler, the earlier Save
Genie projects, or a separately downloaded template.

## Download and start

Download the package for your computer from
[GitHub Releases](https://github.com/AAAMAQ/pkmn-cli/releases):

| Platform | Package | Beginner entry point |
|---|---|---|
| Windows x86-64 | installer or portable ZIP | Start Menu shortcut or `pkmn-interactive.cmd` |
| macOS Apple Silicon | arm64 package/archive | `pkmn-interactive.command` or `pkmn interactive` |
| macOS Intel | x86-64 package/archive | `pkmn-interactive.command` or `pkmn interactive` |
| Linux x86-64 | AppImage, `.deb`, or archive | application launcher or `pkmn interactive` |

After installation, open a terminal and run:

```sh
pkmn interactive
```

The guided mode asks for the source game, target game, save path, output path,
and checksum-repair choice before showing a final confirmation. It calls the
same conversion engine as the direct commands.

Verify any installation with:

```sh
pkmn --version
pkmn doctor --deep
```

The doctor should report `Runtime mode: bundled-private-executable` for a
downloaded release. See the [zero-to-conversion installation guide](docs/INSTALL.md)
if you have never used a command-line program before.

## Convert a save

The four explicit routes are:

```sh
pkmn convert red-firered game.sav
pkmn convert red-leafgreen game.sav
pkmn convert blue-firered game.sav
pkmn convert blue-leafgreen game.sav
```

Convenience commands choose the paired remake by default:

```sh
pkmn red convert game.sav       # Red -> FireRed; writes game_fr.sav
pkmn blue convert game.sav      # Blue -> LeafGreen; writes game_lg.sav
```

Choose the other remake explicitly:

```sh
pkmn red convert game.sav --target leafgreen
pkmn blue convert game.sav --target firered
```

If only the Generation I checksum bytes are damaged, repair a temporary
in-memory copy during conversion:

```sh
pkmn red convert game.sav --auto-repair-checksum
```

The original file is never modified. Add
`--write-repaired-source repaired-game.sav` only if you also want a separate
repaired Gen I copy. Every conversion writes an auditable manifest and readable
report beside the target save.

## Evidence status

`pkmn` labels each route according to the evidence actually completed:

| Route | Status |
|---|---|
| Red → FireRed | `EMULATOR_VERIFIED` — MAQ Phase 5 and Phase 6 acceptance passed |
| Red → LeafGreen | `STATICALLY_VALIDATED_COMMUNITY_TESTING` |
| Blue → FireRed | `STATICALLY_VALIDATED_COMMUNITY_TESTING` |
| Blue → LeafGreen | `STATICALLY_VALIDATED_COMMUNITY_TESTING` |

Static validation is not described as emulator proof. The three new routes use
the shared, regression-tested engines and explicit version overlays, but they
remain open to community testing. Report reproducible problems through
[GitHub Issues](https://github.com/AAAMAQ/pkmn-cli/issues); never upload a ROM
or a private save.

## What is translated

| Domain | Treatment |
|---|---|
| Identity | Player/rival names, public Trainer ID, money, coins, play time, options, and supported target identity |
| Pokémon | Party, PC, Daycare, species, experience, moves, PP, names, OT, DVs/IVs, stat experience/EVs, PID, nature, ability, gender, friendship, origin, and shiny policy |
| Pokédex | Seen/owned state and required target mirrors |
| Trainers | All saved Red trainer events classified; accepted remake counterparts receive translated defeat state |
| Story | Supported starter, rival, Gym, Rocket, ship, tower, Silph, Safari, Cinnabar, League, and Kanto milestone bundles |
| Inventory | Items, Key Items, Balls, TMs/HMs, obtained history, pocket rules, and capacity policy |
| World | Supported blockers, objects, field permissions, transportation, and Fly destinations derived from source visit history |
| Remake-only content | Safe defaults, explicit policy, or omission; unsupported postgame progress is never invented |

The bridge is semantic. Raw event numbers, trainer IDs, item IDs, and physical
offsets are never copied between generations.

```text
Gen I .sav
    → canonical source JSON
    → semantic source state
    → evidence-backed bridge policy
    → deterministic Pokémon conversion
    → target semantic JSON
    → target sector generator
    → validated Gen III .sav + manifest + report
```

## JSON workflows

Canonical JSON remains inspectable and migratable:

```sh
pkmn red decode game.sav
pkmn blue decode game.sav
pkmn fred decode game_fr.sav
pkmn leafgreen decode game_lg.sav

pkmn rjson convert_to_frjson game.red.json
pkmn rjson convert game.red.json
pkmn bjson convert game.blue.json

pkmn rjson update_schema game.red.json
pkmn bjson update_schema game.blue.json
pkmn frjson update_schema game.fred.json
pkmn lgjson update_schema game.lg.json
```

Generation and reconstruction remain separate:

| Mode | Authority |
|---|---|
| Decode | Real source save bytes |
| Generate | Semantic JSON; never hidden `physicalImage` authority |
| Reconstruct | Archived `physicalImage` bytes |
| Convert | Source semantics, bridge policy, and safe target template boundary |
| Edit | Validated working copy; source is not overwritten by default |

## About the bundled template

Physical FireRed/LeafGreen generation needs a structurally valid blank target
container. Version 3 includes an identity-checked, clean save template for this
purpose. It is **not a ROM**, contains no progressed personal journey, and is
not semantic authority. Users normally never mention it.

Advanced users may provide an equivalent clean dump with
`--template clean-save.sav` or `PKMN_FIRERED_TEMPLATE`; strict validation rejects
progressed or unexpected templates.

## Commands and compatibility

Discover the 100+ endpoints with:

```sh
pkmn --help
pkmn get-all-cmds
pkmn convert routes
```

Main domains are `red`, `blue`, `fred`, `leafgreen`, `rjson`, `bjson`,
`frjson`, `lgjson`, `convert`, `compare`, `proof`, and `doctor`. Existing pkmn
2.0 Red → FireRed commands and accepted behavior remain supported, including
the compatibility spelling `pkmn convert red-to-firered`.

See:

- [installation on Windows, macOS, and Linux](docs/INSTALL.md);
- [complete usage guide](docs/COMPLETE_USAGE_GUIDE.md);
- [all commands](docs/ALL_COMMANDS.md);
- [command reference](docs/COMMAND_REFERENCE.md);
- [troubleshooting](docs/TROUBLESHOOTING.md);
- [privacy and publication](docs/PRIVACY_AND_PUBLICATION.md);
- [pkmn 3.0 implementation record](docs/PKMN_3_0_PHASE_3_IMPLEMENTATION.md).

## Build from source

Released downloads have no development prerequisites. Contributors building
from source need CMake 3.20+, a C++20 compiler, and Python 3 for tests and the
developer runtime fallback:

```sh
git clone https://github.com/AAAMAQ/pkmn-cli.git
cd pkmn-cli
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Release maintainers additionally use PyInstaller 6.20 to produce the private
runtime bundle. The tagged workflow builds Windows x86-64, macOS arm64 and
x86-64, and Linux x86-64 packages; it publishes SHA-256 sums, an SPDX SBOM,
licenses, and release notes.

## Safety and privacy

- Existing inputs and outputs are never overwritten by default.
- Wrong-size, malformed, wrong-profile, and inconsistent inputs are rejected.
- Ambiguous trainers default to undefeated.
- Remake-only progression is not inferred from Gen I completion.
- Manifests disclose transfers, translations, defaults, omissions, repairs,
  warnings, and rejection reasons.
- No ROMs, copyrighted game images, private saves, screenshots, credentials,
  or emulator evidence are distributed.
- Release packages are checked by a privacy scan and accompanied by checksums
  and an SBOM.

## Research foundation and credits

This unified program grew from the Pokémon Red Save Genie, Red Save Generator,
FireRed Save Genie, bridge research, and FireRed Save Generator projects. The
research pins:

- `pret/pokered@d70d99ffbd329473d96eaaf19fd97c86d2220b7f`;
- `pret/pokefirered@df4449a27cd78dd747ce269e47d3ab4a0149d8f4`.

Pokémon conversion uses the Pokémon Community Conversion Standard `ORIGINAL`
profile as its foundation, with deterministic project policy recorded in each
manifest. StrategyWiki and Zerokid’s guide supplied human-readable
corroboration while pinned pret remained authoritative.

Thanks to the pret community, Striaton Lab Team and PCCS contributors,
StrategyWiki contributors, Niels Lohmann and JSON for Modern C++ contributors,
the Pokémon and Game Boy communities, everyone who encouraged MAQ’s original
idea, and OpenAI Codex as a transparently used research and engineering
collaborator.

The project is created and maintained by **MAQ / BiG MAQ Studios** for
educational research, preservation, archival work, and personal save
continuity.

## License and independence

MIT License. This independent project is not affiliated with, endorsed by, or
sponsored by Nintendo, Game Freak, Creatures, or The Pokémon Company. Pokémon
and related trademarks belong to their respective owners. No ROMs are included
or required by the CLI.
