# Japanese Pokémon Red support: implementation plan

Status: experimental implementation; real-save and emulator acceptance pending. This plan builds on the source research in
[`JAPANESE_RED_SAVE_RESEARCH.md`](JAPANESE_RED_SAVE_RESEARCH.md). Every phase
must update this plan with its result, evidence, and any changed decisions.
For a chronological account of the actual changes, decisions, and test evidence,
see the [implementation and analysis walkthrough](JAPANESE_RED_IMPLEMENTATION_WALKTHROUGH.md).

## Goal

Add a Japanese Pokémon Red route to PKMN-CLI that can faithfully archive a
Japanese Red save, translate its game state into the existing English-shaped
Red bridge model without losing Japanese Pokémon name provenance, and generate
an international FireRed save that displays supported Japanese Pokémon names.

The core path is:

```text
Japanese Red .sav
    -> faithful .red.jp.json archive
    -> English-shaped .red.json bridge projection + Japanese-name provenance
    -> existing bridge planner with Japanese-name priority
    -> international FireRed .sav (Japanese language marker on source Pokémon)
```

An English Red `.sav` may be generated as an optional compatibility artifact,
but it cannot be the sole carrier of Japanese names. English Red's ordinary
text encoding cannot represent Japanese Red names. Keep the original archive
and provenance connected to the projection all the way through FireRed output.

## Working rules

- Never overwrite the source save; all generated artifacts use safe output
  behavior already established by PKMN-CLI.
- Keep raw Japanese name fields and their decoded glyphs. A Unicode spelling
  alone may not preserve which byte alias was used.
- Do not copy Japanese Gen I name bytes directly into Gen III. Decode glyphs,
  encode them using FireRed's character map, and mark each applicable Pokémon
  as Japanese.
- Treat source code and generated symbols as layout evidence, not as a
  substitute for validation against real saves.
- Make Japanese support opt-in through a declared/detected source profile;
  never guess Japanese layout from English offsets.
- Keep claims proportional to evidence: parser tests, generated fixtures, real
  save comparisons, and emulator gameplay are separate confidence levels.
- Record research sources, exact revisions, test fixture provenance, decisions,
  known limitations, and contributor credit as work proceeds.

## Phase 0: freeze the route contract

Decide and document the format boundary before writing a reader:

1. `.red.jp.json` is the lossless Japanese archive, with a distinct format
   identity and schema. It represents the Japanese physical layout, including
   8 boxes of 30, source profile/revision, raw name bytes, checksum results,
   the active-box cache, and source-image provenance.
2. `.red.json` remains the English-shaped semantic bridge input with 12 boxes
   of 20. Add a versioned, validated provenance field for source Japanese
   names (raw field bytes, decoded Unicode, source location, alias/ambiguity
   information, and any fallback string used by the English-shaped view).
   Existing `.red.json` consumers must continue to accept old documents.
3. Define name precedence: for a Pokémon with valid Japanese source-name
   provenance, the FireRed planner uses the Japanese nickname and OT glyphs;
   otherwise it follows the existing English policy. Record each conversion,
   fallback, unsupported glyph, or truncation in the manifest.
4. Define treatment of player/rival names separately. They do not carry the
   Pokémon language byte and must not be silently rewritten as if they did.

Deliverable: a short schema/bridge decision record in this document (or a
linked ADR), including the selected JSON fields, schema migration behavior,
CLI naming, and manifest shape. Exit when examples validate and old English
Red-to-FireRed behavior remains unchanged by design.

## Phase 1: fixture and evidence foundation

Collect a small, consented test corpus for Japanese Red v1.0 and v1.1 if
available: fresh save, occupied party, Japanese nicknames/OTs, populated and
selected PC boxes, Daycare, Hall of Fame, and late-game progress. Preserve
fixture provenance, game revision, emulator/hardware source, byte length, and
SHA-256. Keep copyrighted ROM images and private saves out of public releases.

Until real fixtures are available, build synthetic cases from the pinned
disassembly layout and clearly label them synthetic. Add independent checks
against an established save parser or emulator before accepting a field map.
Do not call synthetic coverage real-save validation.

Deliverables: fixture manifest (private fixtures remain private), test vectors
with known offsets/expected values, and a fixture-gap list. Exit when every
planned parser behavior has at least a deterministic test vector and fixture
limitations are explicit.

## Phase 2: Japanese save profile and reader

Add a dedicated Japanese Red profile and bounded reader, reusing the existing
save byte-container patterns where safe. Implement the measured Japanese
offset table from named symbols. Validate accepted save size, main checksum,
box dimensions, species terminators, fixed-field bounds, and declared profile.
Report bank checksum bytes diagnostically until behavior is corroborated on
real saves; do not apply English per-box checksum rules.

Decode the current-box cache and eight permanent boxes distinctly. Reconcile
the cache into the selected logical box exactly once before counting or
repacking, with explicit conflict reporting. Add checks for party, inventory,
progress flags, daycare, play time, and Hall of Fame fields that the bridge
claims to support.

Deliverables: profile/layout implementation, source-derived offset table,
structured validation report, and focused tests. Exit when bounds/checksum
tests pass and real fixture comparisons confirm the selected-box and field
interpretations.

## Phase 3: lossless `.red.jp.json` archive

Create Japanese archive encode/decode/inspect/validate behavior parallel to
the current Red JSON lifecycle, but do not force Japanese saves through the
English `.red.json` validator. Preserve a reconstructable physical image when
requested, exact six-byte name slots (including terminator/padding), decoded
glyph strings, and ambiguity information. Make serialization deterministic.

Test archive generation, validation, reconstruction, and re-decoding. Compare
source and reconstructed bytes where the archive promises physical lossless
round-trip; separately compare semantic fields where canonicalization applies.

Deliverables: documented schema, validator, commands, examples using synthetic
data, migration policy, and round-trip reports. Exit when round-trip fixtures
pass and all lossy/canonical fields are explicitly identified.

## Phase 4: English-shaped bridge projection

Translate Japanese save semantics into the current 12×20 English-shaped
bridge model. Pack the 240 source PC slots in stable order and include a
source-slot-to-projection-slot map so no Pokémon is lost or duplicated. Keep
the original Japanese name provenance adjacent to each Pokémon while filling
the ordinary English-shaped name fields only with valid representable text or
a documented fallback. Never let the fallback replace provenance.

Audit current-box handling in the existing English route as well: the source
research found that the planner enumerates `pcStorage.boxes` while the current
box cache is decoded separately. Fix or explicitly reconcile that behavior
before relying on shared planner logic.

Add a comparator that reports preserved identity/progress, repacking, name
projection, omitted domains, and defaults. An optional generated English Red
save can test whether translated gameplay state loads, saves, and reloads; it
is not a lossless name handoff and must not sever the provenance chain.

Deliverables: projection code, versioned `.red.json` provenance contract,
bridge comparison report, and optional English Red save round-trip report.
Exit when all 240 slots and supported progress domains reconcile without
silent drops and existing English Red bridge tests remain stable.

## Phase 5: Japanese-aware FireRed Pokémon writing and reading

Extend the bridge planner, writer, and native FireRed decoder together:

- Encode Japanese glyphs using the pinned Gen III FireRed character map.
- Write the Japanese language value (`1`) for Pokémon whose preserved source
  nickname/OT are Japanese; keep existing English behavior for other Pokémon.
- Map five-character Gen I names to FireRed's available fields, checking every
  glyph and terminator rather than truncating silently.
- Preserve OT/nickname display rules and consider party, PC, battle, Daycare,
  and Hall of Fame records consistently.
- Record chosen text source, language marker, output bytes, and any unsupported
  glyph/fallback in the conversion manifest.
- Keep player and rival names under their separately documented policy.

Deliverables: Japanese-aware codec tests, Pokémon writer/decoder updates,
manifest assertions, and unchanged-English regression evidence. Exit when
encode/decode round-trips Japanese glyphs, aliases are reported, and no
English route regression appears.

## Phase 6: end-to-end validation

Run the complete route from a Japanese Red `.sav` through archive, projection,
bridge planning, and international FireRed generation. Re-decode the result
with Japanese-aware PKMN-CLI code and compare the planned versus generated
Pokémon, names, language values, identity, supported progress, and checksums.
Then open it in an international FireRed emulator and verify names in party,
PC, summary, battle, Daycare, and Hall of Fame; save, close, and reload.

Test both Japanese Red revisions, empty and populated boxes, current-box
cache reconciliation, Japanese alias glyphs, unsupported/malformed glyphs,
full 240-slot capacity, and failure-safe output behavior. Preserve emulator
version/settings, evidence screenshots/logs, input/output hashes, and exact
commands in the private acceptance package; publish no private save data.

Deliverables: automated test report, semantic comparison, emulator acceptance
record, known-limitations list, and release status. Exit only when each
supported domain has stated evidence and no unsupported case is represented
as verified.

## Phase 7: release and ongoing research record

Update the command guide, schema docs, architecture, troubleshooting, release
notes, credits, and source index. Pin source revisions and describe what was
derived by PKMN-CLI versus what is reused from other contributors. Review
licenses and attribution requirements for all reused code/data before
publication. Document fixture privacy handling and exclude private saves,
ROMs, and identifying screenshots from release assets.

The release may claim Japanese Red-to-international-FireRed support only after
the end-to-end acceptance gate passes. Before that, describe it as research or
experimental implementation, with a clear statement of missing evidence.

## Ongoing documentation record

For each phase, append or link a dated record with:

- goal and scope;
- source revisions and references consulted;
- code/schema/CLI changes;
- test fixtures and whether they are synthetic or real;
- commands run and concise results;
- comparison/emulator evidence and hashes where appropriate;
- decisions, rejected hypotheses, unresolved questions, and limitations;
- contributors and attribution/licensing notes.

Keep a phase marked `planned` until its exit criteria are supported by evidence.
Use `in progress`, `passed`, or `blocked` with a short dated rationale, and
never promote source inspection alone to emulator-verified support.

## Current phase status

| Phase | Status | Evidence / next action |
|---|---|---|
| 0. Route contract | `implemented; synthetic validation passed` | Distinct archive schema, versioned `sourceJapanese` projection extension, and Japanese-name priority. |
| 1. Fixtures | `synthetic vectors passed; real-fixture gap open` | Deterministic 32 KiB source vectors cover stale selected box, both declared revisions, malformed glyph, and 240-slot boundary. No consented real save is available. |
| 2. Japanese reader | `implemented; real-save validation pending` | Profile-aware offsets, main checksum, count and terminator checks, bank checksums diagnostic only. |
| 3. `.red.jp.json` | `implemented; synthetic round-trip passed` | `red-jp decode`, `rjpjson validate/inspect/reconstruct`; exact byte reconstruction tested. |
| 4. English-shaped projection | `implemented; synthetic validation passed` | 8×30 to 12×20 repack, mapping, selected-cache reconciliation, English semantic save generation/validation/redecode. |
| 5. FireRed Japanese names | `implemented; synthetic round-trip passed` | Writer/reader use Gen III Japanese charmap and language byte 1; own OT uses target English trainer name to preserve self-ownership. |
| 6. End-to-end acceptance | `synthetic passed; real/emulator blocked` | Synthetic Japanese SRAM → archive → projection → FireRed save → native FireRed decode passes; no real Japanese Red save or emulator acceptance evidence. |
| 7. Release and credits | `experimental documentation added; publication gate open` | Source pins and credit updated; no verified-support or release claim until real-save and emulator gate. |

## 2026-09-26 implementation and test record

The commands are `red-jp validate|decode|convert` and `rjpjson inspect|validate|reconstruct|project|compare`.
The declared profile must be `JP_RED_REV0` or `JP_RED_REV1`; the save bytes do
not independently prove which ROM revision wrote them. `.red.jp.json` is
`pkmn-red-jp-master-save` schema `0.1.0`. Its raw physical image is the
reconstruction authority. The projection is `.red.json` schema `0.1.0` plus a
versioned `sourceJapanese` extension; the English fallback names are never the
authority for FireRed Pokémon text. The extension stores raw six-byte names,
decoded glyphs, selected-box conflict information, source hash, and slot map.

The bridge's ordinary English Red save is optional and deliberately loses
Japanese display names: it uses `RED`/`BLUE` player/rival fallbacks and English
species fallback nicknames. The archive and projected JSON must therefore be
kept for a Japanese-name-preserving FireRed transfer. The source player/rival
spellings remain in provenance. An international FireRed player name has no
per-name Japanese language byte; for Pokémon owned by the source player, the
target OT is the translated player name (`RED`) to retain self-ownership.
Traded Pokémon retain Japanese OT when encodable. This is an explicit tradeoff,
not a lossless OT claim. Unsupported Gen III glyphs reject conversion rather
than being silently substituted.

Automated evidence: `cmake --build build -j4` and
`ctest --test-dir build --output-on-failure`. The dedicated
`tests/japanese_red_synthetic_test.py` covers archive reconstruction against
source bytes, checksum rejection, malformed nickname rejection, an ambiguous
Gen I glyph alias that decodes and transcodes to Gen III, tampered
provenance rejection, selected-box cache authority, full 240-slot projection,
English semantic save generation/validation/redecode, FireRed
generation/validation/redecode, party/PC/Daycare Japanese names and language value 1,
Hall of Fame Japanese font-control encoding and native re-decode,
and one-shot `red-jp convert`. `rjpjson compare` independently regenerates the
projection from its archive and reports exact-match, counts, selected-box
conflict, and slot-map totals. The one-shot test also confirms unchanged
source/target bytes when an output collision is refused. All fixture bytes are generated by that test;
they are not player saves or emulator results. The test does not establish
Japanese Red gameplay correctness across revisions. Source attribution and
exact revisions remain in the linked research file and third-party notices.

Remaining acceptance: obtain consented v1.0 and v1.1 save fixtures, compare
against an independent parser, run a Japanese Red save/load cycle and the
generated international FireRed save through an emulator, inspect party, PC,
summary, battle, Daycare, Hall of Fame, save/reload, and record hashes/logs.
The [fixture and evidence ledger](JAPANESE_RED_FIXTURE_LEDGER.md) records the
reproducible synthetic hashes and real-fixture gaps. Until then this route is
experimental and its manifest says
`SYNTHETICALLY_VALIDATED_REAL_SAVE_PENDING`.

### 2026-09-28 opt-in Japanese player-name experiment

The default identity policy above is unchanged. `red-jp convert
--retain-playername` and `rjpjson project --retain-playername` now record a
versioned Japanese target-player-name decision in the projection. The bridge
encodes the original Japanese player glyphs into FireRed's identity field and
uses that target name for source-owned Pokémon OTs; the rival remains `BLUE`.
The manifest records the intended Unicode and exact target field bytes. The
generic FireRed decoder cannot infer a Japanese font from the unmarked player
field, and the international game's on-screen rendering is **unverified**.
The generator labels this opt-in output `CANDIDATE_REQUIRES_EMULATOR`.
Synthetic opt-in, tampered-policy, ownership-OT, and default-policy regression
tests pass. This is an experimental byte-preservation option, not release
acceptance or proof that the Japanese name displays correctly in game.

## Research pins and attribution starting point

- Japanese Red/Green layout and text: Narishma-gb, `pokegreen` at
  `953f41b34108621b2bf13c3b1e53abfc9c3e5aec`.
- English Red comparison: pret, `pokered` at
  `d70d99ffbd329473d96eaaf19fd97c86d2220b7f`.
- International FireRed target behavior: pret, `pokefirered` at
  `df4449a27cd78dd747ce269e47d3ab4a0149d8f4`.
- Local baseline examined: PKMN-CLI at
  `077249f84c8db9660bb93ad732748f24bd8747f8` before the Japanese research
  document was added.

The Japanese disassembly README credits pret for much of its repository
structure, assembly, tools, and build scripts. Release credits should name
Narishma-gb and the pret contributors, preserve their project links and
license notices, and separately describe PKMN-CLI's analysis, implementation,
and tests. Verify current license files and any copied-code obligations during
the release phase.
