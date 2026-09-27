# TI-68k game techniques: algorithms that fit a 12 MHz 68000

The best techniques found in the old sources in `sources/` (index: `sources/README.md`), compared
with one another. For each topic the **recommended** version comes first, then what the sources did
and why it is worse. Platform facts (interrupts, keyboard, files) are in `ti68k-c-patterns.md`,
measured costs in `ti68k-performance.md`. Nothing here is verified in the emulator unless marked
**verified**; the rules of `ti68k-performance.md` (16-bit types, tables, no division) apply
throughout.

## 1. Game-tree search (board games, turn-based AI) — from TI-Chess

Budget: **~200 nodes/s** with a full evaluation per node (TI-Chess `history.txt`). Every trick
below exists to search fewer nodes.

- **Iterative deepening** (depth 1, 2, 3… until the time is up): doubles as time control, and each
  iteration's best move orders the next one. Keep the best root move at index 0.
- **Root aspiration window**: search the first root move with (previous score ± 100); if the
  result falls outside, search it again with a full window. Other root moves: **null window**
  `(alpha, alpha + 1)`; only a move that beats alpha gets a full re-search and becomes the new first.
- **Lazy move selection instead of sorting**: give each generated move a score, and at each step
  scan for the best remaining one and mark it used (`value = -INF`). Most nodes cut off after one to
  three moves, so the rest of the list is never ordered.
  Ordering scores: best move from the previous iteration +500, killer 1 +250, killer 2 +150,
  capture = victim − attacker/8, recapture on the square just moved to +300.
- **Quiescence in the same function**: at depth ≤ 0 generate only captures/promotions, use the
  static evaluation as a lower bound (stand pat), stop at depth −5. Extend by one on check or on a
  recapture near the window, capped (+3).
- **Pseudo-legal moves**: do not test legality when generating; a move that leaves the king in check
  is caught one ply later (the evaluation sees the king capturable and returns `MATE - depth`, which
  also prefers faster mates).
- **Copy-on-make per ply**: keep the incrementally updated values (material, flags, en passant…) in
  arrays indexed by depth; making a move writes `[d + 1]`, unmaking costs nothing. For counters that
  must be undone, one update function taking `+1` or `-1` serves both make and unmake.
- **Board with sentinel border** (10×12 mailbox with `OUTSIDE` squares): sliding moves run until
  they hit a non-empty square, no bounds checks; one direction table shared by all pieces.
- **Zobrist keys from a small seed**: TI-Chess expands 55 stored `long`s into 768 64-bit keys at
  start-up with a lagged additive generator (`y[k] += y[j]`, lags 24/55) instead of storing 6 KB;
  the PC tool runs the same generator, so both sides agree. 32-bit keys are enough for a table of a
  few thousand entries (half the XOR/compare cost).
- **Transposition table** keyed by the hash (depth, bound, best move) is better than TI-Chess's
  evaluation-only cache, whose cached values depend on search-path state not in the key (a bug).
- **Poll during the search** with count-down counters, not `%`: keys every ~20 nodes, clock every
  ~50; propagate an `abort` flag up the recursion. Keep dialogs (and their screen buffers) out of
  the recursion.
- Repetition check: scan back only to the last irreversible move, two plies at a time (TI-Chess
  scans the whole game history at every node).

## 2. Cheap evaluation AI (real-time games)

- **Tetris AI** (tetrisc, clears 50 lines faster than its author): for each of the 11 columns ×
  4 rotations, drop the piece in a copy of the board and score it: **+4 per empty cell below the
  lowest block of each column (holes) + Σ |height[c] − height[c+1]| (bumpiness) − bonus per
  completed line**; take the lowest score. 44 evaluations once per piece, no search.
- **Formation AI** (ISS89 football): each AI player follows the ball's x but is clamped to
  `home ± range` (attack/defence), which gives believable positioning for a few instructions.
- Free jitter: use the low bits of the tick counter (`ticks & 7`) as a cheap random offset.
- Spread the AI over frames (one unit per frame, round-robin) when there are many units
  (`ti68k-performance.md` §8).

## 3. Grid puzzles (Tetris, Jezzball, Snake)

- **Logical board, never the screen** (tetrisc `char mat[22][13]` beats tetris_src's `GetPix`
  tests at ~600 cycles each), with **sentinel walls** (column 0 and 12, the bottom rows set to 1) so
  collision and hole-scanning loops need no bounds checks.
- **Pieces as bit masks**: one `unsigned short` per piece and rotation (4×4 cells = 16 bits,
  `rot[7][4]` = 56 bytes), board rows as bit masks: collision = 4 row ANDs, a full row =
  `row == FULL`. Keep a separate tile-ID table only for drawing (both Tetris sources store
  `tableau[28][16]` of tile numbers so the I piece is drawn as one continuous bar).
- Rotation: `next = (rot + 1) & 3`, rejected if it collides (try a one-cell kick left/right first
  for a nicer feel).
- **Area capture** (Jezzball): the original assumes the enclosed region is a rectangle and fills
  L-shapes wrongly. Correct and cheap: a coarse cell grid (e.g. 2×2 px per cell, bit-packed), BFS
  from every ball with a static queue, then every unreached cell is captured: fill it
  (`FastFilledRect_Draw_R`) and add it to a running count; percentage = count × 100 / total once per
  capture.
- **Snake body as a ring buffer**: `unsigned char bx[256], by[256]`, head index `h`, tail =
  `(h - length) & 255`; erase the tail cell, draw the head. Nibble's linear `x_record[3000]`
  overflows after 3000 moves.
- **Bit-packed levels**: 1 bit per 8×8 cell, a 160×100 screen = 20×12.5 cells → 16 bytes per level
  with `unsigned short` rows (Nibble: 160 bytes for 10 levels). Decode with
  `(lvl[row] >> (15 - col)) & 1`, not by drawing and reading back pixels.

## 4. Pathfinding (A*) in little RAM — from path.c, corrected

- `status[y][x]` byte per cell: open/closed bits + the **parent direction in 3 bits**: no closed
  list, no parent pointers, path reconstruction by walking directions back from the goal.
- Open list as an array sorted by **decreasing** F, so the best node is popped with `n--` (O(1));
  insert by a linear search and shift (fine for a few hundred nodes). When an open node's G
  improves, **move it** to its new place (path.c forgets and breaks the order).
- Heuristic with 8-way moves costing 10/14: **octile** `10 * max(dx, dy) + 4 * min(dx, dy)`
  (admissible). Manhattan × 10 overestimates diagonals and gives non-optimal paths.
- Test the goal when it is **popped**, not when it is inserted (else the path is not optimal).
- Diagonal moves: forbid cutting corners (both orthogonal neighbours must be free).
- Border walls around the map remove bounds checks. All arrays `static`, never on the stack
  (path.c puts 5.8 KB there and `malloc`s a 100-entry list that can receive 240).
- Time-slice: expand a fixed number of nodes per frame and continue next frame.

## 5. 3D on a 68000

Recommended pipeline (combining the best of the 3D tutorials, edit3d and our tables):

1. Keep the **original vertices** and transform them fresh every frame (never rotate the rotated
   copy: rounding accumulates and the object deforms).
2. Build **one 3×3 rotation matrix per frame** from `sin_tab` (×127): 9 products at frame level,
   then **9 `muls.w` per vertex** and a single `>> 7` per coordinate (round: add 64). Chaining three
   per-axis rotations (3D tutorial ex5) costs 12 multiplies and three roundings per vertex.
   **Verified**: 1,095 cycles per vertex with `muls16` (1,001 with 16-bit products for small
   models) against 1,706 for the chained rotations.
3. Rotate around the object's origin, then `z += distance` (simpler and exact, unlike the
   tutorial's orbiting-camera formula).
4. Project with a **reciprocal table**: `recip_tab[z]` for the known z range, then
   `sx = cx + (muls16(x, recip) >> SHIFT)`: **341 cycles per vertex against 490 for two `divs.w`
   (verified)**. Write the products with `muls16`: `recip` is shared by the x and y products, and
   plain C then calls `__mulsi3` (968 cycles, slower than dividing; performance §4).
5. **All 16-bit**: edit3d stores vertices in `long` and multiplies `long`s, so each product is a
   `__mulsi3` call: **3,734 cycles per vertex against 1,095 (verified)**.
6. **Near plane**: clip edges against `z = zmin` in 3D before dividing. Skipping the vertex
   (edit3d) leaves stale screen coordinates that lines are still drawn to; no check at all (ex1–6)
   divides by zero.
7. **Back-face culling in screen space**: `(long)(x0 - x1) * (y2 - y1) - (long)(x2 - x1) * (y0 - y1) > 0`
   → hidden. Two multiplies per face, correct under perspective. Write the `(long)` casts: the
   `short` products overflow above ±128 and flip the sign. Use three non-collinear vertices.
8. **Painter's sort**: bucket (counting) sort on an 8-bit depth, static buckets, one pass; or
   insertion sort starting from **last frame's order** (frame-to-frame coherence: almost O(n)).
   Never allocate per frame (edit3d's radix sort does 4 `calloc`/`free` per frame), and do not store
   depth in an `unsigned char` that wraps.
9. Fill with ExtGraph `FilledTriangle_R` / `GrayFilledTriangle_R` + `DrawSpan_*_R`; wireframe with
   `ClipLine_R` + `FastDrawLine_R`. No z-buffer: a 16 KB buffer cleared every frame costs ~66k
   cycles before drawing anything.

- Compact models (edit3d): store objects as 2D `char` profiles plus an extrusion/revolution flag and
  generate the faces at load time.

### Portal rendering with lines only — from X3D (catastropher, `sources/x3d-68k/2015-08/src/shared/`)

For indoor levels (Descent-like), the best approach seen on the 68k. Unverified here: the numbers
are the author's (CodeWalrus thread, `sources/web/codewalrus-570-x3d.txt`).
- **Level = convex rooms** ("segments": prisms, two n-gon bases) sharing faces ("portals"). Each
  room is drawn as its **edges only**, clipped to the current view frustum; for each portal face:
  skip it if the camera is behind its precomputed plane (3 multiplies), clip the face polygon to
  the frustum, build a new frustum from the clipped polygon, recurse into the next room. Convexity
  makes it look solid: **no fill, no sort, no z-buffer**. The author: 15–20 fps with 10 portals
  visible (old cube version), "a single room in 2/256 s"; solid mode is *faster* than wireframe
  because hidden rooms are never visited.
- **Frustum planes pass through the eye**: in camera space their `d` is 0, the inside test is the
  sign of a 3-term dot product with 1.15 normals (only the near plane has a `d`).
- **Outcodes per vertex, not per edge** (`clip.c:271-310`): compute each vertex's distances to the
  planes once, one bit per plane outside; reject an edge if `out[a] & out[b]`, draw it unclipped if
  both are 0. A prism has 2n vertices and 3n edges: this halves the work of clipping faces.
  **Bug to avoid** (`clip.c:591-599`): an edge outside through two *different* planes passes the
  AND test; after clipping, check the clipped segment's midpoint against all planes, or it draws
  stray diagonals (the author's "glitches").
- **Sutherland–Hodgman early exit**: a convex polygon crosses a plane at most twice; after the second
  crossing copy the remaining vertices without more dot products (`clip.c:727-744`).
- **Screen-space alternative (author's later design, forum only)**: store each portal's visible
  region as `x_left[y]`/`x_right[y]` spans; child ∩ parent = per-row max/min; clip a line by
  **bisection** (midpoint = 2 adds + 2 shifts, O(log L), no multiply or divide) against the span
  table. The same spans feed `DrawSpan`/`GrayFastDrawHLine` for filled or grey rooms in one pass.
- **Frame stamp instead of clearing flags**: each room stores `last_frame`; skip it if equal to the
  current frame counter. Incrementing the counter "clears" every flag at once (reusable for BFS,
  AI, collision "seen" marks).
- **Doorway case**: within ~100 units of a portal plane, reuse the parent frustum minus its near
  plane instead of building a degenerate one (`render.c:197-225`).
- **Cap the recursion depth**: each level `alloca`s a frustum and two polygons (~400 bytes); the
  stack is small (patterns §9).
- **Collision without BSP** (`collide.c`): inside a room = n dot products with its face planes; on
  a hit push out along the normal; crossing a portal face moves the object into the next room
  (objects may belong to up to 4 rooms). Bounce `v − 2(v·n)n`, slide `v − (v·n)n`.
- **Objects are drawn during their room's pass** with that room's frustum: line objects are clipped
  by portals for free; sprites would need clipping and a depth sort.
- **Level storage**: variable-size room records in one pool, addressed by `uint16` **offsets**, not
  pointers (the blob can be saved as one data file and read in place from the archive); each face
  stores its neighbour id and precomputed plane; even record sizes (68000 alignment). A
  `draw_edges` bitmask per room suppresses edges shared with a neighbour (drawn twice otherwise).
  Editor: extrude the selected face along its normal to add a room.
- **Precision**: rooms 600 units wide need **1.15 fixed point** (sine table in `short` up to 32767,
  32-bit sums then `>> 15`): the ×127 matrix of step 2 jitters by ~5 units at that scale and its
  16-bit-product trick only holds while |coordinate| < ~86. Rule: ×127 for small models, 1.15 for
  level geometry and the camera.
- **Hidden divisions**: X3D projects with `((long)x * scale) / z`, a `__divsi3` call twice per vertex.
  **Verified** (bench3, HW3): 1,259 cycles per vertex for two `long / long` projections, **623** with
  `divs32_16` (`ti68k-performance.md` §4), 341 with a reciprocal table (best when z has a small range).
- **Depth cue / fog idea**: in grayscale, draw far edges in the light plane only.
- The author moved from a greyscale engine ("way too slow") to mono lines, and brought grey back only
  once clipping was fast. He also saw TiEmu grayscale glitches absent on the real calculator: do
  not over-trust grayscale screenshots of fast plane switching.

## 6. Raycasting and texture strips — from TICT S2P1/S2P2

- Grid squares of **64 units**: map cell = `pos >> 6`, texture column = `pos & 63`.
- Per ray, the steps between grid intersections are constant: take them from tables over one
  quadrant (symmetry for the others), ×256 fixed point; cap tan near 90° (it overflows a `short`).
- Choose an **angle unit where one screen column is an integer number of units** (e.g. 1152 units
  per turn for 96 columns over 60°); integer degrees are too coarse.
- Fisheye: multiply the distance by `cos(ray − view)` from the table. Distance → wall height with
  `recip_tab`. Products `(long)(short)a * (short)b` (one `muls.w`), never `long * long`.
- **FAT engine (TICT, `sources/fatsdk`)**, the reference raycaster of the platform: one generated,
  fully unrolled asm scaler **per strip height** (up to 96), dispatched through a jump table indexed
  by the height: no loop, no accumulator, no branch per pixel, for ~8 KB of code. The extreme
  version of the next point when code size is affordable. Its `GetArcTanYX` (binary search in a
  tan table) loses to our `atan2_8` (one table read after the division). FAT is loaded as a DLL
  (needs HW3Patch on the Titanium, patterns §4); Doom89 is built on it.
- **Scaling a texture strip**: an 8.8 fixed-point step `step = recip_tab[h]`, then per pixel
  `acc += step; texel = col[acc >> 8]`. TICT's alternative, a precomputed table of source offsets per
  height, is 4.2 KB with `short` deltas; `unsigned char` deltas halve it. Store textures
  **column-major**, one byte per texel holding both plane bits, so a strip is one byte read per
  pixel.

## 7. Two players over the link cable — lessons from Pong 2P

- AMS `OSWriteLinkBlock(buf, n)` / `OSReadLinkBlock(buf, n)` (non-blocking, returns the byte count)
  are interrupt-driven through **auto-int 4**: never mask it (`OSSetSR(0x0400)` kills the link),
  keep int 4 untouched when you redirect other vectors.
- **Drain the receive queue every frame** (`while (OSReadLinkBlock(&b, 1)) last = b;`): reading one
  byte per frame while the peer sends faster makes the lag grow without bound (Pong's "delay").
- Check the return count and clear the buffer: with no data, Pong parses stale bytes.
- **Frame the messages** (type byte + payload, or values in a range that cannot collide with
  control codes) and **synchronise the game state**, not only the inputs: Pong's two calculators
  each simulate their own ball and drift apart. Deterministic lockstep (exchange inputs, then both
  step) or one authoritative side.
- Handshake: a PING → PONG → ready state machine with timeouts elects server and client.
- `ngetchx`/`kbhit` let AMS look at incoming link bytes (silent link): keep menus off the link.
- **F-Zero's link desync, root cause**: each side sends one key-state byte per physics step and
  both simulate both cars from the exchanged inputs (lockstep), with a single-byte receive buffer,
  no sequence number and no resync: one lost or merged byte shifts one side by a step forever.
  Lockstep needs a step counter in every packet (wait or resend on mismatch) and a periodic
  full-state packet (positions, speeds) to repair drift.
- **Tankers** does it better: every tick it exchanges the full authoritative state (position,
  direction, shots, tile changes) with a timeout.
- Untested here (TiEmu linking not set up); test on both models before relying on it.
- **Split ownership** (ExciteBike, Hockey, T. Fischer): each calculator is authoritative for its
  own objects (the host also owns the AI ones); every frame exchange one status byte (pause, quit,
  finish) plus a few bytes per owned object. Put the networked fields **first in the struct** and
  send only that prefix. Implicit sync: the host sends then receives, the joiner receives then sends.
- **Handing over a shared object** (Hockey puck): the side whose player holds it sends its state;
  on release the last holder sends one more frame; if both claim it, the host wins.
- **Host election**: whoever arrives first is host; if the waiting host receives "I am host" from
  the other side, the other side lost its first byte (AMS ate it before the game opened the link),
  so become the joiner. The host then sends settings plus a random seed.
- **Timeouts need AMS's int 5** (**verified**, HW2): `LIO_RecvData(&b, 1, 20)` with no cable returns
  an error after 1 s with the AMS int-5 handler, but **blocks forever** (until ON) when int 5 is
  replaced by `DUMMY_HANDLER` or a non-chaining clock: its timer (`LIO_TIMER`) is driven by int 5.
  ExciteBike, Hockey and SlimeBall all have this bug. Keep AMS's int 5 (use int 1 for your clock)
  or chain to it, or do your own timeout with non-blocking `OSReadLinkBlock` + your tick counter.
- **Bandwidth**: Hockey sends ~130 bytes in ~16 blocking `LIO_*` calls per frame (full 16-byte
  structs with constant fields): the game slows down in link mode. Pack only the changing fields
  (x, y in 9 bits, velocities in a byte, flag bits: ~6 bytes per player) into **one buffer and one
  call**. Sumo's lockstep sends only 2 bytes per frame (`{status, decoded move}`) after a seed
  exchange; every `random()` call must then happen in the same order on both sides.
- **Menu barrier** (Hockey `Align_Calcs`): the host sends a magic byte and waits for the answer; a
  joiner that already saw it replies at once; −1 = quit in every loop. Magic handshake bytes per
  version keep old versions from linking with new ones (ExciteBike).
- **Exchange world coordinates**, never screen ones (ExciteBike had to patch y by ±28 between a
  TI-89 and a 92+).
- **Link errors**: do not call the main menu recursively from deep in the game (Hockey: the stack
  grows with every link error); `ER_throw` to a `TRY` around the menu loop.

## 8. Mode 7 (pseudo-3D floor) — from Mode7 Engine and F-Zero

- **Precompute the whole projected floor grid** once: `Vert[row]` (distance term per screen row)
  and `Horz[row][col]` (horizontal offset per pixel, already divided by that row's perspective):
  per pixel it is then `u = (cam_x + *horz++) >> ZOOM` (+ one rotation per row with `sin_tab`): no
  division and no accumulated error. A 40×25 grid costs 2 KB of `short`s; the engine reaches
  ~24 fps at that resolution.
- Low resolution with **pixel doubling built into the sampling**: OR pre-shifted 1/2/4/8-bit masks
  (`0x8000`, `0xC000`, `0xF000`, `0xFF00`) into one 16-bit word per sample group, write the word,
  and write it again 30 bytes lower for vertical doubling: no separate scaling pass.
- Large worlds as a **grid of tile pointers** (`MapTiles[]`, fixed-size blocks): tile = coordinate
  `>> TILES_SHIFT`, offset = `& (TILES_SIZE - 1)`; with a fixed map width of 1024 the row multiply
  becomes a shift.
- Sky: a wrapping backdrop copied as two runs per row (before and after the wrap point), the wrap
  offset reduced once per frame, never per pixel.
- Huffman-compressed maps (F-Zero): better ratio than RLE, bit-by-bit decode: fine at level load,
  never per frame.
- **Verified** (`experiments/bench/bench3.c`, HW3): one 40-pixel row costs **4,315 cycles with the
  precomputed grid** (108 per pixel) against 6,080 with incremental 8.8 u/v stepping (152 per
  pixel). A 40×25 frame is then ~110k cycles of sampling, consistent with the engine's ~24 fps
  once drawing is added.

## 9. Roguelikes and tile RPGs — from CalcRogue, Zelda, Crystal Engine

- **Dungeon generation** without recursion, fixed memory (CalcRogue `mkmap.c`, 64×20 map): place
  random rooms with a one-tile margin (restart after 1000 failures), label each area in the tile
  byte, connect areas by picking a random row/column and digging where it crosses two different
  labels, relabel, repeat (restart after 750 failures); doors from a 3×3 neighbourhood rule (1 in 5
  secret). Caves: 45 % random walls + 7 passes of "wall if ≥ 5 walls in the 3×3", using a spare
  byte of each cell as the second buffer.
- **Bit-sliced cellular automaton** (T. Finch's Life, dotat.at/prog/life): store the map as bits
  (32 cells per `long`) and count neighbours for 32 cells at once with adder logic. Per row:
  `L = (c >> 1) | (prev << 31)`, `R = (c << 1) | (next >> 31)`, horizontal 3-sum as two planes
  `s0 = L ^ c ^ R`, `s1 = (L & c) | (R & (L ^ c))`; add the three rows' sums with full adders and
  test "≥ 5" with AND/OR (code: `experiments/bench/bench7.c`, `ca_bits`). **Verified** on 160×100
  (identical grid to the per-cell version): 431k cycles per step vs 5.3M per cell, **12× faster**
  (~465k real: the `<< 31` is a 31-bit shift TiEmu undercounts). Seven cave passes: ~0.3 s instead
  of 3 s. Works for Life, fire/fluid spreading, erosion, any rule on neighbour counts.
- **Field of view**: bound the scan to the light radius's bounding box (CalcRogue scans the whole
  map, its own to-do list calls lighting slow), Bresenham to each candidate; when the target is
  opaque, test the tile one step towards the viewer first (fixes wall corners).
- **Monster AI without pathfinding**: in sight (≤ 12 tiles) or hearing (≤ 4), step to the
  8-neighbour minimising the squared distance to the player, else wander. Good in open rooms; use
  A* (§4) for mazes.
- Content authored in a DSL (m4 macros → packed data with offsets, CalcRogue `tools/mkdat`): the
  same source feeds the calculator and a desktop test build. A Python generator does the same today.
- Screen-to-screen scrolling (Zelda) with `memcpy`/`memset` costs ~78k cycles per step; with
  `FastCopyScreen_R`/GrayDBuf it is a few thousand.
- Big games: split data into files first (patterns §10), DLLs only if code itself exceeds the limit
  (patterns §4: verified, no unpatched binary loads a DLL on both models). An engine/game split through a function table must be **append-only** (FAT: new
  entries only at the end, so old games keep working with a newer engine).

## 10. Physics and animation details

- Ball height from elapsed time: `z = vz * t - g * t²` (ticks since the kick), drawn at `y - z`
  with a shadow sprite at `y` (ISS89): a convincing 3D ball for a few multiplies.
- Animation frame = `base + (ticks - t0) / k` (better: `>> n`): tied to time, not frame count.
- Team or state markers: OR a small overlay sprite over a shared sprite instead of storing
  variants.
- Diagonals: read both direction bits from the same `_rowread` row.
- **Bounce off a round player from a zone table** (SlimeBall): the rebound vector comes from a
  5-entry table indexed by where the ball hit (plus a small random jitter), not from a reflection
  formula; move the ball in **2 sub-steps per frame** so it cannot tunnel through. (SlimeBall does
  its physics in `float`: use 8.8 fixed point.)
- **Inertial steering for free** (Hockey AI): `vel += sign(target - pos)`, a small friction step,
  a speed cap. Goalie reacts only when the puck moves towards its goal; the hard AI aims at the
  half of the goal the goalie does not cover.
- **Fighting-game AI** (Sumo, "SF2TI recipe"): choose an action by distance band (near, medium,
  far) with weighted random choices; difficulty narrows the random range (`random(18 - (lvl << 3))`).
- **Racing AI** (ExciteBike): look at the track tiles 28 and 52 pixels ahead to change lane; speed
  cap per level; AI bikes more than 2 screens away are moved ±4 screens so the race always looks
  crowded (rubber-banding).
- **Push a chain of blocks** (Strategery): move only the first block to the free cell at the end of
  the chain: 2 cell updates whatever the length.
- **Win check after a move** (Connect Four): test only the 4 lines through the last piece, never
  rescan the board.
- **Destructible terrain (Worms68k, `sources/worms68k/c/`)**:
  - Map 320×200 in two column-major planes (16 KB, `malloc`): `light` = land, `dark` = texture
    and outline; drawn as `OR light→L; XOR dark→L; OR dark→D` so 2 stored bits give light grey
    (1,0), dark grey (1,1) and a black edge (0,1).
  - Outline 32 pixels at a time: `c = l | d; edge = (c ^ above | c ^ below | c ^ (c << 1) | c ^ (c >> 1)) & ~c;`
    texture for free: `dark |= tex[y & 31] & mask`. Terrain from per-column random walks that keep
    a direction for `random(5) + 5` columns (smooth hills).
  - Explosions: carve per row with a span `half = isqrt_tab[r² − dy²]` and AND the strips with
    `(~0UL >> a) & (~0UL << (31 − b))`; Worms uses a `malloc` + dozens of AMS `DrawClipEllipse`
    calls per blast (slow). Keep its burnt rim: a 2-pixel ring ORed where land remains.
  - Collision: 4 probe offsets per object (up/down/left/right), step back pixel by pixel to air
    (max 25). Tunnels at 10 px/frame: sub-step. **Walking up slopes**: if `(x + dir, y − 6)` is
    air, scan down ≤ 6 px for ground and snap to it, else it is a wall: two pixel tests per step.
  - Raycast by fixed-point DDA one pixel along the major axis (guns, spawn search).
  - **Sleeping objects**: after 6 frames with |v| ≤ 2 an object is "settled" (bit in a `u16`
    mask) and skipped by physics; the turn ends when all masks are quiet.
  - **Behaviour as data**: 77 weapons described by `unsigned long` flag words (`usesAim`,
    `usesWind`, `usesPhysics`…) driving shared code (the "mix-in" system).
  - Camera follows a *pointer* to the target (weapon > explosion > worm) with easing (use
    `d - (d >> 2)`, not Worms' float 0.3); aim from a 10-entry quarter-circle table mirrored.
  - Worms' anti-patterns: floats in camera, physics and aiming; signed `% 32` / `/ 32`; no frame
    limiter; `Draw.c:183` `&=` where a test was meant.
- **Friction must clamp to zero and round symmetrically** (Hockey puck, reproduced in a simulation
  of its code): velocity in 1/32 px, `v += ((v < 0) - (v > 0)) * 2`, `x += v >> 5`. `>>` rounds
  towards −∞, so −1…−31 still move 1 px while +1…+31 move 0: a shot to the left travels 37 %
  farther; an odd velocity never reaches 0 and oscillates ±1, the puck creeps left forever. Use
  `if (|v| <= f) v = 0; else v -= sign(v) * f` and keep the position in the fixed-point unit
  (`x_fp += v; x = x_fp >> 5`), never `x += v >> 5` (ExciteBike's speeds 32–63 all move exactly 1 px).
- **Wall bounce with `ABS`**: on hitting the left wall `vx = ABS(vx)`, right wall `vx = -ABS(vx)`
  (Hockey), not `vx = -vx`: two collisions in one frame cannot send it back into the wall.
- **Integer speeds with counter friction** (Hockey skaters): whole px/frame capped at ±3, keys add ±1
  per frame, friction removes 1 only every 4th or 5th frame (a counter): no fixed point needed at
  such small speeds. Speed cap with jitter `max + (random(2) << fast)` makes the AI less robotic.
- **Camera with a lead, no division** (Hockey): target keeps the puck 60 px from the left when it
  moves right (100 px when it moves left); if the camera is already past the target move by the
  puck's displacement, if a double step would overshoot snap, else move by 2× (catch-up). Driven by
  the puck's movement since last frame. Sumo instead recentres with hysteresis (5-px margin).
- **Angle = sprite index** (ExciteBike): 16 frames make a full turn; a flip is `tilt = (tilt + 1) & 15`,
  a crash tumble `frame = counter & 15`; landing compares the player's tilt with the frame the slope
  wants: a mismatch is a crash, else speed drops by `12 × |tilt − slope|`. Jumps from a 5-entry
  sine table in 15° steps and the closed-form parabola (§10 above, with 16-bit maths: ExciteBike's
  `long` and `/ 12` cost `__mulsi3` + `__divsi3` per airborne bike per frame).
- **Terrain height from tile tables**, not if-chains: `{base, slope, gfx, flags}` per tile ID and
  `y = base + slope * xcnt` (ExciteBike uses ~50 branches per sample, 3 samples per bike).
- **Re-trigger guards by position**: no second crash before `last_crash_x + 8`, no second jump before
  `jump_x + 8` (ExciteBike): immune to frame-rate changes.
- **Fighting game encoding** (Sumo): one 16-bit `attr` per wrestler (facing, walk phase, one bit per
  action), a frame counter for the current action, sprite IDs in Left/Right pairs
  (`base + dir + 2 * anim`), per-sprite hotspots in the table, motion curves as small `signed char`
  tables; the finisher's sprite ID doubles as the match state. Formulas: reversal
  `random(5) && str1 + h1/4 - 2*random(sta0) > str0 + h0/4`; speed without multiplying: an 8-frame
  phase counter (speed ≥ 11 moves on odd frames, ≥ 22 also on frames 2 and 6). The AI's chance to
  start a move is `1 / ((54 - spd) / 2 - 8 * hard)` per frame; it taunts only when the opponent is
  helpless. Both-start-same-frame conflicts: drop the slower one.
- **Hockey AI details**: chase a loose puck; if the opponent has it, anyone within 90 px (goalie
  50) chases; 50 % chance to skip steering each frame = reaction lag; formations as slot tables
  mirrored for the other team (`x = W - pos - 16`) with a ±2 px dead zone against jitter; goalie
  coverage `y = 100 + 12 * low - 22 * high`; pickup cooldown per player type after losing the puck
  (7/15/19 frames: fat players win scrambles); shot wind-up up to 3 frames, fired on release.
- **Screen shake**: add a small offset table to the camera y for a few frames (Sumo), never move
  the screen pixels (Frustration).
- **Dissolve transition** (ExciteBike): a 15-bit LFSR `s = (s & 1) ? (s >> 1) ^ 0x6000 : s >> 1`
  visits every value once: copy pixel `s` from the new screen to the visible one; no RNG, no
  "done" table.
- Side-scrolling track (ExciteBike): the track is a 1-D byte string, one byte per 8-pixel column
  tile `{dark, light, height}`; each frame `ScrollLeft240_R`, the next column is drawn into the
  hidden part of the 240-pixel buffer (x = 160 + fine x). Tile IDs encode the position inside a
  multi-column object, so ramp height is arithmetic, no height map.
- Emulating a bigger screen (gb68k, 160×144 → 160×100): **crop and pan** (keys move the view in
  4-pixel steps) rather than scale; CHIP-8 (64×32) is pixel-doubled to 128×64 with a 256-entry
  bit-doubling table (`unsigned short dbl[256]`, one lookup per byte; chip8-ti68k does it bit by bit).

## 11. Data encoding and compression

- Bit packing everywhere storage matters: 4-bit board cells (TI-Chess: a board in 32 bytes),
  16-bit moves (from 6 | to 6 | extra 3), flags in spare top bits.
- Save the start state plus the list of moves and **replay on load**: it rebuilds undo and
  repetition history for free.
- TI-Chess opening book: positions Huffman-coded (empty `0`, pawn `10C`, knight/bishop `110xC`,
  rook `1110C`, queen/king `1111xC`, C = colour: ≤ 164 bits), an index of `[hash][offset]` sorted
  by hash (search it by **binary search**; TI-Chess scans it linearly), a "more follows" bit in each
  move word, files split below 64 KB with first/last hash per file to skip whole files.
- **Round-trip check in the PC tool**: the encoder decodes what it wrote and aborts on a mismatch.
- Generate big pseudo-random or regular tables at start-up from a small seed or formula (§1) when
  program size matters more than start-up time.
- Single-value RLE and `ttpack` + ExtGraph `UnpackBuffer`: `ti68k-c-patterns.md` §10.
- **RLE with counting-up runs** (ExciteBike, `Misc.c:134-192`): besides "repeat x n times", a
  second code for "x, x+1, x+2 … n times": big objects made of consecutive tile IDs compress to 3 bytes.
- **RLE before LZ**: running RLE first makes an on-calc LZ pass much faster (smaller input), gb68k.
- **Page table for big address spaces** (gb68k): `const u8 *page[256]`, address `a` →
  `page[a >> 8][a & 255]`; switching a bank or a level chunk = rewriting a few pointers, which can
  point straight into archived files (read in place). Never let a unit that code walks with a raw
  pointer cross from one file to the next.
- **Lazy palette conversion with a tile cache** (gb68k): 2-bit tiles are already the two grey
  planes; remap grey levels with plane logic (per source colour: `mask`, then
  `out_l |= mask & pal_l[c]`, `out_d |= mask & pal_d[c]`, 8 AND/OR per row). A `done[tile]` flag,
  cleared by `memset` on a palette change, converts each tile only when first drawn: grey fades or
  flashes over a tile map at almost no cost.
- **String tables as 16-bit offsets** (Hockey): packed text plus `unsigned short off[]` instead of
  `const char *const[]` (4 bytes + a relocation per pointer); share repeated strings. The author
  credits it (with other changes) for 12–16 KB saved; not measured here.
- **Decompressors for the 68000** (web research 2026-09, figures claimed by their authors, not
  measured here; all are asm, so **ask before adding one**; our own LZ4 and ZX0 asm decoders are measured in §13). The choice depends on when the data is
  unpacked:

  | Format | Ratio | 68000 decoder | Use |
  |---|---|---|---|
  | ZX0 (Saukas), 68000 port by E. Marty | close to Exomizer | ~88 bytes, no tables | once at load (levels from an archived file) |
  | aPLib | ~RNC | 164–212 bytes, 65–80 cycles per output byte (Mega Drive) | load time, good all-rounder |
  | LZSA1 / LZSA2 | 57 % / 52 % | small | LZSA1 ≈ 90 % of LZ4's speed, packs better |
  | LZ4 (A. Carré, `lz4-68k`) | ~60 % | 72 / 180 / 3,722 bytes (×1 / ×1.5 / ×2.4 speed) | data unpacked during play (animation, streaming) |
  | Shrinkler, upkr | best | small but slow (range coder / rANS) | a one-off unpack of the whole game |

  `execram` (github.com/going-digital/execram) counts exact 68000 cycles per depacker in an emulator
  core and would settle the choice on our own data. Cheap wins in a home-made RLE or LZ decoder (Big
  Mess o' Wires, 68k Mac): prefix each literal run with its length (+35 %) instead of testing bytes
  one by one, and copy by words or longs (+10 %).
- **Levels coded as objects, not grids** (Super Mario Bros., SMW, nesdev "Level compression"). Store
  a list of 2- or 3-byte objects (type + x relative to the previous object + y + length packed in the
  type), then run a host-written auto-tiling pass on load that adds edges and corners. The size then
  grows with the content, not the area.
- **Nested metatiles** (Sonic 2/3, Mega Man, Blaster Master): 8×8 tiles, then 16×16 blocks of 4 tile
  indices, then 64×64 or 128×128 chunks of blocks, and a byte grid of chunk indices for the level. A
  huge map costs one byte per chunk, and a lookup is 2–3 indexed byte reads when the sides are powers
  of two. It only pays when the art *is* built from tiles: **verified** on the camp-fire scene
  (`experiments/maps/metatiles.py`), which comes from a rescaled GIF, 797 of its 896 8×8 cells are
  unique (flips: 788), so metatiles save nothing; merging near-duplicates (≤ 8 of 64 pixels) saves
  ~3 KB with visible seams. Rip the original tileset instead of cutting a screenshot (§13).
- **Seeded content** (Elite, 8 galaxies × 256 systems from 6 bytes per galaxy): store a seed,
  regenerate the room, NPC or loot from it with a fixed PRNG, and read attributes as bit fields of the
  seed. Deterministic PRNG in `ti68k-performance.md` §4.

## 12. Ideas from other platforms and from research (measurements in §13)

Collected from web research (2026-09): the 68000 demoscene (Atari ST, Amiga, Mega Drive), NES and
Game Boy development, game-AI literature and embedded C practice. Sources are given so the details
can be checked before use. Most of them have since been measured on the TI-89: see §13 before using one.

**Rendering**
- **Compiled sprites generated at build time** (Atari ST, github.com/ggnkua/half-sprite): a
  host script turns each sprite (and each pre-shift) into C such as
  `p[k] = (p[k] & 0xFFFF00FF) | 0x0000AA00;`, which GCC compiles to `andi.l`/`ori.l` on `d16(An)`.
  There is no data fetch and no loop, and fully transparent words are skipped. The cost is 8–12
  bytes per long touched: use it only for a few big or frequent sprites. Generating the code at run
  time (Wolf3D scalers) is ruled out, because code in RAM is not portable (patterns §4).
- **XOR polygon fill on a bit plane** (ST demos: Chaos, Equinox). Draw only the edge crossings with
  XOR (one pixel per column per edge), then sweep each 32-pixel column down the screen with
  `acc ^= *p; *p = acc`. A single pass fills every polygon, costing ~500 long operations for
  160×100. Fill vertically: a horizontal fill would need a prefix XOR inside each word.
- **Adaptive tile refresh** (Commander Keen, fabiensanglard.net/ega): keep a shadow array of the tile
  drawn in each screen cell, redraw only the cells that differ from the map, and restore the cells
  under sprites from a dirty list. Keep one shadow per GrayDBuf buffer, because the hidden buffer is
  two frames old. It suits screens that scroll by whole tiles or not at all.
- **Deltas against frame N−2** (Amiga IFF ANIM-5/7/8): with double buffering, the hidden buffer
  holds frame N−2, so encode animations as column-wise run-length skips and copies against *that*
  frame. Use it for cut-scenes and big animated backgrounds (the camp-fire flames).
- **Pseudo-3D road** (Lou's Pseudo 3d page, extentofthejam.com/pseudo): a Z table
  `Z = Y_world / (y − horizon)` built once, curves by `dx += ddx; x += dx` per line, hills by
  changing the line step, sprite scale from the same table. No multiply or divide per frame: one
  table read and two adds per line. A cheaper sibling of Mode 7 (§8).
- **Rotation by three shears** (Paeth, Graphics Interface 1986): x-shear by −tan(θ/2), then y-shear
  by sin θ, then the first shear again. Each shear moves whole rows or columns by an offset from a
  per-angle table (word shifts on a 1-bpp plane, no per-pixel multiply, no holes). Two shears
  suffice below ~5°.
- **Blue-noise threshold dithering to 4 greys** (Ulichney's void-and-cluster; textures at
  momentsingraphics.de/BlueNoise.html): the same cost as a Bayer matrix, without its crosshatch.
  Use it in the host asset pipeline (`tools/pyenv`). Atkinson error diffusion (1/8 of the error to 6
  neighbours) keeps contrast on small LCDs. Rotating the matrix each frame (spatio-temporal
  dithering) would need a test on hardware: the LCD ghosts.
- **Flicker-reduced grayscale** (tr1p1ea's TI-84 9-level grey, zephray.me): instead of whole planes,
  show `(dark & M_k) | (light & ~M_k)` with masks rotating each frame, so neighbouring pixels change
  phase in different frames. It needs a custom HW2+ driver (patterns §3), about 3× the copy cost,
  and asm.
- **N-buffer stroboscopic effects** (A. Carré, "4ktribute"): cycling through a few pre-drawn buffers
  gives apparent motion for free (trails, particles, title screens).

**Maps, AI, simulation**
- **Pac-Man target-tile AI** (The Pac-Man Dossier): no pathfinding. At each intersection, drop the
  reverse direction and take the exit whose next tile is closest (squared distance) to a target
  tile. Personalities are just different targets: the player, 4 tiles ahead of the player, a doubled
  vector from another ghost, or a corner when near. Chase and scatter alternate on timers. Cost: ≤ 3
  distance compares per enemy per intersection.
- **Flow field** (Game AI Pro ch. 23): one breadth-first search from the goal (ring-buffer queue, no
  heap), storing a direction byte per cell. Every enemy then does one table read per frame instead
  of an A*. Rebuild over several frames when the goal moves.
- **Jump Point Search** (Harabor & Grastien, AAAI 2011, ICAPS 2014): A* on uniform grids that jumps
  along straight lines until a forced neighbour appears. Few open-list nodes, so little RAM. The
  block-based variant scans 16-bit rows with bit operations and a first-set-bit table. JPS+
  precomputes the jump distance per cell and direction (8 bytes per cell) for static maps.
- **Influence maps and utility AI** (Game AI Pro): a byte grid with `v = max(v, neighbour − decay)`
  propagation (no multiply). Actions are scored as a product of 0–255 factors taken from 16-entry
  response-curve tables (`mulu.w` then `>> 8`). Cheaper and easier to tune than search for real-time
  enemies.
- **Cheap alpha-beta upgrades, in order of value per byte** (chessprogramming.org): transposition
  table move first, PVS, 2 killer moves per ply, history table halved each iteration, null move
  (R = 2), late move reductions. For a TT in 4 KB (~680 entries of 6 bytes), use two-level buckets:
  one slot keeps the bigger subtree, the other is always replaced (Breuker, PhD thesis 1998). Avoid
  MTD(f): it needs a big TT. Compare with TI-Chess (§1).
- **Memory-bounded MCTS** (Powley et al., AIIDE 2017): a fixed node pool that recycles the
  least-recently-used leaf. Visit and win counters are u16. The exploration term comes from tables
  of `sqrt(ln N)` and `1/sqrt(n)`, and the win rate from a reciprocal table, so there are no floats
  or divisions.
- **Small wave function collapse** (Karth & Smith, FDG 2017): a u16 mask of possible tiles per cell,
  collapse the cell with the fewest options (popcount table), propagate with AND/OR of compatibility
  masks. A 20×12 map is 480 bytes of state.
- **Sweep-and-prune and temporal insertion sort** (Ericson, *Real-Time Collision Detection*): re-sort
  last frame's nearly sorted list, which is O(n + swaps). Use it for sprite depth order and for the
  x-sorted broad phase.
- **Height-mask tile collision** (Sonic): each 16×16 tile stores 16 heights plus an angle byte used
  with the sine table. A few sensor points read them: slopes and loops with no geometry.
- **Fixed timestep with a cap on catch-up steps** (gafferongames.com): the game slows down
  gracefully instead of spiralling. Our frame limiter already drops the backlog (`campfire.c`).
- **Deterministic simulation**: integer-only maths, a controlled PRNG seed and a fixed update order.
  Replays then cost the seed plus one input byte per frame, and the same property drives link-cable
  lockstep (§7) and host-side tests.

**Code structure and size**
- **Stackless coroutines** (S. Tatham; Dunkels' protothreads): multi-frame behaviour written as
  straight-line code with 2 bytes of state per entity. Locals do not survive a yield: keep them in
  the struct. Never yield inside a nested `switch`.
  ```c
  #define crBegin(s) switch (s) { case 0:
  #define crYield(s) do { s = __LINE__; return; case __LINE__:; } while (0)
  #define crEnd      }
  ```
- **Bytecode VM for game logic** (Another World: a ~20 KB interpreter with 29 opcodes and 64
  cooperative threads; SCUMM; the Z-machine): a byte opcode plus operand is far denser than 68000
  call sequences, and scripts can live in archived data files read in place, outside the 24 KB
  limit. It needs a host-side assembler and a debug view. Our dispatch costs are in
  `ti68k-performance.md` §6.
- **X-macros**: one list generates the enum, the `const` data tables and the name strings, so parallel
  tables never drift apart. No run-time cost.
- **Arena allocation per level** (Another World; R. Fleury): one `malloc` at start, a bump pointer
  (round sizes to even: word alignment), mark/release for temporaries, nothing allocated during
  play. Object pools keep the free list inside dead objects (Nystrom, *Game Programming Patterns*).
  Refer to entities by a byte index (plus a generation counter if they get recycled) rather than by
  pointer: half the size, and it survives save and load.
- **Data-oriented loops** (M. Acton): replace a per-entity `active`/`type` flag with separate compact
  lists, so loops never test and skip. On a 68000 without cache, the gain is the tests saved.
- **One quarter-wave sine table** plus symmetry serves sin and cos (Doom's `finesine`).
- **Tiny assertions** (Memfault): an assert that records only `__LINE__` and
  `__builtin_return_address(0)` costs a few bytes, where `__FILE__` strings cost KB. Show them on
  screen in debug builds and compile them out in release.
- **Host-side unit tests**: compile the pure logic (rules, AI, collisions, VM) with the PC's gcc.
  Never use a bare `int` in shared code: it is 16-bit in GCC4TI and 32-bit on the PC. A
  deterministic simulation lets both run the same input logs.
- **The "32K" contests** (Mekka & Symposium 32K game compo; Game Boy 32 KB ROM games; Micro Mages,
  a 40 KB NES game): the entries win with a tuned packer, assets unpacked at load time, tile flips
  and symmetry for reuse, and one dense level instead of many sparse ones. The same levers apply to
  our 24 KB limit.
- **Sound through the link port** (J89hw.txt; the PolySnd library): port `0x60000E` bits 0/1 pull
  the tip (right) and ring (left) lines. Disable the byte sender with `0x60000C` bit 6, then toggle
  the lines from auto-int 5, up to 8,192 Hz, for square waves or 1-bit samples. This conflicts with
  keeping AMS's int 5 for the link (patterns §4), so single-player only.

## 13. The §11–12 ideas measured on the TI-89 (2026-09, **verified**)

TiEmu, Titanium profile, GCC4TI -Os, cycles at 12 MHz. Every program checks its result (same
output as a reference version, or the same checksum as the host build) and was read on screen.
TiEmu undercounts multi-bit shifts and `movem`, so code that uses them (ExtGraph, long shifts) is
slower on hardware than shown. Sources are in `experiments/` (the directory is given per item).

**Rendering** (`experiments/render/`)

| Idea | Result | Verdict |
|---|---|---|
| Compiled 16×16 masked sprite, one C function per shift from `gen_sprites.py` (`render1.c`) | B/W 1,066 cycles per draw vs `Sprite16_MASK_R` 2,121; grey 2,226 vs `GraySprite16_MASK_R` 4,101; identical pixels. ~20 memory ops per shift (B/W); datasheet count of the function alone 710 (B/W) / 1,604 (grey) | 2× (more on hardware: no shifts). Costs KB of code for 16 shifts: only for the few sprites drawn most |
| XOR polygon fill (`render2.c`) | 10 triangles: 87k cycles vs C scanline 173k vs ExtGraph `FilledTriangle_R` 93k; 40 triangles: 215k vs 459k vs 259k. The column sweep over the whole buffer is paid even for one polygon. Differs from the scanline filler on a few edge pixels (19 of 868) | Ahead as the polygon count grows; at 10 triangles on par with ExtGraph (whose shifts TiEmu undercounts) |
| Adaptive tile refresh, 20×12 map of 8×8 tiles (`render3.c`) | full redraw 98k cycles; shadow scan 20k / 27k / 54k and dirty list 2.4k / 11.5k / 49k for 5 / 24 / 120 changed cells | Use a dirty list: the cost follows the changes |
| Pseudo-3D road, 70 lines with curves and stripes (`render3.c`) | byte spans 196k, long spans with mask tables 155k per frame (same picture). The projection is 2 adds per line; the cost is filling, ~2,200 cycles per line | 43 % of a 30 fps frame: leaves room for sprites, not for much more |
| Rotation by three shears, 64×64 1-bpp at 30° (`render3.c`) | 202k cycles vs 1,037k for per-pixel inverse mapping (5×); visually identical, 150 of 786 lit pixels differ by a one-pixel edge offset | Use shears for rotating bitmaps; tables per angle |
| Fire as deltas against frame N−1 / N−2 (`maps/metatiles.py`) | 3,620 / 3,400 bytes vs 3,072 raw: flames change almost every long | Deltas only for mostly static animations |

**Maps and data** (`experiments/compress/`, `experiments/maps/`)

| Idea | Result | Verdict |
|---|---|---|
| Decompressors in C (`compress/bench.c`, `unpack.c`; camp-fire tiles / sprites / fire) | ratios: RLE 90 / 71 / 84 %, LZ4 77 / 56 / 43 %, ZX0 62 / 41 / 31 %, ttpack 68 / 46 / 37 %. Speed per output byte: RLE ~25 cycles, LZ4 29–43, ZX0 103–181, ttpack (`UnpackBuffer`) 90–147; copy 5.9. All round trips correct | ZX0 in C packs best and unpacks 13,888 B in ~0.2 s: fine at load. LZ4 in C is 4× faster for data unpacked during play. The asm versions (next row) are faster still |
| Decompressors in asm (`lib/unpack68k.s`, same bench, same data) | cycles per output byte, tiles / sprites / fire: **LZ4 asm 26 / 25 / 19** (C: 43 / 40 / 29, 1.5–1.6×); **ZX0 asm 73 / 57 / 42** (C: 181 / 138 / 103, 2.5×); 13,888 B of tiles in 30 ms (LZ4) / 84 ms (ZX0). Code 222 B (LZ4) / 314 B (ZX0), each with long copies when source and destination have the same parity. The ZX0 gain comes from the bit reader: `add.b d1,d1` + `bne` = 14 cycles per bit, carry = the bit, `addx` builds the gamma value. Few multi-bit shifts and no `movem`, so TiEmu's count is close to the hardware one | ZX0 asm unpacks faster than LZ4 in C and packs 20 % smaller: the default for game data. LZ4 asm (≈ RLE speed at a better ratio) for data unpacked during play |
| Object-coded level (`maps/levels.c`, `sizes.py`) | 256×16 level: 36 objects = 108 B; the grid is 4,096 B raw, 323 RLE, 255 LZ4, 180 ZX0. Decode + auto-tiling 268k cycles (22 ms, once) | Best for designed levels; the decoder is a switch of run loops |
| Seeded rooms 20×12 (`maps/levels.c`) | 10.8k cycles per room from a 16-bit seed, same checksum on host and calculator | Store 2 bytes per room instead of 240 |

**AI** (`experiments/ai/`; `pac.c`, `path.c`, `c4.c` also build and run on the host with identical counts)

| Idea | Result | Verdict |
|---|---|---|
| Pac-Man target tiles (`pac.c`) | 686 cycles per decision (unrolled; 1,064 with a loop), 527 per target computation, 7,148 per step for Pac-Man plus 4 ghosts | The cheapest chase AI with personality |
| Flow field, 40×25 BFS (`path.c`) | 326k cycles per full build (almost a frame: spread it over frames), then 141 cycles per enemy step; 1,134 + 2,268 bytes | Many enemies with one goal |
| A* vs JPS, 8-connected, 12 paths per map (`path.c`) | 22 % walls: A* 1,375 expansions, 707k cycles per path; JPS 765, 539k. 4 % walls: A* 2,055, 1.2M; JPS 447, 410k. Same path costs; max heap 201 → 35 entries | JPS on open maps (3×); on mazes it barely helps. Even so, a path costs more than a frame: search over frames or use a flow field |
| Alpha-beta upgrades, Connect 4 depth 8 (`c4.c`) | empty board, nodes: plain 198,817 (56 s), centre-first 7,177, + killers and history 8,980, + 4 KB TT (two-level buckets) 5,079, + PVS 4,200 (2 s). On 2 other positions centre-first alone searched *more* nodes than plain (26,888 vs 22,034; 32,951 vs 8,729); killers and history fixed that, the TT cut further, PVS helped on 2 of 3. ~3,400 cycles per node (~5,500 with the ordering code). Same score in every variant | Centre-first + killers/history + a small TT is the robust set; PVS is optional |
| Memory-bounded MCTS (`c4.c`) | UCT with log-bucket tables, no float or division: 62,812 cycles per playout (191/s); 2,000 playouts use 589 of 1,500 10-byte nodes. Takes the win and blocks the loss. On the host: 20/20 wins against random (300 playouts), 10/10 against alpha-beta depth 4 with 1,000 playouts (4/10 with 300) | Works for games with no good evaluation function; needs ~5 s per move for real strength |

**Code structure** (`experiments/struct/`)

| Idea | Result | Verdict |
|---|---|---|
| Stackless coroutines vs explicit state machine, 50 entities (`coro.c`) | 197 vs 173 cycles per update (call excluded), same trajectory; 10-byte entity | Readability for +14 %: fine for scripts, not the inner loop |
| Bytecode VM, 17 opcodes (`vm.c`, script by `vmasm.py`) | per opcode: switch 222 cycles, computed goto (`&&label`, works in GCC 4.1) 182. The behaviour costs 1,273 / 1,043 cycles vs 171 native (6–7×); with the step as a native call 900 / 769. Script 96 bytes, read in place from an archived file at the same speed | For level scripts and cut-scenes, not per-frame physics; keep heavy steps native |
| Arena vs AMS heap (`alloc.c`) | AMS `malloc` ~48,000 cycles per call (with the calculator's heap as it was), `free` 1,875; arena alloc 117 (loop included), release = 1 store; pool alloc + free 398, handle lookup with generation check 257, stale handle caught | Never `malloc` during play: one arena or pool per level |

