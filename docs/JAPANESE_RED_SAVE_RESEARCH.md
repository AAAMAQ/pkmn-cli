# Japanese Pokémon Red save research for `pkmn`

Research date: 2026-09-26. This records the pre-implementation source analysis. An experimental implementation now exists; see [the implementation plan and test record](JAPANESE_RED_IMPLEMENTATION_PLAN.md) and [the step-by-step implementation and analysis walkthrough](JAPANESE_RED_IMPLEMENTATION_WALKTHROUGH.md). It has synthetic round-trip evidence, not real-save or emulator acceptance.

## Executive finding

Japanese Red is close enough to the established Red engine to reuse its semantic Pokémon and Kanto bridge, but its SRAM is **not** byte compatible with the English Red reader. The decisive differences are 8 boxes of 30 rather than 12 boxes of 20, six-byte name fields rather than eleven-byte fields, shifted save offsets, a different text map, and different external-box checksum behavior. The 240-PC-Pokémon total capacity is unchanged. These findings come from the pinned Japanese and English disassemblies and, for Japanese Red, an exact-ROM build and linker symbols. [J1] [J2] [E1] [E2]

The international FireRed disassembly contains Japanese glyphs and displays a Pokémon's nickname using the Japanese font when its language byte is Japanese (`1`). Thus preserving original Japanese Pokémon nicknames and OT names in an English FireRed save is technically supported by the target game's code. At the time of this source audit, the `pkmn` bridge did not do it; the experimental implementation described in the linked plan now writes and decodes Japanese Pokémon names. [F1] [F2] [F3] [P4] [P5] [P6]

## Scope, authority, and method

- Primary Japanese source: `Narishma-gb/pokegreen@953f41b34108621b2bf13c3b1e53abfc9c3e5aec`. Its README identifies Japanese Red/Green v1.0 and v1.1. [J0]
- English comparison: the existing project pin, `pret/pokered@d70d99ffbd329473d96eaaf19fd97c86d2220b7f`. [E1]
- International FireRed target: the existing project pin, `pret/pokefirered@df4449a27cd78dd747ce269e47d3ab4a0149d8f4`. Its build defines `GAME_LANGUAGE` as English. [F1]
- Local integration evidence: `pkmn` source and documentation at commit `077249f84c8db9660bb93ad732748f24bd8747f8` before this document. [P1] [P2] [P3]
- The Japanese disassembly was built with RGBDS 1.0.3 in a temporary research checkout. The resulting Red v1.0 and v1.1 ROM SHA-1 hashes matched its `roms.sha1`: `0623ad12f48c259447980d68bd85ddbf8204b2cd` and `ef74c79cded14204ac79e77f4964d9cb25003120`. Red v1.0, Red v1.1, and Green v1.0 produced the same SRAM symbol addresses listed below. The ROMs, build objects, and symbols were **not** added to this repository. [J0] [J3]

To reproduce the symbol derivation in a separate checkout with RGBDS 1.0.3 installed, run `make red red11 green DEBUG=1`, inspect `pokered.sym`, `pokered11.sym`, and `pokegreen.sym`, and map SRAM address `a000..bfff` in bank `n` to file offset `n×0x2000 + (address-0xA000)`. Check the resulting ROM hashes against `roms.sha1`. [J0] [J3]

All numeric file offsets below refer to a conventional 32 KiB SRAM image, with bank 0 at `0x0000`, bank 1 at `0x2000`, bank 2 at `0x4000`, and bank 3 at `0x6000`. For example, the linker symbol `01:a598` is file offset `0x2598`. The disassembly builds a four-bank save cartridge with RAM size code `03`. An emulator export can include extra bytes; retain their provenance separately and do not shift the first `0x8000` bytes. [J1] [J3] [P1]

## SRAM layout: verified differences

| Structure | Japanese Red v1.0/v1.1 | English Red in `pkmn` | Consequence |
|---|---:|---:|---|
| Hall of Fame start | `0x0598` | `0x0598` | Same starting position and 50 teams × 96 bytes, but Japanese nickname occupies at most six bytes inside each 16-byte slot. [J1] [J4] [P2] |
| Main saved game start | `0x2598` | `0x2598` | Shared start does **not** imply shared field offsets. [J1] [E1] |
| Player name | `0x2598`, six bytes | `0x2598`, eleven bytes | Japanese maximum is five text bytes plus terminator. [J2] [E2] |
| Saved main-data body | `0x259E` | `0x25A3` | Every later field must be derived from the Japanese symbols. [J1] [E1] |
| Saved sprite data | `0x2CD5` | `0x2D2C` | Size is `0x200` in Japanese build. [J1] |
| Party block | `0x2ED5`, size `0x158` | `0x2F2C`, size `0x194` | Six Pokémon still, but OT and nickname arrays shrink. [J1] [J2] [P2] |
| Current-box cache | `0x302D`, size `0x566` | `0x30C0`, size `0x462` | The selected box must be read separately from permanent bank storage. [J1] [J2] [P2] |
| Main checksum | stored at `0x3594`; covers `0x2598..0x3593` | stored at `0x3523`; covers `0x2598..0x3522` | Both use complemented 8-bit sum; the ranges differ. [J1] [J5] [P2] |
| Permanent PC boxes | 8 × 30; four per bank; block `0x566` | 12 × 20; six per bank; block `0x462` | Repack for English Red without dropping or duplicating any of the 240 slots. [J1] [J2] [E1] [E2] |
| Box bank checksums | `0x5598`, `0x7598`; no per-box table | `0x5A4C`, `0x7A4C`, then six per-box checksums per bank | English validator cannot be applied to Japanese SRAM. [J1] [J5] [P2] |

Japanese permanent boxes are `0x4000`, `0x4566`, `0x4ACC`, `0x5032`, then `0x6000`, `0x6566`, `0x6ACC`, `0x7032`. These positions are the built symbols `sBox1` through `sBox8`, mapped to file offsets. [J1]

The block sizes are also independently explainable from the source constants and WRAM declarations. Japanese party: `1 count + 7 species/terminator + 6×44 Pokémon bytes + 6×6 OT bytes + 6×6 nickname bytes = 0x158`. Japanese box: `1 count + 31 species/terminator + 30×33 Pokémon bytes + 30×6 OT bytes + 30×6 nickname bytes = 0x566`. The 33-byte boxed and 44-byte party Pokémon *data records* are shared with English Red; the surrounding arrays and positions change. [J2] [E2]

Within a Japanese party block, species starts at `+0x01`, the six records at `+0x08`, OT names at `+0x110`, and nicknames at `+0x134`. Within a Japanese box, species starts at `+0x01`, 30 records at `+0x20`, OT names at `+0x3FE`, and nicknames at `+0x4B2`. Each name slot is six bytes. [J2]

### Selected-box authority and integrity

Japanese Red saves the active box in `sCurBoxData`, inside the main checksummed area, and stores the eight permanent boxes in the two other SRAM banks. Its box-change routine copies the current WRAM box into a bank slot, loads the newly selected bank slot into WRAM, and saves again. The active cache is therefore the authoritative view of the selected box at the time of saving; the corresponding permanent bank slot can be stale. The bridge must merge the current cache into the selected slot **once** before counting or repacking Pokémon. This is also worth auditing in the existing English route: its `pcStorage.boxes` and `currentBoxCache` are decoded separately, while `runtime/bridge_planner/validation.py` currently enumerates only `pcStorage.boxes`. [J5] [P3] [P4]

The Japanese boot/load path verifies the main game checksum. The Japanese box-change code writes one all-boxes checksum per external bank, but its source comments identify a bug: the sum includes the old checksum byte before overwriting that byte. The disassembly contains no read of these external-bank checksum fields in the load path. Therefore a new Japanese validator should treat the main checksum as the primary game-validity gate and report external-bank bytes diagnostically; requiring the English bank/per-box checksum pattern would reject legitimate Japanese saves. Recheck this behavior against real saves before allowing checksum repair. [J5]

## Saved semantic fields and offsets

These file offsets were derived from the Japanese Red v1.0 linker symbols with `sMainData = 0x259E` and `wMainDataStart` as the WRAM base. They were cross-checked against the identical Red v1.1 and Green v1.0 save-symbol layout. They are **source-layout facts**, not yet verified against a Japanese user save. [J1] [J2]

| Semantic field | Japanese file offset | Current English reader |
|---|---:|---:|
| Pokédex owned / seen | `0x259E` / `0x25B1` | `0x25A3` / `0x25B6` |
| Bag count / pairs | `0x25C4` / `0x25C5` | `0x25C9` / `0x25CA` |
| Money / rival name | `0x25EE` / `0x25F1` | `0x25F3` / `0x25F6` |
| Options / badges / Trainer ID | `0x25F7` / `0x25F8` / `0x25FB` | `0x2601` / `0x2602` / `0x2605` |
| Current map / Y / X | `0x2600` / `0x2603` / `0x2604` | `0x260A` / `0x260D` / `0x260E` |
| PC item count / pairs | `0x27DC` / `0x27DD` | `0x27E6` / `0x27E7` |
| Selected-box byte / Hall of Fame count / coins | `0x2842` / `0x2844` / `0x2846` | `0x284C` / `0x284E` / `0x2850` |
| Toggleable/missable object flags | `0x2848` | `0x2852` |
| Saved map-script region start | `0x2892` | `0x289C` |
| Hidden-item flags / hidden-coin flags / visited-town flags | `0x2992` / `0x29A0` / `0x29AD` | `0x299C` / `0x29AA` / `0x29B7` |
| Rival starter / player starter | `0x29B7` / `0x29B9` | `0x29C1` / `0x29C3` |
| Event-flag bitfield | `0x29E9` | `0x29F3` |
| Play time start / Daycare in-use byte | `0x2CA0` / `0x2CA7` | `0x2CED` / `0x2CF4` |
| Daycare nickname / OT / Pokémon record | `0x2CA8` / `0x2CAE` / `0x2CB4` | `0x2CF5` / `0x2D00` / `0x2D0B` |

The current English values above are from `src/red/json/RedDecoder.cpp`; several are hardcoded. The Japanese decoder should use one Japanese layout table derived from named symbols. It must also confirm raw range lengths and every mapped field before claiming full semantic coverage. [P3]

## IDs and Kanto semantics: reuse with checks

At the two pinned Gen I revisions, `pokemon_constants.asm`, `item_constants.asm`, `map_constants.asm`, `event_constants.asm`, and `trainer_constants.asm` are byte-for-byte identical between `pokegreen` and `pokered`. `move_constants.asm` differs only by comments in the animation-ID portion; the playable move-ID declarations are unchanged. This supports reuse of numeric species, item, map, event-bit, trainer-class, and move IDs. It does **not** prove that every localized script has identical progression semantics or that a Japanese save's raw byte positions can be copied into an English save. Use decoded named events and the existing bridge policy, then audit representative progression states. [J6] [E3] [P7]

The Red/Green version distinction still matters for ROM-specific encounter and preset data, but saved Pokémon and completed events are the evidence for a user's actual journey. The existing Red/Blue ledger follows the same principle. A Japanese save should carry an explicit source profile such as `JP_RED_REV0` or `JP_RED_REV1`; no verified SRAM field in this research reliably distinguishes the ROM revision by itself. The three built Japanese profiles have the same save layout. [J0] [J1] [P8]

## Japanese text and losslessness

Japanese Red uses a one-byte game-specific charmap. It shares the `0x50` string terminator with English Red, but the same ordinary byte can mean a different glyph: `0x80` is Japanese `ア` and English `A`. Japanese fields are six bytes, so the normal name limit is five game characters plus terminator. The English `Gen1Codec` and `.red.json` validator cannot encode Japanese names. [J7] [E4] [P2] [P9]

The Japanese charmap contains byte aliases for visually shared tiles, including `べ`/`ベ` at `0x3D`, `ぺ`/`ペ` at `0x47`, `へ`/`ヘ` at `0xCD`, and `り`/`リ` at `0xD8`. A decoder can choose a readable Unicode spelling, but that spelling alone cannot recover which source spelling was intended. Preserve every source name's exact six-byte field as hex, its decoded glyph string, termination/padding information, and an ambiguity marker where applicable. This applies to player, rival, party, box, Daycare, and Hall of Fame names. [J7] [J4]

The FireRed text encoding is **different** again. In its charmap, Japanese `ア` is `0x51` rather than Gen I Japanese `0x80`; Japanese `あ` is `0x01` rather than Gen I Japanese `0xB1`. A transfer must decode Japanese Gen I bytes to glyphs and encode those glyphs into Gen III bytes. Copying the source bytes would corrupt names. The Gen III language byte is stored at Pokémon record byte 18 and must be `1` for Japanese; the native FireRed code then wraps the nickname in Japanese/English font control codes for display and reads at most six Japanese nickname bytes. FireRed's 10-byte nickname and 7-byte OT fields can hold ordinary five-character Japanese Gen I names, subject to exact glyph mapping and target-display checks. [J7] [F1] [F2] [F3] [P5]

FireRed Hall of Fame records are a separate text case: the Hall of Fame writer
copies the value returned by `GetMonData(MON_DATA_NICKNAME)` into a ten-byte
slot, and its display path prints that stored string. For Japanese Pokémon,
`GetMonData` surrounds the glyph bytes with Japanese and English font-control
codes (`FC 15` and `FC 16`). The Hall of Fame slot has no separate Pokémon
language byte, so a generator must store the wrapped display string rather
than bare Japanese glyph bytes. The experimental writer and decoder now do
this; emulator display remains unverified. [F2] [F4]

The international game's Japanese Pokémon-name support does **not** automatically settle the player and rival name fields: those are save identity fields without a per-name Pokémon language byte. The current writer uses the English text encoder for both. Preserve their Japanese originals in the Japanese archive and bridge provenance; select a documented target-display policy only after checking how an international FireRed game renders those fields. [F2] [P5]

## What `pkmn` can reuse, and what must change later

| Existing component | Reuse / adaptation required |
|---|---|
| `src/red/save/RedSave` | Reuse bounded byte access and the `0x8000` base-image concept; select a Japanese profile before decoding. [P1] |
| `src/red/json/RedDecoder` | Reuse Pokémon record interpretation, BCD, stat fields, and semantic output shape; replace all English offsets, name lengths, box dimensions, and text codec. [P3] |
| `src/red/validation/SaveValidator` | Add Japanese layout/checksum policy; English 12-box checks do not apply. [P2] |
| `.red.json` 0.1.0 | Its validator requires 12 boxes of ≤20, an English text codec, and specific raw-field lengths. A Japanese 8×30 archive needs its own `.red.jp.json` identity/schema. A translated `.red.json` can be a separately generated semantic projection with 12×20 packing and English-compatible display strings. [P9] |
| `runtime/bridge_planner` | Existing species, move, inventory, and Kanto conversion policy can start from the translated semantic state. It must preserve the Japanese-name provenance and prioritize it for target Pokémon records. Audit selected-box merge before enumeration. [P4] [P7] |
| `runtime/firered_generator` | Add Japanese Gen III text encoding, write `language=1` for Japanese Pokémon, handle nickname/OT fields and Hall of Fame consistently, and verify the output with a Japanese-aware native re-decoder. Current code writes `language=2` and English names. [P5] [P6] |

An implementation that merely adds Japanese strings as unused metadata to `.red.json` will still produce English Pokémon names, because `runtime/bridge_planner/pokemon.py` currently takes `nickname.value`, truncates it to ten, and explicitly emits `"language": "English"`. A name-priority rule has to be part of the bridge contract and final writer. [P4] [P5]

Recommended data flow for the future implementation:

1. Decode Japanese `.sav` with a declared Japanese Red revision into a faithful `.red.jp.json` archive, including raw bytes, checksums, all 8×30 boxes, selected-box cache, and Unicode text.
2. Reconcile the selected-box cache and produce a semantic English-shaped `.red.json` projection. Repack all 240 PC slots stably into 12×20; record source slot → projected slot mapping and each original Japanese name beside any English-compatible display name. Omit Japanese `physicalImage` from the English projection.
3. Validate the projection against the English JSON contract. If an actual English Red `.sav` is desired, generate it from that semantic projection, re-decode it, and compare the non-text journey and Pokémon identities. A normal English Red game cannot display the original Japanese names correctly through its native English naming codec; the `.red.jp.json` archive/provenance must remain available.
4. Extend Red → FireRed Pokémon conversion to prefer source Japanese nickname and OT glyphs, transcode to Gen III, and mark each target Pokémon Japanese. Re-decode the resulting FireRed save with a Japanese-aware reader and inspect names in an international FireRed emulator. Record every transformation and fallback in the manifest.

This was the research proposal; the linked implementation plan records the subsequent code and synthetic tests.

## Evidence still needed before claiming support

- Real, consented Japanese Red v1.0 and v1.1 `.sav` fixtures, including a fresh save, a full selected box, custom kana nicknames/OTs, Daycare, Hall of Fame, and late-game events. No Japanese save fixture was found in this repository during this research.
- An independent Japanese parser or emulator comparison for exact field decoding and current-box authority. The source build proves layout symbols, not correctness of a future decoder against user data.
- Explicit glyph coverage checks for every Gen I Japanese name byte that can occur in save names against the Gen III charmap. Record aliases and an explicit fallback for unsupported or malformed bytes; never silently replace or truncate.
- International FireRed emulator proof of Japanese nickname and OT display in party, PC, summary, battle, Daycare, and Hall of Fame. The source demonstrates support in code, but this research did not execute an end-to-end generated FireRed save.
- Separate decisions for Japanese player/rival display, source-profile detection, checksum repair, VC exports, and malformed/glitch Pokémon. They should not be inferred from the shared numeric constants.

## Source index

Japanese disassembly:

- [J0] [Japanese Red/Green README and ROM hashes](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/README.md); [ROM hash manifest](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/roms.sha1).
- [J1] [Japanese SRAM sections and box banks](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/ram/sram.asm); exact file offsets above were calculated from the locally built Red v1.0/v1.1 `.sym` files.
- [J2] [Japanese saved WRAM layout](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/ram/wram.asm), [name length](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/constants/text_constants.asm), [Pokémon and box sizes](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/constants/pokemon_data_constants.asm).
- [J3] [Japanese build definitions and SRAM header configuration](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/Makefile).
- [J4] [Hall of Fame record writing](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/engine/movie/hall_of_fame.asm).
- [J5] [Japanese save/load/checksum and box-change routines](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/engine/menus/save.asm).
- [J6] [Japanese species](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/constants/pokemon_constants.asm), [items](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/constants/item_constants.asm), [maps](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/constants/map_constants.asm), [events](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/constants/event_constants.asm), [trainers](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/constants/trainer_constants.asm), [moves](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/constants/move_constants.asm).
- [J7] [Japanese Gen I charmap](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/constants/charmap.asm); [Japanese naming-screen limit](https://github.com/Narishma-gb/pokegreen/blob/953f41b34108621b2bf13c3b1e53abfc9c3e5aec/engine/menus/naming_screen.asm).

English Gen I comparison:

- [E1] [English SRAM sections](https://github.com/pret/pokered/blob/d70d99ffbd329473d96eaaf19fd97c86d2220b7f/ram/sram.asm).
- [E2] [English name and Pokémon/box constants](https://github.com/pret/pokered/blob/d70d99ffbd329473d96eaaf19fd97c86d2220b7f/constants/text_constants.asm), [English saved WRAM layout](https://github.com/pret/pokered/blob/d70d99ffbd329473d96eaaf19fd97c86d2220b7f/ram/wram.asm), [English box constants](https://github.com/pret/pokered/blob/d70d99ffbd329473d96eaaf19fd97c86d2220b7f/constants/pokemon_data_constants.asm).
- [E3] [English event constants](https://github.com/pret/pokered/blob/d70d99ffbd329473d96eaaf19fd97c86d2220b7f/constants/event_constants.asm) and corresponding constants adjacent to [J6].
- [E4] [English Gen I charmap](https://github.com/pret/pokered/blob/d70d99ffbd329473d96eaaf19fd97c86d2220b7f/constants/charmap.asm).

International FireRed target:

- [F1] [FireRed language constants and name lengths](https://github.com/pret/pokefirered/blob/df4449a27cd78dd747ce269e47d3ab4a0149d8f4/include/constants/global.h), [Japanese glyph encoding](https://github.com/pret/pokefirered/blob/df4449a27cd78dd747ce269e47d3ab4a0149d8f4/charmap.txt).
- [F2] [Pokémon nickname/language handling](https://github.com/pret/pokefirered/blob/df4449a27cd78dd747ce269e47d3ab4a0149d8f4/src/pokemon.c), [international-string wrapper](https://github.com/pret/pokefirered/blob/df4449a27cd78dd747ce269e47d3ab4a0149d8f4/src/string_util.c).
- [F3] [Japanese font glyphs and text controls](https://github.com/pret/pokefirered/blob/df4449a27cd78dd747ce269e47d3ab4a0149d8f4/src/text.c), [summary-screen OT handling](https://github.com/pret/pokefirered/blob/df4449a27cd78dd747ce269e47d3ab4a0149d8f4/src/pokemon_summary_screen.c).
- [F4] [FireRed Hall of Fame nickname save/display path](https://github.com/pret/pokefirered/blob/df4449a27cd78dd747ce269e47d3ab4a0149d8f4/src/hall_of_fame.c), [font-control values](https://github.com/pret/pokefirered/blob/df4449a27cd78dd747ce269e47d3ab4a0149d8f4/include/characters.h).

Local `pkmn` integration points:

- [P1] [`RedSave` byte container](../src/red/save/RedSave.hpp).
- [P2] [English `SaveValidator` offsets and checksums](../src/red/validation/SaveValidator.hpp).
- [P3] [English `RedDecoder` fields](../src/red/json/RedDecoder.cpp).
- [P4] [Bridge source Pokémon enumeration](../runtime/bridge_planner/validation.py) and [nickname/language policy](../runtime/bridge_planner/pokemon.py).
- [P5] [FireRed text writer](../runtime/firered_generator/text.py) and [Pokémon writer](../runtime/firered_generator/pokemon.py).
- [P6] [Native FireRed text decoder](../src/firered/native/FireRedTextCodec.cpp) and [Pokémon language byte reader](../src/firered/native/FireRedPokemonCodec.cpp).
- [P7] [Existing Kanto bridge policies](../runtime/data/whole_save_bridge_domains.json) and [route registry](../src/conversion/RouteRegistry.cpp).
- [P8] [Existing Red/Blue difference policy](POKEMON_RED_BLUE_VERSION_DIFFERENCE_LEDGER.md).
- [P9] [`.red.json` schema contract](RED_JSON_SCHEMA.md), [validator](../src/red/json/RedJsonDocument.cpp), and [English Gen I codec](../src/red/codec/Gen1Codec.cpp).
