# Troubleshooting

## Exit categories

- `2`: invalid arguments.
- `3`: invalid input or JSON.
- `4`: Red checksum failure.
- `5`: semantic generation failure.
- `8`: unexpected semantic mismatch.
- `9`: output collision/publication failure.
- `10`: post-emulator validation failure.
- `11`: edit validation failure.
- `12`: unsupported game/domain.

Useful diagnostics:

```sh
pkmn doctor --deep
pkmn --verbose red validate save.sav --format json
pkmn rjson validate save.red.json --profile strict --format json
pkmn proof verify save.pkmn-proof
pkmn red events search articuno
```

Outputs are transactional and never overwritten by default. Remove or rename a
stale output, or use `--auto-suffix` where advertised. FireRed and conversion
remain intentionally unavailable until separately verified.
