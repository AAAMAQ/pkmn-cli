# Automation and batch workflows

Global controls precede the command:

```sh
pkmn --quiet red validate save.sav
pkmn --verbose --no-color doctor --deep
```

`PKMN_QUIET=1`, `PKMN_VERBOSE=1`, and `NO_COLOR` provide environment controls.

Batch commands publish output directories transactionally:

```sh
pkmn red validate-batch a.sav b.sav --format json
pkmn red decode-batch a.sav b.sav --output-dir decoded --no-physical-image
pkmn rjson generate-batch a.red.json b.red.json --output-dir generated
pkmn compare semantic-batch a.red.json b.red.json c.red.json --format json
```

Pipeline examples:

```sh
pkmn red decode save.sav --no-physical-image --output - \
  | pkmn rjson validate - --profile strict --format json
pkmn rjson generate - - < save.red.json > generated.sav
```

Binary stdout contains only save bytes; JSON stdout contains only JSON.
