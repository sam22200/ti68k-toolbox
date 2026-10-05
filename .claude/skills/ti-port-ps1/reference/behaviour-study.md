# The behaviour study: learn a mechanic from the running game, rebuild it small

Reverse engineering here is a tool for learning, never the goal. The output of a study is a
short behavioural description and a table of measured numbers, from which a much smaller
system is designed for the TI. Large MIPS functions are never translated to C.

## RE_NOTES.md (games/<name>/RE_NOTES.md, kept up to date throughout)

```
# <Game>: reverse-engineering notes (<mechanic>)
## Decisions          the grilling's agreed constraints (date), the simplifications chosen
## Tools              disc, executable, boot key script and frames, saved states and how to
                      regenerate them, the psxrun commands used below
## Addresses          RAM: address, size, signed, name, meaning, how found (diff/trace/read)
                      Code: address, our name, what it does in one line
## Structures         the player/object struct as far as known: offset, size, meaning
## Experiments        one entry per experiment: question, command, result, conclusion
## Behaviour          per point: OBSERVED / LIKELY INTERPRETATION / TARGET IMPLEMENTATION
## Numbers            ORIGINAL (logic frames, units, seconds) | TARGET (TI frames, pixels) | why
## Open questions     uncertainties, hypotheses not yet tested
```
Also `games/<name>/<name>.sym` ("800f1234 player_x") for `psxrun.py --sym`.

**The three labels**, on every finding, so original details never become requirements:
```
OBSERVED IN ALUNDRA: the player's height is a separate word that changes during a jump while
  x and y keep following the pad.
LIKELY INTERPRETATION: vertical motion is independent of planar motion.
TARGET IMPLEMENTATION: x, y in pixels (u16) + z (u8) + a 12-entry jump table of z offsets.
```
Numbers likewise: "ORIGINAL: the jump lasts 24 logic frames at 30 Hz (0.8 s). TARGET: 16
frames at 30 fps (0.53 s): a 16-pixel tile is crossed in fewer frames, a shorter jump reads
better" — always the original, the target, and the reason.

## Finding the variables (dynamic first)

1. A saved state in a scene where the player is free (from the boot key script).
2. **RAM diffs**: dump at rest, after holding RIGHT N frames, after N more (`--dump`), plus a
   dump after waiting the same time without input (`--same`): `ramdiff.py rest.bin r1.bin
   r2.bin --same idle.bin` leaves the bytes that move with the input only. Repeat with DOWN
   for y, with the jump button for z.
3. **Trace** the candidates per frame during a scripted action (`--trace a:s2,b:s2,c:s2`,
   `--every 1`), with a GIF of the same run to tie numbers to pictures.
4. **Poke** to confirm (`--poke 800f1234:2=100`): the player moves there, or not (some
   values are copies rewritten every frame: search the dump for the same value and poke each
   candidate; Alundra: the display copy at 801339a0 ignored pokes, the master was 801ac80c).
   Teleport with z too, set high: the player falls onto the floor of the new spot.
5. **Maps**: `psxgrid.py` (one state reload per point, pokes, `--settle`, `--hold KEY`) prints
   the floor height or where a move ends on a grid: heights, ramps, walls appear as ASCII.
6. Neighbours: the struct around a found field usually holds the rest (velocities, state,
   facing, animation, the height of the ground under the player). Trace the whole struct
   (`LO-HI` one byte each, or `:2` words) once with `--diff` during a jump: what changes when.

## Reading the code (static, one question at a time)

- From a field's address or struct offset to its writers: `disasm.s` (see `ps1-facts.md`
  § Reading MIPS), then the pseudo-C of the function that contains them. Rename it in the
  Ghidra project, re-export, note it in RE_NOTES § Addresses.
- Ask one question, read until it is answered, write the answer in plain words:
  "the landing test compares the new z with the floor height of the tile under the player's
  feet; at or below → z = floor, state = ground". Not the code.
- If the code is not in `decomp.c`, it is in an overlay: decompile a RAM dump taken in that
  scene (`ps1-facts.md` § Executables and overlays).
- Collision resolvers of top-down games (Alundra `FUN_80038008`): the wall test fills **one
  flag per box corner**; the resolver halves a blocked step until it fits, then, when nothing
  fitted, switches on the facing direction (often 0-31, cardinals at multiples of 8). That
  switch is the corner rule: which front/back corner flags stop the move, which turn it into
  a sideways nudge (constants like 0xc000 / 0x8000 = 0.75 / 0.5 px in 16.16) and which drop one
  axis of a diagonal. Read it case by case and write the rule per direction.

## Testing a rule on geometry you make

Once the map format is known (cell address, height and flag bytes), poke cells next to the
player in a saved state to build exactly the test case (a +1 step, a +2 step, a flagged edge)
and walk or jump into it. Faster and cleaner than finding such a place in the game, and it
separates the rule from the level's decoration (walls, railings).
Poke **only the map**: a poked player position freezes the player near anything solid
(Alundra keeps per-corner cell pointers that only a real move updates). Reach the exact
position by walking (a few frames of a direction in `--keys`), then hold the test direction.
A corner test needs the box to straddle two cells: block one of them (corner nudge) and both
(flat wall) and compare. Alundra's corner rounding was verified this way in two runs.

## Measuring a traversal mechanic (the list for movement, jumps, heights)

Each with the key script and trace that measured it:
- Walking speed per axis and diagonal; acceleration or immediate; the logic rate.
- Jump: start (button press to the first z change), z per frame up to the apex and back,
  total duration, apex height, horizontal distance standing/walking, steering in the air
  (direction changes allowed? speed?), jump buffering, jump again on landing.
- Heights: what a "height" is in the data (per tile? per collision box? continuous?), the
  step between levels, the highest ledge a jump clears, what happens when it does not
  (blocked like a wall? slides? lands on the edge?).
- Walls during ascent and descent; corners; narrow gaps (the collision box size from the
  stops against walls, as in MGS).
- Falls: walking off a ledge (automatic drop?), the fall speed, a landing pause or damage from
  high falls.
- Depth: the player in front of / behind raised terrain, the shadow (does it stay on the
  ground under a jumping player?), the draw order with objects.

## Stop rules

- Stop studying when the next milestone's questions are answered; resume for the next one.
- Never study combat, enemies, AI, dialogue, inventory, menus, saves, sound, story scripts,
  the world map, shops, spells, cut-scenes, unless one turns out to drive the mechanic (then
  note why).
- A subsystem that keeps growing: write what is known, mark the rest as an open question,
  move on with a simpler target design.
- Exact numbers are welcome when cheap; when they cost hours, an observed approximation
  ("the jump clears one level, never two") is enough.
