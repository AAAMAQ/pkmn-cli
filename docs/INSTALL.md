# Install pkmn 3.1

For step-by-step file preparation, interactive conversion, editing, comparison,
and all other menu options, read the [player's interactive manual](INTERACTIVE_USER_MANUAL.md).
Use release 3.1.0 or newer for the expanded menu and experimental Japanese Green
support. Older 3.0.0 installations show a different menu.

This guide starts from zero. Release users do not need Python, CMake, Git, a
compiler, or any earlier pkmn project.

## Before downloading

You need:

- a computer running 64-bit Windows, macOS, or Linux;
- your own legally obtained `.sav` file;
- enough free space for the program, output save, manifest, and report.

You do not need a ROM to run `pkmn`. ROMs are never distributed by this
project. Back up your save before doing any conversion.

## Windows x86-64

### Installer

1. Open the [pkmn releases page](https://github.com/AAAMAQ/pkmn-cli/releases).
2. Download the Windows x86-64 installer from the newest 3.0 release.
3. Run the installer and keep “add pkmn to PATH” selected.
4. Open `pkmn interactive` from the Start Menu, or open PowerShell and run:

```powershell
pkmn --version
pkmn doctor --deep
pkmn interactive
```

### Portable ZIP

1. Download the Windows x86-64 ZIP.
2. Extract the **whole** folder. Do not move only `pkmn.exe`; its private
   runtime and resources must remain beside the installed layout.
3. Open the extracted folder and run `pkmn-interactive.cmd`, or use PowerShell:

```powershell
.\bin\pkmn.exe doctor --deep
.\bin\pkmn.exe interactive
```

To uninstall the portable edition, delete the extracted folder. To uninstall
the installer edition, use Windows Settings → Apps → Installed apps → pkmn.

## macOS

Choose `macos-arm64` for Apple Silicon (M1/M2/M3/M4 and newer) or
`macos-x86_64` for an Intel Mac.

1. Download the matching package or archive from GitHub Releases.
2. Install the package, or extract the entire archive to a permanent folder.
3. Open Terminal and run:

```sh
pkmn --version
pkmn doctor --deep
pkmn interactive
```

The archive also includes `pkmn-interactive.command`. If macOS blocks an
unsigned package, confirm that the file came from the official
repository, then use System Settings → Privacy & Security → Open Anyway. Final
packages are signed and notarized only when BiG MAQ Studios’ signing
infrastructure is available; the release notes state the exact status.

To uninstall a package, remove the installed `pkmn` files listed by its receipt.
For an extracted archive, delete the whole extracted directory.

## Linux x86-64

### AppImage

```sh
chmod +x pkmn-cli-*.AppImage
./pkmn-cli-*.AppImage doctor --deep
./pkmn-cli-*.AppImage interactive
```

Move it to a permanent folder if desired. Delete the AppImage to uninstall.

### Debian/Ubuntu `.deb`

```sh
sudo apt install ./pkmn-cli-*.deb
pkmn doctor --deep
pkmn interactive
```

Uninstall with:

```sh
sudo apt remove pkmn-cli
```

### Portable archive

Extract the whole archive and run `bin/pkmn`. Keep `bin`, `libexec`, and
`share` together. Delete the extracted directory to uninstall.

## Your first conversion

Guided mode is easiest:

```sh
pkmn interactive
```

It will ask:

1. what you want to do;
2. which source game made the save;
3. where your source `.sav` is;
4. whether the target is FireRed or LeafGreen for English Red/Blue, or which
   supported revision made a Japanese save (Japanese routes target FireRed);
5. whether to create the automatically named new save.

Simple conversion enables in-memory checksum repair for English saves and
leaves the original unchanged. Choose menu 2 or 8 for JSON conversion plans,
custom output paths, templates, and other advanced choices.

The other main-menu tasks cover inspection, JSON, editing, comparison, and
proof. Choose **Browse every current command and option** to search all 124
current endpoints. Type `?` for help, `B` to go back, or `Q` to quit.
See the [interactive guide](INTERACTIVE_V3_GUIDE.md) for the full menu and
[interactive examples](INTERACTIVE_COMMAND_EXAMPLES.md) for every command.

Paths containing spaces may be pasted with or without surrounding quotes:

```text
"C:\Users\MAQ\Desktop\Pokemon Red.sav"
"/path/to/Pokemon Red.sav"
```

Direct commands are also available:

```sh
pkmn convert red-firered game.sav
pkmn convert red-leafgreen game.sav
pkmn convert blue-firered game.sav
pkmn convert blue-leafgreen game.sav
```

Do not replace your original save immediately. Load the generated target copy
in your emulator or restoration workflow, inspect it, save normally in game,
close completely, and reload.

## Verify downloads

Each tagged release includes `SHA256SUMS` and an SPDX SBOM. Compare a file’s
hash before installation:

```sh
# macOS
shasum -a 256 downloaded-package

# Linux
sha256sum downloaded-package

# Windows PowerShell
Get-FileHash downloaded-package -Algorithm SHA256
```

The result must equal the matching line in `SHA256SUMS`.

## Homebrew

Stable bottles are enabled after immutable 3.0 release artifacts and checksums
exist. Until then, the source formula is for contributors:

```sh
brew install --HEAD --build-from-source ./packaging/homebrew/pkmn-cli.rb
```

See [Homebrew installation](HOMEBREW_INSTALL.md).

## Build from source

Contributors need CMake 3.20+, a C++20 compiler, and Python 3:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$HOME/.local"
```

This developer installation uses `developer-python-fallback` unless a private
runtime bundle is supplied with `PKMN_BUNDLED_RUNTIME_EXECUTABLE`. That is
expected for source development; downloaded packages use
`bundled-private-executable`.

## Getting help

Run `pkmn doctor --deep`, then consult [troubleshooting](TROUBLESHOOTING.md).
Report reproducible bugs at [GitHub Issues](https://github.com/AAAMAQ/pkmn-cli/issues).
Do not upload ROMs, personal saves, screenshots containing private data, or
credentials.
