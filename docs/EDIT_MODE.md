# Pokemon Red Edit Mode

Editing is always copy-first. `pkmn` validates the source, records its SHA-256, decodes a semantic-only working document, applies one or more edits, generates a new save from verified semantic writers, regenerates every checksum, and validates the result. It never writes into the source save and never uses `physicalImage` as edit authority.

## Interactive editor

```sh
pkmn red edit savefile.sav
```

The looped menu supports trainer/core values and file-backed complex structures for inventory, party, all boxes, current-box cache, Daycare, Hall of Fame, Pokédex, options, playtime, and verified raw world state. It also provides advanced JSON-pointer editing, pending-edit preview, validation, final save, and discard actions. Each staged action is validated before it becomes pending.

Arbitrary location editing is not offered. Generated edits use the emulator-verified Red's-house second-floor preset.

## Scriptable sessions

```sh
pkmn red begin-edit savefile.sav
pkmn red edit-session savefile.edit-session.json --money 999999 --badges all
pkmn red pending-edits savefile.edit-session.json
pkmn red validate-edit savefile.edit-session.json
pkmn red end-edit savefile.edit-session.json
```

`end-edit --auto-suffix` selects `_2`, `_3`, and later output/report names when the preferred name exists. Without it, collisions fail closed.

Named scalar options are:

- `--trainer-name`, `--rival-name`, `--trainer-id`;
- `--money`, `--coins`, `--badges <0..255|all>`;
- `--selected-box <1..12>`.

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

## Validation

Edit validation covers schema identity, money/coin ranges, Gen I text encoding and length, clock values, Pokédex bitfield sizes, inventory capacities and quantities, party and box counts, Pokémon species/level/move/PP/DV/HP constraints, selected/current-box structures, Daycare, Hall of Fame, world-state byte lengths, source identity, generated write ranges, and all Red checksums.

Output reports list every staged change and state explicitly that the source was not overwritten.
