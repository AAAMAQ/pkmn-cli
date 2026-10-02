# Japanese Green 1.0: experimental implementation

Implemented on 2026-10-02. This is source-layout and synthetic-test evidence;
real Green saves and emulator acceptance remain pending. Green 1.1, Japanese
Green to LeafGreen, and Japanese-to-English Gen I conversion remain deferred.

## Source basis

The existing [Japanese layout research](JAPANESE_RED_SAVE_RESEARCH.md) records
a Green 1.0 build with SRAM symbols matching Red 1.0/1.1. The pinned
[SRAM definitions](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/ram/sram.asm)
and [build definitions](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/Makefile)
were rechecked. No ROM or private save is included in this change.

Green 1.0 reuses the Japanese eight-box, thirty-slots-per-box layout, six-byte
name fields, main checksum policy, selected-box reconciliation, and Japanese
text codec. Sharing a layout does not establish real-save compatibility; that
remains an acceptance gate. Version-specific encounter tables are not used to
invent captured Pokémon or progress. The existing Kanto semantic bridge is
reused experimentally.

## Commands

```sh
pkmn green-jp validate "Pkmn Green JP.sav" --profile JP_GREEN_REV0
pkmn green-jp decode "Pkmn Green JP.sav" --profile JP_GREEN_REV0
pkmn green-jp convert "Pkmn Green JP.sav" --profile JP_GREEN_REV0
pkmn gjpjson inspect "Pkmn Green JP.green.jp.json"
pkmn gjpjson validate "Pkmn Green JP.green.jp.json"
pkmn gjpjson reconstruct "Pkmn Green JP.green.jp.json"
pkmn gjpjson project "Pkmn Green JP.green.jp.json"
pkmn gjpjson compare "Pkmn Green JP.green.jp.json" "Pkmn Green JP.red.json"
```

The archive format is `pkmn-green-jp-master-save` version `0.1.0`, with an
explicit `JP_GREEN_REV0` source profile. Red archives and Green archives cannot
be passed interchangeably to their game-specific commands. The projection uses
the existing `.red.json` semantic contract but keeps the original Green profile,
Japanese name bytes, source hashes, and slot mappings. The conversion manifest
identifies `green-jp-firered` and marks its capability `EXPERIMENTAL`.

Conversion creates an archive and projection beside the source and produces a
FireRed save and reports. Existing files cause refusal; retain those archives
or choose a separate source copy for another one-shot conversion. For an
existing projection, use `pkmn rjson convert` with a new output path.

## Simple interactive conversion

Run `pkmn interactive`, choose menu 1, then source choice 4 (Japanese Green).
Select the save, confirm original release 1.0, and type YES. The output uses
`_fr.sav`. An unknown revision or version 1.1 is not accepted. Advanced Green
commands appear in menus 2 and 8.

## Verification and remaining work

`tests/japanese_green_synthetic_test.py` generates all fixture bytes locally.
It covers size/checksum validation, profile mismatch and unsupported revision
rejection, archive validation, exact reconstruction including trailing bytes,
projection validation/comparison, missing-image refusal, schema mismatch,
conversion and native FireRed validation, manifest identity, output collisions,
source preservation, and the simple interactive path.

Before verified-support claims: compare consented Green saves with an independent
parser/emulator; exercise progression, Daycare, all boxes, and Hall of Fame;
load/save/reload generated FireRed saves and check Japanese glyph display.
