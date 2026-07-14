# Installation

## Build and install from source

Requirements are CMake 3.20 or newer and a C++20 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$HOME/.local"
```

Ensure the selected binary directory is on `PATH`:

```sh
export PATH="$HOME/.local/bin:$PATH"
pkmn --help
pkmn --version
pkmn doctor
```

The install step places the executable in `bin` and the identity-checked public Red template in `share/pkmn/resources`. Moving only the executable without its installed resource directory will disable semantic generation and editing.

## Homebrew readiness

The draft formula is in `packaging/homebrew/pkmn-cli.rb`. At release time, replace its repository and SHA-256 placeholders with the public tagged source archive, then run the checks in `docs/RELEASE_CHECKLIST.md`. The draft intentionally does not pretend that an unpublished release URL or checksum exists.

## Uninstall

When installed to a private prefix, remove that prefix's `bin/pkmn`, `share/pkmn`, and installed `share/doc/pkmn-cli` files. Inspect the prefix first if it is shared with other software.
