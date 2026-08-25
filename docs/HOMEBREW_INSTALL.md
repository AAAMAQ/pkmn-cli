# Homebrew installation

The preferred non-developer installation is a self-contained package from
[GitHub Releases](https://github.com/AAAMAQ/pkmn-cli/releases). It needs no
separate Python installation.

The checked-in Homebrew formula remains a source-build formula until an
immutable stable 3.0 source archive, SHA-256, and bottles exist. It never uses a
fabricated checksum.

Test the current source formula with:

```sh
brew style packaging/homebrew/pkmn-cli.rb
brew install --HEAD --build-from-source ./packaging/homebrew/pkmn-cli.rb
brew test pkmn-cli
pkmn doctor --deep
```

The HEAD formula may use Homebrew Python as a developer runtime dependency.
This does not apply to the downloadable Phase 3 packages, which contain a
private bundled runtime.

For a tap:

```sh
brew tap AAAMAQ/pkmn
brew install --HEAD AAAMAQ/pkmn/pkmn-cli
```

Stable publication procedure:

1. create the signed `v3.0.0` tag;
2. let the release workflow build/test all platform packages;
3. download the immutable GitHub source archive and calculate its SHA-256;
4. update the formula’s `url`, `sha256`, and `version`;
5. publish arm64 and x86-64 bottles with their real checksums;
6. run `brew audit --strict --new-formula`, `brew style`, install, test, and
   `pkmn doctor --deep` before merging the formula.

Never calculate a checksum from a local archive while the formula points at a
different GitHub-generated archive.
