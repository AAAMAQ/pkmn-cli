# Pokémon FireRed / LeafGreen Conversion-Relevant Difference Ledger

Authority: `pret/pokefirered@df4449a27cd78dd747ce269e47d3ab4a0149d8f4`

This ledger covers target differences that affect generation and conversion.
It does not claim every paired-ROM difference is a transferable save state.

| Domain | Shared or versioned | Generator treatment |
|---|---|---|
| Flash size, save slots, sectors, signatures and checksums | Shared in `src/save.c` | One Kanto-remake container engine |
| Core save blocks and Pokémon encryption | Shared encoding | One reader/generator with target profile |
| Main Kanto story flags, variables and object-state bundles used by the bridge | Shared semantic engine at the pinned source | Reuse the verified prerequisite bundles |
| Species/move/item IDs used by converted Kanto data | Shared for supported records | Translate semantically; never copy Gen I raw IDs |
| Pokémon origin game | Versioned saved metadata | FireRed code `4`; LeafGreen code `5` |
| Version-exclusive wild encounters and gifts | Version-selected game content | Do not infer completion from Gen I; preserve converted owned Pokémon only |
| Remake-only and postgame state | No direct Gen I equivalent | Locked/defaulted unless a documented semantic rule derives it |
| Physical target evidence | Route-specific | Red→FireRed is emulator verified; other routes remain static/community testing |

The bundled clean save is used as a structural, privacy-reviewed container
baseline. It is not semantic authority: the planned JSON and target profile
control generated meaning and origin metadata.
