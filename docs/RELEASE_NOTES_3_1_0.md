# pkmn 3.1.0 — Guided Journeys

Convert a save by answering a few questions. The new `pkmn interactive` starts
with a simple conversion flow and provides guided access to all 124 catalog
commands for conversion, inspection, JSON, editing, comparison, and proof.

## Start here

Download the package for your platform from this release's Assets, install or
extract the complete package, then run:

```sh
pkmn --version
pkmn doctor --deep
pkmn interactive
```

The version should be `pkmn 3.1.0`. Portable users should keep the executable,
runtime, and resources together and use the included interactive launcher.
Packages include the conversion runtime; no separate Python is needed for
these downloads. The Homebrew source formula uses Homebrew Python.

### Homebrew

```sh
brew tap AAAMAQ/pkmn
brew install AAAMAQ/pkmn/pkmn-cli
```

For an existing stable installation, run `brew update` followed by
`brew upgrade AAAMAQ/pkmn/pkmn-cli`. To switch from an old `--HEAD` development
installation, run `brew uninstall AAAMAQ/pkmn/pkmn-cli` and then
`brew install AAAMAQ/pkmn/pkmn-cli`. This removes only the installed CLI package,
not your save files. `brew reinstall` preserves the original `--HEAD` option.

Read the [interactive player's manual](https://github.com/AAAMAQ/pkmn-cli/blob/v3.1.0/docs/INTERACTIVE_USER_MANUAL.md)
for installation, sample files, every menu, and the full command index.

## What's new

- Simple conversion: select the source game, select a save, choose the remake,
  and confirm. Output names are chosen automatically; English conversion can
  repair checksum bytes in memory and numbers existing output families.
- A searchable, paginated command browser with typed questions, path browsing,
  quoted/spaced paths, review, cancellation, and output collision checks.
- Guided editing sessions, comparisons, JSON workflows, and proof tools.
- Experimental Japanese Red 1.0/1.1 and Japanese Green 1.0 to international
  FireRed, with separate archives, name provenance, selected-box reconciliation,
  projection checks, exact reconstruction, and game-specific manifest identity.
- Supported Japanese Pokémon nickname/OT encoding and decoding in the FireRed
  engine, plus an experimental opt-in player-name retention policy.
- A detailed player manual with English and Japanese examples and troubleshooting.

## Compatibility and evidence

Existing direct commands and document schema versions are retained. Generated
reports identify tool version 3.1.0 separately from their schema version.

English Red-to-FireRed retains the project's recorded emulator acceptance.
The other English paired routes remain statically validated/community-testing
routes. Japanese routes are **EXPERIMENTAL**: synthetic tests pass, but real-save
and emulator acceptance are pending. Green 1.1, Japanese-to-LeafGreen, and
Japanese-to-English Gen I conversion are not implemented.

Glitch species, unsupported levels, and invalid semantic records can stop
conversion even when checksums are valid. The CLI does not silently delete
unsupported Pokémon or progress. Review conversion reports and test a generated
save by loading, saving, closing, and reloading it in your emulator.

## Downloads and integrity

Release automation builds Windows x86-64 installer/ZIP, macOS arm64 and Intel
PKG/archive, and Linux x86-64 AppImage/DEB/archive. `SHA256SUMS` lists the uploaded
asset hashes and `pkmn-cli-release.spdx.json` describes package contents.
These packages are not code-signed/notarized by this release workflow.

Source builds are also available through GitHub's source archives. No ROMs,
private saves, emulator binaries, or player proof evidence are included.

## Verification

The test suites cover command routing, direct/guided conversion parity, output
safety, deterministic conversion, Japanese archive round-trips, name handling,
profile rejection, and source preservation. The release workflow runs tests and
installed-package smoke checks on each package platform before publishing assets.
Catalog coverage does not imply exhaustive testing of every possible prompt
combination or emulator acceptance for every supported route.
