# Reconstruction Mode

Reconstruction is archival byte restoration, not semantic generation.

```sh
pkmn rjson reconstruct savefile.red.json
pkmn rjson reconstruct savefile.red.json --output restored.sav
```

The input must validate and contain a valid `physicalImage`. `pkmn` decodes the standard SRAM and trailing bytes, writes a new collision-safe save, and verifies its SHA-256 against source metadata. The default name is `savefile_reconstructed_from-physical-image.sav`.

Use `--auto-suffix` to select `_2`, `_3`, and later names when the preferred output exists. Existing data is never overwritten.

Use `pkmn rjson generate` when the goal is an independently generated gameplay-semantic save. Generation ignores `physicalImage`; reconstruction requires it. The two modes intentionally cannot substitute for one another.
