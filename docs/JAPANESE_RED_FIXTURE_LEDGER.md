# Japanese Red fixture and evidence ledger

Status: synthetic fixtures only, as of 2026-09-26. No user-provided or
otherwise consented real Japanese Red save is present in this repository.
No ROM or save dump is committed with this ledger.

| Fixture | Source / revision declaration | SHA-256 of generated 32 KiB SRAM | Covered behavior |
|---|---|---|---|
| `fixture()` | Locally constructed from pinned `pokegreen` layout; test declares REV0 | `4c2d4ac10525201cc9875b01073922e55b702a09d783293765e297793aed188b` | Party, selected-box cache differing from permanent bank, another occupied PC box, kana names, main checksum, archive/reconstruction, English and FireRed output. |
| `full_box_fixture()` | Same synthetic layout; test declares REV0 | `336771fa9db0406f443d9dabff41af221db08de859610c27290fdbe5bee0460a` | 8×30 logical PC capacity, current-box replacement, 12×20 projection and mapping. |
| Derived negative/alias/Daycare/Hall of Fame vectors | Byte mutations of `fixture()`; one path declares REV1 solely to exercise the explicit profile | Created in a temporary test directory; not kept | Invalid checksum, unsupported name byte, shared-tile alias `0xCD` (へ/ヘ), tampered JSON provenance, Japanese Daycare nickname, and target Hall of Fame Japanese font-control re-decode. |

The generator and assertions are in
[`tests/japanese_red_synthetic_test.py`](../tests/japanese_red_synthetic_test.py).
The initial source vector's save bytes are deliberately minimal and do not
represent an emulator-produced game journey. A declared REV1 vector does
not verify v1.1-specific gameplay; the pinned disassembly showed the same
save layout for v1.0 and v1.1.

Required real-save corpus before release acceptance:

1. Consented Japanese Red v1.0 fresh and progressed saves.
2. Consented Japanese Red v1.1 fresh and progressed saves.
3. Saves containing five-glyph kana names, shared-tile aliases, a traded
   Japanese OT, selected/full PC boxes, Daycare, Hall of Fame, and late-game
   flags; each fixture needs provenance, source ROM revision, length, and hash.
4. Independent parser or emulator comparisons, plus Japanese Red save/load
   and international FireRed play/save/reload observations.

Keep personal saves in a private fixture location, never commit ROMs or
private saves, and publish only permissioned, redacted evidence. The
[implementation plan](JAPANESE_RED_IMPLEMENTATION_PLAN.md) records the
acceptance criteria; the [research file](JAPANESE_RED_SAVE_RESEARCH.md)
indexes the third-party sources.
