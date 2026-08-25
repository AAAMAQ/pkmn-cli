# pkmn 3.0 Phase 3 implementation record

Date: 2026-08-25
Version: `3.0.0`

## Outcome

Phase 3 converts the Phase 2 codebase into a self-contained, beginner-oriented
release system. Downloaded packages no longer require users to install Python,
CMake, Git, or a compiler.

## Runtime architecture

The C++ `pkmn` executable now searches for a private `pkmn-runtime` beside its
installed package layout. Release builds freeze the deterministic Python
planner/generator and all required data into a platform-native PyInstaller
one-directory bundle under `libexec/pkmn/runtime`.

One-directory packaging was deliberately chosen over a single-file extractor:
it avoids temporary self-extraction and operating-system semaphore failures in
hardened environments. The executable and private runtime are still one
application package from the user’s perspective.

Runtime selection is explicit:

- `bundled-private-executable`: release package; no separate Python required;
- `developer-python-fallback`: source build; uses the checked-in runtime and
  the contributor’s Python installation.

`pkmn doctor --deep` and `pkmn config show --format json` report this status.

## Distribution matrix

The tagged GitHub Actions workflow builds:

- Windows x86-64 portable ZIP and NSIS installer;
- macOS arm64 archive and package;
- macOS x86-64 archive and package;
- Linux x86-64 archive, Debian package, and AppImage.

It builds and tests the private runtime, runs CTest, installs to an empty
prefix, runs installed-tree smoke tests, packages the result, produces
`SHA256SUMS`, generates an SPDX release SBOM, and attaches everything to the
tagged GitHub Release. Signing/notarization is performed only when real
maintainer signing infrastructure is available and is never simulated.

## Beginner entry points

- `pkmn interactive` on every platform;
- Windows command/Start Menu launcher;
- macOS `.command` launcher;
- Linux terminal desktop launcher and AppImage entry point.

All call the same typed four-route conversion engine.

## Release gates implemented

- clean build/test/install jobs for Windows, macOS, and Linux;
- installed runtime/resource discovery;
- private-runtime mode assertion;
- deep deterministic doctor test;
- four-route registry verification;
- schema availability check;
- privacy scan;
- CPack packages and Linux AppImage construction;
- checksums, SBOM, license, and release-note automation;
- release and GitHub issue documentation.

## Evidence boundary

No new manual gameplay campaign is required for Phase 3. Red → FireRed retains
MAQ’s completed Phase 5/6 emulator verification. Red → LeafGreen, Blue →
FireRed, and Blue → LeafGreen remain
`STATICALLY_VALIDATED_COMMUNITY_TESTING`. Issues discovered by users should be
reported through the public issue templates without attaching ROMs or private
saves.

## Local verification completed

On macOS arm64, the final source tree passed:

- CMake Release build;
- all four CTest suites;
- private PyInstaller runtime build;
- empty-prefix installation;
- `pkmn doctor --deep` with bundled-runtime discovery;
- config/runtime status checks;
- source Red template repair and canonical decode fixture;
- Red JSON → proposed FireRed JSON conversion with the system Python command
  removed from `PATH`;
- target JSON validation.

Cross-platform artifact production remains the responsibility of the tagged CI
matrix because Windows, Intel macOS, and Linux packages must be built on their
native runners.

## Freeze status

The version 3 command names, route IDs, profile labels, evidence labels,
Manifest 3.0 envelope, bundled-runtime layout, and package families are frozen
for version 3.0. MAQ authorized the final `v3.0.0` tag and public release; the
tagged workflow supplies the remaining native-platform artifact evidence.
