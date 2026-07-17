# Future FireRed Plan

The command router reserves `fred`, `frjson`, and `convert`, but no FireRed support is claimed in the Red-first release.

FireRed implementation begins only after separate parsing, checksum/slot selection, semantic schema, generation, reconstruction, determinism, physical-image-isolation, and emulator workflows have verified engines and public synthetic fixtures. Those engines will be adapted into internal modules under the same standalone executable rather than invoked as sidecar programs.

Planned namespaces are:

```text
pkmn fred decode|inspect|validate|edit
pkmn frjson inspect|validate|generate|reconstruct
pkmn compare physical|semantic
pkmn proof fred
pkmn convert red-to-firered
```

Conversion additionally requires explicit mapped, unmapped, canonicalized, unsupported, warning, and FireRed-validation reports. Until these gates pass, every reserved command returns an unsupported-operation exit code and states that FireRed is not implemented.
