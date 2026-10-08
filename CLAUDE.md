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
ti-gif /path/x.gif 20 &         # animated GIF of the LCD (20 s) while ti-key plays
ti-play keys/win.txt            # a runtime key script played in TiEmu in real time (held keys, diagonals)
ti-view                         # PNG/GIF captures on the LAN, newest first: http://<PC IP>:8000 (iPad Safari, /?only=gif)
ti-cycles [--arg N] [--png F] name.89z   # the program on the PC: datasheet cycles per zone, screen as PNG
make xcheck                     # runtime game: TI binary under ti-cycles = PC headless, per scenario
```

**Measure cycles with `ti-cycles`, not TiEmu** (`tools/m68kbench/`): a headless 68000 (Musashi) with
the MC68000 datasheet timings (movem, shifts, mulu/muls: all verified by `test/cyctest.c`), AMS ROM
calls emulated with an estimated cost, zones marked with `bench.h` (`BENCH_BEGIN/END`, `BENCH_SHOT`
for the screen from memory). A 20-million-cycle run takes 0.05 s. Programs run under it must not
touch the I/O ports (build them with a `-DBENCH` variant; runtime games: `make cycles`, `-DRT_CYCLES`).
It also emulates the VAT (`--file data.89y`, saves written by `--save-dir`), the AMS fonts and key
scripts (`--keys`, `--frames`): `make xcheck` checks a runtime game's TI binary against the PC,
same screen checksum per scenario. TiEmu has no CLI, D-Bus or GDB to read memory, so only the
hardware paths (grayscale, keyboard, interrupts) are still checked in the emulator.

**Development flow: tests without UI, the calculator last of all.** The TI emulator is by far the
slowest way to check anything (a restart, keys typed, a screenshot to take and read: tens of
seconds per check, and flaky): avoid it at all costs. In this order:
1. **Unit tests** on the PC, headless: `make test` (`sw_step` + asserts on the state, pixel and
   plane checksums), as much of the program as possible.
2. **Real runs without UI**: PC headless runs (`--headless --keys F --frames N --shot F.png`) and
   the TI binary itself under `ti-cycles` (cycles per zone, `BENCH_VALUE` numbers, the screen read
   from memory as a PNG and a checksum per scenario; `make xcheck` for runtime games, data files
   and saves included). Compare screens by checksum; read a PNG (`ti-cycles --png`, PC `--shot`)
   only when a checksum differs or for an art review.
3. **The emulator, last of the last**: once, at a milestone or a release, and only for what the
   PC cannot run (grayscale driver, keyboard matrix, interrupts, timers, link, real archive
   and Flash, AMS dialogs). Prefer printed numbers; one screenshot at most.
A change is "done" once 1 and 2 pass; code that touches the hardware also gets 3, at the end.
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
- On the TI side, measure and check without UI: `ti-cycles` (cycles, screen from memory), or
  printed numbers (`make bench`, printed checksums); screenshots of drawn scenes only as the last
  step (flow above).

## Skills and knowledge

Collaboration rules (French replies, emulator etiquette, git, notes for other
agents such as Codex; `AGENTS.md` links to this file): `docs/working-rules.md`. The skills below
are plain Markdown (`.claude/skills/<name>/SKILL.md`): any agent reads the matching one before
that kind of task.

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
- `ti89-emulator`: drive TiEmu (keys, sending files, screenshots, saved states, pitfalls): the last step only.
- `ti-port-sdl`: port an open SDL game (upstream pick, engine + tests on the PC, TI bench and run,
  graphics variants compared on screenshots, TI again, knowledge update, commit).
- `ti-port-tibasic`: remake a TI-Basic game (FFA): extraction, understanding with the guide, part by
  part and room by room, new 16-bit style engine and art, `PROGRESS.md` checkpoints.
- `ti-port-pico8`: port a PICO-8 cart (Celeste Classic…): cart decoded with shrinko8, run
  headless under z8lua for reference traces, `/grilling` on the port decisions, Lua translated
  to bit-exact 16.16 C (`assets/p8num.h`) diffed against the traces, native rendering.
- `ti-port-gb`: port a Game Boy game from its ROM (Bubble Ghost…): decompiled with Ghidra +
  GhidraBoy (headless, pseudo-C per function), run headless under PyBoy for RAM traces and
  VRAM dumps, `/grilling` on the port decisions, every routine translated to C on a flat copy
  of the GB memory (protothreads for the waits, the ROM itself as the data file), traces
  comparing whole RAM regions, a generic VRAM/OAM renderer; skeleton in its `assets/`. Big
  games (MBC, CGB: Metal Gear Solid…) take its big-game track by default: our own engine with
  the behaviour measured on the ROM, a `ROADMAP.md` of milestones, few screen comparisons
  (`reference/big-game.md`). ROMs in `roms/gb/` (local).
- `ti-port-ti83`: port a TI-83/83+/84+ asm game (Desolate…): program file read and its load
  address scored, Ghidra's Z80 processor (headless), the original run headless without a TI
  ROM (`ti83run.py`: Z80 core, LCD, keypad, interrupts, grey, the OS and shell routines in
  Python) for reference screens and dumps, analysis split between sub-agents, `/grilling`,
  the logic rewritten routine by routine in C, light tests (one per mechanic, a bot-played
  walkthrough, TI = PC by `xcheck`). Programs in `roms/ti83/` (local).
- `ti-port-md`: study a Mega Drive ROM with the headless `tools/md/` runner,
  RAM/VDP snapshots and controlled behavior traces; rebuild a small slice in
  portable C through grilling and milestones (Sonic 1 GHZ1, half scale).
  ROM art is extracted offline; performance is checked per TI frame, including
  dense particle spills. ROMs in `roms/md/` (local).
- `ti-port-snes`: the same measured, bounded approach for SNES ROMs through
  `tools/snes/` (pinned Snes9x, WRAM/cartridge RAM, PPU and video snapshots).
  First experiment: Yoshi's Island PAL, level 1-1 over five original view widths;
  portable movement and real-terrain traversal, original collision traces and
  complete PC/TI replay/cycle checks; tongue/capture/egg reserve and ROM
  scenery/animated actors, contact damage, crying Baby Mario bubble, rescue
  and ten-second countdown work; animated coins, idle poses, following eggs
  and aimed throws work.
  Second experiment: Mega Man X USA's opening Highway section, running,
  short/held jumps, wall slide/kick, shots/charge, roller/contact damage and
  outlined ROM animations; original mechanics and native PC/TI replays checked.
  Super FX is a reference dependency; ROMs in `roms/snes/` remain local.
- `ti-port-neogeo`: the same measured, bounded approach for Neo Geo MVS/AES
  cartridge sets. First preparation case: Windjammers in
  `roms/neogeo/wjammers/`; `tools/neogeo/` checks chip identities, stages a
  lossless reference ZIP and exports the big-endian 68000 program image.
  The pinned FBNeo headless runner now builds, with two-player scripts,
  RAM/VRAM/palette/status exports and replay checks. Original cold boot,
  native inputs, CPU-bus pokes and 120 exact replay frames pass with the
  local BIOS. A two-human Beach serve door, both characters' walking/clamps
  and ordinary disc trajectories pass 1882 replay frames. Exact ordinary wall
  positions, neutral contact boxes/catch classification, Beach points and
  both losing players' next serve pass another 1390 replay frames. The native
  Beach training engine in `games/windjammers/` now runs on the Portable Game
  Runtime: measured neutral movement/ordinary shots/walls/contacts/points,
  ordinary captures/recoil checked through 660 complete action steps, animated
  outlined ROM actors, adapted deflection/service/AI and a full-LCD native court
  with compact two-digit scores, a central countdown, centered numbered 3/5/3
  bands and an outlined net without shadow on plain white sand. Reference scenery decoding
  matches 41,344 original RGB pixels.
  M2b1 adds possession-dependent ordinary strength and automatic releases
  (4058 native fixture steps, 29 trials/7140 exact original replay frames).
  M2b2 adds timed stationary lifts, complete charging/recapture and powerful
  immediate returns (6704 equality steps, 78 trials/10920 original replay
  frames), including high-speed catches. Failed-preparation block flight
  remains adapted. M3a adds lobs, charged Mita/Yoo specials, airborne rebounds
  and counter windows (8573 bounded steps, 62 twice-replayed original trials).
  Normal-lob jitter is conditioned on original targets. M3b adds directional
  arcs, curved flights/wall transitions and immediate/settled returns
  (17764 equality steps, 180 twice-replayed no-write trials).
  M3c adds dash motion (48 twice-replayed no-write trials, 816 equality steps),
  a larger disc, fixed lob targets and explicit charge guidance. Twenty-nine
  PC/TI scenarios plus a replay through clock zero pass 7380 state hashes
  and 30 final screens. Powerful/special flight trails add fading afterimages
  and sparks, with six active-effect PC/TI screen comparisons. Actor planes
  are read in place from wjart.89y.
  Original RGB actor
  decoding matches 1,329,378 pixels across 1256 scenes; peak game frame is
  under the 360k budget (current measurements in the game README). Moving/other action-pose contacts, rear flight and
  remaining Beach/FIX scenery within the requested plain-floor style remain next.
- `ti-port-ps1`: study a PlayStation game and rebuild one of its mechanics small (Alundra's
  jumps and terrain heights in one room): disc read (`psxiso.py`), Ghidra's built-in MIPS on
  the executable or a RAM dump, the disc run headless (`psxrun.py`: pcsx_rearmed libretro
  core, HLE BIOS, traces, RAM diffs, states), `/grilling` first, `RE_NOTES.md` with every
  finding labelled OBSERVED / INTERPRETATION / TARGET, a behavioural model (never a
  translation), milestones on the runtime. Discs in `roms/ps1/` (local).
- `ti-art-refs`: sprite banks to draw from (scenery, characters, UI/HUD/menus/dialogue, portraits),
  the pick per category for 160×100 in 4 greys; references only, redrawn, never committed.
- `ti-commit`: commit (Conventional Commits, one concern per commit, docs delta check) on a branch
  per task, push to `github.com/sam22200/ti68k-toolbox`, merge into `main` once the tests pass.
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
  (`campfire/`: Chrono Trigger camp-fire scene, TileMap + sprites, asset pipeline in `tools/extract.py`, data packed as ZX0 by `tools/pack.py`; `life/`: Game of Life on the runtime, glider start; `flappy/`: Flappy Bird ported from sdlbird with `ti-port-sdl`; `ffa/`: Final Fantasy Alternative remade from the TI-Basic `ffa_en/` with `ti-port-tibasic`, part I in progress, see its `PROGRESS.md`; `mode7/`: David Coz's Mode 7 demo, decompiled from its binary and optimised, benchmarked with `ti-cycles`, see its `OPTIMISATIONS.md`; `celeste/`: Celeste Classic ported from the PICO-8 cart with `ti-port-pico8`, room 0, its logic bit-exact with the cart run under z8lua; `mgs/`: Metal Gear Solid (GBC) on our own engine with `ti-port-gb`'s big-game track, milestone 1 = VR Sneaking Lv.01, data generated from the local ROM, see its `ROADMAP.md`; `alundra/`: Alundra (PS1) traversal, a test room (jumps, heights, ledges, depth) and the whole village of Inoa (stairs, roofs, scrolling, the game's own image and depth in four data files) on our own engine with `ti-port-ps1`, art, image and map generated from the local disc by `tools/extract.py`, see its `README.md`)
- `games/sonic/`: Sonic 1 (Mega Drive) traversal prototype: first 4.8 GHZ1 view
  widths, half scale, C on the runtime, measured original flat physics, real
  collision terrain, rigid bridge, rings, three enemy types, damage and ROM graphics
  (four greys, animated actors, white outlines). `tools/md/` runs the
  original under a local pinned Genesis Plus GX core; ROM-derived data stays
  local. Decisions and milestones: its `README.md`, `RE_NOTES.md`, `ROADMAP.md`.
- `runtime/`: Portable Game Runtime (core API, PC software/SDL backends, TI backend, `rt.mk`,
  self-tests, demo). `tools/sdl2/`: SDL2 headers extracted locally (the library is the system's).
- `lib/`: shared code to link into programs: `unpack68k.s`/`.h` (ZX0 and LZ4 decoders in asm),
  `zx0pack.py` (ZX0 packer on the PC; recipe in `ti68k-c-patterns.md` §10).
- `experiments/`: small verified tests (`hwinfo/`: HW version, AMS, timer values; `bench/`:
  micro-benchmark harness, `bench7.c` PRNGs and bit-sliced CA; `codegen/`: what C compiles to; `tables/`: table-driven rotation and
  atan2 demo; `timers/`: int-5 rate per start value; `gray/`: GrayDBuf; `keys/`: keyboard latch;
  `files/`: save-file round trips, manual and stdio; `dll/`: DLL loading per model; `errors/`: TRY/FINALLY
  and CPU exceptions; `bigprog/`: size limits, packing, data files read from archive; `heapcode/`: running code from the heap; `link/`: link timeouts;
  `sprites/`: ExtGraph mirror routines; `tilemap/`: TileMap engine + pre-shifted sprites; `fonts/`: AMS fonts read in place; `hwsync/`: LCD sync bit and 16 kHz fine timer; `demoscene/`: ideas from the pouet.net TI-68k demos measured (voxel terrain, raster wobble; `gamefx.c`: bump-mapped torch, lake reflection, twister, TV static, plasma title as GIFs), game-techniques §14; `render/`, `ai/`, `struct/`, `compress/`, `maps/`: the
  measured ideas of game-techniques §13; `bench/m7row.s`: C-callable asm example). `sources/`: old reference sources.
- `tools/m68kbench/`: `ti-cycles` sources, `bench.h` markers, self-test. `docs/INSTALL.md`: toolchain install (Ubuntu, macOS).
- `tools/bin/`: `ti-cc ti-emu ti-run ti-send ti-group ti-key ti-shot ti-gif ti-play ti-table ti-view`, `ti-cycles` (host 68000 cycle counter, built from `tools/m68kbench/` with Musashi in `tools/musashi/`), `zx0`/`dzx0` (host ZX0 v2 packer and unpacker). `tools/pyenv/`: Python venv
  (numpy, scipy, pillow) for asset pipelines.
- `tools/gcc4ti-bin/`: installed GCC4TI (HTML docs in `doc/html/`); `tools/build-gcc4ti.sh`
  rebuilds it in Docker (GCC 4.1.2 does not build with the host gcc).
- `tools/extgraph/`: ExtGraph 2. `tools/patches/`: HW3Patch. `tools/rom/`: official OSes + AMSpatch.
- `tools/tiemu/{89t,89,89u}/`: TiEmu profiles (config, image, `.sav` state; absolute paths, do not move).
- `ti89decode.py`: TI-Basic file decoder (`ffa_en/`: the TI-Basic game it was written for).
- Local only, not in git (`.gitignore`, see `README.md`): `games/ffa_ct/` (FFA with Chrono Trigger
  sprites, an art test: `.git/info/exclude`, state in its `PROGRESS.md`), `games/bubble_ghost/`
  (Game Boy Bubble Ghost ported with `ti-port-gb`, commercial: `.git/info/exclude`, state in its
  `README.md`), `games/desolate/` (TI-83 Desolate ported with `ti-port-ti83`, freeware whose
  readme forbids redistribution: `.git/info/exclude`, state in its `README.md`), `sources/`,
  `ffa_en/`, third-party and
  generated tools (`tools/gcc4ti*`, `extgraph`, `rom`, `tiemu`, `pyenv`, `sdl2`, `tarballs`,
  `patches`, `musashi`, `shrinko8`, `z8lua`, `ghidra`, `pcsx_rearmed`), `roms/` (commercial ROMs, TI-83 programs), `tools/bin/zx0`/`dzx0`/`ti-cycles` (host builds), `runtime/platform-sw/amsfont.h` (extracted
  from the TI OS by `make`), build outputs.
- `docs/resources.md`: tutorials, game sources, sites, sprite/tileset/map sites for assets.

## Environment

Linux host with X11, AZERTY keyboard, xdotool installed, Docker available, no passwordless sudo.
