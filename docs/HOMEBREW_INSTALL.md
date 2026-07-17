# Homebrew installation and testing

The checked-in formula is intentionally head-only until a public release
supplies an immutable source-archive URL and SHA-256. It contains no placeholder
or fabricated release data and can be used to test the public `main` branch.

After `AAAMAQ/pkmn-cli` is public, test directly from this checkout:

```sh
brew style packaging/homebrew/pkmn-cli.rb
brew install --HEAD --build-from-source ./packaging/homebrew/pkmn-cli.rb
brew test pkmn-cli
pkmn --version
pkmn doctor --deep
```

The formula builds with CMake, runs CTest, installs the man page and Red
template, and generates bash, zsh, and fish completions.

For tap testing, create `AAAMAQ/homebrew-pkmn`, copy the formula to
`Formula/pkmn-cli.rb`, and run:

```sh
brew tap AAAMAQ/pkmn
brew install --HEAD AAAMAQ/pkmn/pkmn-cli
brew test AAAMAQ/pkmn/pkmn-cli
```

After creating a signed stable tag, replace `head` with (or add alongside it)
the exact GitHub release source URL, its downloaded SHA-256, and the stable
version. Then run `brew audit --strict --new-formula`, `brew style`, install,
test, and `doctor --deep` before publishing the stable formula. Never calculate
the formula checksum from a locally produced archive while pointing at a
different GitHub-generated archive.
