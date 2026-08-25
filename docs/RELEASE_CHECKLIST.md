# pkmn 3.0 release checklist

## Source and privacy

- [ ] Review the complete diff and intended version.
- [ ] `scripts/privacy-scan.sh` passes.
- [ ] No ROM, private/progressed save, screenshot, semantic export, secret,
      absolute private path, or unpublished evidence is tracked.
- [ ] Approved `.bin` generation resources still match recorded SHA-256 values.
- [ ] MIT license, notices, MAQ / BiG MAQ Studios stewardship, and independent
      project statement are current.

## Build and test matrix

- [ ] Windows x86-64 builds, runs CTest, installs, and passes doctor.
- [ ] macOS arm64 builds, runs CTest, installs, and passes doctor.
- [ ] macOS x86-64 builds, runs CTest, installs, and passes doctor.
- [ ] Linux x86-64 builds, runs CTest, installs, and passes doctor.
- [ ] All four route registries are available with correct evidence labels.
- [ ] Red → FireRed regression remains accepted.

## Self-contained package gate

- [ ] Private `pkmn-runtime` directory is bundled under `libexec`.
- [ ] `pkmn doctor --deep` reports `bundled-private-executable`.
- [ ] Installed conversion works without invoking a system Python command.
- [ ] Runtime data, schemas, templates, and bridge authorities resolve relative
      to the installed executable.
- [ ] Moving only the executable fails clearly rather than silently guessing.
- [ ] Interactive launcher opens the same `pkmn interactive` engine.

## Package formats

- [ ] Windows portable ZIP passes extraction smoke test.
- [ ] Windows installer installs, adds PATH/launcher, and uninstalls cleanly.
- [ ] macOS arm64 and x86-64 archives/packages pass clean-prefix tests.
- [ ] macOS signing/notarization status is explicit in release notes.
- [ ] Linux archive, `.deb`, and AppImage pass clean-environment tests.
- [ ] Package removal does not delete unrelated shared-prefix files.

## Public artifact integrity

- [ ] Every artifact is included in `SHA256SUMS`.
- [ ] Generated SPDX release SBOM covers the published artifacts.
- [ ] License and notices are present inside each package.
- [ ] Package scan finds no ROM, private save, screenshot, secret, or private
      absolute path.
- [ ] GitHub Release notes distinguish emulator-verified and community-testing
      routes.

## Documentation

- [ ] Download/package links and `pkmn interactive` are at README top.
- [ ] Windows, macOS, Linux install/uninstall/upgrade instructions are current.
- [ ] All routes, JSON formats, checksum repair, template boundary, manifests,
      privacy, troubleshooting, and v2 compatibility are documented.
- [ ] GitHub issue templates warn users not to upload private game material.

## Publication

- [ ] Create signed `v3.0.0` tag only after explicit MAQ approval.
- [ ] Release workflow and all platform jobs pass.
- [ ] Inspect downloaded release artifacts, not only local build products.
- [ ] Update Homebrew stable URL/checksum and bottles only from immutable
      published artifacts.
- [ ] Do not call static checks emulator verification.
