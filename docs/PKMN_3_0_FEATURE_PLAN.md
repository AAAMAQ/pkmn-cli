# pkmn 3.0 Feature Plan

## Status and purpose

This document defines and records the staged scope of `pkmn 3.0`. All three
implementation phases are complete in the final `3.0.0` source tree. Tagged
release CI provides the native Windows, macOS, and Linux artifact gate.

`pkmn 2.0` proved complete-save Pokémon Red → Pokémon FireRed conversion.
Version 3.0 will turn that single proven route into a reusable Kanto remake
conversion system supporting Pokémon Red, Blue, FireRed, and LeafGreen.

The release theme is:

> One safe converter for continuing a Pokémon Red or Blue journey in either
> Pokémon FireRed or LeafGreen.

## Principal features

`pkmn 3.0` will add:

1. Pokémon Blue save support.
2. Pokémon LeafGreen save support.
3. Four complete Gen I → Gen III remake conversion routes.
4. A shared version-aware conversion-route engine.
5. Safe automatic source-checksum repair during conversion.
6. A guided `pkmn interactive` mode for beginners.
7. Blue and LeafGreen canonical JSON formats and schema migration.
8. Version-specific target templates, policies, manifests, and verification.
9. Regression protection for the accepted Red → FireRed route.

## Supported conversion matrix

The unified route registry will support:

| Source | Target | Super-conversion command | Default output |
|---|---|---|---|
| Pokémon Red | Pokémon FireRed | `pkmn convert red-firered save.sav` | `save_fr.sav` |
| Pokémon Red | Pokémon LeafGreen | `pkmn convert red-leafgreen save.sav` | `save_lg.sav` |
| Pokémon Blue | Pokémon FireRed | `pkmn convert blue-firered save.sav` | `save_fr.sav` |
| Pokémon Blue | Pokémon LeafGreen | `pkmn convert blue-leafgreen save.sav` | `save_lg.sav` |

The existing command remains compatible:

```sh
pkmn red convert save.sav
```

It continues to mean Red → FireRed. The new convenient Blue command defaults
to Blue → LeafGreen:

```sh
pkmn blue convert save.sav
```

Either source domain may select the other target explicitly:

```sh
pkmn red convert save.sav --target leafgreen
pkmn blue convert save.sav --target firered
```

The route commands will accept either a compatible physical save or its
matching canonical source JSON. The route, source declaration, target game,
policy version, and target template identity will be recorded in the manifest.

## Shared route architecture

The four routes must not be implemented as four duplicated converters.

The internal model will define:

```text
SourceProfile
  GEN1_RED
  GEN1_BLUE

TargetProfile
  GEN3_FIRERED
  GEN3_LEAFGREEN
```

Each registered route selects:

- the source reader and validator;
- the source canonical schema;
- the shared Kanto semantic model;
- source-version overrides;
- event, trainer, item, gift, encounter, and location bridge rules;
- the Pokémon conversion policy;
- target-version overrides and safe defaults;
- the target template and generator;
- invariant and corruption checks;
- output naming and manifest metadata.

Research and executable mapping data will be layered as:

```text
Gen I Kanto common semantics
    + Red or Blue source overrides
    + FireRed or LeafGreen target overrides
    → selected conversion route
```

No route may copy raw event, trainer, item, map, or address identifiers between
games.

## Pokémon Blue support

Pokémon Red and Blue share most of their physical save structure. Version 3.0
will refactor the current Red engine into a shared Gen I engine while retaining
version-specific provenance and policy.

Planned Blue commands include:

```sh
pkmn blue summary save.sav
pkmn blue inspect save.sav
pkmn blue validate save.sav
pkmn blue repair-checksums save.sav
pkmn blue decode save.sav
pkmn blue convert save.sav
```

Where safe and verified, Blue should gain parity with the Red workflows:

- immutable 32 KiB loading;
- main, bank, and all-box checksum validation;
- readable summaries;
- canonical `.blue.json` decoding;
- semantic generation and archival reconstruction;
- party, PC, Daycare, Pokédex, inventory, badge, Hall of Fame, and event data;
- named event discovery;
- copy-first editing;
- comparison and proof workflows;
- direct conversion to FireRed or LeafGreen.

Blue research must account for:

- version-exclusive species and encounter tables;
- different in-game trades or gifts where applicable;
- version-dependent scripts or text affecting saved meaning;
- source-version provenance for converted Pokémon;
- any event, trainer, item, or map-state differences found in pinned pret.

### Red versus Blue identification limitation

A physical Red or Blue save does not always contain enough trustworthy evidence
to identify its original version with certainty. The layouts are extremely
similar, and version-exclusive Pokémon are not proof because they may have been
traded.

Therefore:

- `pkmn identify` may report `gen1-red-blue-compatible`;
- it must not invent a definite version from weak clues;
- `pkmn red ...` or `pkmn blue ...` is the user's explicit source declaration;
- canonical JSON records that declaration and its confidence;
- conversion manifests preserve the declaration rather than claiming byte-level
  certainty.

## Pokémon LeafGreen support

LeafGreen will use a shared Gen III Kanto-remake engine derived from the
verified FireRed implementation.

Planned commands include:

```sh
pkmn leafgreen summary save.sav
pkmn leafgreen inspect save.sav
pkmn leafgreen validate save.sav
pkmn leafgreen repair-checksums save.sav
pkmn leafgreen decode save.sav
pkmn leafgreen edit save.sav
```

The JSON command family is proposed as:

```sh
pkmn lgjson inspect save.lg.json
pkmn lgjson validate save.lg.json
pkmn lgjson update_schema save.lg.json
pkmn lgjson generate save.lg.json
pkmn lgjson reconstruct save.lg.json
```

LeafGreen will reuse only what source research confirms is shared:

- 128 KiB flash layout;
- rotating save slots and sector checksums;
- logical save blocks;
- Pokémon encryption and checksums;
- party, PC, Daycare, inventory, mail, Pokédex, Hall of Fame, flags, and
  variables;
- deterministic generation infrastructure.

LeafGreen-specific work still includes:

- a clean redistributable LeafGreen generator template;
- a template identity and clean-state profile;
- LeafGreen source and target provenance;
- version-specific Pokémon origin metadata;
- event, item, gift, trade, encounter, and script differences;
- target-specific defaults and invariant bundles;
- native `.lg.json` generation without physical-image authority;
- deterministic and structural verification, followed by clearly labeled
  community testing after release.

FireRed and LeafGreen physical saves can also be difficult to distinguish from
save bytes alone. Target identity is selected explicitly by the command and
enforced through the target policy, template, generator, and manifest.

## Canonical JSON additions

The proposed canonical extensions are:

| Game | Extension | Command family |
|---|---|---|
| Pokémon Red | `.red.json` | `pkmn rjson` |
| Pokémon Blue | `.blue.json` | `pkmn bjson` |
| Pokémon FireRed | `.fred.json` | `pkmn frjson` |
| Pokémon LeafGreen | `.lg.json` | `pkmn lgjson` |

Version 3.0 schema support will include:

- explicit game/version declarations;
- schema-version and policy-version fields;
- source-confidence and user-declaration fields;
- source-game-neutral Gen I semantic structures;
- target-game-neutral remake semantic structures;
- version-specific extension sections;
- deterministic `update_schema` migrations;
- rejection of incompatible JSON/game combinations;
- continued separation of semantic generation from archival reconstruction.

Examples:

```sh
pkmn bjson convert save.blue.json --target leafgreen
pkmn bjson convert_to_lgjson save.blue.json
pkmn lgjson generate save.lg.json
pkmn bjson update_schema save.blue.json
pkmn lgjson update_schema save.lg.json
```

Final command names will be frozen before implementation so aliases do not
create ambiguous long-term contracts.

## Safe automatic checksum repair

Version 3.0 will add automatic repair for otherwise usable Gen I source saves:

```sh
pkmn red convert save.sav --auto-repair-checksum
pkmn blue convert save.sav --auto-repair-checksum
pkmn convert red-leafgreen save.sav --auto-repair-checksum
pkmn convert blue-firered save.sav --auto-repair-checksum
```

`--auto_repair_checksum` may be accepted as a compatibility alias, but the
documented spelling will be `--auto-repair-checksum`.

The repair pipeline will be:

```text
Read immutable source bytes
→ record original SHA-256
→ validate structure and every known checksum
→ repair known checksum bytes in memory
→ validate the repaired working copy
→ run semantic corruption checks
→ decode and convert only when both checks pass
→ record the repair in the conversion manifest
```

Safety rules:

- the original source is never modified;
- repair occurs only when the option or interactive approval is present;
- a healthy source passes through unchanged;
- every changed checksum byte is reported;
- original and working-copy hashes are recorded;
- repaired checksums do not excuse impossible semantic data;
- corruption outside known checksum bytes causes rejection;
- the target generator always calculates valid target checksums normally;
- writing a repaired Gen I copy requires a separate explicit output option.

An optional repaired copy will require:

```sh
--write-repaired-source save_repaired.sav
```

The manifest will distinguish:

```json
{
  "sourceIntegrity": {
    "originalChecksumsValid": false,
    "autoRepairRequested": true,
    "repairApplied": true,
    "sourceFileModified": false,
    "originalSha256": "...",
    "workingCopySha256": "...",
    "repairedChecksumFields": ["main", "box_4"],
    "semanticValidationPassed": true
  }
}
```

Checksum repair can correct checksum fields. It cannot reconstruct gameplay
bytes that were actually damaged.

## Interactive beginner mode

Version 3.0 will add:

```sh
pkmn interactive
```

The opening menu will provide:

```text
1. Convert a save
2. Inspect or summarize a save
3. Validate or repair checksums
4. Decode a save to JSON
5. Generate a save from JSON
6. Update a JSON schema
7. Run pkmn doctor
8. Read beginner help
0. Exit
```

The conversion wizard will ask for:

1. source game: Red or Blue;
2. target game: FireRed or LeafGreen;
3. confirmation of the selected route;
4. source save path;
5. checksum-repair permission only if repair is needed;
6. default or custom target template;
7. output path or automatic filename;
8. final conversion confirmation.

Interactive paths will:

- read the entire line, so spaces do not require shell quoting;
- trim surrounding whitespace;
- remove matching pasted quotation marks;
- expand `~` correctly;
- accept dragged-in terminal paths;
- normalize the result and verify that it exists.

Every screen should accept case-insensitive input and provide:

```text
B = Back
Q = Quit
? = Help
```

If checksums fail but appear repairable, the wizard will explain:

```text
The original file will not be modified.
Repair an internal working copy and continue? [Y/n]
```

Before execution it will show a complete summary of source, target, input,
output, template, checksum treatment, manifest, and report paths.

The interactive interface must call the same internal C++ and runtime APIs as
the non-interactive commands. It must not construct or execute shell commands.
Given identical selections, interactive and non-interactive modes must produce
the same save, manifest, and report.

Only verified routes will appear as selectable. The menu will read available
routes from the shared registry so unfinished Blue or LeafGreen support cannot
be advertised accidentally.

## Conversion coverage

Every route will address the same whole-save domains established by version
2.0, subject to explicit source/target policies:

- player and rival identity;
- public Trainer ID and deterministic target Secret ID;
- money, coins, options, and play time;
- party, PC boxes, current box, and Daycare Pokémon;
- species, experience, moves, PP, names, OT identity, DVs/IVs, stat
  experience/EVs, PID, nature, ability, gender, friendship, shininess, language,
  origin data, Poké Ball, ribbons, and met data;
- Pokédex seen and owned state;
- inventory, Key Items, Poké Balls, TMs, HMs, and obtained-history semantics;
- badges and field-move permissions;
- ordinary trainers and story-controlled battles;
- rival, Gym, Team Rocket, and League progression;
- gifts, fossils, static encounters, and mutually exclusive choices;
- map objects, blockers, transportation, and visited Fly destinations;
- Hall of Fame and completion state;
- hidden-item and game-stat policies;
- target-only content, safe defaults, and omissions.

FireRed/LeafGreen-only content will never be inferred merely because its Red or
Blue source journey is complete. Sevii Islands, National Dex, rematches, Celio,
and other remake extensions require explicit target policies.

## Conversion manifest 3.0

The version 3 manifest will remain the audit authority for every decision. It
will add:

- declared and detected-compatible source profiles;
- selected target profile;
- route registry ID and version;
- original and repaired source hashes;
- checksum-repair evidence;
- source, common-Kanto, and target override layers;
- target template profile and hash;
- version-specific mappings and defaults;
- transferred, translated, derived, defaulted, omitted, rejected, and
  user-selected decisions;
- ambiguity and confidence classifications;
- semantic invariant results;
- target checksum and deterministic-generation results;
- interactive or direct-command invocation provenance.

The manifest must make cross-color conversions just as explicit as natural
pairs. Red → LeafGreen and Blue → FireRed cannot silently pretend that every
version-exclusive detail has a direct equivalent.

## Templates and generation

Version 3.0 will ship only templates that have passed provenance, privacy,
structural, checksum, and deterministic review. The existing FireRed template
also retains its completed MAQ emulator evidence. A new LeafGreen template will
be labeled statically validated and community-testing until equivalent gameplay
evidence exists.

Expected target resources:

```text
pokemon-firered-usa-europe-v1.template.bin
pokemon-leafgreen-usa-europe-v1.template.bin
```

Users may provide equivalent clean dumps through `--template` or the relevant
environment setting. Custom templates must pass the same strict clean-state
policy used by the FireRed generator: correct size and checksums, approved
opening location, empty Pokémon/Pokédex/progression state, erased inactive
slot, and erased special sectors.

No ROM is required by conversion or generation, and no ROM will be bundled.

## Backward compatibility

Version 3.0 must preserve accepted version 2 commands, including:

```sh
pkmn red convert save.sav
pkmn rjson convert save.red.json
pkmn rjson convert_to_frjson save.red.json
pkmn frjson generate save.fred.json
pkmn fred validate save.sav
```

Rules for compatibility:

- Red → FireRed remains the default meaning of `pkmn red convert`;
- existing default FireRed output naming remains unchanged;
- existing v2 manifests remain readable;
- schema migrations are copy-first;
- deprecated aliases produce warnings before any later removal;
- scripts do not enter interactive mode unless `pkmn interactive` is requested;
- non-interactive commands remain deterministic and automation-friendly.

## Verification and evidence policy

Version 3.0 does not require a new MAQ manual in-game verification campaign.
The documentation and CLI must never imply otherwise.

The evidence classifications are:

- `EMULATOR_VERIFIED`: Pokémon Red → FireRed only, based on the completed
  version 2 Phase 5 and Phase 6 testing;
- `STATICALLY_VALIDATED_COMMUNITY_TESTING`: Blue → FireRed, Red → LeafGreen,
  Blue → LeafGreen, and the LeafGreen generator;
- `EXPERIMENTAL`: any incomplete route or subsystem that has not met the full
  static gate and therefore must not be enabled by default.

The status must appear in command output, generated manifests, the interactive
confirmation screen, README support tables, and release notes. A successful
checksum is structural evidence, not gameplay proof.

### Automated verification

- source size, structure, and checksum tests;
- healthy, repairable-checksum, and truly corrupted source fixtures;
- semantic validation after source repair;
- schema and migration tests;
- generator determinism and physical-image isolation;
- target sector and checksum validation;
- Pokémon encryption and legality checks;
- event, trainer, item, location, and invariant tests;
- route-registry uniqueness and overlay tests;
- output collision and source-nonmutation tests;
- interactive/direct output-equivalence tests;
- v2 Red → FireRed regression suite;
- installed-resource and packaging tests on supported platforms.

### Community issue reporting

All new-route output will point users to:

```text
https://github.com/AAAMAQ/pkmn-cli/issues
```

The repository will provide a version 3 conversion bug template requesting:

- `pkmn --version` and operating system;
- conversion route and exact command or interactive selections;
- `pkmn doctor --format json` output;
- the conversion manifest and report after privacy review;
- expected and observed behavior;
- whether the generated save booted, saved, and reloaded;
- reproducible steps.

Users must not be asked to upload ROMs. Save files may contain personal trainer
names, IDs, nicknames, and play history, so the issue template must recommend
redacted reports or private reproduction data rather than public saves.

## Three-phase implementation plan

### Phase 1 — Common 3.0 foundation, recovery, and interface — COMPLETE

Objective: create the shared architecture once while preserving every accepted
version 2 behavior.

Implementation:

- create typed `GEN1_RED`, `GEN1_BLUE`, `GEN3_FIRERED`, and
  `GEN3_LEAFGREEN` profiles;
- implement the conversion-route registry and capability/status discovery;
- refactor the current Red → FireRed path to run through that registry without
  changing its generated result;
- implement Manifest 3.0 route, evidence, source-integrity, repair, template,
  and policy fields;
- implement safe in-memory `--auto-repair-checksum` for the existing
  `pkmn red convert` and `pkmn convert red-firered` commands;
- support the `--auto_repair_checksum` compatibility alias;
- add explicit `--write-repaired-source` for users who request a separate
  repaired Gen I copy;
- reject sources whose checksums can be repaired but whose semantic data
  remains impossible;
- build the reusable `pkmn interactive` menu, path reader, back/help/quit
  controls, confirmations, checksum dialogue, and collision handling;
- expose only the already supported Red → FireRed route during this phase;
- make interactive and direct conversion produce identical saves and manifests;
- begin the self-contained runtime layout so packaged builds no longer depend
  on a user-installed Python interpreter;
- preserve all version 2 commands, schemas, templates, and proof records.

Automated phase gate:

- Red → FireRed output regression is byte-for-byte or policy-equivalent;
- checksum-only damage is repaired in memory and audited;
- payload corruption is rejected even after checksum repair;
- original source files are never modified;
- interactive/direct equivalence passes;
- all current tests and privacy scans pass.

Deliverable: a stable common 3.0 core with Red → FireRed still marked
`EMULATOR_VERIFIED`.

Completion record (2026-08-24): implemented as `3.0.0-alpha.1`. The typed
four-game profile registry, four-route capability registry, canonical
`red-firered` route and v2 alias, Manifest 3.0 audit envelope, source-preserving
in-memory checksum recovery, explicit repaired-copy output, semantic rejection
after repair, interactive Red → FireRed workflow, collision handling through
the shared converter, and automated direct/interactive equivalence checks are
present. The accepted FireRed engine remains behind the bundled runtime
boundary during Phase 1 so its emulator-proven output is not rewritten before
the cross-platform packaging phase.

### Phase 2 — Blue, LeafGreen, and the four-route super converter

Objective: add both new games and compose the complete conversion matrix using
shared Kanto semantics plus version overlays.

Implementation:

- refactor the Red implementation into a common Gen I engine;
- implement Blue summary, inspection, validation, checksum repair, decoding,
  canonical `.blue.json`, generation, comparison, editing, and proof functions
  wherever static evidence supports parity;
- record Red/Blue source declarations without pretending that ambiguous
  physical save bytes prove a version;
- refactor FireRed into a shared Gen III Kanto-remake engine;
- implement LeafGreen summary, inspection, validation, checksum repair,
  decoding, canonical `.lg.json`, generation, comparison, editing, and proof
  functions;
- approve and bundle a structurally clean LeafGreen target template after
  provenance and privacy review;
- complete pinned pret Red/Blue and FireRed/LeafGreen difference ledgers;
- build common-Kanto bridge data with Red, Blue, FireRed, and LeafGreen
  overlays;
- implement all four commands: `red-firered`, `red-leafgreen`,
  `blue-firered`, and `blue-leafgreen`;
- apply the established Pokémon Community Conversion Standard policy to both
  Gen I sources and both Gen III targets;
- implement version-specific gifts, encounters, items, events, trainers,
  target-only defaults, Pokémon origin metadata, and invariants;
- add source/target JSON conversions and schema migrations;
- expose all statically complete routes in `pkmn interactive` with their
  evidence classification clearly displayed;
- default ambiguous state conservatively and never infer remake-only
  progression from Gen I completion.

Automated phase gate:

- all four route registries and overlays validate without duplicate targets or
  raw cross-generation ID copying;
- every output passes target size, sector, checksum, Pokémon, schema, and
  invariant validation;
- conversion is deterministic for identical source, route, policy, salt, and
  template;
- generators ignore archival `physicalImage` authority;
- Blue and LeafGreen JSON generation round trips through their readers;
- every unsupported or ambiguous mapping is manifested rather than guessed;
- Red → FireRed continues to pass the complete regression suite.

Deliverable: the four-route super converter. Red → FireRed retains
`EMULATOR_VERIFIED`; the three new routes and LeafGreen generator are labeled
`STATICALLY_VALIDATED_COMMUNITY_TESTING`.

Completion record (2026-08-24): implemented as `3.0.0-alpha.2`. All four
routes execute through the shared engines; explicit Blue/LeafGreen profiles,
canonical suffixes, route-aware Manifest 3.0 records, target origin metadata,
interactive selection, paired-version overlay authority, and the static route
matrix regression gate are present. No new route is promoted to emulator
verified. See `PKMN_3_0_PHASE_2_IMPLEMENTATION.md`.

### Phase 3 — Self-contained downloads, documentation, and public release

Objective: make version 3.0 downloadable and usable by ordinary Windows,
macOS, and Linux users without compilers, CMake, Git, or a separate Python
installation.

Implementation:

- bundle the conversion runtime privately inside each platform package;
- make runtime, schemas, bridge data, and templates resolve relative to the
  installed executable;
- publish Windows x86-64 portable ZIP and installer packages;
- publish signed/notarized macOS arm64, x86-64, or universal packages where
  signing infrastructure is available;
- publish Linux x86-64 portable archive, `.deb`, and AppImage packages;
- provide a Start Menu or launcher entry for `pkmn interactive` where
  appropriate;
- automate tagged GitHub Releases with packages, SHA-256 sums, SBOM, licenses,
  and release notes;
- update Homebrew from HEAD-only development installation to the stable release
  and bottles when available;
- add clean-machine installation, uninstallation, runtime-discovery, and
  conversion tests on Windows, macOS, and Linux;
- put platform download links and `pkmn interactive` at the top of the README;
- document all four routes, JSON formats, checksum repair, templates, evidence
  labels, troubleshooting, privacy, and version 2 compatibility;
- add GitHub issue templates and print the issue URL after warnings or failures;
- freeze version 3 command names, schemas, route policies, manifests, and
  package layouts.

Automated release gate:

- every package works on a clean environment without Python, CMake, Git, or a
  compiler;
- `pkmn doctor --deep`, interactive mode, static fixture conversions, target
  validation, and uninstall checks pass;
- package contents contain no ROM, progressed private save, screenshot, secret,
  or unpublished evidence;
- checksums and SBOM match the attached release artifacts;
- CLI and documentation consistently distinguish emulator-proven Red → FireRed
  from community-testing routes.

Deliverable: public `pkmn 3.0` downloads and the beginner-oriented interactive
experience, with GitHub Issues serving as the feedback and bug-report channel.

Completion record (2026-08-25): implemented as `3.0.0`. Release builds
now prefer a private one-directory runtime bundle and require no separate
Python installation. Windows, macOS, and Linux packaging, launchers, installed
smoke tests, checksums, SBOM, privacy scanning, release publication, and the
complete user documentation are automated. Local macOS arm64 build, install,
runtime-discovery, deep-doctor, no-system-Python conversion, and target JSON
validation passed. Native Windows, Intel macOS, and Linux artifacts are built
and verified by the tagged CI matrix. See `PKMN_3_0_PHASE_3_IMPLEMENTATION.md`.

## Completion standard

`pkmn 3.0` is complete only when:

- Red and Blue sources have verified readers and canonical semantic models;
- FireRed and LeafGreen targets have independently verified generators;
- all four routes have explicit event, trainer, item, Pokémon, and whole-save
  policies;
- source checksum repair is copy-free by default, auditable, and unable to hide
  semantic corruption;
- interactive mode uses the same conversion engine and produces identical
  artifacts;
- v2 commands and accepted Red → FireRed behavior remain compatible;
- every released route passes its declared automated evidence gate;
- only Red → FireRed is described as MAQ emulator-verified;
- Blue → FireRed, Red → LeafGreen, Blue → LeafGreen, and LeafGreen generation
  are visibly labeled statically validated and community-testing;
- manifests explain every transfer, translation, default, omission, repair,
  warning, and rejection;
- no raw cross-generation IDs are copied and no unsupported target progression
  is invented.

The goal of version 3.0 is not merely to add more command names. It is to turn
the proven Red → FireRed work into a safe, auditable, beginner-friendly and
version-aware Kanto save-translation platform.
