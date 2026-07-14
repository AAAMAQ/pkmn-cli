# Bundled Pokemon Red Template

`pokemon-red-usa-europe-v1.template.bin` is the public synthetic/canonical initialization resource from the MIT-licensed Pokemon Red Save Generator project, reference commit `d1e54b3`.

- Size: 32768 bytes
- SHA-256: `248bc35328be435b16b47e2bb87c4e9732c2b5c92a95450839ed4619f74eb2e7`
- Purpose: retain the emulator-verified Red's-house runtime baseline while every supported semantic subsystem, storage block, and checksum is rewritten internally.
- Privacy: it contains no user save, proof evidence, screenshot, ROM, or emulator binary.

The template is never read from target `.red.json physicalImage`. Generation validates this resource's exact identity before use.
