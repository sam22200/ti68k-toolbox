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

## The emulator (TiEmu)

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

## Copyrighted art

- Sheets ripped from commercial games (The Spriters Resource) are references, never assets in
  git: they live in `sources/art/<game>/` (ignored). The picks per category:
  `.claude/skills/ti-art-refs/SKILL.md`.
- **`games/ffa_ct/` is local only**: the FFA copy restyled with Chrono Trigger sprites (an art
  test). It is excluded by `.git/info/exclude`, never committed, never published; its state is
  in `games/ffa_ct/PROGRESS.md`. Its gameplay fixes (no CT art) may be ported to `games/ffa`.

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
