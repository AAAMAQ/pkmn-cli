# Pokémon Red / Blue Conversion-Relevant Difference Ledger

Authority: `pret/pokered@d70d99ffbd329473d96eaaf19fd97c86d2220b7f`

This ledger is complete for the fields consumed by `pkmn 3.0` conversion. It
is not a catalogue of every visual, encounter-table, or ROM difference.

| Domain | Shared or versioned | Conversion treatment |
|---|---|---|
| SRAM layout and checksums | Shared paired engine | One validated Gen I binary engine |
| Trainer identity, money, badges, play time, options | Shared encoding | Decode the actual bytes; attach explicit source profile |
| Party, boxes, Daycare and Pokédex bits | Shared encoding | Convert actual saved Pokémon/state |
| Kanto event bits, trainer-defeat state and map progression | Shared encoding for bridge-consumed state | Apply the evidence-backed semantic Kanto bridge |
| Version-exclusive wild availability | Versioned ROM data, not proof of capture | Never infer; owned Pokémon come from the save |
| Version-specific parties/rewards or encounter availability | Versioned where selected by source build | Preserve actual saved outcome; do not manufacture an unavailable encounter history |
| Preset/default names | Version-selected ROM behavior | Saved player/rival text is authoritative |
| Physical-version detection | Ambiguous from the common save container | Require `red`/`blue` command or route declaration |

The command declaration is recorded as `GEN1_RED` or `GEN1_BLUE`. A JSON file
that already declares the opposite profile is rejected.
