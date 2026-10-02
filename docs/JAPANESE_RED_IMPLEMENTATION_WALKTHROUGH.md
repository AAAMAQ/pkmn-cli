# Japanese Red bridge: implementation and analysis walkthrough

Recorded 2026-09-26. Status: **experimental; synthetic tests pass, real-save and emulator acceptance remain open**. This is an audit of what was built and observed, not a claim that every Japanese Red save or every international FireRed screen has been verified. The [research record](JAPANESE_RED_SAVE_RESEARCH.md) contains the full offset tables and source index; the [phase plan](JAPANESE_RED_IMPLEMENTATION_PLAN.md) holds the acceptance gates; the [fixture ledger](JAPANESE_RED_FIXTURE_LEDGER.md) records test-vector provenance and hashes.

## What existed, and what was added

PKMN-CLI already had an English Red `.sav` reader, `.red.json` format and validator, Red-to-FireRed bridge planner, FireRed writer and native re-reader, and tests. The Japanese research and phase-plan files were also present in the working tree when implementation began. This work used and updated that foundation; it did **not** rebuild the Pokémon and Kanto conversion engine from zero.

The new route adds a distinct Japanese save reader and `.red.jp.json` archive, a checked projection into the existing `.red.json` bridge format, Japanese-name provenance and Gen III transcoding, CLI commands, and synthetic end-to-end tests. The English Red route remains usable without the new extension. The implementation did not copy ROM images, player saves, or source files from the disassemblies into this repository.

```text
Japanese Red .sav
  -> Japanese layout/checksum/text reader
  -> .red.jp.json (faithful archive, with raw source bytes by default)
  -> .red.json (English-shaped bridge, with sourceJapanese provenance)
  -> existing Kanto/FireRed planner, now giving Japanese names priority
  -> international FireRed .sav + manifest
  -> native FireRed validation and re-decode
```

An English Red `.sav` can also be generated from the projection to test compatible game state. It is **not** the Japanese-name handoff: English Red cannot natively encode these names. Keep the Japanese archive and the projected `.red.json` if Japanese-name preservation matters.

## Step-by-step record

### 1. Established the byte-level differences and source authority

The research compared [Narishma-gb's Japanese Red/Green disassembly at the pinned revision](https://github.com/Narishma-gb/pokegreen/tree/953f41b34108621b2bf13c3b1e53abfc9c3e5aec), [pret's English Red disassembly](https://github.com/pret/pokered/tree/d70d99ffbd329473d96eaaf19fd97c86d2220b7f), and [pret's FireRed disassembly](https://github.com/pret/pokefirered/tree/df4449a27cd78dd747ce269e47d3ab4a0149d8f4). The earlier research built the Japanese disassembly in a temporary checkout and used its linker symbols to derive file offsets; it recorded matching ROM hashes for Red v1.0/v1.1, but did not put those ROMs in this repo. The local PKMN-CLI baseline was `077249f84c8db9660bb93ad732748f24bd8747f8`.

The decisive differences: Japanese Red uses 8 PC boxes of 30 instead of 12 of 20; name fields are six bytes instead of eleven; many later SRAM offsets shift; its text bytes are not the English charmap; and its external-box checksum arrangement differs. Both versions still have 240 total PC slots and share many Pokémon record and game ID meanings. These are **source-derived layout findings**; real-save confirmation remains a separate gate. See the [research tables and citations](JAPANESE_RED_SAVE_RESEARCH.md).

### 2. Built a Japanese-specific validator and decoder

The Japanese validator in [`JapaneseSaveValidator.cpp`](../src/red/validation/JapaneseSaveValidator.cpp) checks the 32 KiB base image, the main complemented checksum over the Japanese range, selected-box bounds, box counts and species terminators. It requires `JP_RED_REV0` or `JP_RED_REV1` as a declared profile; SRAM alone does not prove which ROM revision wrote it. External-bank checksum bytes are reported diagnostically rather than enforced, because the pinned Japanese save routine has an identified checksum quirk and real-save behavior is not yet corroborated.

The decoder in [`RedDecoder.cpp`](../src/red/json/RedDecoder.cpp) now selects Japanese or English layout parameters while reusing shared Pokémon-record interpretation and semantic fields. [`JapaneseGen1Codec.cpp`](../src/red/codec/JapaneseGen1Codec.cpp) decodes Japanese six-byte names, retaining the raw bytes, terminator state, unsupported-byte status, and shared-tile ambiguity. It reads the selected current-box cache separately from the eight permanent boxes; a permanent copy can be stale.

### 3. Made a faithful Japanese archive and a reversible check

[`JapaneseRedDocument.cpp`](../src/red/json/JapaneseRedDocument.cpp) defines the separate `pkmn-red-jp-master-save` schema (`0.1.0`). Its archive includes decoded game state, exact six-byte names, source profile/checksum information, and normally the original physical image. Its validator checks that the readable text agrees with the raw fields and, when present, that the image hash and re-decoded state agree. `rjpjson reconstruct` writes back the archived source image; the synthetic test compares all resulting bytes with the input. `--no-physical-image` deliberately removes that physical reconstruction ability.

### 4. Projected into the existing bridge without discarding Japanese names

`rjpjson project` first substitutes the selected current-box cache for that logical box's potentially stale permanent copy. It then packs occupied Pokémon in stable order from Japanese 8×30 storage into English-shaped 12×20 storage, recording the original-to-projected slot map and any selected-box conflict. The result validates as ordinary `pkmn-red-master-save` `.red.json` (`0.1.0`) with an additional versioned `sourceJapanese` extension; [`RedJsonDocument.cpp`](../src/red/json/RedJsonDocument.cpp) checks that provenance for consistency and tampering.

The ordinary `.red.json` name fields get English-compatible fallback text; each Pokémon's `sourceJapanese` field keeps its original raw nickname/OT bytes and decoded glyphs. The root extension keeps the source hash, profile, original trainer/rival names, selected-box details, and slot map. `rjpjson compare` regenerates the expected projection from the archive and demands an exact JSON match, while reporting party/PC/mapping counts. It is a projection-integrity check, **not** an independent gameplay parser or emulator comparison.

The existing English bridge planner's box enumeration was also corrected to treat its selected current-box cache as authoritative. This prevents a stale permanent copy from suppressing Pokémon in either route.

### 5. Transcoded the names at the FireRed boundary

The bridge planner in [`runtime/bridge_planner/pokemon.py`](../runtime/bridge_planner/pokemon.py) prefers valid Japanese provenance over English fallback text, then uses the FireRed Gen III charmap from [`runtime/firered_generator/text.py`](../runtime/firered_generator/text.py). It writes Japanese language value `1` on those Pokémon; the writer and native decoder were both extended to handle it. The manifest records the chosen text, source and target bytes, language, warnings, and policy choices. Unsupported target glyphs fail instead of silently becoming another name.

One test example is a Bulbasaur nicknamed `カキ`. Its Japanese Gen I name starts `85 86 50` (with six-byte-field padding), the English projection displays the fallback `BULBASAUR`, and the FireRed Pokémon has the Gen III sequence `56 57 FF…`, nickname `カキ`, and language `1`. The original Japanese name comes from `sourceJapanese`, not from the English fallback.

OT names need a separate identity decision. A Pokémon owned by the source player uses the target English player name `RED` as OT to preserve self-ownership in international FireRed; the original Japanese OT stays in provenance/manifest. A traded Pokémon's Japanese OT is retained if encodable. Thus “preserve Japanese names” is accurate for Pokémon nicknames and traded OTs, but **not** a promise that the active trainer's Japanese name is displayed as the target player's name. Player/rival source names remain in provenance; their English-compatible target policy is explicit in the [schema/limitations record](JAPANESE_RED_JSON_SCHEMA.md).

Hall of Fame required a separate correction: those FireRed records have a nickname string but no per-Pokémon language byte. The writer now places Japanese font-control bytes around the encoded nickname, and the native reader recognizes them. This behavior follows [pret's Hall of Fame path](https://github.com/pret/pokefirered/blob/df4449a27cd78dd747ce269e47d3ab4a0149d8f4/src/hall_of_fame.c) and [Pokémon text handling](https://github.com/pret/pokefirered/blob/df4449a27cd78dd747ce269e47d3ab4a0149d8f4/src/pokemon.c); actual emulator display is still untested. Daycare uses the Pokémon-name path and has a synthetic re-decode assertion.

### 6. Exposed, integrated, and documented the route

[`JapaneseRedCommand.cpp`](../src/commands/red/JapaneseRedCommand.cpp) adds `red-jp validate|decode|convert` and `rjpjson inspect|validate|reconstruct|project|compare`. The one-shot `red-jp convert` writes archive and projection files, validates the projection, and delegates to the established `rjson convert` FireRed route. CMake, routing, command reference, completion, README, troubleshooting, schema, changelog, and third-party notices were updated. The generated conversion manifest labels this route `EXPERIMENTAL` with evidence `SYNTHETICALLY_VALIDATED_REAL_SAVE_PENDING`.

These commands refuse existing output collisions and do not overwrite the input save. The one-shot sequence is **not** one all-or-nothing transaction: if a later FireRed conversion step fails, earlier archive/projection outputs may already exist.

### 7. Tested and diagnosed the full synthetic path

[`tests/japanese_red_synthetic_test.py`](../tests/japanese_red_synthetic_test.py) constructs temporary 32 KiB Japanese-layout vectors; none is a user save or emulator export. Its assertions cover checksum rejection; malformed/unsupported names; raw archive reconstruction; projection validation and tamper rejection; selected-cache authority; a full 240-Pokémon PC; optional English Red save generation/validation/re-decode; FireRed planning/generation/validation/re-decode; party, PC, traded-OT, Daycare, and Hall of Fame Japanese names; shared-tile alias warnings; and refusal to overwrite an existing target. One path declares REV1 to exercise profile handling, but that does **not** establish v1.1 gameplay compatibility. Vector hashes and gaps are in the [fixture ledger](JAPANESE_RED_FIXTURE_LEDGER.md).

During testing, the C++ executable was current but the CMake-copied Python runtime was stale; the build now syncs runtime files so a re-build exercises current Python code. A planner import was made local to its Japanese branch to avoid a circular import. These were implementation/test-harness corrections, not discoveries about Japanese save data. The Hall of Fame control-byte correction and selected-box-cache correction above *were* semantic findings from source comparison and tests.

## Verification and boundaries

The recorded implementation run completed `cmake --build build -j4`, `ctest --test-dir build --output-on-failure` (all five repository tests), `build/pkmn doctor --deep`, and `git diff --check`. Re-run these after further code changes. The strongest demonstrated result is **synthetic Japanese-layout SRAM → archive → English-shaped projection → generated international FireRed save → PKMN-CLI native re-decode**. The same tool family performs generation and re-decode, so this is not independent target-game verification.

Before calling the route release-ready, obtain consented Japanese Red v1.0 and v1.1 saves; compare parsed fields with an independent parser or Japanese Red emulator; then open generated saves in international FireRed and inspect party, PC, summary, battle, Daycare, Hall of Fame, and save/close/reload. Record emulator version, hashes, redacted observations, and any limits. Do not publish private saves or copyrighted ROMs. The research record and [`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md) credit Narishma-gb and pret; revisit licenses and attribution before publication.

## 2026-09-28 addendum: experimental player-name opt-in

The normal route still puts `RED` in the international FireRed player field.
The new `red-jp convert --retain-playername` flag (also available on
`rjpjson project`) records an opt-in policy in `sourceJapanese`, writes the
original Japanese player glyphs as Gen III bytes in FireRed's eight-byte
identity field, and uses that target name for source-owned Pokémon OTs. The
rival stays `BLUE`. The manifest records intended Unicode, output field hex,
and `UNVERIFIED_IN_ENGLISH_FIRERED`; the generator emits
`CANDIDATE_REQUIRES_EMULATOR`. The generic FireRed reader has no language
marker for the player field, so it can show byte tokens rather than the
intended Japanese name. No in-game display promise is made. Synthetic opt-in
and policy-tamper regression tests pass; a real-save CLI trial produced a
checksum-valid target, but that private save is not a public fixture and no
emulator result has been recorded.
