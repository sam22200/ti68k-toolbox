---
name: ti-port-ti83
description: "Port a TI-83 / TI-83+ / TI-84+ assembly game (a .83p or .8xp program, Z80; Ion, MirageOS, Venus, DoorsCS or plain Send(9) to the TI-89 Titanium through the Portable Game Runtime: read the program file and find its load address, disassemble and decompile it with Ghidra's Z80 processor (headless), run the original headless on the PC without a TI ROM (scripts/ti83run.py: Z80 core, LCD, keypad, interrupts, grayscale, the OS and shell routines it calls) for reference screens, extract the maps, tiles, sprites and text, grill the user on the port decisions (96x64 into 160x100, grey, controls), then rewrite the game in C on the runtime, routine by routine for the logic, with a light test set (unit tests on the mechanics, a walkthrough script, screens per room), measure under ti-cycles, the emulator once. Use it whenever the user wants to port, convert, \"porter\" or remake a TI-83, TI-83+, TI-84 or TI-82/85/86 asm game for the TI-89, mentions a .83p / .8xp / .8xk file or roms/ti83/, even without saying \"TI-83\". TI-Basic games go to ti-port-tibasic."
---

# ti-port-ti83 (port a TI-83/84 asm game to the TI-89)

Same spirit as `ti-port-gb` (read its steps 0-2 once: same tools, same order), lighter on
verification. What changes with a TI-83 program:

1. **A small Z80 program, no source.** Ghidra's built-in Z80 processor (no extension) turns
   the program into pseudo-C per routine (`scripts/ti83decomp.sh`): a reading aid; the C is
   written by hand from it and from the disassembly.
2. **The original runs on the PC without a ROM.** TI-83 games call few OS routines (text,
   numbers, keys): `scripts/ti83run.py` runs the program on a Z80 core with the LCD, keypad,
   timer interrupts and those routines in Python. It gives reference screens (grey levels
   averaged as on the LCD), memory dumps for the data, and measured numbers.
3. **The screens differ a lot**: 96×64 (1 bit, grey by flicker) against 160×100 in 4 greys.
   How to use the larger screen is the main decision, put to the user with `/grilling`.
4. **Rewrite, do not emulate.** The logic is translated into plain C on the game's own
   variables (a struct, not a flat Z80 memory: TI-83 games keep few variables, often in
   their own program image); the data (maps, tiles, sprites, text) is extracted at build time
   into C arrays or a data file; the rendering is the runtime's. No Z80 interpreter on the TI.

**Verification is light** (the user's choice for this skill): no frame-by-frame traces. A
port is done when: the unit tests of the mechanics pass, a walkthrough key script plays
the game from the start to the end (or to the slice's end) on the PC headless, each room's
first screen was compared once with the original's (a contact sheet, by eye), the TI binary
runs the same script under `ti-cycles` with the same final state, and the emulator ran it
once.

Constraints (`CLAUDE.md`): **1. performance**, **2. visibility** (white outline on the main
sprites), Titanium only, **ASM only after asking**. Answer the user in French, write files in
English.

References: `reference/ti83-facts.md` (machine, file formats, memory map, ports, OS and
shell routines, data formats, the runner), `reference/grilling.md` (the decisions).

## 0. Tools (once per machine)

- Ghidra in `tools/ghidra` (see `ti-port-gb` §0; the Z80 processor is built in, JDK ≥ 21).
- `tools/pyenv/bin/python -m pip install z80 z80dis` (Z80 core in C; disassembler for
  regions Ghidra missed: `z80dis.z80.decode(bytes, addr)`).
- ROM call names: `curl -sLo .claude/skills/ti-port-ti83/scripts/ti83asm.inc
  https://raw.githubusercontent.com/alberthdev/spasm-ng/master/inc/ti83asm.inc` (and
  `ti83plus.inc` for 83+ programs); not committed.
- Programs live in `roms/ti83/<game>/` (not in git). Pick the TI-83 build when there is one
  (one file); the 83+ one often splits its data into an archived variable.

## 1. The program

```sh
S=.claude/skills/ti-port-ti83/scripts; PY=tools/pyenv/bin/python
$PY $S/ti83var.py roms/ti83/<game>/<prog>.83p sources/<name>_83   # body.bin, info.json
$S/ti83decomp.sh sources/<name>_83 [ADDR...]                      # decomp.c, disasm.s
$PY $S/ti83run.py sources/<name>_83 --ticks 3000 --keys k.txt --shot 2000:s.png \
    --gif run.gif --every 20 --dump 3000:mem.bin --watch d100-d110
```

- `info.json`: model, shell, load address (check the scores: one base must win clearly),
  entry, the ROM calls used with their names.
- Ghidra: give it the interrupt handler and code reached through pointers as extra
  addresses (an IM 2 game: `ld a,XX; ld i,a; im 2`, the vector table points at a `jp`).
  Ghidra's function boundaries are right only if the base is: a function starting with an
  odd instruction (`ADC A,(HL)` where `ld hl,...` was expected) means a wrong base.
- The runner: read `ti83run.py`'s header. Watch its log: `unknown ROM call` or `unknown shell
  call` lines name the routines to add (Desolate needed Venus's random at FE72); a jump into
  the stack area (PC in FFxx) means a shell routine is missing. Step through a problem with
  `ticks_to_stop = 1` and a ring of PCs (`import ti83run`).
- Find the key script that reaches play (games wait for a specific key per screen: grep
  `CP 0x..` after each call of the key routine) and save its memory dump at the first
  playable frame: `--load mem.bin` restarts from there (the injection door of the reference).
- Keep everything in `sources/<name>_83/` (not in git). Note author, year, licence in
  `games/<name>/README.md`: a freeware game's art and text stay local unless its readme
  allows redistribution; our code and spec are committed, the build regenerates the data
  from the local program file (`make` fails clearly without it).

## 2. Understand (sub-agents)

Split the reading by subsystem and give each to a sub-agent with the files and the runner
(Desolate: rendering and data formats; game logic and variables; text, menus and saves).
Each writes `notes_<part>.md` in `sources/<name>_83/` and an extractor when its part has
data (`extract.py`: rooms and tiles to PNG and JSON; `text.py`: every string). Ask for
addresses, formats, every variable with its meaning, and claims checked with the runner
(`--watch` while walking, `--poke` to confirm a meaning, screenshots of rooms reached).
Then write the spec `games/<name>/README.md` (states, variables, per-loop order, controls,
rooms, items, puzzles, text, saves) from the notes.

## 3. Grill (before any C)

`/grilling` on the port plan with `reference/grilling.md`; record the answers in
`games/<name>/README.md` § Port decisions. One round of the important questions is enough
when the user wants to move fast (AskUserQuestion with the data and a default each).

## 4. Data, then logic, on the PC

- `games/<name>/`: `Makefile` (`include ../../runtime/rt.mk`), a build tool (`tools/data.py`)
  that reads the local program file (or `sources/<name>_83/body.bin`) and writes the
  generated headers (`gen_*.h`: maps, tiles, sprites, strings; never committed when the art
  is not ours), the game (`<name>.c`, split per subsystem when large), `test_<name>.c`,
  `keys/*.txt`.
- Translate the logic routine by routine from the disassembly (Ghidra's C hides what callees
  leave in registers and flags): same variables (a struct named after the `.sym` names), same
  order per loop, same rules. Keep the game's own timing unit (one main-loop iteration = one
  logic step) and map it to frames (decision 6).
- The injection door: `game_scenario(n)`: 0 = title, 1 = the first room, n = a room or a
  puzzle state; `--scenario N` on the PC, `name(N)` on the TI.
- Tests (`make test`), deliberately few: one per mechanic (move and collide, room change,
  pick up and use an item, each puzzle's solve, each enemy, death, save/load), plus the
  walkthrough: a bot in the test (BFS over rooms and tiles, fights, the progression in
  order) that plays to the end with asserts at each milestone and writes `keys/walk.txt`
  for the PC binary and `ti-cycles`. Compare each room's first screen with the original's once: a
  contact sheet PNG of both (PC `--shot` vs `ti83run.py --shot`), read by eye.

## 5. Graphics

The art comes from the game (converted at build time) or is redrawn (decided in step 3).
White outline on the hero and enemies (mask dilated by one pixel), the TI-83's two grey
buffers to the light and dark planes. Sprites masked; a static room is drawn once into a
background (the runtime's tilemap or a pre-rendered plane pair) and sprites over it.
`make pc && ./<name>_pc` to feel it; headless shots to check.

## 6. TI without UI, then the emulator once

`make ti`, `make cycles` with the walkthrough (`ti-cycles --keys --frames`: cycles per
frame, the same final screen checksum as the PC: `make xcheck`), size (AMS 2's 24,576-byte
limit only matters for a TI-89 HW2 release). Then once on the Titanium: `ti-run <name>.89z`,
one screenshot or printed numbers, controls, a room change, ESC back to HOME.

## 7. Conclude, knowledge, commit

Report (French): program and shell, decisions, what was kept or changed, tests, cycles per
frame, `.89z` size, limits. Update `games/<name>/README.md`, the root `README.md`, `CLAUDE.md`
(layout, skills list), verified platform facts in the knowledge base, a lesson below, then
`/ti-commit`.

## Lessons

- **Desolate** (`roms/ti83/desolate/`, tr1p1ea 2004, TI-83 Venus build, 25,920 bytes; first
  use of this skill, 2026-10-03; `games/desolate/`, local). Load address 9329 (the Venus
  stub), won by the score (476 against < 0 for the others). Nine ROM calls (`_vputs`,
  `_vputmap`, `_DispOP1A`, `_SetXXOP1`, `_SetXXXXOP2`, `_OP2toOP1`, `_cphlde`,
  `_clrScrnFull`, `_homeUp`) and one Venus routine (random, FE72, found as a jump into the
  stack): the runner played the original from its frequency prompt to the first room with no
  ROM, 600 ticks in 0.6 s. Grey by Durk Kingma's dithered interrupt (two buffers, ~70k
  T-states per tick: menus advance ~1 iteration per 25-40 ticks, hold keys 30+ ticks).
  Ghidra: 131 functions with the IM 2 handler added.
- Three sub-agents in parallel (rendering and formats, logic and variables, text and menus)
  produced notes, a 330-symbol `.sym`, `extract.py` (every room rendered, checked pixel for
  pixel against the original's buffers) and `text.py` (137 Huffman strings) in ~15 minutes;
  the C port was then written in one pass from the notes plus the disassembly of the action
  routines. Everything is data-driven (room maps and 49-byte records), so slice 1 covers
  every room; what remains is code (enemies, shots, ending).
- Light tests were enough to catch real bugs: a bot (BFS over the floor tiles, doors, keypad)
  plays the slice and writes `keys/walk.txt`; `make xcheck` with that script caught GCC4TI's
  16-bit `int` sign-extending `(u32)(hi << 8 | lo)` (TI ≠ PC checksum) that every PC test
  passed. Seed `rt_seed` in the test before `sw_init` or the random door codes differ from
  the PC binary's run of the same script.
- 1.5× screen: 12-px tiles at x = 8 + 12c are byte-aligned in pairs (3 bytes per pair row):
  212k cycles per 96-tile screen instead of 1.22M for a generic shift blit. Compose the static
  screen into a private plane pair on changes only, copy it per frame (45k) and draw the
  moving sprites over it: 59k per play frame. The runtime's PC `draw_sprite`/`draw_rect`/
  `draw_clear` wrote to the screen planes, not `rt_light`/`rt_dark` like the TI's: fixed in
  `rt_sw.c` (all games' tests unchanged); `SwScript` raised to 4,096 lines, then 32,768 (walkthroughs).
- Program size: 59.5 KB for slice 1 with all data in the program (Titanium limit ~64 KB):
  for a game this size, put the generated data in an archived file (`rt_file`) from the start.
  Slice 2 did (`tools/data.py` writes the blocks at even offsets, words in both byte orders,
  `OFF_*` macros): program 25.2 KB, data file 37.4 KB.
- **Slice 2, the whole game, the same day**: enemies, the shot, game over, ending, awards,
  credits; ~300 lines of C. The original's main loop has no frame sync: an "iteration clock"
  runs the shot, enemy and hero updates every 5 frames, or at once on a fresh key press (2
  frames after the last at least) so input stays responsive without tapping speeding up the
  enemies. The original's stale-variable quirks (the shot hit test against the last room's
  enemy) come for free when the C keeps the same variables and order.
- **A bot that finishes the game is the walkthrough**: BFS over (room, arrival tile) on the
  records' doors (a room can be cut in parts: Desolate 59 is unreachable from its east door),
  known codes, blocked doors; fight from 2+ tiles (closer, the bites drain the 100 HP over
  the route); one BFS per step because enemies move. It found that the notes' progression
  order was wrong (cartridge 4 needs life support first). 26,815 frames to the ending,
  18,952 key changes: the runtime's script limit went to 32,768 lines (PC and `ti-cycles`).
- Tests leave saves behind: remove the save at the start of each test and at the end, or the
  PC binary's replay of `keys/walk.txt` loads it and diverges at "New".
