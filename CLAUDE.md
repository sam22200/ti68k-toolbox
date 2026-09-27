# CLAUDE.md — Games and programs for the TI-89 / TI-89 Titanium

We write programs for the TI-89 (Motorola 68000), mostly **in C with GCC4TI**, in **NOSTUB mode**
(run directly as `name()` from HOME, no kernel). Graphics: ExtGraph 2.
**ASM: very rarely, only for heavy computation, and always ask me first.** Exception: common
operations that already have a verified asm version (decompressors in `lib/`: table at the top of `ti68k-asm.md`): C code
calls that version (linked `.s`, or an `asm()` block for a few instructions), not a C rewrite.

**Two hard constraints drive every choice: 1. performance (the strongest), 2. visibility.**

**1. Performance. The machine is extremely slow** (68000 at ~12 MHz, no FPU, no cache, 16-bit bus: ~400k cycles
per frame at 30 fps). Every design choice must be performance-driven, using everything already
measured in this project, not re-derived: the verified asm routines (`lib/`, `ti68k-asm.md`), ExtGraph
and its TileMap engine / pre-shifted sprites, the runtime primitive costs (performance §7), the
benchmarks and verdicts of `experiments/` (game-techniques §13):
- **Types**: `unsigned` whenever a value cannot be negative (signed `x % 8` compiles to a full
  division, unsigned to one AND; signed `/ 4` needs a fix-up, unsigned is one shift); the
  **smallest type** that fits for storage (arrays, tables, structs); `short` for computations;
  `long` only when the range requires it.
- **Static lookup tables instead of computing**: trigonometry, rotations, square roots,
  reciprocals, row offsets… generated with `tools/bin/ti-table` (picks the smallest type).
- **Shifts instead of divisions and multiplications**, reciprocal multiplies for constant
  divisors, never a division or a 32-bit multiply in a loop, never a float at run time
  (a float op costs ~7,500 cycles).
- Pointers instead of indexing, ExtGraph instead of AMS drawing, redraw only what changes,
  spread work over frames, cheap heuristics over exact algorithms. Measure hot paths
(`experiments/bench/`) and read the generated asm; details in
`.claude/skills/ti89-c-dev/reference/ti68k-performance.md`.

**2. Visibility.** 160×100 in 4 greys on a small, low-contrast LCD: every sprite must read at a
glance. Main sprites (hero, NPCs, enemies) get a **white outline** (mask dilated by one pixel:
free at run time, `ti68k-c-patterns.md` § sprites), contrast against the background (a busy or
mid-grey scenery behind a black/white character), no detail smaller than 2 pixels that matters.

## Workflow

```sh
export PATH=$PWD/tools/bin:$PATH
ti-cc -o name src.c             # build → name.89z (skill ti89-c-dev)
ti-emu start                    # TiEmu (Titanium by default; TI_CALC=89 TI-89 HW2 AMSpatch, 89u unpatched)
ti-run name.89z [data.89y]      # clean restart from the .sav, files sent at boot, run name()
ti-shot /path/x.png --lcd       # screenshot, then read the image to check
```

A change is only "done" once it has been **verified running**: for runtime games, unit tests on the
PC plus a TI run at milestones; for code that touches the hardware directly (interrupts, keyboard,
grayscale, timers, runtime backend), seen in the emulator (printed numbers or screenshot read).
Test on the **Titanium** (default profile); the TI-89 HW2 (`TI_CALC=89`/`89u`) only for a release.

**New games use the Portable Game Runtime** (`runtime/README.md`; roadmap:
`TI68K_Game_Development_Toolbox.pdf`): portable C against `runtime/core/rt.h`, built for the PC
(SDL) and the TI from one Makefile (`make test`, `make pc`, `make ti`, `make bench`).
- Order per game: engine first, then game design, graphics/painting last.
- Test on the PC first: unit tests (`sw_step` + asserts, no window) before any window; the
  emulator last, on the Titanium. The runtime itself is
  already cross-checked (identical plane checksums PC/TI), so game logic needs no TI run per change.
- Every game with state has an **injection door**: `game_scenario(n)` jumps to a precise state
  (`--scenario N` on the PC, `name(N)` on the TI), plus PC state files and input scripts.
- On the TI prefer non-UI checks that print numbers (`make bench`, printed checksums) over
  screenshots of drawn scenes.

## Skills and knowledge

- `ti89-c-dev`: write, build and port C code. Its references are the project's knowledge base,
  **read them before writing non-trivial code**:
  - `.claude/skills/ti89-c-dev/reference/ti68k-c-patterns.md`: screen, grayscale planes,
    interrupts (auto-int 1/5, `DUMMY_HANDLER`, timer speed), keyboard (`_rowread` matrix),
    frame loop, fixed point, collisions, memory, portability.
  - `.claude/skills/ti89-c-dev/reference/ti68k-performance.md`: measured costs, optimisation
    rules, frame budget, heuristics, benchmarking method.
  - `.claude/skills/ti89-c-dev/reference/ti68k-game-techniques.md`: game algorithms that fit the
    machine (search/AI, grid puzzles, A*, 3D, raycasting, Mode 7, roguelikes, link cable, data
    encoding).
  - `.claude/skills/ti89-c-dev/reference/ti68k-asm.md`: when and how to write 68000 asm (C-callable
    `.s` recipe, register convention, cycle table, old-source idioms). Ask before using it.
  - `.claude/skills/ti89-c-dev/reference/puzzle-bobble-analysis.md`: a complete game annotated,
    with its bugs.
  - `sources/`: old reference sources (TICT tutorials, TI-Chess, TICT-Explorer, small games),
    indexed in `sources/README.md`. Read-only.
- `ti89-emulator`: drive TiEmu (keys, sending files, screenshots, saved states, pitfalls).
- `ti-port-sdl`: port an open SDL game (upstream pick, engine + tests on the PC, TI bench and run,
  graphics variants compared on screenshots, TI again, knowledge update, commit).
- `ti-port-tibasic`: remake a TI-Basic game (FFA): extraction, understanding with the guide, part by
  part and room by room, new 16-bit style engine and art, `PROGRESS.md` checkpoints.
- `ti-commit`: commit (Conventional Commits, one concern per commit, docs delta check) and push to
  `github.com/sam22200/ti68k-toolbox`.
- When you learn something new and verified about the platform, add it to
  `ti68k-c-patterns.md` or `ti68k-performance.md` (mark it **verified** if it was tested in the
  emulator).

## Hard-won rules

- Never hard-code ExtGraph colour values: use `COLOR_*` (their values changed in ExtGraph 2).
- Redirect auto-int 1 to `DUMMY_HANDLER` **before** `GrayOn()`; tear down in reverse order
  (`GrayOff()`, then timer start value, then vectors). Save the timer with `PRG_getStart()`
  (AMS default 0xCC = 19.3 Hz on both models; 0xB2 is the HW1 value).
- Grayscale games: double-buffer with GrayDBuf (no plane copies, no tearing).
- TiEmu's cycle counts are wrong for `movem` (~12 cycles whatever the register count) and
  multi-bit shifts (the 2 cycles per bit are ignored): ExtGraph/sprite/scroll/asm code looks up to
  7× faster than on hardware. Use datasheet counts for such code (performance §1), or time it with the
  16 kHz fine timer (performance §10). The HW2+ grayscale driver copies ~57 planes/s with `movem`
  (~9 % of the CPU on hardware, invisible in TiEmu): budget ~360k cycles per 30 fps frame.
- Never time with empty loops (-Os deletes them); never detect hardware from the ROM base (wrong on
  the Titanium): use `HW_VERSION`.
- Save files: unarchive before rewriting a variable (`SymAdd` keeps the archived flag → corrupt).
- Keep AMS's auto-int 5 (or chain to it) when using the link: replacing it makes `LIO_RecvData`
  timeouts block forever. Frame limiter: sleep with `pokeIO(0x600005, 0x1D)` inside the tick wait.
- Code generated or copied into RAM does not run portably unpatched (HW2: +0x40000 mirror,
  Titanium: HW3Patch): keep code in the program.
- `ngetchx()` needs auto-int 1; inside the game loop use `_rowread`/`_keytest`.
- Program size: AMS 2.xx (TI-89) refuses programs above 24,576 bytes, the Titanium does not. Keep
  data in archived data files read in place, `-pack NAME` beyond that; `malloc` big buffers
  (`-mno-bss` stores global arrays in the program file).
- Recompile old sources; old `.89z` binaries crash on the Titanium.
- Never `free(NULL)` (AMS crashes with an Address Error); statics survive between runs, so reset
  pointers to freed memory at the top of `_main`.
- Use the Docker TiEmu from `tools/bin/ti-emu`, never the system `tiemu` (the image adds
  `tools/tiemu-keyfix.c`: evdev keycodes translated for TiEmu 3.04, AltGr = 2nd, Right Ctrl =
  alpha, lost releases replayed; without it the PC arrows do nothing and keys stick).
- `ti-cc` builds with -Os plus dead-code removal and `-mregparm=5` (asm routines reading stack
  arguments need `__stkparm__`, or `TI_CC_PLAIN=1`). Build with -Os (default `ti-cc`); -O2 can be slower (it turned a 16-bit multiply into a library
  call). Check hot loops for `__mulsi3`/`__divsi3`/float helpers in the asm (`tigcc -S`).
  A `short` shared by several `(long)a * b` products turns them all into `__mulsi3`: use `muls16()`.

## Layout

- `hello/`: reference Hello World. `games/`: ported games (`puzzle_bobble/`) and our own
  (`campfire/`: Chrono Trigger camp-fire scene, TileMap + sprites, asset pipeline in `tools/extract.py`, data packed as ZX0 by `tools/pack.py`; `life/`: Game of Life on the runtime, glider start; `flappy/`: Flappy Bird ported from sdlbird with `ti-port-sdl`; `ffa/`: Final Fantasy Alternative remade from the TI-Basic `ffa_en/` with `ti-port-tibasic`, part I in progress, see its `PROGRESS.md`)
- `runtime/`: Portable Game Runtime (core API, PC software/SDL backends, TI backend, `rt.mk`,
  self-tests, demo). `tools/sdl2/`: SDL2 headers extracted locally (the library is the system's).
- `lib/`: shared code to link into programs: `unpack68k.s`/`.h` (ZX0 and LZ4 decoders in asm),
  `zx0pack.py` (ZX0 packer on the PC; recipe in `ti68k-c-patterns.md` §10).
- `experiments/`: small verified tests (`hwinfo/`: HW version, AMS, timer values; `bench/`:
  micro-benchmark harness, `bench7.c` PRNGs and bit-sliced CA; `codegen/`: what C compiles to; `tables/`: table-driven rotation and
  atan2 demo; `timers/`: int-5 rate per start value; `gray/`: GrayDBuf; `keys/`: keyboard latch;
  `files/`: save-file round trips, manual and stdio; `dll/`: DLL loading per model; `errors/`: TRY/FINALLY
  and CPU exceptions; `bigprog/`: size limits, packing, data files read from archive; `heapcode/`: running code from the heap; `link/`: link timeouts;
  `sprites/`: ExtGraph mirror routines; `tilemap/`: TileMap engine + pre-shifted sprites; `fonts/`: AMS fonts read in place; `hwsync/`: LCD sync bit and 16 kHz fine timer; `render/`, `ai/`, `struct/`, `compress/`, `maps/`: the
  measured ideas of game-techniques §13; `bench/m7row.s`: C-callable asm example). `sources/`: old reference sources.
- `tools/bin/`: `ti-cc ti-emu ti-run ti-send ti-group ti-key ti-shot ti-table`, `zx0`/`dzx0` (host ZX0 v2 packer and unpacker). `tools/pyenv/`: Python venv
  (numpy, scipy, pillow) for asset pipelines.
- `tools/gcc4ti-bin/`: installed GCC4TI (HTML docs in `doc/html/`); `tools/build-gcc4ti.sh`
  rebuilds it in Docker (GCC 4.1.2 does not build with the host gcc).
- `tools/extgraph/`: ExtGraph 2. `tools/patches/`: HW3Patch. `tools/rom/`: official OSes + AMSpatch.
- `tools/tiemu/{89t,89,89u}/`: TiEmu profiles (config, image, `.sav` state; absolute paths, do not move).
- `ti89decode.py`: TI-Basic file decoder (`ffa_en/`: the TI-Basic game it was written for).
- Local only, not in git (`.gitignore`, see `README.md`): `sources/`, `ffa_en/`, third-party and
  generated tools (`tools/gcc4ti*`, `extgraph`, `rom`, `tiemu`, `pyenv`, `sdl2`, `tarballs`,
  `patches`), `tools/bin/zx0`/`dzx0` (host builds), `runtime/platform-sw/amsfont.h` (extracted
  from the TI OS by `make`), build outputs.
- `docs/resources.md`: tutorials, game sources, sites, sprite/tileset/map sites for assets.

## Environment

Linux host with X11, AZERTY keyboard, xdotool installed, Docker available, no passwordless sudo.
