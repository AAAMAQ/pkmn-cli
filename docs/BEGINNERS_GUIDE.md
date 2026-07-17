# Beginner's Guide to pkmn

This guide assumes you have never built or used a command-line program. `pkmn`
works with Pokemon Red `.sav` files. It never needs a game ROM, never changes
your input save in place, and refuses to overwrite an existing output by
default. FireRed is not supported yet.

## 1. Install on a Mac with Homebrew

Open **Terminal** (Applications > Utilities > Terminal), then paste these lines
one at a time:

```sh
brew tap AAAMAQ/pkmn
brew install --HEAD AAAMAQ/pkmn/pkmn-cli
pkmn --version
pkmn doctor --deep
```

If `brew` is not installed, first follow the instructions at
[brew.sh](https://brew.sh/). The current `pkmn` package is a public pre-release
installed from the latest tested source branch.

To get a later version and its updated command catalog:

```sh
brew update
brew upgrade --fetch-HEAD AAAMAQ/pkmn/pkmn-cli
pkmn get-all-cmds
```

Windows and Linux users currently build from source; follow
[INSTALL.md](INSTALL.md). Prebuilt release downloads are not published yet.

## 2. Put a save in an easy folder

Make a folder such as `Pokemon Saves` in your home folder. Copy—not move—your
save into it and keep a separate untouched backup. In Terminal, change into the
folder (quotes are important because the name contains a space):

```sh
cd "$HOME/Pokemon Saves"
```

In the examples below, replace `backup.sav` with your real filename. You can
drag a file from Finder into Terminal to insert its full path.

## 3. Check a save first

```sh
pkmn red validate backup.sav
pkmn red inspect backup.sav
```

Validation checks the expected 32 KiB size and every known Red save checksum.
If validation fails, preserve the original. To create a repaired copy without
changing it:

```sh
pkmn red repair-checksums backup.sav
```

## 4. Read a friendly summary

```sh
pkmn red summary backup.sav
```

The summary includes trainer and rival, money, coins, badges, playtime,
location, Pokedex counts, party, inventory, PC storage, Daycare, Hall of Fame,
and supported story/event totals.

Save a report for a person to read:

```sh
pkmn red summary backup.sav --format markdown --output backup-summary.md
```

Save structured statistics for another program:

```sh
pkmn red summary backup.sav --format json --output backup-summary.json
```

## 5. Explain progress between two backups

Put the older save first and newer save second:

```sh
pkmn compare progress before-brock.sav after-brock.sav
```

This report explains elapsed playtime, location, money and coins gained or
spent, badges gained, Pokedex changes, party changes, inventory changes,
storage, Hall of Fame, verified events, trainer battles, static encounters, and
story flags. It checks trainer name and ID to guard against comparing unrelated
playthroughs.

Save both human-readable and machine-readable reports:

```sh
pkmn compare progress before-brock.sav after-brock.sav \
  --output-markdown progress.md --output-json progress.json
```

For technical comparisons, use `compare physical` on two `.sav` files or
decode them and use `compare semantic` on two `.red.json` files.

## 6. Decode a save to canonical JSON

```sh
pkmn red decode backup.sav
```

This creates `backup.red.json` beside the save. By default it includes an
archival `physicalImage`, so treat the JSON as private save data too. For a
smaller semantic-only document:

```sh
pkmn red decode backup.sav --no-physical-image
```

If an output already exists, `pkmn` stops safely. Add `--auto-suffix` only when
you want a new numbered name such as `_2`.

## 7. Generate, reconstruct, and edit safely

- `pkmn rjson generate backup.red.json generated.sav` creates a deterministic
  save from supported semantic fields and never treats `physicalImage` as the
  source of gameplay data.
- `pkmn rjson reconstruct backup.red.json` restores the exact archived byte
  image and requires `physicalImage`. Reconstruction is not generation.
- `pkmn red edit backup.sav` opens the guided interactive editor and writes a
  validated copy.

For repeatable scripted edits:

```sh
pkmn red begin-edit backup.sav
pkmn red edit-session backup.edit-session.json --money 5000
pkmn red pending-edits backup.edit-session.json
pkmn red validate-edit backup.edit-session.json
pkmn red end-edit backup.edit-session.json
```

Read [EDIT_MODE.md](EDIT_MODE.md) before advanced inventory, party, box, or
event editing.

## 8. See every command in your installed version

```sh
pkmn --help
pkmn get-all-cmds
pkmn get-all-cmds --format markdown --output my-pkmn-commands.md
```

`get-all-cmds` does not read a stale downloaded file. It renders the catalog
compiled into the installed executable, so a Homebrew update automatically
updates the list. The repository copy is [ALL_COMMANDS.md](ALL_COMMANDS.md).

## 9. Understand outputs and privacy

Files ending in `.sav`, `.red.json`, `.edit-session.json`, and proof packages
can contain private gameplay data. Do not upload them publicly unless you have
reviewed them. Generated Markdown/JSON reports can also include trainer names,
hashes, or save details. ROMs are never needed and must not be added to this
project.

For proof and emulator workflows, read [PROOF_WORKFLOW.md](PROOF_WORKFLOW.md).
For common errors, read [TROUBLESHOOTING.md](TROUBLESHOOTING.md).
