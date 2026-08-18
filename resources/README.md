# Bundled Generation Templates

These files are save-container baselines, not ROMs. The generators replace the
supported semantic data and repair checksums; they never use a source JSON
`physicalImage` as generation authority.

## Pokemon Red

`pokemon-red-usa-europe-v1.template.bin` is the public synthetic/canonical initialization resource from the MIT-licensed Pokemon Red Save Generator project, reference commit `d1e54b3`.

- Size: 32768 bytes
- SHA-256: `248bc35328be435b16b47e2bb87c4e9732c2b5c92a95450839ed4619f74eb2e7`
- Purpose: retain the emulator-verified Red's-house runtime baseline while every supported semantic subsystem, storage block, and checksum is rewritten internally.
- Privacy: it contains no user save, proof evidence, screenshot, ROM, or emulator binary.

The template is never read from target `.red.json physicalImage`. Generation validates this resource's exact identity before use.

## Pokemon FireRed

`pokemon-firered-usa-europe-v1.template.bin` is the clean pre-starter baseline
supplied by MAQ for public use in `pkmn`. It was dumped immediately after the
first in-game save in the upstairs bedroom, before choosing a starter.

- Size: 131072 bytes
- SHA-256: `5fe341091ea41f17ddccaae1aed0ee4802894c94f5281e53dfe795e837a830c0`
- Game profile: Pokemon FireRed, English USA/Europe v1.0-compatible save layout
- State: no party or stored Pokemon, no badges, no Pokedex progress, no Hall of
  Fame data, and an erased inactive save slot
- Purpose: provide FireRed sector layout and a coherent clean world baseline
- Privacy: contains no progressed personal journey, Pokemon, ROM, screenshot,
  proof evidence, or emulator binary

The bundled file is the automatic default. Users may select their own equivalent
clean dump with `--template` or `PKMN_FIRERED_TEMPLATE`. Custom templates pass
strict structural, progression, empty-storage, and privacy checks before use.
