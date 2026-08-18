# pkmn 2.0

## The first complete Pokémon Red → FireRed save translator

`pkmn` is, to the best of our knowledge, the first publicly documented
complete-save translator designed to turn an entire **Pokémon Red** save
journey into a consistent, playable **Pokémon FireRed** save—not merely
transfer individual Pokémon.

> **From Pokémon Red `.sav` to Pokémon FireRed `.sav`, with the journey in
> between translated and explained.**

To the best of our knowledge after reviewing publicly available Pokémon save
editors, transporters, and conversion projects, no earlier released tool
translated the complete Red save experience—Pokémon, identity, inventory,
trainers, badges, story progression, world state, and remake-specific
defaults—into FireRed. Existing cross-generation projects generally move
individual Pokémon or convert save containers. `pkmn` translates the meaning
of the whole supported save.

The project is created and maintained by **MAQ / BiG MAQ Studios**. It is free,
open source, MIT licensed, and built for educational research, archival work,
game preservation, and personal save continuity.

## Convert Pokémon Red to FireRed

```sh
pkmn red convert game.sav
```

If no output name is supplied, the result is written as:

```text
game_fr.sav
```

The conversion also produces an audit manifest and readable report explaining
what was transferred, translated, derived, defaulted, omitted, warned about,
or rejected.

Canonical JSON is supported too:

```sh
# Red JSON directly to a FireRed save
pkmn rjson convert game.red.json

# Red JSON to FireRed JSON without writing a physical save
pkmn rjson convert_to_frjson game.red.json

# Generate a FireRed save from canonical FireRed JSON
pkmn frjson generate game.fred.json
```

Advanced conversion workflows provide preview, plan-only, custom-output,
manifest-validation, policy, and inspection commands:

```sh
pkmn convert red-to-firered game.sav
pkmn convert red-to-firered game.sav --plan-only --output-json game.fred.json
pkmn convert inspect trainer EVENT_BEAT_VIRIDIAN_GYM_TRAINER_0
pkmn convert validate-manifest game_fr.conversion-manifest.json
```

The release includes a validated clean FireRed template, so beginners do not
need to find or create one. Advanced users can select their own equivalent
clean dump with `--template clean-fire-red.sav` or the
`PKMN_FIRERED_TEMPLATE` environment variable. Personal templates are accepted
only after strict clean-state and privacy validation.

**New to emulators, cartridge dumping, save files, or Terminal?** Follow
[Pokémon Red to FireRed: the complete beginner guide](docs/RED_TO_FIRERED_BEGINNER_CONVERSION_GUIDE.md).

## What the translator carries forward

| Domain | Red → FireRed treatment |
|---|---|
| Player identity | Player name, rival name, public Trainer ID, gender policy, and deterministic target identity |
| General state | Money, Game Corner coins, play time, options, badges, and supported completion state |
| Pokémon | Party, PC storage, Daycare, species, experience, level, moves, PP, names, OT identity, and supported held-state semantics |
| Gen I → Gen III mechanics | DVs → IVs, stat experience → legal EVs, deterministic PID and Secret ID, nature, gender, ability, friendship, origin data, and shiny policy |
| Pokédex | Seen and owned species translated into consistent FireRed Pokédex mirrors |
| Trainers | All 322 saved Red trainer events classified; confirmed remake counterparts receive translated defeat state |
| Story | Starter, rival, Gyms, Team Rocket, S.S. Anne, Pokémon Tower, Silph Co., Safari Zone, Cinnabar, League, and other supported Kanto milestones |
| Inventory | Items, Key Items, Poké Balls, TMs, HMs, obtained-history semantics, and pocket/capacity rules |
| World state | Supported blockers, objects, transportation, field permissions, and Fly destinations based on Red visit history |
| FireRed-only systems | Explicit safe defaults or policy decisions for content Red cannot represent, including Sevii, rematches, and other remake extensions |

The conversion is semantic. It does **not** copy Red flag numbers, trainer IDs,
item IDs, or raw offsets into FireRed.

```text
Pokémon Red physical state
    → canonical .red.json
    → semantic Red meaning
    → evidence-backed bridge policy
    → Generation I-to-III Pokémon conversion
    → proposed .fred.json
    → FireRed sector generation
    → validated FireRed .sav
```

When the games cannot represent the same fact identically, the manifest makes
the chosen policy visible. Unresolved state is never silently guessed.

## Why this is more than a Pokémon transporter

Tools such as Poké Transporter GB and multi-generation save editors established
important ways to move or edit individual Pokémon. `pkmn` addresses a different
problem: continuing the saved **journey**.

FireRed is a remake built on a different generation of technology. It has a
different save layout, event system, Pokémon format, mechanics, scripts,
world-state dependencies, and additional story. A Red badge bit alone is not
enough to reproduce a completed FireRed Gym. The target may also need the
Leader battle state, dialogue, reward history, map scene, companion flags, and
trainer-engine state.

That is why `pkmn` uses researched semantic bundles and prerequisite closure
rather than pretending similarly named flags are interchangeable.

## Verified release status

`pkmn 2.0.0` is the public Red → FireRed conversion release.

### Phase 5 — FireRed generator: PASS

MAQ verified that a complete native `.fred.json` could generate a
gameplay-equivalent FireRed save without reading or reconstructing the original
physical save image. The candidate passed deterministic generation, sector and
checksum validation, independent reanalysis, emulator boot, visible gameplay
review, in-game saving, complete close, and reload.

See [Phase 5 generator acceptance](docs/PHASE_5_GENERATOR_ACCEPTANCE.md).

### Phase 6 — Red → FireRed conversion: PASS

MAQ verified the complete path:

```text
Pokémon Red .sav
    → .red.json
    → semantic bridge
    → .fred.json
    → generated FireRed .sav
```

The accepted candidate preserved or correctly translated the tested supported
identity, Pokémon, Pokédex, inventory, badges, trainers, Fly access, story
progression, and FireRed-only policies. It booted in the emulator and could be
saved and reloaded normally.

See [Phase 6 conversion acceptance](docs/PHASE_6_CONVERSION_ACCEPTANCE.md).

Static validity is not treated as emulator proof. Earlier verification phases
found and corrected runtime-only problems involving location state, map layout,
Oak and Viridian scenes, starter state, Brock's separate battle flag, Fly
destinations, story prerequisites, and TM/HM Case handling.

## Requirements

- **CMake 3.20 or newer**;
- a **C++20 compiler**: Apple Clang/Xcode Command Line Tools, GCC, Clang, or a
  modern Visual Studio/MSVC toolchain;
- **Python 3.9 or newer** for the installed FireRed bridge and generator
  runtime;
- **Git** when cloning or updating the project;
- no FireRed ROM or separate template download; a validated clean template is
  bundled for physical FireRed generation.

The FireRed template is a save container, not a ROM. A ROM is not required by
the CLI and is never included. No separate Save Genie or Save Generator
executable is required after installation.

## Install

### Homebrew development installation

```sh
brew tap AAAMAQ/pkmn
brew install --HEAD AAAMAQ/pkmn/pkmn-cli
pkmn doctor --deep
```

To update an existing `--HEAD` installation:

```sh
brew update
brew reinstall --HEAD AAAMAQ/pkmn/pkmn-cli
pkmn --version
pkmn doctor --deep
```

### Build from source

```sh
git clone https://github.com/AAAMAQ/pkmn-cli.git
cd pkmn-cli
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the development build:

```sh
build/pkmn --version
build/pkmn --help
build/pkmn doctor --deep
```

Install to a chosen prefix:

```sh
cmake --install build --prefix /your/install/prefix
```

For a user-local installation, use `--prefix "$HOME/.local"` and ensure
`$HOME/.local/bin` is on `PATH`.

See the [complete Red → FireRed beginner guide](docs/RED_TO_FIRERED_BEGINNER_CONVERSION_GUIDE.md),
the [general beginner's guide](docs/BEGINNERS_GUIDE.md), and
[installation/Homebrew guide](docs/HOMEBREW_INSTALL.md) for step-by-step help.

## Command families

`pkmn` is now the unified maintained home for both Pokémon Red and FireRed save
workflows:

| Command family | Purpose |
|---|---|
| `pkmn red` | Identify, inspect, validate, decode, edit, repair, prove, and convert Red saves |
| `pkmn rjson` | Validate, inspect, migrate, generate, reconstruct, and convert canonical Red JSON |
| `pkmn fred` | Inspect, validate, decode, summarize, repair, edit, and recheck FireRed saves |
| `pkmn frjson` | Validate, migrate, reconstruct, and generate FireRed JSON |
| `pkmn convert` | Preview, plan, inspect, execute, and validate Red → FireRed translation |
| `pkmn compare` | Compare physical saves, semantic JSON, Pokémon, progression, and bridge results |
| `pkmn proof` | Produce deterministic generation/conversion evidence and verify proof packages |
| `pkmn doctor` | Check the installed command router, engines, resources, and deep round trips |

Useful discovery commands:

```sh
pkmn --help
pkmn get-all-cmds
pkmn red --help
pkmn fred --help
pkmn rjson --help
pkmn frjson --help
pkmn convert --help
```

The full command inventory and examples live in:

- [complete usage guide](docs/COMPLETE_USAGE_GUIDE.md);
- [all commands](docs/ALL_COMMANDS.md);
- [command reference](docs/COMMAND_REFERENCE.md);
- [editing guide](docs/EDIT_MODE.md);
- [v2 implementation status](docs/PKMN_V2_IMPLEMENTATION_STATUS.md).

## Canonical JSON and schema updates

The canonical formats make save meaning inspectable and allow future schema
migrations without declaring older exports useless.

```sh
pkmn red decode game.sav
pkmn fred decode game_fr.sav

pkmn rjson update_schema game.red.json
pkmn frjson update_schema game.fred.json
```

From version 2.0 onward, schema evolution and bug fixes are maintained in this
unified repository rather than released independently through the earlier Save
Genie and Save Generator research projects.

## Generation is not reconstruction

| Mode | Authority | Purpose |
|---|---|---|
| Decode | Source `.sav` bytes | Parse a real save into canonical data |
| Generate | Semantic JSON fields | Create an independent gameplay-equivalent save |
| Reconstruct | Archived `physicalImage` | Restore a preserved byte image |
| Edit | Protected working copy | Apply verified changes without overwriting the source |
| Convert | Source-game semantics plus bridge policy | Translate a journey into the target game's representation |

Generation never uses `physicalImage` as hidden authority. Reconstruction is a
separate archival operation and is labeled accordingly.

## Safety and privacy

- Inputs and existing outputs are never overwritten by default.
- Wrong-game, malformed, wrong-sized, and checksum-invalid inputs are rejected
  according to explicit policy.
- Output collisions are refused unless `--auto-suffix` is requested.
- Ambiguous ordinary trainers default to undefeated.
- Unsupported FireRed-only progression is not invented from Red.
- ROMs, progressed private saves, screenshots, semantic exports, arbitrary
  templates, and emulator evidence are excluded. The two documented clean
  generation `.bin` resources are the only template exceptions.
- Every released generation claim requires more than parser acceptance and
  green checksums; the verification model includes real gameplay testing.

Read [privacy and publication](docs/PRIVACY_AND_PUBLICATION.md),
[architecture](docs/ARCHITECTURE.md), and the
[release checklist](docs/RELEASE_CHECKLIST.md).

## Research foundation and credits

This release combines the completed work of:

- **Pkmn Red Save Genie**;
- **Pkmn Red Save Generator**;
- **Pkmn FireRed Save Genie**;
- **Pkmn Bridge Research**;
- **Pkmn FireRed Save Generator**;
- the unified **pkmn CLI**.

The cross-game research uses pinned revisions of:

- `pret/pokered@d70d99ffbd329473d96eaaf19fd97c86d2220b7f`;
- `pret/pokefirered@df4449a27cd78dd747ce269e47d3ab4a0149d8f4`.

The Pokémon conversion policy is founded on the Pokémon Community Conversion
Standard `ORIGINAL` profile, with project-specific deterministic decisions and
overrides recorded in the conversion policy and manifest. Walkthrough research
used StrategyWiki and Zerokid's detailed Pokémon Red guide as corroboration;
pinned pret source remained authoritative.

Thanks to:

- the pret Pokémon reverse-engineering community;
- the Striaton Lab Team, Pokémon Community Conversion Standard, and Poké
  Transporter GB contributors;
- StrategyWiki and community walkthrough contributors;
- Niels Lohmann and JSON for Modern C++ contributors;
- the Game Boy and Pokémon communities that encouraged the original idea and
  preserved decades of technical knowledge;
- OpenAI Codex, used transparently as a research and engineering collaborator
  while MAQ directed the project, supplied the conversion policy, and performed
  the final gameplay verification.

See [third-party notices](THIRD_PARTY_NOTICES.md) for incorporated provenance,
commit references, and licenses.

## Project history

This project began with a public question:

> Could a Pokémon Red save be exported to FireRed so the same journey could
> continue in the remake?

The answer became a collection of decoders, canonical schemas, independent
generators, trainer/event/item bridges, deterministic conversion policies,
manifests, and emulator verification. The complete first-person research story
is maintained in the bridge repository as **The Journey of Converting Pokémon
Red Save Data to FireRed**.

## Roadmap

Future candidates include:

- Pokémon Blue support;
- Pokémon LeafGreen support;
- Blue → FireRed;
- Red → LeafGreen;
- Generation II and corresponding remake paths;
- additional policies, schemas, platform packages, and community-verified
  conversion profiles.

Community help is welcome. Useful contributions include legally shareable save
samples, reproducible emulator observations, bug reports, schema review,
research corrections, documentation, and testing.

## Legal and license

MIT License, copyright **MAQ / BiG MAQ Studios**.

This is an independent educational, research, archival, and game-preservation
project. It is not affiliated with, endorsed by, or sponsored by Nintendo,
Game Freak, Creatures, or The Pokémon Company. Pokémon and related names and
trademarks belong to their respective owners.

No ROMs or copyrighted game images are included or distributed.
