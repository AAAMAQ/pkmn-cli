# pkmn 3.0 Phase 2 Implementation Record

## Result

Phase 2 implements the four-route Kanto-remake conversion matrix:

```text
Red  -> FireRed
Red  -> LeafGreen
Blue -> FireRed
Blue -> LeafGreen
```

The established Red-to-FireRed route retains its completed MAQ emulator
evidence. The other three routes and LeafGreen-profile generation are marked
`STATICALLY_VALIDATED_COMMUNITY_TESTING`; this label must remain visible until
equivalent emulator evidence exists.

## Shared-engine architecture

The paired games are profiles over two engines, not four unrelated formats.
The pinned `pret/pokered` project builds Red and Blue from one source tree with
version selection. The pinned `pret/pokefirered` project similarly selects
FireRed or LeafGreen with `GAME_VERSION`, while `src/save.c` implements their
shared sector-based save container.

This does **not** mean the versions are semantically interchangeable. The CLI
keeps explicit profiles and overlays:

- `GEN1_RED` and `GEN1_BLUE` use the common Gen I reader, checksum engine,
  canonical semantic structure, generator, editor, and proof machinery.
- `GEN3_FIRERED` and `GEN3_LEAFGREEN` use the common Kanto-remake sector,
  checksum, Pokémon, JSON, and generator machinery.
- target Pokémon origin metadata writes game code `4` for FireRed and `5` for
  LeafGreen.
- version-only encounters or content are never inferred merely from the game
  label.

## Explicit source declarations

Red and Blue physical saves use the paired Gen I layout. A byte-valid save is
therefore not advertised as proof of which paired ROM created it. Identity is
declared by the command or conversion route and recorded in JSON and Manifest
3.0.

Examples:

```sh
pkmn blue decode game.sav
pkmn convert blue-leafgreen game.sav
pkmn convert red-leafgreen game.sav
```

If a canonical JSON file already contains a conflicting `gameProfile`, the
converter rejects it rather than silently relabeling it.

## Canonical files

The paired formats preserve compatible semantic structure while adding a
required public profile declaration:

| Game | Profile | Extension |
|---|---|---|
| Red | `GEN1_RED` | `.red.json` |
| Blue | `GEN1_BLUE` | `.blue.json` |
| FireRed | `GEN3_FIRERED` | `.fred.json` |
| LeafGreen | `GEN3_LEAFGREEN` | `.lg.json` |

## Static validation gate

The automated gate covers:

- all four route registrations and profile labels;
- deterministic conversion through the same policy and template;
- valid 128 KiB target images, sector maps, signatures, and checksums;
- Manifest 3.0 route/evidence/source-integrity records;
- immutable Gen I source files and in-memory checksum repair;
- target-specific Pokémon origin-game encoding;
- profile-conflict rejection;
- continued Red-to-FireRed regression tests;
- deterministic `pkmn proof convert <route> <source.json>` packages for every
  route;
- JSON parsing, documentation consistency, and privacy scans.

No ROM or private save is part of this repository.

## Source authority

- `pret/pokered@d70d99ffbd329473d96eaaf19fd97c86d2220b7f`
- `pret/pokefirered@df4449a27cd78dd747ce269e47d3ab4a0149d8f4`
- executable overlay: `runtime/data/paired_version_overlays.json`
- Red/Blue ledger: `POKEMON_RED_BLUE_VERSION_DIFFERENCE_LEDGER.md`
- FireRed/LeafGreen ledger: `POKEMON_FIRERED_LEAFGREEN_VERSION_DIFFERENCE_LEDGER.md`

Walkthroughs remain chronology aids only. Pinned pret source is authoritative.
