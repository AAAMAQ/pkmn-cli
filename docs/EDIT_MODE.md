# Pokemon Red Edit Mode

Editing is always copy-first. `pkmn` validates the source, records its SHA-256, decodes a semantic-only working document, applies one or more edits, generates a new save from verified semantic writers, regenerates every checksum, and validates the result. It never writes into the source save and never uses `physicalImage` as edit authority.

## Interactive editor

```sh
pkmn red edit savefile.sav
```

The looped menu supports trainer/core values, verified named events, and
file-backed complex structures for inventory, party, all boxes, current-box
cache, Daycare, Hall of Fame, Pokédex, options, playtime, and verified raw world
state. It also provides advanced JSON-pointer editing, pending-edit preview,
validation, final save, and discard actions. Each staged action is validated
before it becomes pending.

Arbitrary location editing is not offered. Generated edits use the emulator-verified Red's-house second-floor preset.

## Scriptable sessions

```sh
pkmn red begin-edit savefile.sav
pkmn red edit-session savefile.edit-session.json --money 999999 --badges all
pkmn red pokemon savefile.edit-session.json party 1 level 100
pkmn red pokemon savefile.edit-session.json party 1 move replace 1 FLY
pkmn red bag savefile.edit-session.json add "MASTER BALL" 99
pkmn red progress savefile.edit-session.json fly-destinations all
pkmn red pending-edits savefile.edit-session.json
pkmn red annotate-edit savefile.edit-session.json "reason for this change"
pkmn red edit-history savefile.edit-session.json
pkmn red undo-edit savefile.edit-session.json
pkmn red validate-edit savefile.edit-session.json
pkmn red end-edit savefile.edit-session.json
```

`end-edit --auto-suffix` selects `_2`, `_3`, and later output/report names when the preferred name exists. Without it, collisions fail closed.

`edit-session --dry-run` evaluates requested edits without changing the session. `end-edit --dry-run` executes generation, checksum validation, re-decode, and semantic comparison without publishing a save or reports. Use `--format json` for automation. Session history and annotations remain semantic metadata and never enter generated save bytes.

Semantic commands remove the need to coordinate dependent JSON fields manually:

- `pokemon ... rename` synchronizes encoded and lossless nickname values;
- `pokemon ... level` recalculates growth-rate experience, party battle stats,
  maximum HP, and current HP;
- `pokemon ... move replace` synchronizes move ID, name, PP, PP Ups, and the
  packed PP byte;
- `bag ... add|remove` maintains item IDs, canonical names, slots, counts,
  capacity, and stack limits;
- `progress ... fly-destinations all` sets only the 11 verified Fly-location
  bits. It does not fabricate arbitrary map visitation.

All semantic commands accept `--dry-run`. Party Pokémon can be selected by
one-based party slot or by a unique species or nickname. Move replacement
validates the Gen I move and PP data but intentionally does not enforce a
species learnset, allowing explicit test scenarios.

Named scalar options are:

- `--trainer-name`, `--rival-name`, `--trainer-id`;
- `--money`, `--coins`, `--badges <0..255|all>`;
- `--selected-box <1..12>`;
- `--location-preset reds-house-2f` (the only emulator-verified preset currently enabled);
- `--event EVENT_NAME on|off`, with `--story-flag`, `--trainer-battle`, and
  `--static-encounter` aliases.

Complex options read the complete JSON value for their semantic field:

- `--bag-file`, `--pc-items-file`, `--party-file`;
- `--boxes-file`, `--current-box-file`;
- `--daycare-file`, `--hall-of-fame-file`;
- `--pokedex-file`, `--options-file`, `--playtime-file`;
- `--world-state-file`.

Values can be extracted from a canonical semantic-only export with tools such as `jq`, for example:

```sh
jq '.decoded.party' savefile.red.json > party.json
pkmn red edit-session savefile.edit-session.json --party-file party.json
```

Advanced editing supports either an inline JSON value or a JSON file:

```sh
pkmn red edit-session session.json \
  --set /decoded/party/pokemon/0/currentHp 25

pkmn red edit-session session.json \
  --set-file /decoded/daycare daycare.json
```

Only existing fields under the supported `/decoded` edit surface are accepted. Location pointers and unsupported roots are rejected. Collection counts and badge mirrors are synchronized automatically.

Raw `scriptsHex` mutation is rejected with a dedicated unsupported-script diagnostic. The field remains preserved for lossless supported generation, but public arbitrary script editing is intentionally unavailable.

## Validation

Edit validation covers schema identity, money/coin ranges, Gen I text encoding and length, clock values, Pokédex bitfield sizes, inventory capacities and quantities, party and box counts, Pokémon species, level/growth-rate experience, the 24-bit experience limit, move-specific PP, packed PP, DVs and HP, selected/current-box structures, Daycare, Hall of Fame, world-state byte lengths, source identity, generated write ranges, and all Red checksums. Erased `0xFF` markers in unused PC boxes, inventories, party, Daycare, and Hall of Fame state are normalized as empty rather than decoded as fake records.

Validation also generates a candidate in memory, validates all checksums, re-decodes it, and compares the requested supported semantics against what was actually encoded. Output reports list every staged change and state explicitly that the source was not overwritten.
