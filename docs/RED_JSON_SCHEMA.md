# Canonical Red JSON Schema 0.1.0

The canonical extension is `.red.json`, format identity is `pkmn-red-master-save`, and the current supported schema version is `0.1.0`. Serialization uses deterministic insertion ordering and two-space indentation.

## Top-level sections

- `schema`: format, version, game/generation, region assumption, extension, lossless status, and stability.
- `tool`: producing tool name and version.
- `source`: logical filename, sizes, trailing-byte count, and SHA-256 hashes.
- `integrity`: main, bank, and all 12 box checksum results.
- `decoded`: supported semantic state.
- `reconstruction`: archival availability and policy.
- `diagnostics`: deferred classification and location-policy notes.
- `physicalImage`: optional archival byte image encoded as uppercase continuous hexadecimal.

## Decoded state

`decoded` contains:

- trainer name and ID, rival name;
- money, coins, badges and badge mirror;
- playtime, location, and options;
- Pokédex owned/seen counts and 19-byte bitfields;
- bag and PC-item collections;
- party summary and complete supported party Pokémon records;
- all 12 permanent PC boxes;
- selected-box state and current working-box cache;
- Daycare and Hall of Fame;
- aggregate summary counts;
- supported raw event, script, missable-object, hidden-item, hidden-coin, and visited-town byte fields;
- a 507-entry verified named-event catalog with synchronized trainer-battle,
  static-encounter, and story-progress views.

Pokémon records include species, level, names, HP/status/types, moves/PP/PP Ups, trainer ID, experience, stat experience, DVs, and party stats when applicable. Raw record fields are diagnostic/derived and are not semantic-generation authority.

## Physical-image policy

`physicalImage` is included by default during decode and can be omitted with `--no-physical-image`. It enables exact archival reconstruction. Semantic generation and editing ignore it completely, including when it is replaced with malformed data.

## Validation and compatibility

`pkmn rjson validate` rejects unsupported schema versions, missing required sections, semantic range/structure failures, and invalid physical images. Future incompatible schemas must use a new version and explicit migration tooling rather than silently changing `0.1.0` meaning.

Validation profiles are:

- `standard`: schema, semantic, and optional physical-image integrity;
- `strict`: standard checks plus the complete verified named-event views;
- `generation`: standard checks plus an in-memory semantic-generation acceptance pass;
- `archival`: standard checks plus a required valid `physicalImage`.

`pkmn rjson schema --format json` exposes the supported contract for scripts. `pkmn rjson migrate` performs deterministic compatible enrichment within schema `0.1.0`; it does not silently reinterpret semantic values.

Named event records carry their verified bit index, source constant, human label,
category, and state. Overlapping views must agree. Named editing synchronizes the
views and the lossless raw event range before deterministic generation.

Verified Gen I species, move, item, and map names are descriptive derived fields. Numeric IDs remain the serialized semantic values. Validators reject unsupported canonical IDs and warn when a badge and its corresponding verified gym-battle event disagree; such a mismatch may represent a transient gameplay moment and is not silently repaired.
