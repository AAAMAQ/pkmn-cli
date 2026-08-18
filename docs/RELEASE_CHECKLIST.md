# Release Checklist

## Source and privacy

- [ ] Working tree contains only intended release files.
- [ ] Save Genie and Save Generator reference repositories remain unmodified.
- [ ] No saves, ROMs, screenshots, videos, PDFs, semantic exports, edit sessions, proof evidence, credentials, or private absolute paths are tracked except the two explicitly approved named `.bin` generation resources.
- [ ] `scripts/privacy-scan.sh` passes from the release working tree.
- [ ] Bundled resources match `THIRD_PARTY_NOTICES.md` and their recorded SHA-256.
- [ ] Version, changelog, license, MAQ / BiG MAQ Studios stewardship note, and independent-project notice are current.

## Clean build

- [ ] Build from a clean source archive on macOS, Linux, and Windows.
- [ ] Run CMake configure and Release build.
- [ ] Run CTest with failure output.
- [ ] Install into an empty temporary prefix.
- [ ] Run installed `pkmn --help`, `pkmn --version`, and `pkmn doctor` outside the source tree.
- [ ] Run installed `pkmn doctor --deep` and verify deterministic self-test output.
- [ ] Run installed `pkmn frjson schema --format json` outside the source tree.
- [ ] Exercise Red JSON to proposed FireRed JSON planning without a private template.
- [ ] Exercise physical conversion with the installed bundled FireRed template and verify a progressed custom template is rejected.
- [ ] Exercise decode, validation, generation, reconstruction, comparison, proof, and edit workflows only with synthetic/public fixtures.

## Packaging

- [ ] Create and sign the release tag.
- [ ] Publish the source archive and calculate its SHA-256.
- [ ] Convert the tested head-only formula to an immutable release URL and its downloaded SHA-256.
- [ ] Run `brew audit --strict`, `brew style`, install, and formula test.
- [ ] Confirm the installed generation resource is found relative to the installed executable.
- [ ] Confirm the installed FireRed Python runtime is found relative to the executable.
- [ ] Generate CPack artifacts, inspect their contents, and include the SPDX SBOM.
- [ ] Verify each proof directory and ZIP with `pkmn proof verify` before publication.
- [ ] Build/test universal macOS output on real arm64 and x86_64 toolchains when publishing that artifact.
- [ ] Sign release artifacts only with maintainer-controlled signing identities.

## Claims and verification gates

- [x] Record Phase 5 native `.fred.json` generation acceptance without committing private evidence.
- [x] Native `.fred.json` generation passed deterministic, physical-image-isolation, authority, checksum, and MAQ emulator review.
- [x] Record Phase 6 Red-to-FireRed acceptance without committing private evidence.
- [x] Red-to-FireRed conversion passed deterministic, bridge, manifest, checksum, proof-package, and MAQ emulator review.
- [x] Publish only MAQ's explicitly approved clean pre-starter FireRed `.bin` baseline; do not bundle any progressed/private template, ROM, screenshot, or emulator artifact.
- [ ] Do not describe automated proof as emulator proof.
- [ ] Keep the semantic-generation versus archival-reconstruction boundary explicit.
- [ ] Record any emulator-tested public sample separately without committing private evidence.

## Publication

- [ ] Review the public repository from a fresh clone.
- [ ] Confirm CI passes on supported operating systems.
- [ ] Publish release notes from `CHANGELOG.md`.
- [ ] Push only after explicit maintainer approval.
