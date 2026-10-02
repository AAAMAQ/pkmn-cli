# Experimental Japanese Red archive and bridge projection

`pkmn red-jp decode` writes `.red.jp.json` with format
`pkmn-red-jp-master-save`, schema `0.1.0`. It is a distinct Japanese SRAM
archive, not English `.red.json` with a new file suffix. The source revision
must be declared as `JP_RED_REV0` or `JP_RED_REV1`; the bytes alone are not a
revision detector. The first 32 KiB hold the standard SRAM; any trailing data
is retained in the archive's physical image.

The archive records the Japanese main checksum, two diagnostic external-bank
checksums, Japanese trainer/rival fields, party, eight 30-Pokémon boxes,
separate selected-box cache, Daycare, Hall of Fame, and other decoded bridge
fields. Each name has its exact six-byte `rawHex`, decoded `value`, and
`terminated`, `ambiguousGlyph`, and `unsupportedByte` flags. The physical
image, when present, is the only lossless reconstruction authority.

`pkmn rjpjson project` produces an English-shaped `.red.json` schema `0.1.0`
for the existing planner. It replaces the selected permanent box with the
authoritative current-box cache, then packs occupied PC Pokémon in stable
order into 12 boxes of 20. Root `sourceJapanese` version `1.0.0` records the
source hash/profile, selected-box conflict, permanent copy, source→target
slot map, and original player/rival names. Each party, PC, and in-use Daycare
Pokémon has `sourceJapanese` version `1.0.0` containing the original nickname,
OT, and source locator. An English-compatible fallback is placed in ordinary
name fields. The original Japanese physical image is never copied into this
projection.

The FireRed bridge prefers valid Japanese per-Pokémon provenance over the
English fallback, transcodes the Unicode glyphs into Gen III text bytes, and
sets Pokémon language `1`. Malformed or unsupported glyphs are rejected
instead of silently replaced. For a source player's own Pokémon, the target
OT uses the English target player name to preserve self-ownership; the
original Japanese OT remains in the provenance and manifest. Traded Pokémon
keep Japanese OT if all glyphs are encodable. Player/rival names do not have
the Pokémon language flag; the current target display policy is `RED`/`BLUE`
with original names retained only in provenance.

An explicit experimental exception is available: `pkmn red-jp convert ...
--retain-playername` (or `pkmn rjpjson project ... --retain-playername` before
`pkmn rjson convert`). The projection still has the English-compatible
`decoded.trainer.name` so an optional English Red save remains valid, but its
root `sourceJapanese.targetPlayerNamePolicy` tells the FireRed planner to use
the original Japanese player glyphs. The writer stores those glyphs as Gen III
Japanese bytes in the eight-byte FireRed player-name field. A source-owned
Pokémon's OT uses the same target player name, preserving the name/ID ownership
comparison; traded OTs remain separate. The rival still uses `BLUE`.

This option preserves bytes, **not proven on-screen Japanese rendering**.
International FireRed has no language byte for its player name, and some
screens print that field with the default English font. The generic native
FireRed decoder likewise cannot infer the intended font from the raw identity
field and may show byte tokens rather than Japanese glyphs. The conversion
manifest records the intended Unicode name, exact eight-byte target field,
policy, and `UNVERIFIED_IN_ENGLISH_FIRERED` status; generator output is marked
`CANDIDATE_REQUIRES_EMULATOR`. Use a copy in an emulator and check the continue
screen, trainer card, dialogue, Pokémon ownership, save, and reload before
treating this as a display solution. The default conversion remains `RED`.

When Hall of Fame generation is triggered, its ten-byte nickname slot carries
the target game's Japanese/English font-control wrapper around Japanese glyphs
because that record has no separate language byte. The native FireRed decoder
recognizes the wrapper. This is supported by pinned target source and synthetic
tests, not yet by an emulator display check.

Example workflow:

```sh
pkmn red-jp validate original.sav --profile JP_RED_REV0
pkmn red-jp decode original.sav --profile JP_RED_REV0
pkmn rjpjson validate original.red.jp.json
pkmn rjpjson reconstruct original.red.jp.json
pkmn rjpjson project original.red.jp.json
pkmn rjpjson compare original.red.jp.json original.red.json
pkmn rjson validate original.red.json
pkmn rjson generate original.red.json english-compatibility.sav
pkmn rjson convert original.red.json --output international-fr.sav
pkmn fred validate international-fr.sav
pkmn fred decode international-fr.sav
```

`pkmn red-jp convert original.sav --profile JP_RED_REV0` combines the archive,
projection, and FireRed conversion steps. It writes separate artifacts and
refuses collisions. The generated English Red save is optional and cannot
display the original Japanese names: keep the archive and projection to
retain that provenance. Do not feed an English save re-decoded from that
compatibility artifact into FireRed conversion if Japanese names are required.

Status: experimental. Synthetic byte vectors pass reconstruction and semantic
round trips, but no consented real Japanese save or emulator playthrough has
passed the acceptance gate. See the [research and source index](JAPANESE_RED_SAVE_RESEARCH.md)
and [phase-by-phase evidence record](JAPANESE_RED_IMPLEMENTATION_PLAN.md).
