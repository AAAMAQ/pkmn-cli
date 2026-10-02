# Homebrew installation

The preferred non-developer installation is a self-contained package from
[GitHub Releases](https://github.com/AAAMAQ/pkmn-cli/releases). It needs no
separate Python installation.

The stable tap formula builds version 3.1.0 from its versioned GitHub archive
and verifies its real SHA-256. This is a source build, not a prebuilt bottle.
Homebrew installs the build tools and Python runtime automatically.

Install and start guided mode:

```sh
brew tap AAAMAQ/pkmn
brew install AAAMAQ/pkmn/pkmn-cli
pkmn --version
pkmn doctor --deep
pkmn interactive
```

The version should be `pkmn 3.1.0` or newer. Downloadable release packages
contain a private bundled runtime and need no separate Python installation.

Update an existing stable installation:

```sh
brew update
brew upgrade AAAMAQ/pkmn/pkmn-cli
pkmn --version
```

For a previous development `--HEAD` installation, run
`brew uninstall AAAMAQ/pkmn/pkmn-cli` followed by
`brew install AAAMAQ/pkmn/pkmn-cli` to switch to stable. This removes only the
installed CLI package, not your save files. `brew reinstall` preserves `--HEAD`.
If the executable is
not linked, run `brew link pkmn-cli`. Inspect any reported conflicts before
replacing files; do not use blanket overwrite flags.

Maintainer publication procedure:

1. create the immutable release tag (sign it if a signing key is configured);
2. let the release workflow build/test all platform packages;
3. download the immutable GitHub source archive and calculate its SHA-256;
4. update the formula’s `url`, `sha256`, and `version`;
5. optionally publish arm64 and x86-64 bottles with their real checksums;
6. run `brew audit --strict --new-formula`, `brew style`, install, test, and
   `pkmn doctor --deep` before merging the formula.

Never calculate a checksum from a local archive while the formula points at a
different GitHub-generated archive.
