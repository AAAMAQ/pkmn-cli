# Pokémon Red to FireRed: Complete Beginner Guide

This guide starts at the cartridge or emulator and ends with a playable
Pokémon FireRed save. You do not need to understand hexadecimal data, JSON,
checksums, or programming.

`pkmn` converts the save data from your legally owned Pokémon Red game. It does
not download, include, patch, or require a ROM while converting. You must supply
your own legally obtained game software when you load the resulting save.

## Before you begin

You need:

- `pkmn` 2.0 or newer;
- a normal Pokémon Red battery save, usually ending in `.sav` or `.srm`;
- your own compatible Pokémon FireRed game or ROM dump for playing the output;
- an emulator or cartridge dumper capable of importing and exporting battery
  saves;
- a safe folder in which to keep backups.

The FireRed save-container template is already included with `pkmn`. You do not
need to download a mysterious template from the internet. Creating your own
clean FireRed template is optional and explained later.

## 1. Back up everything

Before replacing or converting any save:

1. Close the emulator completely.
2. Copy the original Pokémon Red save into a backup folder.
3. If you already have a FireRed journey, back up that save too.
4. Never test by overwriting your only copy.

A battery save is not the same thing as an emulator save state. Files such as
`.sgm`, `.state`, or numbered quick-save slots are usually save states and are
not accepted. Open Pokémon Red, use the game's **SAVE** menu, then fully close
the emulator before copying the battery save.

## 2A. Export a Red save from an emulator

Emulators store battery saves differently, but the reliable procedure is:

1. Open Pokémon Red.
2. Save normally from the in-game menu.
3. Wait until the game confirms the save completed.
4. Quit the emulator completely—not merely pause it.
5. Use the emulator's **Export Battery Save**, **Export Save**, or similar
   command if it has one.
6. Otherwise, open the emulator's configured save directory and find the file
   with the same base name as the Red game.

Example:

```text
Pokemon Red.gb
Pokemon Red.sav
```

A standard Pokémon Red save used by this tool is 32 KiB (32,768 bytes). Some
emulators use `.srm`; copy it first, then give the copy a `.sav` extension if
the CLI or your later workflow expects that name. Renaming does not change its
contents.

## 2B. Dump a Red save from a cartridge

Use a compatible cartridge reader/writer and its official software:

1. Insert your Pokémon Red cartridge.
2. Choose the tool's **Backup Save**, **Read Save**, or **Dump SRAM** action.
3. Do not choose **Dump ROM** when your goal is the save.
4. Save the exported battery data as something recognizable, such as
   `my-red-original.sav`.
5. Make a second backup before continuing.

Exact buttons vary by dumper. Follow its manufacturer instructions, and never
disconnect the cartridge while it is reading or writing.

## 3. Confirm that `pkmn` is ready

In Terminal:

```sh
pkmn --version
pkmn doctor --deep
```

The version should be 2.0 or newer and the doctor should finish successfully.
If your terminal still reports 0.1.0, update or reinstall the program before
converting.

## 4. Validate and inspect the Red save

Change the example path to your actual file. Quotation marks are important when
a folder or filename contains spaces.

```sh
pkmn red validate "/path/to/my-red-original.sav"
pkmn red summary "/path/to/my-red-original.sav"
```

Continue only when validation passes and the summary looks like your journey.
If validation fails, return to the emulator or cartridge dumper and export a
normal in-game battery save again.

## 5. Convert Red directly to FireRed

The simplest command is:

```sh
pkmn red convert "/path/to/my-red-original.sav"
```

If the input is `my-red-original.sav`, the default output is:

```text
my-red-original_fr.sav
```

The same folder also receives an audit manifest and a readable conversion
report. They explain what was transferred, translated, defaulted, omitted, or
warned about. The original Red save is not overwritten.

Choose a different output filename when needed:

```sh
pkmn red convert "/path/to/my-red-original.sav" "/path/to/FireRed-converted.sav"
```

If the destination already exists, `pkmn` refuses to overwrite it. This is a
safety feature.

## 6. Optional JSON workflows

Most users can skip this section. JSON is useful for research, auditing, or
future schema upgrades.

Decode Red:

```sh
pkmn red decode my-red-original.sav
```

Convert canonical Red JSON directly to a FireRed save:

```sh
pkmn rjson convert my-red-original.red.json
```

Create only a planned FireRed JSON and no physical save:

```sh
pkmn rjson convert_to_frjson my-red-original.red.json
```

Generate a FireRed save from that planned `.fred.json`:

```sh
pkmn frjson generate my-red-original.fred.json
```

## 7. Load the converted save in FireRed

First close the FireRed emulator. Back up any existing FireRed save. Then use
one of these methods:

- choose **Import Battery Save** or **Import Save** in the emulator; or
- place the converted save in the emulator's save directory.

Many emulators require the game and save to have matching base names:

```text
Pokemon FireRed.gba
Pokemon FireRed.sav
```

If necessary, copy or rename `my-red-original_fr.sav` to match your FireRed
game filename. Do not rename or modify the ROM through `pkmn`; the CLI does not
need or touch it.

Start FireRed and check:

1. player and rival names;
2. party and PC Pokémon;
3. badges, money, items, and Pokédex;
4. important story locations and NPCs;
5. Fly destinations appropriate to places visited in Red.

Then save normally in FireRed, close the emulator completely, reopen it, and
confirm the save reloads. This final save/close/reload cycle proves the emulator
accepted the generated save rather than only holding it in memory.

## 8. Restore the converted save to a cartridge (optional)

Only use hardware that explicitly supports the target FireRed cartridge and
its save-memory type:

1. Back up the cartridge's current FireRed save.
2. Confirm the converted file is exactly 128 KiB (131,072 bytes).
3. Use the dumper's **Restore Save**, **Write Save**, or equivalent action.
4. Do not interrupt power or remove the cartridge while writing.
5. Start the cartridge, inspect the result, save in game, restart, and verify
   it reloads.

Writing the wrong file or using incompatible hardware can destroy the existing
cartridge save, which is why the backup is mandatory.

## Use your own clean FireRed template (optional)

The bundled template is recommended. A custom template changes only the clean
FireRed container baseline; it does not make conversion more complete.

To create a supported custom template:

1. Use an English Pokémon FireRed USA/Europe v1.0-compatible game.
2. Start a completely new game.
3. Remain upstairs in the player's Pallet Town bedroom.
4. Do not choose a starter, obtain the Pokédex, battle, collect anything, or
   leave the clean opening state.
5. Save **once** with the in-game SAVE command.
6. Close the emulator completely.
7. Export the 128 KiB battery save.
8. Keep the original private and work from a copy.

Validate and inspect it:

```sh
pkmn fred validate my-clean-firered.sav
pkmn fred summary my-clean-firered.sav
```

Select it for one conversion:

```sh
pkmn red convert my-red-original.sav --template my-clean-firered.sav
```

Or select it for the current terminal session:

```sh
export PKMN_FIRERED_TEMPLATE="/full/path/to/my-clean-firered.sav"
pkmn red convert my-red-original.sav
```

The selection order is:

1. `--template` supplied to this command;
2. `PKMN_FIRERED_TEMPLATE` environment variable;
3. the bundled standard template.

Custom templates undergo stricter checks than ordinary FireRed validation.
They must have one checksum-valid first save, an erased inactive slot, erased
special/Hall of Fame sectors, the approved bedroom and heal location, no party
or PC Pokémon, no Pokédex or badges, and the exact clean pre-starter event and
variable baseline. Trainer name, IDs, play time, options, encryption keys, and
physical sector rotation may differ. A progressed or personal journey is
rejected instead of being silently inherited.

## Troubleshooting

### “Input is not a standard Pokémon Red save”

You may have selected a save state, ROM, compressed archive, or FireRed save.
Export the normal Pokémon Red battery save. It should normally be 32 KiB.

### “Red checksum validation failed”

Open the game, save normally, close the emulator, and export again. Work from a
copy; do not repair the only original without a backup.

### “Custom FireRed template is not a clean supported dump”

The save contains progress, Pokémon, Pokédex data, a second save slot, special
sector data, or a different opening scene. Make a brand-new game and save once
upstairs before choosing a starter, or omit `--template` to use the bundled
standard.

### FireRed starts a new game instead of showing the conversion

Confirm that the converted file is 128 KiB, is in the emulator's correct save
directory, and has the same base name the emulator expects for the FireRed
game. Import it as a battery save, not as a save state.

### The emulator recreates the old save

Close the emulator before replacing files. Cloud synchronization, emulator
autosave, or an open process may overwrite your copy when quitting.

### I already have an output with that name

Choose another explicit output or use `--auto-suffix` where supported. `pkmn`
does not overwrite conversion output families by default.

## Privacy and support

Save files can contain trainer names, IDs, Pokémon nicknames, play history, and
other personal choices. Do not upload them publicly unless you intend to share
that information. Bug reports should prefer summaries, hashes, sanitized JSON,
or the smallest reproducible fixture.

For every command and advanced option, see the
[complete command reference](ALL_COMMANDS.md). For installation help, see the
[Homebrew and installation guide](HOMEBREW_INSTALL.md).
