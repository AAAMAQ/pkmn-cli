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

Enable completion in the current shell or install it through the shell's normal completion directory:

```sh
source <(pkmn completion zsh)  # zsh
source <(pkmn completion bash) # bash
pkmn completion fish | source  # fish
```

The manual page is installed as `pkmn(1)`.

Maintainers can run `scripts/verify-release.sh` for a committed-source archive,
fresh temporary build, test, install, and installed-binary smoke test. It also
runs `scripts/privacy-scan.sh` before preparing public artifacts.

CPack creates `.tar.gz` packages on supported platforms and `.deb` packages on
Linux. Windows CI verifies `pkmn.exe`; the macOS universal helper is under
`packaging/macos`. The installed packaging documentation includes the SPDX SBOM.

The install step places the executable in `bin` and the identity-checked public Red template in `share/pkmn/resources`. Moving only the executable without its installed resource directory will disable semantic generation and editing.

## Homebrew readiness

The draft formula is in `packaging/homebrew/pkmn-cli.rb`. See
`docs/HOMEBREW_INSTALL.md`; placeholders remain deliberate until publication.

## Uninstall

When installed to a private prefix, remove that prefix's `bin/pkmn`, `share/pkmn`, and installed `share/doc/pkmn-cli` files. Inspect the prefix first if it is shared with other software.
