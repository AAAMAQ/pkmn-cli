# Contributing

Thank you for helping improve `pkmn-cli`. The current supported product scope
is the standalone Pokemon Red workflow engine. FireRed and Red-to-FireRed work
must remain explicitly unsupported until separate research and emulator
acceptance exist.

## Development checks

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
scripts/privacy-scan.sh
```

Run `scripts/verify-release.sh` before proposing release or packaging changes.
Homebrew changes must also pass `brew style packaging/homebrew/pkmn-cli.rb`.

## Safety rules

- Never commit saves, ROMs, screenshots, emulator output, proof packages, or
  private absolute paths.
- Never overwrite an input save or silently overwrite an existing output.
- Never use `physicalImage` as semantic-generation authority.
- Keep reconstruction clearly archival and generation clearly semantic.
- Use only synthetic or explicitly public fixtures in tests.
- Do not claim emulator acceptance from automated tests.

Keep changes focused, document user-facing commands, and add deterministic
tests for new behavior.
