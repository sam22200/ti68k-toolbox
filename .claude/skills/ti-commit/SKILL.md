---
name: ti-commit
description: "Commit and push this TI-89 project's changes to its GitHub repository (sam22200/ti68k-toolbox) with Conventional / Semantic Commit Messages, split into small review-friendly commits (one per concern), updating the project's docs (README.md files, CLAUDE.md, SKILL.md files, the knowledge base) first when the change makes them stale. Use whenever the user asks to commit, \"commite\", \"fais un commit\", push, or wants the work saved to GitHub."
---

# ti-commit (Conventional Commits, docs kept in sync, push)

Commits follow the Semantic Commit Messages convention
(https://gist.github.com/joshbuchea/6f47e86d2510bce28f8e7f42ae84c716). Default to **several small
commits, one per concern**; a single big commit is the fallback, not the target. Then push.

## Message format

```
<type>(<optional scope>): <subject>

<optional body: what & why, one bullet per logical change>

Co-Authored-By: <model trailer required by the environment, if any>
```

Example: `feat(runtime): add TileMap engine to the TI backend`

- **subject**: imperative, present tense ("add", not "added"), no trailing period, ≤ ~70 chars,
  in English (project files are in English).
- **scope** (optional): the top-level area: `runtime`, `campfire`, `lib`, `experiments`, `tools`,
  `skills`, `kb` (knowledge base), a game name.
- **body**: bullets, not prose, one line each; nest a sub-bullet for the detail (file, flag,
  measured value); backtick identifiers; skip the body when the subject says it all. Measured
  results (cycles, sizes, checksums) and "verified on the Titanium" belong in the body.

| type | when |
|------|------|
| `feat` | a new feature, game, runtime capability |
| `fix` | a bug fix |
| `perf` | faster or smaller with the same behaviour (measured) |
| `docs` | documentation / knowledge base only |
| `style` | formatting only |
| `refactor` | restructuring, no behaviour change |
| `test` | tests, cross-checks, benchmarks only |
| `chore` | build, tooling, `.gitignore`, deps |

## Workflow

1. **Repository check.** `git status`, `git remote -v`. The remote is
   `https://github.com/sam22200/ti68k-toolbox` (branch `main`). If the repo or remote is missing,
   stop and tell the user rather than creating one silently.
   Identity: this repo commits as the personal address, set locally
   (`git config user.email` must print `coz.samuel@gmail.com`, not the global work address); if it
   does not, run `git config user.email coz.samuel@gmail.com` before committing.

2. **Files to commit: only this session's work.**
   - Stage by **explicit path** (`git add <path> ...`); never `git add -A`, `git add .` or `-u`.
   - Leave the user's own unrelated modifications alone (do not stage, do not unstage).
   - **Never commit what `.gitignore` keeps out**, even with `-f`: TI OS images and TiEmu
     profiles (`tools/rom`, `tools/tiemu`), third-party tools and sources (`tools/gcc4ti*`,
     `tools/extgraph`, `sources/`, `ffa_en/`), `runtime/platform-sw/amsfont.h` (TI font data),
     build outputs (`*.89z`, `*_pc`, `*_test`). Check `git status --short` before each commit.
   - If unsure whether a file is yours, list the candidates and ask.

3. **Docs delta check** (skip silently for trivial changes: typos, formatting, test-only).
   Look at the diff and ask whether it makes any of these stale, then fix the **minimal delta**
   (only the sections the diff invalidates, same structure and tone, no rewrites):
   - `README.md` (root: layout table, quick start, "not in the repository") and the `README.md`
     of every touched directory (`runtime/README.md`, `games/<game>/README.md`);
   - `CLAUDE.md` (workflow, hard-won rules, layout);
   - `SKILL.md` files under `.claude/skills/` (`ti89-c-dev`, `ti89-emulator`, this one);
   - the knowledge base `.claude/skills/ti89-c-dev/reference/*.md`: a new **verified** platform fact
     (bug, cost, pitfall) goes to `ti68k-c-patterns.md` or `ti68k-performance.md`, marked
     **verified** only if it ran in the emulator.
   Typical triggers: new directory or game, new/changed `make` target or tool option, new runtime
   API, changed build command or size, a measured cost, a platform bug found.
   Doc edits are their own `docs:` commit, last, unless a one-liner belongs with its change.
   Never write project facts into the user-global `~/.claude/CLAUDE.md`.

4. **Split into commits** (below), in dependency order. Each commit must build on its own.

5. **Messages from context.** Use what was done and measured in the conversation. If your
   confidence in the type or the subject is below 80 %, ask the user.

6. **Commit group by group**: stage that group's explicit paths, commit, next group. Append the
   `Co-Authored-By:` trailer the environment mandates, if any.

7. **Push**: `git push origin main` (first push: `git push -u origin main`). Never force-push.
   If the push is rejected, stop and report; do not rebase or overwrite without the user.

8. **Report** each commit (short hash, subject, files) and the push result with the repo URL.

## Splitting into commits

**Size target: ~300 changed lines per commit** (`git diff --numstat -- <paths>`; for new files
their line count). A review-ergonomics target, not a limit.

Group by concern, in this order when present:
1. tooling / build — `tools/bin/*`, `rt.mk`, Makefiles, `.gitignore` (`chore`)
2. runtime core and backends — `runtime/core`, `runtime/platform-*` (`feat`, `fix`, `perf`)
3. shared code — `lib/` (`feat`, `perf`)
4. games — one game per commit (`feat(<game>)`, `fix(<game>)`)
5. experiments and benchmarks (`test`, `perf`)
6. tests (`test`) — with the code they cover when that stays near the target
7. docs, skills, knowledge base (`docs`)

Rules:
- A file belongs entirely to one commit: no `git add -p`, no partial hunks.
- Every commit stands on its own (builds; references nothing that lands later). If a split would
  break that, keep the files together even over the target.
- Don't split for the metric's sake: one generated file, one new game or one mechanical rename
  stays in one commit. A single commit with a bulleted body is a normal outcome; say so.
- A first import of existing work may be several commits by area (runtime, games, lib, tools,
  experiments, docs), each a coherent snapshot.
