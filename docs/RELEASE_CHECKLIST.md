# Release Checklist

## Source and privacy

- [ ] Working tree contains only intended release files.
- [ ] Save Genie and Save Generator reference repositories remain unmodified.
- [ ] No saves, ROMs, screenshots, videos, PDFs, semantic exports, edit sessions, proof evidence, credentials, or private absolute paths are tracked.
- [ ] Bundled resources match `THIRD_PARTY_NOTICES.md` and their recorded SHA-256.
- [ ] Version, changelog, license, MAQ / BiG MAQ Studios stewardship note, and independent-project notice are current.

## Clean build

- [ ] Build from a clean source archive on macOS and Linux.
- [ ] Run CMake configure and Release build.
- [ ] Run CTest with failure output.
- [ ] Install into an empty temporary prefix.
- [ ] Run installed `pkmn --help`, `pkmn --version`, and `pkmn doctor` outside the source tree.
- [ ] Exercise decode, validation, generation, reconstruction, comparison, proof, and edit workflows only with synthetic/public fixtures.

## Packaging

- [ ] Create and sign the release tag.
- [ ] Publish the source archive and calculate its SHA-256.
- [ ] Replace the repository and SHA-256 placeholders in the Homebrew formula.
- [ ] Run `brew audit --strict`, `brew style`, install, and formula test.
- [ ] Confirm the installed generation resource is found relative to the installed executable.

## Claims

- [ ] Do not claim FireRed or conversion support.
- [ ] Do not describe automated proof as emulator proof.
- [ ] Keep the semantic-generation versus archival-reconstruction boundary explicit.
- [ ] Record any emulator-tested public sample separately without committing private evidence.

## Publication

- [ ] Review the public repository from a fresh clone.
- [ ] Confirm CI passes on supported operating systems.
- [ ] Publish release notes from `CHANGELOG.md`.
- [ ] Push only after explicit maintainer approval.
