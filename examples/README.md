# Public Workflow Examples

These examples use logical filenames only. Supply a synthetic or legally shareable Pokemon Red save; never substitute private material in committed files.

```sh
pkmn red validate public-synthetic.sav
pkmn red decode public-synthetic.sav --no-physical-image
pkmn rjson inspect public-synthetic.red.json
pkmn rjson generate public-synthetic.red.json public-synthetic_generated_semantic.sav
pkmn compare physical public-synthetic.sav public-synthetic_generated_semantic.sav
pkmn proof red public-synthetic.sav --output-dir public-synthetic.pkmn-proof
```

Scriptable copy-first edit:

```sh
pkmn red begin-edit public-synthetic.sav
pkmn red edit-session public-synthetic.edit-session.json --trainer-name RED --money 5000
pkmn red validate-edit public-synthetic.edit-session.json
pkmn red end-edit public-synthetic.edit-session.json
```

The resulting saves, JSON files, sessions, reports, and proof folders are ignored artifacts and must not be committed.
