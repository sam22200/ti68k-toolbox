# The big-game track: our own engine, the ROM measured, a roadmap

For a game too big to translate routine by routine: banked ROMs (MBC1/3/5, 128 KB and up),
CGB-only games, games of several hours (an RPG, an adventure, many level types). Verified on
Metal Gear Solid (GBC, 2 MB MBC5, `games/mgs/`, 2026-10-04). Same skill otherwise: the ROM
runs headless under PyBoy, PC first, TI last, the user decides through `/grilling`.

## 1. When (the default to recommend, the user confirms)

| Sign | Small game: 1:1 track (`SKILL.md` §4) | Big game: this track |
|---|---|---|
| ROM | 32 KB, ROM only | MBC, ≥ 128 KB, many banks of code |
| Code | ≤ ~5k SM83 instructions of game logic | tens of thousands, far calls between banks |
| Machine | DMG | CGB only (colour palettes, VRAM bank 1, WRAM banks, double speed) |
| Content | a few screens, one mechanic family | menus, many level types, story, items, text |

**Recommend this track by default for a big game**; put it to the user as grilling question 3
(fidelity) with the 1:1 cost spelled out. Bit-exact traces are out of reach here: the port is
*behaviour-equal* (same numbers, same feel), its own code.

## 2. Roadmap first

Write `games/<name>/ROADMAP.md` before any C: milestones that are each playable end to end on
the PC and the TI, tested and committed, from the smallest slice up (MGS: VR Lv.01 → VR Lv.02-05
with the level format decoded → menus → Snake's actions → weapons → the rest of VR → story area
by area). Each row names what it needs that the previous ones did not (a decoded format, a new
system). **Propose the first milestone** in the grilling (default: one level or one mission,
no menus, started directly by the injection door) and say what comes after.

## 3. Measure, do not translate

Each milestone's spec is a list of numbers measured on the running ROM, written in
`games/<name>/README.md` § Measured with how each was obtained:

- **Find the variables** by RAM diffs between key scripts (hold right vs nothing: the bytes
  that move are x), then confirm with pokes. Objects are structs (MGS: Snake at C5C0, the
  guard at C700, same layout: active flag, x, y as 16-bit words, direction, state). The OAM
  shows positions one frame late: find the "drawn position" copy for sprite captures.
- **Find the code that reads a variable** when a poke does not behave: `gbdis.py ROM --find
  fa LO HI` (every `ld a,(var)`), then `pb.hook_register(bank, addr, cb, ctx)` on each site
  and count which ones run during the action: the routine is there; `gbdis.py ROM BANK:ADDR
  N` reads it. (MGS: the goal test at 0C:4627 read this way in minutes.)
- **Measure rules on a grid**: from a saved state, poke the player at each point of a grid
  around an object, run 2 to 6 frames, record the outcome (MGS: the guards' vision boxes on
  a 2-pixel grid, the goal box). Print the grid as ASCII: boxes, cones and asymmetries show.
- **When pokes "do nothing", suspect a condition outside the rule**: MGS guards only see while
  on screen. The poke tests failed until the camera was brought up (walk the player so the
  game scrolls; poking the camera byte is overwritten).
- **Timelines**: run 1,000+ frames with the player parked out of the way and log (state,
  direction, position) segments: patrol speeds, waits, turns, all in one table
  (`extract.py` `patrol()` prints it on every extraction).
- **Movement**: hold each direction from a known spot until a wall stops it (box extents),
  turns from standing (frames before the first pixel), diagonals over 40 frames (the
  2-of-3 pattern), along a wall (the free axis' speed), 180° turns (the rotation sense).
- **Level data**: the BG map in VRAM holds a whole small level (≤ 256 × 256): render it at
  two scroll positions and merge (MGS: intro at SCY 0 + play at SCY 104 = 160 × 240). The
  game's own metatile/collision map is usually in WRAM (MGS: bank 5 D000, id + solidity
  nibble): find it by the density census of each WRAM bank. Decode the ROM's level format
  only when a milestone needs many levels (MGS milestone 2).
- **Sprites**: per frame, draw the OAM entries of one object (filter by CGB palette and
  distance to its drawn position) on a canvas anchored at its feet, keyed by (direction,
  animation step variable): one image per key, no hash soup. Compute one sprite window from
  the union of all frames (MGS: 16 × 28, 68 outline pixels clipped of 65 images).

## 4. CGB specifics (PyBoy 2.7, verified 2026-10-04)

- VRAM banks: `pb.memory[bank, 0x8000:0x9fff]` (the end 0xA000 raises: read 9FFF alone).
  Bank 1 holds the BG attributes at the map addresses: bits 0-2 palette, 3 tile bank,
  5 x-flip, 6 y-flip, 7 priority. Sprites: flags bit 3 = tile bank, bits 0-2 = OBJ palette.
- Palettes: write the index to FF68 (BG) / FF6A (OBJ), read FF69 / FF6B: 8 palettes × 4
  colours of 15-bit BGR. WRAM banks: `pb.memory[b, 0xD000:...]` for b = 1..7 (SVBK FF70 tells
  the current one; game variables at C000-CFFF are always there).
- Colour to 4 greys: one table per scene, colour → grey chosen by role (floor light, walls
  and shadows dark, outside black, the hero dark and the enemies light, white outline on
  both); a colour missing from the table stops the extraction (a new scene needs a look).
- MBC ROMs: Ghidra + GhidraBoy analyse bank 0 only (MGS: 48 functions of 2 MB). Use
  `scripts/gbdis.py` (PyBoy's opcode table, bank-aware addresses, `--find` patterns) on the
  routines the hooks point to; banked far calls in MGS are `rst 10` with HL = address,
  B = bank.

## 5. Build and check

- `games/<name>/tools/extract.py`: from the local ROM, a fixed key script to the milestone's
  start (no hand-saved states: regenerate them), then the data (`level.h`, `gfx.h`, a data
  file for tiles and sprites in both byte orders), review sheets in `x/`. Everything it writes
  is gitignored.
- Engine: one logic step per GB frame, two per TI frame at 30.1 fps (measured timings hold
  as they are; MGS logic: ~3k cycles per TI frame).
- Tests: the measured numbers as unit tests (walls, speeds, timelines, rule grids on both
  sides of every edge), a winning key script and a losing one, the injection door scenarios.
  TI = PC by `xcheck` (screen checksum per scenario) and `tihash` (a per-field state hash per
  frame: never hash the raw struct, byte order and padding differ on the 68000).
- **Screens**: no comparison against the ROM's screens (the port's differ by design: camera,
  greys, HUD). One review PNG per milestone (`--shot`), read by eye, and one emulator run.
- **The emulator run and its GIF**: play the winning key script in TiEmu with `ti-play`
  (keyboard keydown/keyup: diagonals) while `ti-gif` records, then ESC back to HOME. It is open
  loop and TiEmu's speed drifts: the MGS route timed to the frame (stop going up at y 78, a
  0.13 s window) was spotted on the first try. Rewrite the route so every move ends against a
  wall or slides along one (hold RIGHT to the wall, then UP+RIGHT slides up the corridor's wall
  into the side room by itself), and check it on the PC with the whole timeline scaled by 0.9
  to 1.1 before the emulator: the second try cleared the level.
