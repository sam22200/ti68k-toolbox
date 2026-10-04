---
name: ti-port-gb
description: "Port a Game Boy (DMG, .gb; or a DMG-compatible .gbc) game to the TI-89 Titanium from its ROM through the Portable Game Runtime: read the header, decompile the ROM with Ghidra + GhidraBoy (pseudo-C and disassembly per function, headless), run the original ROM headless under PyBoy to record RAM traces frame by frame for key scripts and to dump VRAM tiles, maps and sprites, name the RAM variables and functions in a .sym file, grill the user on the port decisions (/grilling: 160x144 into 160x100, 59.7 Hz into ~30 fps, sprites and tiles, sound), then translate the game logic into fast C bit-exact with the traces, test on the PC (unit, trace, SDL), measure the TI binary under ti-cycles, the emulator last. Use it whenever the user wants to port, convert, \"porter\", decompile or remake a Game Boy / GB / DMG / Game Boy Color game or ROM for the calculator (Bubble Ghost, Tetris, Kirby, Zelda...), mentions a .gb file, roms/gb/, Ghidra or GhidraBoy for a game, even without saying \"Game Boy\"."
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
4. **Translate the logic, rebuild the rendering.** The game logic becomes C function by
   function on the same RAM variables (u8/u16 wrap-around, carries, BCD), the rendering is
   native (pre-rendered hall or TileMap, sprites converted from VRAM). No SM83 emulator on
   the TI: an interpreter costs ~20-50 68000 cycles per SM83 cycle, the GB does 70,224 per frame.

Constraints (`CLAUDE.md`): **1. performance**, **2. visibility** (white outline on the main
sprites), Titanium only (the TI-89 HW2 for a release), **ASM only after asking**.

References (read when the step says so):
- `reference/gb-facts.md`: the machine as a port sees it (memory map, PPU, OAM, timings,
  joypad, MBC, sound), what each maps to on the TI, the SM83 semantics the C must keep,
  tools, pitfalls.
- `reference/asm-to-c.md`: reading Ghidra's output, naming, translation patterns (flags and
  carries, BCD, jump tables, OAM shadow, VRAM writes, frame sync), the trace test.
- `reference/grilling.md`: the decision tree to grill (defaults and the data each needs).

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
  games (colour, double speed, 2 VRAM banks) are out of scope unless the user insists.
- **Decompile**: `.claude/skills/ti-port-gb/scripts/gbdecomp.sh roms/gb/<rom>.gb
  sources/<name>_gb/ghidra_out` → `decomp.c`, `disasm.s`, the Ghidra project (open it in the
  GUI to rename; a second run re-exports without reanalysis). Check: the function count, no
  `decompile failed`. **Jump tables hide whole state machines**: a state byte doubled and
  added to a table address, then `jp hl`; Ghidra leaves the targets as data. Find each
  table (a `jp hl` in `disasm.s`, the table address loaded just before) and its entry count,
  then rerun with them: `gbdecomp.sh ROM OUT 31d3:6 34c8:5 23df:18` (`TABLE:N` words, or a
  bare `ADDR`; `scripts/gb_funcs.py` makes them functions before the export).
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

## 4. Engine and logic, against the traces (placeholder graphics)

- `games/<name>/`: `Makefile` (`include ../../runtime/rt.mk`), `<name>.h` (the state as the
  game keeps it: same widths, u8 where the GB has a byte), `<name>.c`, `test_<name>.c`,
  `keys/*.txt`, `scripts.txt`; references in `traces/` only if the licence allows (commercial
  ROM: generated by `make traces` from the local ROM, not committed; the tests skip with a
  message when the ROM is missing).
- Translate per `reference/asm-to-c.md`: same variables, same order of updates, same
  wrap-around; the RNG is the game's own (same seed source, same calls).
- **Injection door**: `game_scenario(n)` = a level; its reference is `pre.state` + the
  matching `--poke`. Both listed in the README.
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

As `ti-port-pico8` §6: `make cycles` under `ti-cycles` on the heaviest level, `make xcheck`,
a per-frame state hash TI = PC (`-DSTATE_HASH`; GCC4TI's 16-bit `int` breaks code that
passes every PC test). Budget ~360k cycles per 30 fps frame with grayscale; two GB logic
steps per frame if that was decided.

## 7. Graphics (SDL first)

- Convert from the VRAM dumps with a build tool (`tools/gfx.py`): tiles through BGP/OBP to
  the 4 greys (shade 0 of a sprite = transparent → the mask), flipped variants baked, the
  level pre-rendered or the metatile set, the white outline on the main sprites
  (`ti68k-c-patterns.md` § sprites), the HUD re-laid out as decided. Generated data from a
  commercial ROM stays local (`gfx.h` in `.gitignore`, built by `make` from the ROM).
- When the art is the ROM's own (no redraw decided), check the screen **pixel by pixel**
  against PyBoy (a game tool like Bubble Ghost's `tools/screencmp.py`): the PC's headless shot
  after N runtime frames against `gbtrace --logic --shot 2N` (its picture shows the sprites
  and BG of logic frame 2N-1, the port's last), the playfield as 4 grey ranks. Key scripts are
  read as runtime frames by the PC: give PyBoy the same script with doubled frame numbers.
  What it caught: DMG sprite priority is per 8-pixel OAM entry (smaller x in front, then the
  lower index), so compose 8-wide columns; a `u8` x wrapping at the left edge; the blow frame.
- Otherwise compare 2 to 4 variants on the same headless shot, keep the winner; the traces
  still pass.

## 8. TI again, then the emulator once

`make cycles`, `make xcheck`, then once `ti-run <name>.89z` on the Titanium: one screenshot or
printed numbers, the controls, a level change, ESC back to HOME.

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
