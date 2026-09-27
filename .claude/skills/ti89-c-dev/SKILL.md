---
name: ti89-c-dev
description: Write, compile, optimise or port C code (GCC4TI/TIGCC) for the TI-89 / TI-89 Titanium in NOSTUB mode with ExtGraph — performance on a very slow 68000 (measured costs, fixed point, no floats, heuristics), program skeleton, build command, interrupts, keyboard (_rowread), grayscale, timing, fixed-point maths, porting old TIGCC sources (.tpr files, ExtGraph 1.x, HW3 compatibility), OS patches (HW3Patch, AMSpatch, tiosmod). Use it for any new TI-89 program or game, any build, any question about how TI-68k C code works, or to get sources from ticalc.org running.
---

# C development for the TI-89

## Project rules

- **NOSTUB only** (no kernel): the `.89z` runs as `name()` from HOME.
- **ASM** (68000): only for very heavy computation, rarely, and **always ask first**. Exception:
  frequent operations with a verified asm version (table at the top of `ti68k-asm.md`) are always called in
  that asm version from C, never rewritten in C.
- Graphics, sprites, grayscale: **ExtGraph 2** (`tools/extgraph/lib/extgraph.h`,
  docs in `tools/extgraph/DOCS/extgraph.html`).
- **New games: the Portable Game Runtime** (`runtime/README.md`): the game is portable C against
  `runtime/core/rt.h` (hooks `game_init/scenario/update/render`), unit-tested and played on the PC
  (SDL) first, built for the TI with the same Makefile. Engine → game design → graphics last;
  an injection door (`game_scenario(n)`) per game; TI checks as printed numbers (`make bench`).

## Knowledge base — read before writing non-trivial code

- `reference/ti68k-c-patterns.md`: screen and LCD layout, grayscale planes, **interrupts**
  (auto-int 1/5, `DUMMY_HANDLER`, chaining, timer speed), **keyboard** (`_rowread` matrix, AMS vs
  direct reading), frame loop, fixed-point maths, collisions, memory and stack, portability
  checklist.
- `reference/ti68k-performance.md`: **the machine is very slow (68000, ~12 MHz, no FPU/cache)**.
  Measured costs (float ≈ 300× an integer op, 32-bit mul/div = library calls, ROM calls vs direct
  access, ExtGraph 10× faster than memcpy), **types** (unsigned, smallest storage type — with the
  generated code as proof), **lookup tables** (`tools/bin/ti-table`: sin/atan/recip/row/sqrt,
  rotation and atan2 code), arithmetic and loop rules, graphics strategy, frame
  budget, heuristics, compiler traps (-O2 can be slower), how to benchmark, fastest pixel/text/
  scroll methods (measured). **Read it before
  designing any game loop, renderer or algorithm.**
- `reference/ti68k-game-techniques.md`: algorithms that fit the machine, best version first,
  from the old sources: game-tree search (TI-Chess), cheap AIs (Tetris), grid puzzles, area
  capture, low-RAM A*, 3D pipeline, portal rendering (X3D), raycasting (FAT), Mode 7, roguelikes,
  link-cable play (ownership, timeouts), game AIs and physics (Fischer games, SlimeBall), data
  encoding, 68000 decompressors, and §12: ideas from the demoscene, NES/Mega Drive and research
  papers (compiled sprites, XOR fill, metatiles, flow fields, JPS, coroutines, bytecode VMs…),
  and §13: those ideas measured on the TI-89 (cycles, sizes, verdicts; C decompressors, MCTS, arenas).
- `reference/ti68k-asm.md`: 68000 assembly (only after asking the user): verified C-callable `.s`
  recipe with explicit register bindings, register convention, cycle table, what GCC already does,
  idioms and hazards of the old kernel-era ASM sources.
- `sources/README.md`: the downloaded corpus of old sources (TICT tutorials, TI-Chess,
  TICT-Explorer, small games) and what each is worth reading for.
- `docs/resources.md` § Graphics assets: sprite, tileset and map sites (Spriters Resource,
  Sprite Database, VideoGameSprites, VGMaps) and how to adapt them to 160×100 in 4 greys.
- `reference/puzzle-bobble-analysis.md`: a complete game (`games/puzzle_bobble/`) annotated —
  setup/teardown order, clock interrupt, frame loop, keys, physics, flood fills, time-driven
  animations, and its bugs.

## Build

```sh
tools/bin/ti-cc -o name src.c [more.c]     # → name.89z; ExtGraph always available
```
`ti-cc` = -Os + dead-code removal + `-mregparm=5` + compressed relocs (patterns §1; `TI_CC_PLAIN=1` to
disable). ExtGraph's TileMap engine needs `tools/extgraph/lib/tilemap.a` on the command line
(performance §7; pre-shifted sprites `preshift.h` are in `extgraph.a`). The
variable name on the calculator is fixed at build time by `-o`: renaming the .89z does not change it.
`name` = variable name on the calculator (≤ 8 characters, lowercase). Then test with the
`ti89-emulator` skill (`ti-run name.89z`) on the Titanium (`TI_CALC=89t`, default); add
`TI_CALC=89` / `89u` (TI-89 HW2) only for a release. Runtime games: unit tests on the PC first.

Skeleton: see `hello/hello.c` (`#define USE_TI89`, `OPTIMIZE_ROM_CALLS`, `SAVE_SCREEN`,
`#include <tigcclib.h>`, `void _main(void)`). Library docs: `tools/gcc4ti-bin/doc/html/`, one page
per function (e.g. `kbd__rowread.html`, `intr_SetIntVec.html`, `gray_GrayOn.html`). Examples:
`tools/gcc4ti-bin/examples/`. Throwaway hardware tests go in `experiments/` (see
`experiments/hwinfo/`).

## Porting old sources (ticalc.org, 2000–2006)

1. Read the `.tpr`: `[Included Files]` lists the .c/.s/.a/.h files, `GCC Switches` the options.
   Translate it into `ti-cc -o <Project Name> <C/S files>` (ExtGraph is already provided: ignore a
   missing `extgraph.a`/`extgraph.h`).
2. Do not trust bundled `.89z` binaries: binaries from before ~2005 **often crash on the Titanium
   (HW3)**. Always recompile.
3. **ExtGraph 1.x → 2.x**: the values of `enum GrayColors` changed. Look for hard-coded colours
   (`grep -n "Gray.*2B(.*,[0-3],"`) and replace them with `COLOR_WHITE/LIGHTGRAY/DARKGRAY/BLACK`.
   Typical symptom: phantom collisions (a line ends up in the wrong plane). Real case:
   `games/puzzle_bobble` (the bubble stopped on the limit line; fixed at line 897).
4. Other traps of old code (patterns §11): **empty delay loops are deleted by -Os** (the game runs
   with no delay), ROM-base hardware detection answers "HW1" on the Titanium, hard-coded int-5
   start values, ghost-space launchers, TSRs.
5. Old warnings (`implicit declaration`, unused variables): do not "clean up" the original code
   beyond what is needed.

## OS patches: when are they needed?

A NOSTUB program built with a recent GCC4TI runs unpatched on the Titanium (AMS 3.10) and on the
TI-89 HW2. Patches are only for specific needs:

| Patch | Effect | When |
|---|---|---|
| HW3Patch 1.03 (Kevin Kofler), `tools/patches/hw3patch/hw3patch.89z` | disables RAM execution protection (HW2/HW3/HW4, AMS 2.00–3.10); permanent until the OS is reinstalled | TSRs, DLLs (FAT-Engine, Doom89…), kernels (PreOS), GCC nested-function trampolines |
| h220xTSR | same goal, RAM-resident, does not modify AMS (HW2) | non-permanent alternative |
| AMSpatch (L. Debroux), `tools/rom/TI89_AMS209_amspatch.89u` | TI-89 OS 2.09 pre-patched: no RAM/Flash execution protection, no ASM size limit, unsigned apps, +64 KB archive, English locked | the "asmpatch" the user remembered; emulator profile `TI_CALC=89` |
| tiosmod (https://github.com/debrouxl/tiosmod) | generic OS patcher (89/89T/92+/V200, AMS 2.05–3.10): the same unlocks plus AMS bug fixes (OSVRegisterTimer, contrast, HeapDeref, 0^0…); outputs xdelta/bsdiff/IPS diffs | to produce a patched Titanium OS; not used here yet |

Program size (**verified**, patterns §1): AMS 2.xx refuses programs above **24,576 bytes**
("ASAP or Exec string too long"); the Titanium (AMS 3.10) has no such limit, only 64 KB per variable.
Stay under 24 KB for the TI-89 by moving data into archived data files read in place, or build with
`-pack NAME` (compressed program + 1 KB launcher, verified on both models up to 60 KB).

Resources (TICT tutorials, game sources, sites): `docs/resources.md`.
