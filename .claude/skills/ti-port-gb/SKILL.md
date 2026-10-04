---
name: ti-port-gb
description: "Port a Game Boy (DMG .gb, or Game Boy Color .gbc) game to the TI-89 Titanium from its ROM through the Portable Game Runtime: read the header, decompile the ROM with Ghidra + GhidraBoy (pseudo-C and disassembly per function, headless), run the original ROM headless under PyBoy to record RAM traces frame by frame for key scripts and to dump VRAM tiles, maps and sprites, name the RAM variables and functions in a .sym file, grill the user on the port decisions (/grilling: 160x144 into 160x100, 59.7 Hz into ~30 fps, sprites and tiles, sound), then translate the game logic into fast C bit-exact with the traces, test on the PC (unit, trace, SDL), measure the TI binary under ti-cycles, the emulator last. Big games (MBC banks, CGB only, several hours: Metal Gear Solid, Zelda, Pokemon) take the big-game track by default, confirmed with the user: our own engine with the behaviour measured on the ROM, a ROADMAP.md of milestones and a first small milestone (one level), no screen comparisons against the ROM. Use it whenever the user wants to port, convert, \"porter\", decompile or remake a Game Boy / GB / DMG / Game Boy Color game or ROM for the calculator (Bubble Ghost, Tetris, Kirby, Zelda...), mentions a .gb or .gbc file, roms/gb/, Ghidra or GhidraBoy for a game, even without saying \"Game Boy\"."
---

# ti-port-gb (port a Game Boy game to the TI-89 from its ROM)

Same spirit and steps as `ti-port-pico8` (read it first; and `ti-port-sdl` before it: upstream
= specification, runtime = target, every step ends with a check, PC first, TI last, French to
the user, English in the files). What changes with a Game Boy ROM:

1. **No source, but a small machine code program.** Most GB games were written in SM83
   assembly. Ghidra with the GhidraBoy extension turns the ROM into one pseudo-C function per
   routine (`scripts/gbdecomp.sh`): readable for the logic (Bubble Ghost: 143 functions, the
   hall loop and the exit rule read at once), noisy for the stack and the 16-bit register
   pairs. It is a reading aid, never compiled: the C port is written by hand from it.
2. **The original runs on the PC.** PyBoy runs the ROM headless (thousands of frames per
   second): `scripts/gbtrace.py` prints chosen RAM bytes frame by frame for a key script,
   saves and loads states, pokes RAM, writes PNG/GIF. Those traces are the reference the C
   port is diffed against, as z8lua's are for PICO-8.
3. **The screens are close**: 160 pixels wide on both, 4 shades on both (BGP/OBP0/OBP1 →
   the 4 greys directly), 144 rows against 100, 59.73 Hz against ~30 fps. The fitting
   decisions (which 100 rows, two logic steps per frame or not) are put to the user with
   `/grilling` before any C.
4. **Translate the routines, keep the GB's memory, rebuild the rendering.** Every game
   routine becomes a C function by hand, on a flat copy of the GB memory (WRAM, HRAM, VRAM
   as byte arrays at their GB addresses: the game's tables, pointers and layouts keep
   working, and the traces compare whole RAM regions); the ROM itself is the data file
   (32 KB read in place on the TI); blocking code (waits for VBlanks) becomes protothreads;
   the screen is rebuilt from VRAM, I/O and OAM by a generic renderer. No SM83 emulator on
   the TI: an interpreter costs ~20-50 68000 cycles per SM83 cycle, the GB does 70,224 per
   frame. `assets/` holds this skeleton (Bubble Ghost's, the whole game ported this way).

**Two tracks, chosen in the grilling (question 3), the default by the game's size:**
- **Small game** (32 KB ROM only, DMG, a few thousand instructions of logic: Bubble Ghost):
  the 1:1 track below, every routine translated, bit-exact traces.
- **Big game** (MBC banks, CGB only, many level types or hours of play: Metal Gear Solid):
  **recommend our own engine** with the behaviour measured on the ROM (speeds, walls, patrols,
  vision boxes, timelines), a `ROADMAP.md` of milestones and a small first milestone (one
  level, no menus), confirmed with the user; `reference/big-game.md` replaces §4 and §7.
  Steps 1-3 (ROM, understand, grill) and 5-9 stay the same.

Constraints (`CLAUDE.md`): **1. performance**, **2. visibility** (white outline on the main
sprites), Titanium only (the TI-89 HW2 for a release), **ASM only after asking**.

References (read when the step says so):
- `reference/gb-facts.md`: the machine as a port sees it (memory map, PPU, OAM, timings,
  joypad, MBC, sound), what each maps to on the TI, the SM83 semantics the C must keep,
  tools, pitfalls.
- `reference/asm-to-c.md`: reading Ghidra's output, naming, translation patterns (flags and
  carries, BCD, jump tables, OAM shadow, VRAM writes, frame sync), the trace test.
- `reference/grilling.md`: the decision tree to grill (defaults and the data each needs).
- `reference/big-game.md`: the big-game track (when, the roadmap, measuring on the ROM: RAM
  diffs, the code behind a variable, rule grids, timelines, sprites; CGB and MBC specifics).

## 0. Tools (once per machine)

```sh
# Ghidra 11.4.2 + GhidraBoy (the latest GhidraBoy release is built for 11.4.2), local, not in git
curl -LO https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_11.4.2_build/ghidra_11.4.2_PUBLIC_20250826.zip
curl -LO https://github.com/Gekkio/GhidraBoy/releases/download/20250830/ghidra_11.4.2_PUBLIC_20250830_GhidraBoy.zip
unzip -q ghidra_11.4.2_PUBLIC_20250826.zip && mv ghidra_11.4.2_PUBLIC tools/ghidra
unzip -q ghidra_11.4.2_PUBLIC_20250830_GhidraBoy.zip -d tools/ghidra/Ghidra/Extensions/
tools/pyenv/bin/python -m pip install pyboy      # PyBoy 2.x, headless GB emulator
```
Ghidra needs a JDK ≥ 21 (`java -version`). With JDK 25 its OSGi loader refuses **Java**
scripts (`osgi.ee=UNKNOWN`): our post-script is Jython (`scripts/gb_export.py`), which works.
ROMs live in `roms/gb/` (not in git: commercial).

## 1. The ROM

- **Header**: `tools/pyenv/bin/python .claude/skills/ti-port-gb/scripts/gbextract.py
  roms/gb/<rom>.gb sources/<name>_gb/x` → `info.json`: title, DMG/CGB, cartridge type (ROM
  only / MBC1/3/5: banks), sizes, checksums, a census (interrupt vectors used, writes to
  LCDC/STAT/LYC/SCX/SCY/WX/WY: raster effects and scrolling, sound registers). CGB-only
  games (colour palettes, 2 VRAM banks, WRAM banks) go through the big-game track
  (`reference/big-game.md` §4: rendering their VRAM, colours to greys).
- **Decompile**: `.claude/skills/ti-port-gb/scripts/gbdecomp.sh roms/gb/<rom>.gb
  sources/<name>_gb/ghidra_out` → `decomp.c`, `disasm.s`, the Ghidra project (open it in the
  GUI to rename; a second run re-exports without reanalysis). Check: the function count, no
  `decompile failed`. **Jump tables hide whole state machines**: a state byte doubled and
  added to a table address, then `jp hl`; Ghidra leaves the targets as data. Find each
  table (a `jp hl` in `disasm.s`, the table address loaded just before) and its entry count,
  then rerun with them: `gbdecomp.sh ROM OUT 31d3:6 34c8:5 23df:18` (`TABLE:N` words, or a
  bare `ADDR`; `scripts/gb_funcs.py` makes them functions before the export).
- **Banked ROMs** (MBC): GhidraBoy analyses bank 0 only (MGS: 48 functions of 2 MB). Read
  banked code with `scripts/gbdis.py ROM BANK:ADDR N` and find the code that uses a variable
  with `gbdis.py ROM --find fa LO HI` (loads; `ea` stores) plus PyBoy hooks on those sites
  (`reference/big-game.md` §3).
- **Run**: `scripts/gbtrace.py roms/gb/<rom>.gb --frames 1500 --keys start.txt --gif run.gif
  --every 4 --shot 1499:last.png`: boot, title, first level, headless. Find the key script
  that reaches play (`C` = Start by default) and **save a state on the title's last wait**
  (`--save pre.state`, made by the game's Makefile from a boot key script).
- **The injection door of the reference**: `--load pre.state --boot-keys A --poke
  <level vars> --poke-at <level start>`: the keys that leave the title are held until the CPU
  reaches the routine that starts a level, then the pokes land (pokes made earlier are
  overwritten by the game's own init) and the scenario's script starts.
- **Count logic frames, not VBlanks**: `--logic <addr>` (an address reached once per logic
  frame, after the update) numbers frames, keys and samples there. Loading, fades, intros
  and score counts then take no frame, so the C port's update matches one to one and the
  traces stay aligned across level changes. `--stall N` ends a run that stops producing
  logic frames (game over) with a `# stalled` line.
- Keep everything in `sources/<name>_gb/` (not in git). Note title, year, publisher,
  licence in `games/<name>/README.md`: a commercial ROM and everything generated from it
  (tiles, maps, traces with its data) stay local; only our code and spec are committed.
- `ti-view` shows the GIFs and PNGs on the LAN (iPad).

## 2. Understand

- Read the main loop in `decomp.c` from `entry` (0x100): init, the game state machine
  (title, play, death, level change, game over), the per-frame calls and the VBlank sync
  (`halt` + a flag set by the VBlank handler). Mark the logic rate: every VBlank (60 Hz) or
  every 2nd.
- **Find the variables by experiment, name them by reading**: RAM diffs between two key
  scripts (a button held vs not), `--trace` ranges with `--diff`, `--poke` to confirm a
  meaning (lives, level index, position). OAM shadow (the DMA source, often `c000`) gives
  screen positions, not the game state: find the variables it is built from.
- Write `games/<name>/<name>.sym` (`00:c0ac hall`, `00:0223 play_hall`): `gbtrace.py --sym`
  traces by name; rename in Ghidra and re-export as the picture clears.
- **Measure, do not derive**: speeds, accelerations, timers, hit boxes, the frame a mechanic
  triggers on, the number of objects, scripted through `gbtrace.py`.
- Level data: where, which format (raw tile maps, metatiles, RLE, compressed), how many
  levels; dump each level's screen through the injection door (`--poke`) and VRAM
  (`gbextract.py --state`): `<tag>_tiles.png`, `_bg.png` (viewport outlined), `_oam.txt`.
- Count what the decisions need: the rows that matter in 144 (playfield vs HUD, status
  bars), distinct tiles and metatiles, sprite frames (orientations, animations), what is
  drawn with raster effects.
- Draft spec in `games/<name>/README.md` (states, RAM map, per-frame order, controls,
  measured numbers, levels, the gbtrace commands that reproduce each number).

## 3. Grill (before any conversion)

`/grilling` on the port plan with `reference/grilling.md` as the tree; record the answers in
`games/<name>/README.md` § Port decisions; no step 4 before the user confirms the summary.
A big game: question 3 defaults to our own engine (say why and what 1:1 would cost), question
4 to a first milestone, and `games/<name>/ROADMAP.md` is written before any C.

## 4. Engine and logic, against the traces

(The 1:1 track. Big games: `reference/big-game.md` §5 instead: an `extract.py` from the local
ROM, our engine, unit tests on the measured numbers, key scripts, TI = PC.)

- `games/<name>/` from `assets/` (its `README.md` says what to adapt): `gb.h` (memory,
  protothreads), `flow.c` (ISR, waits, input, main thread, runtime hooks), `render.c`,
  `test_<name>.c`, `tools/door.py`, plus the translated routines in one file per subsystem
  (Bubble Ghost: `play.c` hall loop, `loader.c` unpacker and display helpers, `hazards.c`,
  `screens.c` main flow and every non-play screen), `PORTING.md` (the conventions), `vars.h`
  (the `.sym` names as macros), `Makefile` (`include ../../runtime/rt.mk`; the ROM copied to
  `bgrom.bin` / `bgrom.89y`), `keys/*.txt`, `scripts.txt`; `traces/` and `build/` generated
  from the local ROM, never committed.
- Translate per `reference/asm-to-c.md` and `PORTING.md`, from `disasm.s` (Ghidra's C hides
  what callees leave in registers). **Split the work by subsystem and give each to a
  sub-agent** (fork) with `PORTING.md`, the files it owns and a standalone check against
  PyBoy dumps (Bubble Ghost: the loader verified on all 35 halls' VRAM/WRAM, the hazards on
  17 types, in parallel with the screens); then integrate: harmonise signatures (the same
  routine translated twice: keep one), wire the externs, run the traces.
- The sound driver is not translated (keep the writes to its request bytes); its RAM, the
  CPU stack, the VBlank counter and flags a lag frame changes stay out of the comparison.
- **Injection door**: the ROM's memory when the level routine starts (`tools/door.py`: WRAM,
  VRAM, HRAM, I/O, OAM after the door's pokes); the test loads it and runs the game's own
  loop from there, so even regions the port never wrote compare equal.
- **Sample points**: the logic point (`--logic 0283`, after the update) for play; **every key
  read** (`--logic <read_keys>`) for whole runs from power-on or a door: games read the pad
  once per VBlank in every loop (title, menus, name entry, ending) and never while loading,
  so the numbering does not depend on loading times. The C side calls a hook at the same
  two places. Scenario 99 = power-on, 100 + n = door n with read sampling.
- Check: `make test`, as `ti-port-pico8` §4: trace tests per key script (first divergent
  frame and variable printed), seeded random key scripts, a coverage gate on the mechanics,
  a mutation check per mechanic, unit tests for what the traces do not see. Trace every
  gameplay variable plus a Fletcher-16 of the big tables (`--trace LO-HI:h`: a collision
  mask in one field); the test binary prints the `--trace` list itself (`--vars`), so the C
  table is the only list. A script that leaves the slice ends in a level the port does not
  have yet: check hall, lives and score on that frame and stop, as a success.

## 5. PC for real

`make pc && ./<name>_pc` (SDL), headless `--shot` read back only to check. Feel check against
PyBoy with a window (`PyBoy(rom)` without `window='null'`), same keys.

## 6. TI without UI

As `ti-port-pico8` §6: `make cycles` under `ti-cycles` (`--file bgrom.89y`, `--max` for long
runs) on the heaviest level, `make xcheck`, a per-frame state hash TI = PC (`-DSTATE_HASH`;
GCC4TI's 16-bit `int` breaks code that passes every PC test; no `%` in the hash: two
divisions per byte cost 3.5M cycles a frame and made everything look slow). Budget ~360k
cycles per 30 fps frame with grayscale; two GB logic steps per frame if that was decided.
Measure steady play as the difference of two runs (550 and 50 frames): averages include the
loads. Zones per part (`BENCH_NAME`/`BENCH_BEGIN`): maps, rows, sprites, sort, ISR, main.

## 7. Graphics (SDL first)

- The ROM's own art (no redraw decided): the generic renderer (`assets/render.c`) draws
  whatever the game put in VRAM: BG/window map caches redrawn per dirty cell (VRAM writes go
  through `wr`), BGP E4 = no conversion (a tile row's two bytes are the light and dark plane
  bytes), sprites from the OAM copy the last DMA left (`gb_oam`), DMG priority per 8-pixel
  entry (smaller x in front, then the lower index), BG priority (flag 80) from the BG map
  cache. Invalidate the caches when LCDC's tile addressing or BGP changes, even when the game
  writes them with `IO()` directly. The 144 rows into 100: a play view (the playfield 1:1 +
  the port's HUD) and, per non-play screen, one band or two stitched (decided with the user).
- **Compare few screens**: a handful per milestone at chosen frames (each scenario's first
  frame, one per mechanic, a level change), not every sample: Bubble Ghost's 1,167 screens
  cost much and found what a dozen would have. Big games: none against the ROM (§ big-game).
- Check the screen **pixel by pixel** against PyBoy (`assets/screencmp.py`): the test binary
  renders at logic frame F from the door memory (`--shot`), PyBoy's screen is read **after**
  the tick of frame F (a hook runs while the PPU draws: its buffer mixes lines); both show the
  OAM and BG of frame F-1.
- A redraw (other art): convert from VRAM dumps with a build tool, the white outline on the
  main sprites (`ti68k-c-patterns.md` § sprites), 2 to 4 variants on the same headless shot;
  generated data from a commercial ROM stays local.

## 8. TI again, then the emulator once

`make cycles`, `make xcheck`, then once on the Titanium: `ti-emu restart <name>.89z
bgrom.89y`, wait ~8 s for the group to arrive, then type the call (`ti-run` types it before a
two-file group is received; a held [2nd] from an earlier test garbles the typing: restart):
one screenshot or printed numbers, the controls, a level change, ESC back to HOME. A demo GIF:
`ti-gif` in the background and the winning key script played by `ti-play` (held keys,
diagonals; open loop: `reference/big-game.md` §5). No X
display over SSH: the desktop session's `DISPLAY=:1` with its gdm `XAUTHORITY` works; Xvfb
does not (no window manager: `xdotool windowactivate` fails).

## 9. Conclude, knowledge, commit

Report (French): ROM and header, decisions, what was kept or changed, scripts and test counts,
cycles per frame, `.89z` size, limits, scenarios. Update `games/<name>/README.md`, the root
`README.md` and `CLAUDE.md` layout, verified platform facts in the knowledge base, a lesson
below, then `/ti-commit`.

## Lessons

- **Bubble Ghost** (`roms/gb/Bubble_Ghost.gb`, 32 KB ROM only, DMG, FCI / Pony Canyon /
  Infogrames 1990; first test of this skill, 2026-10-03): GhidraBoy decompiles 143 functions,
  the hall loop (`0223`) and the next-hall rule (`02e9`) read directly; the ghost, bubble and
  hazard handlers sat behind three jump tables (31D3, 34C8, 23DF): 180 functions with them.
  Slice 1 (hall 1, `games/bubble_ghost/`, local): 15 scripts / 14,672 logic frames bit-exact
  (47 variables, 6 pistons, the mask hash), 13 mutations caught, TI = PC by state hash, 287
  screens identical to the ROM's; ~14k cycles of logic per frame (two GB frames) + ~72k of
  rendering. BGP E4 made the BG free: a GB tile row's two bytes are the light and dark plane
  bytes. The traces caught a register clobber the pseudo-C hides (`blow_bubble` leaves E = 2
  or 3, so the caller's timer test sees that) and the A held from the title at frame 0:
  read the disassembly, not Ghidra's C, for what a callee leaves in registers.
  Logic once per VBlank, never skipped, no RNG (traces fully deterministic); the bubble in
  13.3 fixed point; collision on a 1-bit pixel mask in WRAM (C300, 20 bytes × 96 rows). Boot to hall 1 headless
  in 1,400 frames (Start at 400, 600, 800, A at 1,000); `pre.state` saved at frame 995 + poke
  `c0ac` = hall + 1 (and the entry direction `c0b0`, intro flag `c0b3` = 0) starts any of the
  35 halls. The playfield is the BG rows between a title
  bar and a HUD (~80 rows): it fits the TI's 100 rows without a camera.
- **Bubble Ghost, the whole game** (2026-10-03): ~2,500 SM83 instructions translated in four
  files (three of them by parallel sub-agents), on the flat GB memory; the sound driver
  (0850-12FF) left out. 127 trace scripts, 142,977 samples identical to the ROM with whole
  RAM regions compared: all 35 halls (idle + two random scripts each), power-on to play,
  game over with START and with CONTINUE, a finished game (the ending, the name entry, the
  title again: a bot recorded the hall-35 route), random scripts across menus; TI = PC by
  state hash; screens pixel-identical (play and hazards with BG priority). Program 26 KB
  (+ the 32 KB ROM as `bgrom`). Steady cost per 30 fps frame: hall 1 ~205k, heavy halls
  (10 hazards with BG priority) ~470k: sprites dominate (28 OAM entries: ExtGraph ~2.1k each,
  the rest the C around it).
  Traps met: a protothread called as a plain function (`r_05f0();` instead of `PT_CALL`) runs
  once and silently stops: grep every call of a blocking routine; the GB lags (fuzz_06 frame
  475: the logic overran a VBlank, the ISR ran mid-frame and cleared C0E7, set FF91): exclude
  those bytes; a hook-based sample must happen at the exact address (the hall end ran in the
  same VBlank after 0283 and decremented the lives before a loop-based sample); PyBoy drops
  inputs sent from inside a hook (queue them for the next tick); a bot's keys decided at the
  end of frame n drive frame n + 1. The screen check caught what the RAM traces cannot see:
  a routine that calls the OAM DMA directly (0523, translated as a no-op: after a death the
  GB shows an empty OAM for two frames), the 10-sprites-per-line limit (kept), and two
  parallel runs of the checker overwriting each other's PNGs (give each run its own files).
  1,164 of 1,167 screens identical; the 3 others differ by 2-14 pixels where the GB rewrites
  BG tiles while its PPU draws (tearing: not reproduced, not wanted). In hindsight a dozen
  chosen screens would have found the same bugs: compare few screens.
- **Metal Gear Solid** (`roms/gb/Metal_Gear_Solid_.gbc`, CGB only, MBC5 2 MB, Konami 2000;
  first big-game track, 2026-10-04, `games/mgs/`, code committed, data local): the user chose
  our own engine, milestone 1 = VR Sneaking Practice Lv.01 (`ROADMAP.md`: 8 milestones).
  Ghidra saw 48 functions (bank 0 only); `gbdis.py` + hooks on `--find` sites found the goal
  test (0C:4627) and the vision readers in minutes. Measured in one session: Snake's speeds,
  turn timing, diagonal 2-of-3, collision box from four wall stops, the guard's whole timeline
  (one 1,300-frame log), vision boxes on a 2-pixel grid. Trap: the vision grid stayed empty
  until it turned out guards only see while on screen (the camera had to be walked up; poking
  the camera byte is overwritten). Port: ~330 lines of C, 51 tiles + 65 sprite images (14 KB
  data file), tests on the measured numbers + a winning and a losing key script, TI = PC by
  `xcheck` and `tihash` (hash the fields, not the raw struct: the first try differed by byte
  order), ~70k cycles per frame (19 %), one emulator run. No screen compared with the ROM.
  The demo GIF in TiEmu (`ti-play` + `ti-gif`) failed once with a route timed to the frame and
  passed with a route made of wall stops and slides (robust to ±10 % speed on the PC).
