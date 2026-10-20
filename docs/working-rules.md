# Working rules (for any agent: Claude Code, Codex, ...)

The rules the user gave across sessions that are not about the platform itself. `CLAUDE.md`
(`AGENTS.md` is a link to it) holds the technical rules; this file holds the collaboration ones.
Both apply to every agent working in this repository.

## Talking and writing

- **Answer the user in French** (full accents); write every project file (code, comments, docs,
  commit messages) in English.
- Be concise: say what was done, what was measured, what is left; no recap of the question.
- Ask before writing any 68000 asm (`CLAUDE.md`), and before anything hard to undo or public.

## Roadmap and order of work

- The roadmap is `TI68K_Game_Development_Toolbox.pdf` (repository root): 1 the Portable Game
  Runtime (C, SDL and TI backends), 2 the Open Flappy Bird port (`games/flappy`), 3 TI-Basic to C
  (`games/ffa`), 4 keypad "touch" gestures, 5 Bubble Ghost (Game Boy ROM reverse engineering),
  6 Prince of Persia.
- Per game: engine first, then game design, graphics last.
- Tests without UI first (`make test`, PC headless runs, `ti-cycles`, `make xcheck`), the emulator
  last of all (`CLAUDE.md`, development flow). Titanium only; the TI-89 HW2 only for a release.
- Every game with state has an injection door (`game_scenario(n)`, `--scenario N`, `name(N)`).
- **The user validates every visual change** before it is kept or built on. This covers
  rendering, art, palette, asset pipelines, and compression or format experiments that change
  the picture. Show before/after or A/B variants side by side, at the same scale and view,
  as image files with absolute paths. Wait for the verdict before optimising, integrating
  or benchmarking further. A pixel-difference metric is no substitute. Exempt: output proven
  identical to the reference (same checksums/planes). Details: `ti89-c-dev/SKILL.md`.

## The emulator (TiEmu)

- FFT perspective: the user explicitly authorized TiEmu FPS tests after visual review.
  Compare flat and textured scenery with actors in both; separate cached replay from
  offline texture preparation. Headless TI checks and an isolated TiEmu run are authorized.
  For camera changes, report complete transition/attack durations and cold preparation,
  as well as warmed animation FPS. List native TI feature gaps explicitly: PC textured
  GIFs do not validate TI travelling textures or usable camera latency. The native audit
  and feature matrix are in `games/fft/CAMERA_AUDIT.md`.
  The user subsequently rejected animated perspective travelling and automatic attack
  shots: fixed close/wide inspection now uses direct cuts by default; motion requires
  explicit `FFT_CAMERA_MOTION` research builds. For future rendering work, native cold
  and consecutive moving frames must be measured before promising animation speed.
  GIF export cadence is never measured FPS. See `games/fft/CAMERA_CUTS.md` for the
  PC/native implementation mismatch, cache-invalidation failure and prevention rules.
  F1/F5 isometric rotations must remain animated: the no-transition decision applies
  only to close/wide perspective entry, zoom and return. Use the native low-poly turn
  fallback when moving textures are unavailable/too expensive; do not substitute a
  hold/switch. Current implementation and measurements: `games/fft/ROTATION_LOWPOLY.md`.
  An opt-in native affine-textured turn mapper now exists (`FFT_TURN_TEXTURE`), initially
  measured at 3.312-3.535 s per turn in Titanium TiEmu. Its surface/visibility follow-up
  retains 80x50 textures and three angles, measured at 2.281-2.480 s; see
  `games/fft/SURFACE_TURNS.md`. It maps runtime UVs, not cached angle
  pictures. It does not restore perspective travelling or make the PC filtered mapper
  portable. Keep low-poly as default pending the user's review; details and same-mesh
  texture-off control: `games/fft/NATIVE_TEXTURED_TURNS.md`.
  The user has now selected **80x25** as the preferred textured moving-view resolution;
  use it for future textured previews. Preserve the other variants as controls. Their
  requested low-poly 160x100 comparison is genuinely native resolution, not an enlarged
  80x50 render; see `games/fft/TURN_COMPARISON.md`. Do not infer an original-TI-89
  full-game packaging result from these bounded review executables.
  **Rotation capture correction:** the original comparison GIF presents low-poly
  steps as 2,1,3 despite correct angular math and PC/TI memory checksums. Native
  numbered-frame checks reproduce it; extra swap waits do not cure it. The local
  fixed-address plane-copy presentation control restores order. See the direction
  audit in `games/fft/TURN_COMPARISON.md`; verify actual captured steps, and do not
  silently reorder footage or reuse old timings for a changed presenter.
  The CATALOG configuration build now exposes three unchecked defaults: low-poly
  rotation, wide perspective and textured scenery. Checked values are80x25 textured
  rotation, close perspective and filled untextured mesh (the user's "filled wireframe"). CATALOG is handled before
  its legacy APPS alias; APPS retains the turn list. See `games/fft/CONFIGURATION.md`.
  Its selected forward inspection view now has an optional20,284-byte cooperative
  geometry cache prepared during menu idle time. Completed geometry survives ISO return;
  actors/masks remain live. Distinguish background preparation, prepared entry and fully
  warmed animation. Native CPU/latency evidence and limits: `games/fft/PREFETCH.md`.
  **Current storage decision (2026-10-17):** textured rotations are retired from
  the active configuration/native package, including their atlases, UV and surface
  banks. F1/F5 keep native160x100 low-poly animation. CATALOG now exposes only the
  two perspective options. Disabled attack fixture banks are also retired. Earlier
  textured-turn scripts/captures are historical controls, not shipping assets.
  Current formats, savings and measured costs: `games/fft/STORAGE_CLEANUP.md`.

- **Every TI run is a clean restart**: `ti-run prog.89z [data.89y]` (or `ti-emu restart
  file...` to send without running). Never quit a running game to HOME to resend a build,
  never type paths into the file chooser (`ti-send` is a last resort).
- **Never send keys (`ti-key`, xdotool) while the user is playing** in the emulator: once a build
  is handed over, the keyboard is theirs. Stop, restart or relaunch only when asked.
- One screenshot at most per check; prefer printed numbers or `ti-cycles` screens from memory.
- A stuck modifier or garbled typing (`fl=#bc` instead of `ffal()`): `ti-emu stop`, then
  `ti-run` again.

## Git

- Repository `github.com/sam22200/ti68k-toolbox`, committed as `coz.samuel@gmail.com`
  (`git config user.email`, set locally; the global address is a work one).
- Procedure: `.claude/skills/ti-commit/SKILL.md` (a branch per task, Conventional Commits, one
  concern per commit, explicit paths only, docs delta check, merge into `main` once the tests
  pass). Never `git add -A`/`.`/`-u`, never force-push, never commit an ignored file.
- The user's own files at the root (`*.csv`, `*.gif`, `roadmap_ti.txt`, editor swap files) are
  not ours: never stage them.

## For agents other than Claude Code

- The skills are plain Markdown: `.claude/skills/<name>/SKILL.md`, listed in `CLAUDE.md`
  § Skills and knowledge. Read the matching one before its kind of task (porting an SDL game,
  remaking a TI-Basic game, art, emulator, commit); their `reference/` files are the knowledge base.
- `/ti-commit` is a Claude Code slash command: follow `ti-commit/SKILL.md` by hand. Its
  `Co-Authored-By` trailer is the one your own environment requires, if any.
- Claude Code also kept a private memory outside the repository; everything durable from it is
  in this file and in `CLAUDE.md`.
- Game state to resume from: `games/<game>/PROGRESS.md` (FFA: `games/ffa/PROGRESS.md`, the CT
  test: `games/ffa_ct/PROGRESS.md`).
