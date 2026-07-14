# Third-Party and Incorporated Project Notices

## Pokemon Red engine provenance

Portions of the Pokemon Red save-format knowledge and implementation in this repository are adapted from these completed projects:

- **Pkmn Red Save Genie**, source reference commit `5bd364c1e60443d5cdd9389c8c76cdc87d2a9fec`, copyright 2026 MAQ / BiG MAQ Studios.
- **Pkmn Red Save Generator**, source reference commit `d1e54b3eeb95125bf8f4f04b012b0e94ce240362`, copyright 2026 MAQ / BiG MAQ Studios.

Both source projects are licensed under the MIT License. The complete MIT terms are reproduced in this repository's `LICENSE` file. Both projects and `pkmn-cli` share the same owner; this notice preserves the technical provenance of the incorporated work.

The initial internal save reader and checksum validator adapt the verified Gen I layout and checksum rules from Save Genie's `SaveStructure` module. Later imports will be recorded here as they are incorporated.

No ROMs, private saves, screenshots, emulator artifacts, personal resources, or proof evidence from the source projects are incorporated.

## JSON for Modern C++

This repository vendors JSON for Modern C++ 3.12.0 by Niels Lohmann under the MIT License. Its license is stored at `third_party/nlohmann/LICENSE.MIT`.
