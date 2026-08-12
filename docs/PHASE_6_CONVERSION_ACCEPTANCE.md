# Phase 6 Red to FireRed Conversion Acceptance

## Result

**PASS — accepted by MAQ on 2026-08-12.**

Phase 6 tested the complete supported conversion path:

```text
Pokémon Red .sav
  → canonical .red.json
  → semantic bridge and PCCS ORIGINAL Pokémon conversion
  → proposed .fred.json
  → FireRed generation
  → converted FireRed .sav
```

The accepted candidate passed:

- source Pokémon Red identity and checksum validation;
- deterministic Red → FireRed planning and generation;
- player/rival identity, currency, badges, options, Pokémon, inventory, Fly,
  trainer, event, story, and FireRed-only-default policies;
- source-backed Charmander starter and Squirtle rival-starter translation;
- FireRed sector-layout and checksum validation;
- conversion-manifest and proof-package verification;
- emulator boot and MAQ's detailed visible equivalence review.

MAQ reported that the tested supported state was overwhelmingly true and
identical in FireRed and accepted the converter as a PASS. Deliberate policy
translations, safe defaults, warnings, omissions, and Generation I → III
mechanical differences remain auditable in each conversion manifest.

The private Red save, FireRed save, ROM, source JSON, screenshots, and proof
evidence are not included in this repository. This document records only the
non-private acceptance conclusion.

## Consequence

The Pokémon Red → FireRed converter is accepted for release in `pkmn` 2.0.
Future schema changes, bug fixes, and additional game support will be maintained
in this unified CLI repository.
