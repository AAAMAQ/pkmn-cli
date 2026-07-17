# Homebrew installation

The checked-in formula is a draft until a signed public release supplies a
real repository URL and source-archive SHA-256. Never publish the placeholder
formula.

After release, replace its placeholders, then verify:

```sh
brew style packaging/homebrew/pkmn-cli.rb
brew audit --strict --new-formula packaging/homebrew/pkmn-cli.rb
brew install --build-from-source packaging/homebrew/pkmn-cli.rb
pkmn --version
pkmn doctor --deep
```

The formula builds with CMake, runs CTest, installs the man page and Red
template, and generates bash, zsh, and fish completions.
