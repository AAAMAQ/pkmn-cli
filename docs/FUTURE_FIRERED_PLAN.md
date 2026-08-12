# FireRed and Conversion Status in pkmn 2.0

FireRed is no longer a reserved placeholder. Version 2.0 contains:

```text
pkmn fred summary|inspect|validate|decode|edit
pkmn fred repair-checksums|validate-batch|decode-batch|validate-post-emulator
pkmn frjson inspect|validate|schema|update_schema|migrate|generate|generate-batch|reconstruct
pkmn red convert
pkmn rjson convert|convert_to_frjson|update_schema
pkmn convert red-to-firered|inspect|explain|validate-manifest|batch
pkmn proof fred|red-to-firered
```

The native FireRed reader and schema 0.4.0 exporter are compiled C++ modules.
The deterministic conversion planner and generator are installed as a bundled
Python 3 runtime with pinned authority data; they do not call the standalone
Save Genie or Generator applications.

## Completed master verification gates

Implementation required final proof:

- Phase 5 proved that a complete native `.fred.json` independently generates a
  gameplay-equivalent FireRed save without source physical bytes.
- Phase 6 proved the supported Red domains convert to their intended FireRed
  meanings or documented policy outcomes.

Both gates passed automated proof, audit, checksum validation, and MAQ emulator
review. Individual output reports still identify per-save warnings and policy
decisions rather than hiding them.

The automated portions of both gates remain available for regression testing.
Native schema 0.4.0 generation
uses its complete logical-block and special-sector authorities and explicitly
ignores `physicalImage`; `proof fred` checks determinism and that isolation.
MAQ's exhaustive in-game acceptance checks are complete.

## Template boundary

Physical FireRed generation currently requires either:

```text
--template /path/to/clean-fire-red.sav
PKMN_FIRERED_TEMPLATE=/path/to/clean-fire-red.sav
```

No ROM is required. Phase 5 proved the template approach sufficient. A public
template may be bundled only if a separate publication, provenance, licensing,
and privacy audit approves it.
