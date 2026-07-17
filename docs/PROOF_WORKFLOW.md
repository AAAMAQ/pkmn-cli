# Proof Workflow

`pkmn proof red <source.sav>` creates a collision-safe proof directory without modifying the source. The internal workflow validates and decodes the source, performs physical-image-isolated semantic generation twice, re-decodes the generated save, compares physical and semantic state, and records determinism, isolation, integrity, and the outstanding emulator gate.

Use `--output-dir <directory>` to select the proof directory. Use `--zip` for the adjacent default archive or `--zip-output <archive.zip>` for an explicit path. ZIP files are deterministic ZIP32 archives containing logical artifact names only. They contain save data and require publication review; they are not safe to publish merely because they contain no ROM.

Use `--auto-suffix` to select a collision-safe numbered proof directory/archive. The default remains strict refusal.

The package includes:

- original and generated canonical JSON;
- generated semantic save;
- combined, physical, semantic, summary, semantic-JSON, and archival-JSON reports in JSON and Markdown;
- generation report;
- proof manifest;
- manual emulator checklist.

Automated proof intentionally stops at `required-manual-gate`. After loading, interacting, saving, shutting down, and reloading in an emulator, preserve the post-emulator battery save and run:

```sh
pkmn proof post-emulator \
  --before generated.sav \
  --after generated-post-emulator.sav \
  --proof-dir source.pkmn-proof
```

The continuation validates both saves, records their hashes, classifies expected gameplay/runtime drift, rejects unexpected identity changes, writes post-emulator JSON/Markdown reports, and updates the existing manifest. Without `--proof-dir`, use `--output-dir` to create an independent validation package.

`pkmn red validate-post-emulator <before.sav> <after.sav>` provides the same independent validation in positional Red-command form.

Neither command runs an emulator, distributes a ROM, or modifies either save. Proof directories and ZIP files are private ignored evidence by default.
