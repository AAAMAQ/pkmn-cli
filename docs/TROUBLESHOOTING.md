# Troubleshooting pkmn 3.0

Start with:

```sh
pkmn --version
pkmn doctor --deep
pkmn convert routes
```

## “pkmn is not recognized” or “command not found”

- Windows installer: reopen PowerShell after installation.
- Windows ZIP: run `.\bin\pkmn.exe` from the extracted folder.
- macOS/Linux archive: run `./bin/pkmn` or add its `bin` directory to `PATH`.
- AppImage: run `chmod +x file.AppImage` first.

## Runtime or resource not found

Do not move only the `pkmn` executable. Release layouts include `bin`,
`libexec`, and `share`; keep them together. Re-extract or reinstall the complete
package, then run `pkmn doctor --deep`.

Downloaded packages should report:

```text
Runtime mode: bundled-private-executable
```

`developer-python-fallback` is expected only for an ordinary source build.

## macOS refuses to open the package

Use only an official release. If the release notes say the candidate is
unsigned, use System Settings → Privacy & Security → Open Anyway. Do not bypass
warnings for files obtained elsewhere.

## Checksum failure

First preserve the original. If the report says only Generation I checksum
bytes are invalid, conversion can use:

```sh
pkmn red convert game.sav --auto-repair-checksum
```

The source stays unchanged. Semantic corruption, wrong sizes, and wrong game
profiles are not hidden by checksum repair.

## Existing output

`pkmn` refuses to overwrite an output family. Rename/remove the old output or
add `--auto-suffix` where supported.

## Wrong route

Red and Blue share a physical Generation I layout; FireRed and LeafGreen share
a Generation III remake layout. Choose the actual source and target explicitly:

```sh
pkmn convert red-firered game.sav
pkmn convert blue-leafgreen game.sav
```

The route declaration is recorded in the manifest.

## Experimental Japanese Red route

Use `red-jp`, not the English `red` reader, for a Japanese Red save. Declare
the source ROM revision as `--profile JP_RED_REV0` or `JP_RED_REV1`; the SRAM
layout cannot prove that revision by itself. Preserve the original save and
start with `pkmn red-jp validate game.sav --profile JP_RED_REV0`.

This route does not auto-repair Japanese checksums. A bad main checksum or
malformed/unsupported Japanese nickname blocks conversion rather than
guessing. `rjpjson inspect` and `rjpjson compare` can help locate archive or
projection problems. An optional English Red compatibility save uses English
name fallbacks and cannot be used as the Japanese-name source for a later
FireRed transfer; use the `.red.jp.json` archive and `.red.json` projection.

The Japanese route's `EXPERIMENTAL` manifest label means synthetic round-trip
tests pass, but real-save and emulator acceptance have not been recorded.
See the [Japanese bridge schema](JAPANESE_RED_JSON_SCHEMA.md).

## Evidence warning

Red → FireRed is MAQ emulator-verified. The other three routes are statically
validated and community-testing. This label is informational, not corruption.

## Exit categories

- `2`: invalid arguments;
- `3`: invalid input or JSON;
- `4`: source checksum failure;
- `5`: generation failure;
- `8`: semantic mismatch;
- `9`: output collision/publication failure;
- `10`: post-emulator validation failure;
- `11`: edit validation failure;
- `12`: unsupported game/domain.

Useful diagnostics:

```sh
pkmn doctor --deep --format json
pkmn --verbose red validate save.sav --format json
pkmn rjson validate save.red.json --profile strict --format json
pkmn proof verify save.pkmn-proof
```

## Report a bug

Open [GitHub Issues](https://github.com/AAAMAQ/pkmn-cli/issues) and include the
pkmn version, operating system, route, command, sanitized output, expected
behavior, and actual behavior. Never attach ROMs, private saves, credentials,
or private proof evidence.
