# `pkmn interactive` — design and maintenance plan

For the full checked-out command-by-command interaction map and the separate
future-command treatment, see [`INTERACTIVE_V3_GUIDE.md`](INTERACTIVE_V3_GUIDE.md).

## Purpose

`pkmn interactive` is the conversational front door to the `pkmn` command-line
program. The version 3.0 implementation offers guided paths for the 124
current catalog endpoints; `interactive` itself opens the session. The direct
CLI remains available for scripting and experienced users. This document
records the design and continuing acceptance criteria; the user-facing guide
is [INTERACTIVE_V3_GUIDE.md](INTERACTIVE_V3_GUIDE.md).

The leading advertised task is whole-playthrough conversion:

1. Convert an international Pokémon Red or Blue save to FireRed or LeafGreen.
2. Convert a supported Japanese Pokémon Red save to a supported remake target.
3. Explore the rest of the save-management tools: summarize, validate, decode,
   generate, edit, compare, prove, and inspect command options.

The main menu begins with Red/Blue conversion, then offers experimental
Japanese Red work, save and JSON tools, editing, comparison, proof, a command
browser, and general tools. Proposed commands in
[FUTURE_COMMANDS.md](FUTURE_COMMANDS.md) remain unavailable.

## Design principles

- **Conversion comes first.** The first menu choice must clearly state that
  `pkmn` converts Pokémon Red/Blue playthrough saves to FireRed/LeafGreen.
- **Every catalog endpoint is discoverable.** Each supported command endpoint
  must be reachable through a guided workflow, either from a task menu or from
  the command browser. Its relevant options must also be selectable there.
- **One source of truth.** The interactive browser uses the compiled command
  catalog and typed workflow metadata. It must not maintain a separate,
  drifting hand-written list of command names.
- **Conversational, not shell emulation.** Prompts explain the next input,
  accept one complete answer per line, validate it, and offer a useful next
  step. Interactive mode does not build shell strings or invoke a shell.
- **Same engine, same result.** A selection dispatches to the same in-process
  command handlers as the corresponding direct command. Identical choices
  should produce identical outputs and reports.
- **Honest capability status.** Stable, community-test, experimental, planned,
  and unavailable capabilities must be visibly distinguished. Planned or
  placeholder commands must not appear executable.
- **Safety by default.** Never modify an input save in place or silently
  overwrite outputs. Explain repairs, generation, reconstruction, and other
  consequential actions before execution.
- **No private-path leakage.** Do not persist entered paths in telemetry,
  reports, or shared configuration. Only include a path in an output report
  when the relevant direct command already documents that behavior.

## Main menu structure

```text
pkmn 3.1 interactive - guided mode
1 Convert Pokemon Red/Blue to FireRed/LeafGreen
2 Convert supported Japanese Red
3 Inspect, validate, summarize, or repair a save
4 Decode, generate, reconstruct, or migrate save data
5 Edit a save safely
6 Compare saves or playthrough progress
7 Run proof or emulator checks
8 Browse every current command and option
9 Doctor, settings, and beginner help
0 Exit
? Help  B Back  Q Quit
Choose a task:
```

Option 2 should only be enabled for source/target routes actually implemented
and supported by the current build. If Japanese-to-LeafGreen is not yet
supported, say so in the screen and offer the available Japanese-to-FireRed
route; do not imply that the missing route works.

## Main task flows

### 1. Red/Blue to FireRed/LeafGreen conversion

The wizard asks for source game, target game, save path, optional conversion
settings, and output choice. It identifies route evidence before the user
commits. For a damaged-but-repairable source, it explains that checksum repair
is performed on an in-memory copy and asks before continuing. It then previews
source, route, target, output/report names, checksum treatment, and evidence
status before running.

### 2. Japanese Red conversion

Ask for the Japanese Red revision/profile when required, then the source path,
supported remake target, template/options if relevant, and output path. Explain
how Japanese names are preserved or projected, which fields are mapped, and
which information is not representable. Display the route's evidence status
and ask for confirmation before writing. Do not guess the save revision.

### 3. Inspect, validate, summarize, and auto-fix checksums

Let users select a game/profile, then choose a focused action: readable
summary, structural inspection, checksum/schema validation, batch validation,
or automatic checksum repair. “Auto-fix” means `pkmn` checks the save, repairs
only recognized checksum fields when the payload/layout is eligible, validates
the repaired result, and writes a **separate copy**. It never silently changes
the selected source file. The wizard previews the proposed copy path and asks
before writing. If validation finds payload corruption, an unknown layout,
unsupported revision, or another condition that checksum recalculation cannot
safely fix, stop and explain the problem instead of rewriting the file. Batch
mode supports selecting multiple files and reports each file's outcome
independently.

The direct command vocabulary already includes game-specific
`repair-checksums` endpoints (for example, `pkmn red repair-checksums
backup.sav`). The interactive flow should make this action easy to discover
and may label it “Auto-fix checksums,” while showing the exact underlying
command in the review. Do not add a vague `--auto-fix` flag to validation
unless its scope is precisely defined for each supported format; conversion's
existing `--auto-repair-checksum` means in-memory repair for that conversion
only, not writing a repaired source copy.

### 4. JSON and save-file workflows

Guide users through decode, inspect, validate, schema description/migration,
semantic generation, archival reconstruction, batch workflows, and supported
cross-game JSON conversion. Explain the distinction between semantic
generation and physical-image reconstruction before asking which action they
want. Never treat an archival physical image as semantic generation input.

### 5. Editing

Offer the supported game's editor/session workflow: start a session, stage
edits, inspect pending changes/history, undo or annotate, validate, and publish
a new save. Before publishing, summarize edits and validation results. Keep
unsupported fields disabled with an explanation rather than exposing unsafe
raw mutation.

### 6. Comparison

Ask what the user wants to learn:

- **Progress comparison:** explain meaningful gameplay changes between two
  backups from the same playthrough (the user-facing default).
- **Physical comparison:** byte-level differences, ranges, and hashes.
- **Semantic comparison:** compare decoded JSON fields, excluding archival
  physical-image data where appropriate.
- **Game-specific comparison:** expose supported FireRed comparison domains.
- **Bridge comparison:** audit the Red source against a remake target and
  optional conversion manifest.

The wizard must label assumptions and distinguish expected changes from
unexpected differences. It must not assert that two files are from the same
playthrough unless that can be established; otherwise ask the user to confirm.

### 7. Proof and emulator checks

Guide users through supported proof creation, post-emulator continuation, and
proof-package verification. Explain which files will be created and that
emulator acceptance is a manual step; never mark it passed automatically.
Allow a destination directory/ZIP choice only where the underlying command
supports it.

### 8. Browse every command and option

Provide a searchable, paginated browser over the **available** compiled
catalog. Users can filter by game, task, or command text, open a command to
read its description and full usage/options, then select “Walk me through it.”
For a command with a typed interactive workflow, collect/validate its
arguments and run it. Include advanced flags in the guided flow where they
apply. The browser must also show explicit status for commands unavailable in
this build.

Every catalog endpoint must have a coverage record:

| Coverage state | Meaning |
|---|---|
| Guided | Inputs/options are collected with friendly prompts and validation. |
| Guided advanced | Common path is wizard-driven; specialist flags are under Advanced options. |
| Read-only launch | Browser explains and displays exact syntax, but does not claim to run interactively. |
| Unavailable/planned | Clearly disabled with reason; never dispatched. |

The completion target for supported commands is **Guided** or **Guided
advanced**. “Read-only launch” is a temporary migration state, not a way to
claim complete conversational coverage. Shell completion generation is a
special case: explain and generate the selected shell script, then show how to
install it rather than trying to execute shell setup on the user's behalf.

### 9. Doctor, settings, and learning

Expose internal readiness/doctor checks, compiled safety defaults, beginner
installation and usage help, route evidence, and links to the command guide.
The command browser is the canonical way to find exact syntax; avoid duplicating
static command inventories in menu code.

## File and path entry

Save paths commonly contain spaces, quotes, Unicode characters, or shell-style
escaping. A path prompt reads the entire line and must not split on spaces.

### Required behavior

- Accept `backup.sav`, `/path/to/my backup.sav`, `"/path/to/my backup.sav"`,
  and `'...path...'` as one path.
- Trim surrounding whitespace and remove one matching pair of outer quotes.
- Expand `~` at the beginning of a path, where supported by the platform.
- Handle a pasted terminal path with escaped spaces such as
  `/path/to/my\\ backup.sav` without treating it as two arguments.
- Support paste/drag-and-drop paths as a whole line; document platform-specific
  quoting if the terminal inserts it.
- Resolve relative paths from the current working directory and show the
  normalized path back to the user before opening it.
- If the entered file is not found, offer: retry, browse the current folder,
  browse another folder, or go back. Show close filename suggestions only as
  suggestions—never silently choose a “similar” save.
- A folder browser can filter by expected extensions (`.sav`, `.json`, etc.)
  while still allowing “show all files” for unusual valid inputs.
- For output paths, show the parent folder, detect output-family collisions
  (save plus reports/manifests), and let the user choose another name or an
  explicit supported auto-suffix option.
- Keep file picking portable and dependency-light; use a terminal browser by
  default. Do not assume a graphical file-picker exists.

Example missing-file recovery:

```text
Save file: /path/to/Pokemon Backups/backup.sav
I couldn't find that file.
 1  Try entering the path again
 2  Browse /path/to/Pokemon Backups
 3  Choose another folder
 4  Go back
Choice:
```

## Conversational interaction model

- Every prompt accepts `?` for context-sensitive help, `B`/`back` to return,
  and `Q`/`quit` to leave safely. At the top level, Back returns to the menu.
- Use numbered choices for known alternatives and accept their readable names
  as aliases when unambiguous.
- Validate input immediately and state what valid values look like.
- Preserve completed answers when stepping back unless the user changes them.
- Before a write or potentially long workflow, show a concise review and ask
  for confirmation. `N` returns to editing the choices; `B` returns to the
  prior screen.
- “Auto-fix” is not permission to overwrite. Checksum repair creates a
  validated copy, while conversion-only in-memory repair remains a separate,
  explicitly explained choice.
- After completion, summarize the result, list output files, and offer relevant
  next actions (inspect, validate, compare, or open the output folder).
- Support EOF/Ctrl-D and Ctrl-C as clean cancellation; never mistake an empty
  line for approval.
- Optional `--no-color`, `--quiet`, and `--verbose` behavior remains
  consistent with the non-interactive CLI.
- Avoid requiring arrow keys, terminal resizing, or a full-screen UI library;
  numbered menus must work in basic terminals and pasted-input tests.

## Implementation architecture

1. **Interactive options/workflow directory.** Add a focused directory such as
   `src/commands/interactive/options/` for menu definitions, typed prompt
   descriptors, path input/browsing, and per-domain workflow registration.
   Keep business logic in existing command/engine modules.
2. **Command metadata.** Extend or pair the command catalog with structured
   metadata: availability/evidence state, argument types, option types/defaults,
   interaction coverage, and handler identity. Do not parse the display `usage`
   string to discover required inputs.
3. **Workflow registry.** Register the first-screen tasks and command-browser
   guides against the compiled catalog/capability registry. Hide or disable
   unavailable commands based on actual build capabilities.
4. **Typed prompts.** Reusable prompt types handle paths, game/profile choices,
   booleans, enums, output destinations, repeated files, numbers, and optional
   advanced settings.
5. **In-process dispatch.** Route validated selections through the same
   internal C++ handler/router entry points used by direct commands. Do not
   spawn `pkmn`, Save Genie, Save Generator, or shell subprocesses.
6. **Central safety review.** Use shared preview/collision/confirmation
   behavior for every action that writes files or repairs data.
7. **Generated documentation.** Generate/help-reference the command browser
   from the same descriptors as `get-all-cmds`; keep the beginner guide aligned.

## Original delivery phases

These phases record the implementation plan. The active guided registry covers
the current catalog; the details below remain useful as maintenance checks
when commands and options change.

### Phase 1 — Main menu and conversion-first experience

- Replace the two-choice menu with the proposed task groups.
- Make Red/Blue-to-FireRed/LeafGreen the first option and Japanese Red the
  second only for supported routes.
- Improve path line parsing, path normalization, not-found recovery, previews,
  and global Back/Help/Quit behavior.
- Preserve current conversion safety and evidence messaging.

### Phase 2 — High-frequency save workflows

- Add summarize, inspect, validate, repair-copy, decode, JSON validate,
  generate, and reconstruct wizards for available game profiles.
- Add an explicit “Auto-fix save checksums” wizard that detects the save
  profile, applies only that profile's supported checksum repair, validates
  the result, and writes a collision-safe copy after confirmation.
- Add supported batch flows with clear multi-file selection and transactional
  output previews.

### Phase 3 — Editing, comparison, and proof

- Add guided edit sessions, progress/physical/semantic comparisons, proof
  generation, post-emulator checks, and proof verification.
- Give each action domain-specific previews, report summaries, and safety
  validation.

### Phase 4 — Complete command browser and option coverage

- Make every currently supported catalog endpoint searchable and selectable.
- Add typed guided argument/options metadata for every endpoint.
- Add advanced option pages, batch/multi-path collection, and exact mapping
  tests proving no supported catalog command is omitted.
- Clearly label unsupported/planned catalog records and keep them
  non-executable until implementation is available.

### Phase 5 — Polish and release acceptance

- Beginner testing on clean macOS, Windows, and Linux terminals.
- Check interactive behavior with spaces, quotes, Unicode, relative paths,
  pasted paths, missing paths, output collisions, cancellation, and EOF.
- Verify route outputs match direct command invocation for the same inputs.
- Update `README.md`, beginner/complete guides, command reference, release
  notes, and packaged launchers so `pkmn interactive` enters this experience.

## Testing and completion criteria

- Snapshot tests cover the main menu and every submenu, including status labels.
- Navigation tests cover every prompt's Help, Back, Quit, invalid input,
  cancellation, and EOF behavior.
- Path tests cover unquoted/spaced, quoted, single-quoted, tilde, escaped-space,
  Unicode, relative, nonexistent, ambiguous, and pasted paths.
- Catalog coverage test fails if a supported command lacks an interactive
  workflow/typed descriptor or explicit documented exception.
- Dispatch tests prove the interactive path calls the same handlers and returns
  consistent exit status and output artifacts as direct invocation.
- Safety tests prove no source overwrite, no silent output overwrite, explicit
  confirmation before repair/write, collision handling, and clean cancellation.
- Integration tests use synthetic/public fixtures only; no private saves or
  ROMs are added to the repository.
- Cross-platform CI runs the interactive transcript tests without requiring a
  real terminal or user input.

Keep the interactive implementation aligned with these criteria as the direct
CLI changes. A newly added command is incomplete until its typed guided flow,
safe review behavior, documentation, and relevant tests are added too.
