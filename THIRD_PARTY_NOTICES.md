# Third-Party and Incorporated Project Notices

## Pokemon Red engine provenance

Portions of the Pokemon Red save-format knowledge and implementation in this repository are adapted from these completed projects:

- **Pkmn Red Save Genie**, source reference commit `5bd364c1e60443d5cdd9389c8c76cdc87d2a9fec`, copyright 2026 MAQ / BiG MAQ Studios.
- **Pkmn Red Save Generator**, source reference commit `d1e54b3eeb95125bf8f4f04b012b0e94ce240362`, copyright 2026 MAQ / BiG MAQ Studios.

Both source projects are licensed under the MIT License. The complete MIT terms are reproduced in this repository's `LICENSE` file. Both projects and `pkmn-cli` share the same owner; this notice preserves the technical provenance of the incorporated work.

The internal reader, codecs, decoder, validators, semantic serializers, checksum repair, safe-location policy, storage/current-box distinction, generation template policy, species base-stat and growth-rate data, and move PP data adapt the verified Gen I implementation and findings from those projects.

The public synthetic canonical initialization resource is incorporated as `resources/pokemon-red-usa-europe-v1.template.bin`; its identity and purpose are documented in `resources/README.md`.

No ROMs, private saves, screenshots, emulator artifacts, personal resources, or proof evidence from the source projects are incorporated.

## Pokemon FireRed and conversion engine provenance

The native FireRed reader, checksum/sector analysis, schema 0.4.0 exporter,
summary engine, archival importer, and narrow safe editor are adapted from
**Pkmn FireRed Save Genie**, source reference commit
`713f053`, copyright 2026 MAQ / BiG MAQ Studios, MIT License.

The bundled deterministic bridge planner, Pokémon conversion policy, event,
trainer and item authorities, and template-backed generator are adapted from:

- **Pkmn Bridge Research**, source reference commit `ea10edc`, MIT License;
- **Pkmn FireRed Save Generator**, source reference commit `a1a0743`, MIT
  License.

The pinned research authorities remain:

- `pret/pokered@d70d99ffbd329473d96eaaf19fd97c86d2220b7f`;
- `pret/pokefirered@df4449a27cd78dd747ce269e47d3ab4a0149d8f4`.

The Pokémon conversion policy uses the **Pokémon Community Conversion
Standard** `ORIGINAL` profile as a research/specification foundation at commit
`c59d238f1b4dac5221498077bb0a228d21a4c0ff`, with every project-specific
override recorded in `runtime/data/pokemon_policy_original_v1.json`. No PCCS
source code or repository files are copied into this project. The upstream
repository did not declare a GitHub-detectable license when this notice was
reviewed, so this project treats it as a cited standard rather than incorporated
licensed code.

No FireRed ROM, progressed user save, screenshot, or emulator state is
incorporated. The clean pre-starter FireRed save-container baseline supplied by
MAQ for public project use is incorporated as
`resources/pokemon-firered-usa-europe-v1.template.bin`; its identity, limited
state, and purpose are documented in `resources/README.md`. Users may instead
provide their own strictly validated equivalent clean dump.

## JSON for Modern C++

This repository vendors JSON for Modern C++ 3.12.0 by Niels Lohmann under the MIT License. Its license is stored at `third_party/nlohmann/LICENSE.MIT`.
