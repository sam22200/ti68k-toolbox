# TI-68k performance: writing fast code for a very slow machine

The TI-89 is a **Motorola 68000 at ~12 MHz** (HW2/HW3; 10 MHz on HW1), with no cache, no FPU, no
barrel shifter and a **16-bit data bus**. At 30 fps a frame has about **400,000 cycles**, shared
with the grayscale interrupt and AMS (on HW2+ the grayscale driver alone takes ~9 %, ~35k per
frame, invisible in TiEmu: patterns §3). Performance is a design constraint from the first line, not a
final polish: pick data structures, number formats and algorithms for this machine.

## 1. Measured costs (verified in TiEmu)

Measured with `experiments/bench/bench.c` (auto-int 1 as a 256 Hz clock, other interrupts
silenced, best of 2 runs, loop overhead subtracted), converted to cycles at 12 MHz. TiEmu uses
UAE's per-instruction cycle table (e.g. MULS/MULU fixed at 66 cycles, while a real 68000 takes
38–70), so read these as **orders of magnitude and ratios**, not exact cycle counts.

> **TiEmu undercounts `movem` and shifts** (**verified**, `experiments/bench/bench6.c`, loops of one
> instruction compared with the MC68000 datasheet): `move.l (a0)+,(a1)+` 23 (datasheet 20),
> `muls.w` 73 (70), `move.l #imm` + `divu.w` 164 (152) are right, but **`movem.l` costs ~12 cycles
> whatever the register count** (datasheet 12 + 8 per register: 44 for 4, 108 for 12) and
> **`lsl.l #8` costs 7 instead of 24** (the 2 cycles per bit are ignored); `lea abs` also looks
> cheaper. So every routine built on `movem` (ExtGraph copies, clears, scrolling, sprites) or on
> multi-bit shifts **looks faster in TiEmu than on a real calculator**. For such code, count
> cycles from the datasheet (`ti68k-asm.md` §3) and compare real-hardware estimates, not TiEmu
> ticks; TiEmu stays fine for code dominated by moves, multiplies, divisions and ROM calls.

| Operation | -Os | -O2 | Note |
|---|---|---|---|
| empty `for` loop, `long` counter | 24 | 30 | per iteration (`addq.l`, `cmp.l`, `bgt`) |
| 16×16→32 multiply (`(long)(short)x * s`) | **90** | **395** | `muls.w` at -Os; **-O2 turned it into a `__mulsi3` call** in the loop |
| 32×32 multiply (`long * long`) | 397 | 387 | always a library call (`__mulsi3`) |
| 16-bit unsigned divide | 196 | 195 | `divu.w` |
| 32-bit divide (`long / long`) | 510 | 498 | library call (`__divsi3`) |
| `long >> 4` | 24 | 24 | |
| float multiply + add | **~7,500** | ~7,700 | software floating point: ~300× an integer op |
| `grid[y][x]` read (row size 30) | 130 | 130 | index maths with `muls #30` |
| `*p++` read | 44 | 38 | pointer walk: 3× cheaper than 2D indexing |
| `DrawPix` (AMS ROM call) | 600 | 595 | |
| direct pixel XOR in LCD memory | 194 | 144 | 3–4× faster than `DrawPix` |
| `memcpy` of a 3840-byte screen | 23,400 | 23,400 | |
| ExtGraph `FastCopyScreen_R` | ~2,400 in TiEmu, **~17,500 real** | | `movem`-based: TiEmu undercounts it; real bus floor for 3840 bytes is 15,360; **~1.35× faster** than `memcpy` on hardware |
| `memset` of 3840 bytes | 15,900 | 15,900 | |
| ExtGraph `FastClearScreen_R` | ~1,500 in TiEmu, **~8,500 real** | | same caveat; real floor 7,680; ~1.9× faster than `memset` |

Second set (`experiments/bench/bench2.c`, -Os, HW3; techniques that compete in old sources):

| Operation | Cycles | Note |
|---|---|---|
| pixel OR, mask `0x80 >> (x & 7)`, offset `(y<<5)-(y<<1)` | 182 | variable shift: 6+2n cycles |
| pixel OR, mask from an 8-byte table | 200 | **slower** in TiEmu; on hardware the shift costs ~7 more cycles (TiEmu ignores the 2n), still slightly ahead |
| ExtGraph `EXT_SETPIX(p, x, y)` | **142** | `bset` with bit number `~x` (the 68000 takes memory bit numbers mod 8): no mask at all |
| AMS `DrawStr`, 20 characters (F_6x8, via `PortSet`) | 120,210 | ~6,000 per character |
| own pre-rendered font, one aligned `long` XOR per glyph row, 20 characters | **17,741** | **6.8× faster** than `DrawStr` |
| ExtGraph `ScrollUp160_R`, 100 lines, 1 pixel | 14,350 | |
| ExtGraph `ScrollLeft160_R`, 100 lines, 1 pixel | 19,350 | |
| grayscale frame: 2 × `FastCopyScreen_R` vs `GrayDBufToggleSync` | ~35,000 real vs 0 | but `…ToggleSync` **waits** for the next plane switch (~85/s): up to 1/85 s idle per frame (patterns §3) |

What it means for one frame (400k cycles at 30 fps):
- 2 plane copies with `FastCopyScreen_R` ≈ 35,000 real cycles (9 %); with `memcpy` ≈ 47,000 (12 %);
  copying only the visible 160×100 (20 bytes per row, Fischer's `movem.l` 5-register loop) ≈ 25,000.
- One float operation ≈ 7,500 cycles: 50 of them per frame eat the whole budget.
- One 32-bit division ≈ 500 cycles: fine at init, not per pixel.

## 2. Types: unsigned, and as small as the range allows

Checked in the code GCC4TI generates at -Os (`experiments/codegen/types.c`, **verified**):

| C code | Generated code | Lesson |
|---|---|---|
| `short x; x % 8` | `ext.l` + **`divs.w`** + `swap` (a full division, ~150 cycles) | signed `%` is a division |
| `unsigned short x; x % 8` | `and.w #7` (4 cycles) | **unsigned** `%` by 2ⁿ is an AND |
| `short x; x / 4` | test + branch + `addq` + `asr` | signed `/` needs a rounding fix-up |
| `unsigned short x; x / 4` | `lsr.w #2` | **unsigned** `/` by 2ⁿ is one shift |
| `short x; x >> 2` | `asr.w #2` | explicit shift: one instruction (rounds towards −∞) |
| `short x; x / 10` | `divs.w` | real division |
| `unsigned short x; x / 10` and `x % 10` | `mulu.w #52429` + `swap` + shift, remainder by `muls #10` | **unsigned** constant division: GCC already uses a reciprocal |
| `(unsigned short)(((unsigned long)x * 6554) >> 16)` | `mulu.w` + `clr.w` + `swap` | reciprocal: exact x/10 for 0 ≤ x ≤ 16388 |
| `short x; x * 30` | `muls.w #30` (66 cycles in TiEmu) | GCC does **not** use shifts for 16-bit |
| `(x << 5) - (x << 1)` or `long x; x * 30` | shifts + add/sub | write the shifts yourself |
| loop counter `unsigned char` vs `unsigned short` | identical code | small types do not speed up locals |
| `unsigned char a, b; return a + b;` | `add.b` + `and.w #255` | byte maths adds zero-extensions when promoted |
| `short` vs `long` operands | `.w` vs `.l` instructions, 2 bus accesses per `long` | 16 bits is the native size |

Rules:
- **Unsigned whenever the value cannot be negative** (indexes, sizes, counters, colours, tiles,
  coordinates kept on screen, angles): divisions and modulos by powers of two become single
  shifts/ANDs, range checks become one compare (`(unsigned short)(x - lo) < n`).
- **Signed values**: divide with an explicit `>>` (and accept rounding towards −∞) instead of `/`.
- **Smallest type for storage** (arrays, tables, structs, level data, sprites): `unsigned char`
  / `signed char` halve the memory of `short` and a byte read costs the same bus cycle as a word
  read. RAM is tight: about 188 KB of user RAM on both models (TI specification).
- **16-bit types for computations and locals** (`short`/`unsigned short`): it is the CPU's native
  size; byte locals bring nothing and byte arithmetic adds extensions.
- **`long` only when the range needs it** (scores, 32-bit timers, intermediate products): `long`
  multiply/divide are library calls (400–500 cycles).
- `int` is 16 bits here (`-mshort`), but write `short` so the intent is explicit.
- Use `unsigned char` for 8-bit angles (0–255 = one full turn): wrapping is free.

## 3. Lookup tables instead of computing

Compute once on the PC, store as `static const` data, read at run time. `tools/bin/ti-table`
generates the C code and picks the smallest type automatically:

```sh
tools/bin/ti-table sin atan > tables.h        # sin_tab[256] (signed char, ×127), atan_tab[65]
tools/bin/ti-table recip --n 64 > recip.h     # recip_tab[d] = ceil(2^15/d), exactness range in comment
tools/bin/ti-table row --n 100 > rows.h       # row_tab[y] = y*30 (LCD row offsets)
tools/bin/ti-table sqrt > sqrt.h              # isqrt(0..255)
```

- **Trigonometry**: one 256-entry sine table in `signed char` (256 bytes) covers everything
  (the old sources — Puzzle Bobble, the 3D tutorials, edit3d, Pong — all carry the same pair of
  `int` tables ×128, 1 KB; ISS89 uses degree tables `char[360]`: both worse):
  `#define SIN(a) sin_tab[(unsigned char)(a)]`, `#define COS(a) sin_tab[(unsigned char)((a) + 64)]`.
- **Rotation** of a point (x, y) by angle a (values ×127, so `>> 7` ≈ divide by 128, 0.8 % smaller):
  ```c
  short xr = (x * COS(a) - y * SIN(a)) >> 7;   // 16×16 products: muls.w, no float, no divide
  short yr = (x * SIN(a) + y * COS(a)) >> 7;
  ```
  For sprites, **pre-rotate** them offline (8 or 16 directions) instead of rotating pixels at run time.
- **atan2 with 8-bit angles** from a first-octant table (one `divu.w` per call, **verified** on 7
  vectors in `experiments/tables/rot.c`):
  ```c
  static unsigned char atan2_8(short x, short y)          // |x|, |y| < 1024; 0 = +x, 64 = +y
  {
      unsigned short ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
      unsigned char a;
      if (ax == 0 && ay == 0) return 0;
      if (ax >= ay) a = atan_tab[(unsigned short)(ay << 6) / ax];
      else          a = 64 - atan_tab[(unsigned short)(ax << 6) / ay];
      if (x < 0) a = 128 - a;
      if (y < 0) a = -a;
      return a;
  }
  ```
- **Divisions by a variable in a known small range**: `x / d` = `(x * recip_tab[d]) >> 15`
  (`mulu` instead of `divu`); `ti-table` prints the range of x for which it is exact
  (x ≤ 4693 for d < 16 with SHIFT=15).
- **Row offsets / multiplications by a constant**: `row_tab[y]` instead of `y * 30`.
- Also worth tabulating: perspective (1/z) for Mode 7 or raycasting, sprite masks, bit counts
  (`popcount[256]`), gamma/fade ramps, "next state" tables for automata, key→action maps.
- Tables cost memory: choose the size (entries × type) for the precision you really need.
- Demo: `experiments/tables/rot.c` rotates a square from `sin_tab` with ExtGraph double buffering
  at **170 fps** in black and white (clear + 4 corners + 4 lines + text + copy), **verified**.

## 4. Arithmetic

- **Multiply**: the only hardware multiply is 16×16→32 (`muls.w`/`mulu.w`, 38–70 cycles on real
  hardware). Write it so that both operands are visibly 16-bit: `(long)(short)a * (short)b`.
- **Trap (verified in the asm, `experiments/bench/bench3.c`)**: GCC 4.1 emits `muls.w` for
  `(long)a * b` only when each operand is used once. As soon as one `short` feeds **two or more**
  widening products (`(long)x * r` and `(long)y * r`, a matrix row, a shared scale), it widens it
  to `long` once and calls `__mulsi3` for every product. Measured: a projection with a shared
  reciprocal costs 968 cycles in plain C, 341 with `muls16`, 490 with two divisions: the "optimised"
  C version is slower than dividing. Use `muls16()` for every widening product in hot code, or keep
  the product in 16 bits when the range allows (`short * short` → `short`, one `muls.w`).
- **Check the assembly of hot loops** (`tigcc -Os -S file.c`, or with the flags you build with)
  for `__mulsi3`, `__divsi3`, `__udivsi3`, `__modsi3` and float helpers (`__mulbf3`, `__addbf3`…).
  If GCC picked a library call where a 16-bit instruction would do, force it:
  ```c
  static inline long muls16(short a, short b)   // always a single muls.w (verified at -Os and -O2)
  { long r; asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r; }
  ```
- Measured 3D costs (**verified**, HW3, all inputs varying; cycles per vertex): three chained
  per-axis rotations 1,706; one 3×3 matrix with `muls16` 1,095; the same matrix with 16-bit products
  1,001 (valid while |coordinate| × 127 × 3 < 32768, i.e. |coord| < 86); edit3d-style `long` matrix
  3,734. Projection of one vertex: two `divs.w` 490, reciprocal table + `muls16` 341.
- **32/16 division in one instruction** when the dividend needs 32 bits (`(long)x * scale / z`):
  ```c
  static inline short divs32_16(long n, short d)   // quotient must fit 16 bits (else garbage)
  { asm("divs.w %1,%0" : "+d"(n) : "dmi"(d)); return (short)n; }
  ```
  **Verified** (bench3, HW3, one vertex = 2 products + 2 divisions): `long / long` in C (X3D style,
  2 × `__divsi3`) 1,259 cycles, `divs32_16(muls16(x, s), z)` **623**, reciprocal table 341. Range-
  check or clip first (near plane, screen bounds) so the quotient cannot overflow.
- **Constant multiplies**: write shifts and adds (`(y << 5) - (y << 1)` for ×30), or use a table.
- **Never divide in a loop.** Powers of two: shifts on unsigned values (§2). Constant divisor:
  reciprocal multiply (`x / 10` = `(x * 6554UL) >> 16`, exact for 0 ≤ x ≤ 16388). Variable divisor
  in a small range: `recip_tab` (§3). `%` by a power of two: `& (n - 1)` on an unsigned value.
- **No floats at run time.** Use fixed point (patterns doc): positions `<< 4` or `<< 8`, angles
  0–255 in an `unsigned char`, trig from tables (§3). Floats are acceptable once at start-up.
- Prefer squared distances to `sqrt`; if a distance is needed, use an integer approximation
  (e.g. `max + min/2`, error ~12 %) or `sqrt_tab`. Better: **alpha max plus beta min**
  `mx + (mn >> 2) + (mn >> 3)` (max error ~6.8 %), 122 cycles measured (bench7).
- **`long / unsigned short` is two library calls** (`__umodsi3` + `__udivsi3`, verified in the asm,
  `experiments/codegen/traps.c`), while `unsigned short / unsigned short` with `%` of the same
  operands is **one** `divu.w` (quotient and remainder together). Keep dividends 16-bit, or use
  `divs32_16` above (quotient must fit 16 bits).
- **Random number in `[0, n)` without division** (Lemire's fastrange, ACM TOMACS 2019):
  `((unsigned long)r * n) >> 16` on a 16-bit random `r` compiles to `mulu.w` + `clr.w` + `swap`:
  107 cycles vs 202 for `r % n` (bench7). Slightly biased (bias < n/65536), fine for games.
- **Randomness, measured (bench7, TiEmu, cycles per call incl. the call)**: `random(100)` 499;
  16-bit Galois LFSR `lf = (lf >> 1) ^ (-(lf & 1) & 0xB400)` 74 (period 65535, weakest);
  wyhash16 `w += 0xfc15; h = (unsigned long)w * 0x2ab; return (h >> 16) ^ h` 132 (one `mulu.w`, good
  quality, shift-free: the figure is exact on hardware); xorshift16 `x ^= x << 7; x ^= x >> 9;
  x ^= x << 8` 141 in TiEmu, ~190 real (24 shifted bits); Marsaglia MWC 2×16 286 (period ~2^60).
  Default: **wyhash16**, or the LFSR for particles. Seed it once, save the state in save files.

## 5. Memory access and loops

- Walk arrays with **pointers** (`*p++`, `p += stride`) instead of recomputing `a[y][x]`
  (3× faster measured). Keep row strides as powers of two when you design your own buffers
  (32 instead of 30 → a shift instead of a multiply), except for the LCD layout, which is fixed.
- **Count down with a 16-bit counter** in hot loops. Which form gives `dbra` (10 cycles), verified in
  the asm at -Os (`experiments/codegen/dbra.c`): `while (n--)` with a `short` or `unsigned short n`
  → `dbra`; `for (i = n - 1; i >= 0; i--)` → `subq` + `bpl` (12–14); `do … while (--n)` → `subq` +
  `bne`; counting up `for (i = 0; i < 100; i++) p[i] = 0` → a **32-bit** index, `addq.l` + `moveq`
  + `cmp.l` + indexed store every iteration (worst). Write `while (n--) *p++ = …;`.
- Move data as `long` words when you can (fewer instructions even on a 16-bit bus); `short` and
  `long` accesses **must be even-aligned** (odd address = Address Error crash).
- Hoist invariant work out of loops yourself: GCC 4.1 does not always do it, and at -O2 it may
  "optimise" in the wrong direction (the multiply above).
- Unroll only the innermost, hottest loops, and only a little (×2/×4): code size is limited too.
- Keep hot globals together; ExtGraph's docs mention `-freg-relative-a4` / global register
  variables as the fastest way to reach globals (advanced: read the docs before using it).

## 6. Calls, ROM calls and libraries

- Function calls cost a `jsr`/`rts` plus stack traffic: make small hot helpers `static inline`.
- Register parameters are faster: `-mregparm` (all user functions), or
  `__attribute__((__regparm__))` per function. **Prefer ExtGraph's `_R` variants**, which take
  their parameters in registers and are faster and smaller than the `__stkparm__` ones.
- **AMS ROM calls are slow in loops** (`DrawPix` 600 cycles, `DrawLine`, `DrawStr`, `printf`…):
  use ExtGraph (`Fast*`, sprites) or direct memory writes in the game loop. ROM calls are fine
  for menus and static screens.
- The library `memcpy`/`memset` are ~10× slower than ExtGraph on screen-sized blocks (measured):
  use `FastCopyScreen_R`, `FastClearScreen_R` and friends there.
- **Dispatch in interpreters and state machines** (**verified**, `experiments/bench/bench4.c`, toy
  VM with 8 opcodes, cycles per executed opcode including the loop): dense `switch` 136, function-
  pointer table 152, GCC computed goto (`goto *lbl[op]`) 130. Keep the `switch` (small cases inline
  their work in registers; a pointer call loses that). Only hand asm does much better: gb68k's
  threaded dispatch costs ~26 cycles (`ti68k-asm.md` §4).

## 7. Graphics

- **Runtime primitive costs** (`runtime/tests/rbench.c`, HW3 TiEmu cycles per call, grey,
  **verified**): full-screen TileMap (`DrawPlane`, 272×160 buffer, no refresh) 81k; the same
  11×7 view drawn tile by tile with `GrayClipSprite16_RPLC_R` 254k (**3× slower**: use the
  engine); masked grey 16×16 sprite ~4.6k, 8×8 ~2.7k (clipped routines); `GrayFastFillRect_R`
  160×100 37k; `GrayClearScreen2B_R` 3.3k (TiEmu; `movem`, ~17k real); 6 characters of text
  through AMS `DrawStr` + `PortSet` on two planes 92k (F_4x6) / 75k (F_6x8), with the in-place
  font and one `long` OR per row 9.7k / 11.4k (**~8× faster**).
- **Per-call overhead dominates small `draw_rect`s: draw repeated shapes as sprites** (**verified**,
  `games/flappy/`, Titanium TiEmu): a Flappy frame with 4 pipes drawn as 6-colour stripes (28
  `draw_rect` per pipe) plus the game-over panel cost 431k cycles; with each pipe half as one
  opaque 16-px sprite of identical rows (`RtSprite` mask `RT_NULL`, `h` set per call) and the caps
  as one masked 32-px sprite, 244k; a playing frame with 3 pipes, 158k. Same for a scrolling
  striped band: six 32x4 opaque sprites instead of 20 rects.
- **Non-16-px tiles: draw them in byte-aligned groups, and cache the static screen** (`ti-cycles`
  datasheet counts, `games/desolate/`, 1.5× of a TI-83 game: 12×12 tiles at x = 8 + 12c): a
  generic any-x blit (12-bit row shifted into a `long`, three masked byte writes) cost 1.22M
  cycles for the 96 tiles of a screen (two planes); two tiles side by side are exactly 3 whole
  bytes, so per pair row `p[0] = a0; p[1] = a1 | b0 >> 4; p[2] = b0 << 4 | b1 >> 4` from tiles
  stored as bytes: 212k (**5.7× faster**; with `u16` rows GCC's `lsr.w #8/#12` kept it at 317k).
  The room is composed once into a private plane pair (on changes only) and copied each frame
  with unrolled `long` moves: 45k, a whole play frame 59k with the hero sprite.
- **`RT_BENCH` renders the final state**: pick the scenario and the `BENCH` count so that this
  state is the worst case (an autopilot that died and restarted measured an empty screen:
  100k instead of the real cost). Check the state with the same `--scenario N --frames BENCH
  --shot` on the PC first.

- **Shifted full-view copy in C: one `long` read per destination word** (`ti-cycles`
  datasheet counts, Minish 70% view, 160x100, two planes): rebuilding a 32-bit
  accumulator per word (`acc = acc << 16 | *s++; *d++ = acc << sh >> 16`) cost 183k
  cycles; reading `*(u32 *)(row + k)` at each word's even address and shifting it
  once (`<< sh >> 16` for sh <= 8, `>> (16 - sh)` above) cost 128k (**-30%**,
  ~64 cycles per word), unrolled over the ten words of a row. Same pixels. Alundra village
  (stride `iwb`): 149k → 110k.
- **Precompose static HUD text and aligned counters** (headless PC/TI verified,
  Minish M4): the existing 66x8 label uses two aligned `long` row copies plus a
  two-pixel masked clear, rather than shifting every AMS glyph on both planes.
  Three hearts are thirteen precomputed 32x8 rows, also copied in place. With
  pre-shifted rocks/bush deltas, the normal dense combat case drops from385638
  to352332 datasheet cycles. This is a combined frame result, including state
  hashes, not an isolated text-blitter measurement. At70%, sixteen pre-shifts
  of twenty small enemy poses fit a48908-byte archived bank; shared clipped
  fallbacks and pointer/clip-mask reuse in foreground restoration keep its
  dense peak at342646 cycles. Hardware grayscale cost is excluded.

- **Pixels**: `EXT_SETPIX`/`EXT_CLRPIX`/`EXT_XORPIX`/`EXT_GETPIX` (ExtGraph) are the fastest
  single-pixel writes (measured above). For lines and walks, step the address and mask instead of
  recomputing them: `EXT_PIXLEFT_AM`/`EXT_PIXRIGHT_AM` (`ror.b #1,m; bcc; addq #1,a`) and
  `EXT_PIXUP`/`EXT_PIXDOWN` (`a -= 30`). Lines: `FastDrawLine_R`/`FastLine_Draw_R` (asm) and
  `ClipLine_R` for lines that may leave the screen (a home-made Bresenham that tests bounds per
  pixel walks the whole off-screen length). Triangles: `FilledTriangle_R` /
  `GrayFilledTriangle_R` with the `DrawSpan_*_R` callbacks instead of a home-made filler.
- **Anything up to 16 pixels wide at any x: one aligned `long` read-modify-write per row.** Take
  the word-aligned address `row + ((x >> 3) & ~1)` and shift by `24 - (x & 15)` (for a
  left-aligned byte) or `16 - (x & 15)` (for a word): the glyph never needs splitting, no mask
  table, and the address is always even (a `long` at `x >> 3` is odd half the time: Address Error).
  This is how TICT S1P6, TI-Chess and TICT-Explorer draw text, and how ExtGraph's `Sprite16` works.
- **Text in the game loop**: pre-render the AMS font once into your own table, then draw it with
  the rule above (6.8× faster than `DrawStr`, **verified**):
  ```c
  static unsigned char font[256 * 8];              // 1 byte per row, glyph left-aligned
  FontSetSys(F_6x8); PortSet(font, 7, 256 * 8 - 1); ClrScr();
  for (k = 0; k < 256; k++) DrawChar(0, k << 3, k, A_REPLACE);
  PortRestore();
  // draw: unsigned long *p = (unsigned long *)(row + ((x >> 3) & ~1)); short sh = 24 - (x & 15);
  //       for 8 rows: *p ^= (unsigned long)*g++ << sh; p = (unsigned long *)((char *)p + 30);
  ```
  (full routine: `font_xor` in `experiments/bench/bench2.c`; F_4x6 glyphs are proportional: keep a
  width table in `unsigned char`). Do not pull fonts out of the ROM at hard-coded offsets (TICT
  S1P6, TI-Chess): they differ between AMS versions.
- **Or read the AMS fonts in place, the documented way** (**verified** identical to `DrawStr`, AMS
  2.09 HW2 and AMS 3.10 HW3, `experiments/fonts/amsfont.c`; needs `MIN_AMS 200`): no 2 KB table.
  ```c
  pFrame fr = 0xFF000000UL; short app = EV_runningApp; const unsigned char *font[3];
  if (app) fr = *(pFrame *)((char *)HeapDeref(app) + 20);        // running app's ACB.pFrame
  OO_CondGetAttr(fr, OO_SFONT + k, (void **)&font[k]);           // k = F_4x6, F_6x8, F_8x10
  ```
  Formats: F_4x6 = 6 bytes per char `[advance width][5 rows]` (proportional); F_6x8 = 8 bytes, 6-px
  advance; F_8x10 = 10 bytes, 8-px advance; each row a left-aligned byte, glyph at `font + c * size`.
  AMS draws 4x6 'g' one row lower (descender): the in-place data has it at the same place, draw
  one row below if you want the AMS look. TICT/Fischer's `FS_DrawString.s` uses this, with a
  cheaper write rule: if `(x & 15) <= 8` the byte fits in the word, `or.w (byte << (8 - (x&15)))`,
  else `swap` + `lsr.l` by at most 8 and `or.l` (shift ≤ 8 instead of up to 24). It draws 'g' too
  low on the HUD row and overflowed the plane (Hockey's state-corruption bug): leave a guard row.
- **Scrolling**: `ScrollUp160_R`/`ScrollDown160_R`/`ScrollLeft160_R`/`ScrollRight160_R` (160
  variants skip the 10 invisible bytes per row: a third less work than 240-wide routines such as
  TICT S1P8's `roxl.w` loop). A full 1-pixel scroll of both planes costs ~30–40k cycles (TiEmu
  count: more on hardware, §1). **Never call a 1-pixel scroll n times for n pixels** (ExciteBike
  scrolls 3× per frame at 3 px/frame: ~170k cycles, 40 % of a 30 fps frame); for a
  scrolling world, scroll by redrawing tiles (tilemap engine) or scroll in 8-pixel steps and draw
  only the new edge (Tunnel, Falldown).
- **Grayscale double buffering: GrayDBuf** (patterns §3) instead of copying two planes per frame.
- **Scrolling tile worlds: ExtGraph's TileMap engine** (J. Richard-Foy, `tools/extgraph/lib/tilemap.h`
  + `tilemap.a`, docs `tools/extgraph/DOCS/Tilemap/English/`; **verified** HW2 and HW3,
  `experiments/tilemap/tmdemo.c`: a 512×320 grey map scrolling diagonally plus 6 sprites at 81 fps,
  capped by `GrayDBufToggleSync`). `tilemap.a` is not linked by default: add it to the command line
  (`ti-cc -o g g.c ../../tools/extgraph/lib/tilemap.a`).
  ```c
  static unsigned char map[MH][MW];                  // tile numbers (…B types; …W = short)
  static unsigned short tiles[N][32];                // grey 16×16 tiles, rows interlaced
  char *big = malloc(GRAY_BIG_VSCREEN_SIZE);         // 272×160 ×2 planes = 10,880 bytes
  Plane pl = { map, MW, tiles, big, 0, 0, 1 };       // force_update = 1 at start / after edits
  DrawPlane(cam_x, cam_y, &pl, GrayDBufGetHiddenPlane(DARK_PLANE), TM_GRPLC89, TM_G16B);
  ```
  - How it works: tiles are copied word-aligned into the big buffer (screen + 32-pixel margin),
    rebuilt only when the view leaves the margin; each frame the buffer is blitted with a 0–31
    pixel offset (the only shifting). `RPLC` needs no clear; `…89` modes draw only 160×100.
  - **Grey destination = both planes contiguous, first plane first**: with GrayDBuf the **dark
    hidden plane sits 3840 bytes *before* the light one** (measured on HW2 and HW3, both buffers;
    also for `GrayGetPlane`). So pass the dark plane as destination and interlace tile rows as
    **(dark row, light row)**. No plane copy needed.
  - Map ≥ 15×8 tiles (16×16) or 30×16 (8×8). Modes `RPLC`, `OR`, `MASK`, `TRANW`/`TRANB` (white or
    black transparent); animated planes (`AnimatedPlane`, `TM_GA16B`: matrix holds animation
    numbers, `tabanim[step][anim]` the tiles); `Draw[Gray]BufferWithShifts…` takes per-line x
    shifts and line steps (water, heat waves, cheap Mode-7-like effects, ExtGraph `demo16`).
  - Cost not measured: the engine is `movem`-heavy, which TiEmu undercounts (§1).
- **Pre-shifted sprites** (ExtGraph `preshift.h`, in `extgraph.a`; **verified** drawing on HW2/HW3
  in the same demo). Drawing a 16-pixel sprite at x = 5 means shifting every row by 5 bits and
  splitting it over two words, every frame. Pre-shifting does it **once at start-up**: keep copies of
  the sprite already shifted to each sub-word position, then drawing is a plain OR of 32-bit rows.
  ExtGraph stores the 8 even shifts ("semi-pre-shifted", an odd x costs one `ror.l #1` per row):
  ```c
  unsigned long *ps = malloc(SIZE_OF_PGSPRITE16x16);   // 1,024 bytes (grey) vs 64 for the sprite
  PreshiftGrayISprite16x16(sprite_interlaced, ps);     // B/W: PreshiftSprite16x16, 512 bytes
  GrayPSprite16x16_OR_R(x, y, ps, light, dark);        // also _XOR_R; B/W PSprite16x16_OR_R
  ```
  16× the memory, 16×16 only, OR/XOR only (no masked mode), **no clipping** (keep x ≤ 208,
  y ≤ 112: use a guard band). Worth it for many copies of a few sprites (bullets, particles,
  shmup enemies) over a background you redraw anyway; the saved work is exactly the shifts that
  TiEmu undercounts, so the real gain is larger than TiEmu shows.
  **Headless verified, masked alternative (Yoshi coins/eggs):** for repeated8px
  sprites, bake all16 sub-word positions as interleaved32-bit light/dark/mask
  rows. A C blit writes `(destination & mask) | pixels` to aligned word-based
  addresses; transparent padding preserves neighboring art. Use the ordinary
  ExtGraph path for partially clipped sprites. Five10-row poses padded to16
  rows cost15360 external bank bytes. Pixel checks match PC across all scroll
  offsets; this plus coin X bins and pointer loops reduced the full dense
  scene from226950 to209572 datasheet cycles. That is a combined-game result,
  not an isolated primitive speedup; hardware grayscale is excluded.
- **Big destructible bitmap worlds** (Worms68k): store each plane **column-major** in 32-pixel
  strips (`buf[(x >> 5) * H + y]`, bit `31 - (x & 31)`): a strip is a valid ExtGraph 32-wide sprite,
  so the visible window is ~6 `ClipSprite32_OR_R` calls per plane, clipped for free
  (`ti68k-game-techniques.md` §10).
- **Save/restore under sprites instead of redrawing the background** (Sumo, "~20 fps" gained):
  before drawing each sprite, save the area under it from both planes (`SpriteX8Get_R`), next frame
  put it back (`SpriteX8_RPLC_R`, or BLIT with an all-zero mask) **in reverse draw order** (the
  second saved area contains the first sprite), and redraw the full background only on camera moves
  or animations (`Force_Update`). Needs one persistent back buffer (page flipping would need one
  set of saved areas per buffer), then a partial 160×100 copy to the visible planes.
- **Keep the HUD out of the scroll**: bake the static HUD rows into the background layer and scroll
  only the play rows (ExciteBike: rows 5–100, "~8 fps"); redraw only the numbers that change, when
  they change (Sumo redraws its timer only when the seconds change; Hockey's 3 `sprintf` per frame
  are the anti-pattern).
- **Scrolling a strip with an incoming column** without a hidden margin (ExciteBike, 92+ path):
  after `ScrollLeft160_R`, OR the new tile column `strip >> (x & 7)` into the last visible byte of
  each row every frame: OR is idempotent and last frame's bits have shifted into place.
- **Parallax band from one tile**: rotate the 16 words of one 16×16 tile by a pixel each frame
  (about 32 word operations) and fill the band with it (Sumo tournament screen).
- **Wavy / distortion effects during the copy**: shift each row by a small offset from a triangle
  table while copying hidden → visible (Sumo, Hockey menus): no extra pass.
- **Two tile maps for animated crowds**: build `map[2][h][w]` once with the animated tiles out of
  phase (`map[i] = t`, `map[!i] = t + 1`, `i` random per cell); animate by drawing the other map.
- **Self-erasing points** (TICT starfield): each particle stores the byte offset and bit number it
  was drawn at; per frame `bclr` the old one, `bset` the new one. No screen clear at all: the cost
  is proportional to the number of particles, not to the screen size.
- Region full/empty tests: compare aligned `long`s (or `long long`s) against 0 / −1 instead of
  testing pixels.

- **Draw only what changes.** Redraw the background under moving sprites (ExtGraph
  `FastGetBkgrnd*`/`FastPutBkgrnd*`), update the HUD only when a value changes (Puzzle Bobble's
  `score_refresh` flag), use dirty rectangles instead of full-screen redraws.
- Prefer sprite routines to pixel loops, and the non-clipped `Fast…`/`Sprite…` routines when you
  can guarantee coordinates are on screen (clipping costs; ExtGraph notes that clipped 32-pixel
  sprites get slow near the left and right edges).
- **Preshifted sprites** (`preshift.h`) trade memory for speed: 8 pre-rotated copies remove the
  per-pixel shifting.
- **Tilemap engine** (`tilemap.h`/`tilemap.a`, ExtGraph) for scrolling worlds; it needs the forked
  grayscale support `tools/extgraph/lib/gray.o` for double buffering.
- Grayscale has a cost: its auto-int 1 handler runs 256 times per second and each frame needs two
  planes to be drawn and copied. Use black and white when the game does not need grays.
- On the TI-89 only 160×100 pixels are visible: avoid spending time on the rest of the 240×128
  buffer (ExtGraph has 160-column variants such as `FastCopyScreen160to240_R`,
  `FastFillRect160_R`; read their prototypes in `extgraph.h` before use).

## 8. Game-loop strategy and heuristics

- **Budget per frame**: decide a frame rate (20–30 fps) and give each subsystem a budget; show a
  tick counter on screen during development to see the real frame time.
- **Fixed timestep driven by the timer** (patterns doc), with frame skipping for drawing when
  late: the game keeps the same speed on HW1/HW2/HW3.
- **Spread expensive work over frames**: AI every 2nd–4th frame, pathfinding a few nodes per frame,
  collision checks for distant objects less often. Time-slice anything longer than a frame.
- **Precompute** at start-up: trig/sqrt/division tables, row offsets, sprite masks, level data
  decoded once.
- **Cheap tests first**: grid cell or bounding box before pixel-perfect collision; early exit as
  soon as the answer is known; spatial buckets (grid of lists) instead of all-pairs checks.
- **Incremental updates** instead of recomputing: keep counters, running sums, "dirty" flags.
- **Compact state**: bit masks and bitboards for grids (a 16×16 grid is 16 `short`s), small integer
  types, lookup tables instead of branches.
- Avoid recursion (small stack): iterative BFS/DFS with `static` queues (Puzzle Bobble's
  `three()`/`fall()` pattern).
- **Budget reference for AI**: TI-Chess (a strong engine for the platform) reaches **~200 nodes/s**
  with a full evaluation at each node (~60,000 cycles per node). An AI that runs during the game can
  afford a handful of nodes per frame, not thousands: evaluate cheaply, search shallow, cache.
  Worked recipes (game-tree search, Tetris AI, A*, area capture, 3D pipeline, sorting): see
  `ti68k-game-techniques.md`.
- Anti-pattern seen in Tankers: a `divide()` by repeated subtraction used for `/16`, O(quotient)
  instead of one shift.
- Sparse special cells (doors, switches) on a big map (FAT engine): a small array sorted by cell,
  binary search, plus a one-entry "last hit" cache checked first; better than a flag per cell when
  they are rare.
- Square roots beyond `sqrt_tab`'s 0–255: find the leading byte by comparisons, look up a 256-entry
  table, shift, then 0–2 Newton steps (FAT `fastsqrt.c`): covers 32-bit inputs.
- **Hidden divisions in innocent code**: TI-Chess computes `square / 10` and `square % 10` on a
  signed `short` (two `divs.w`) 4–6 times per move, and `counter % 20` / `% 50` / `% 200` at every
  node. Replace them with a table (`file_of[120]`) and count-down counters (`if (!--n) { n = 20; … }`).
- Heuristics over exact algorithms when the player cannot tell the difference (approximate
  distances, limited search depth, cached results).

## 9. Compiler flags

- `ti-cc` adds dead-code removal and register parameters (`-mregparm=5`, `--remove-unused`…;
  patterns §1): ~6 % smaller on Puzzle Bobble, same speed on our demos (**verified**).
- Default optimisation (`ti-cc`) is **-Os**. -O2 is **not** reliably faster on GCC4TI: measured slower on
  16-bit multiplies in loops (above) and on the empty loop. -O3 inlines and unrolls a lot and
  grows the program. If you try another level, measure the hot paths and read the asm.
- Program size matters too (RAM, AMS limits on ASM programs, compressed launchers): keep -Os and
  optimise the few hot functions by hand.
- GCC m68k traps reported for newer GCCs, checked on GCC4TI 4.1.2 -Os (**verified in the asm**,
  `experiments/codegen/traps.c`): byte loads are widened with `clr.w` + `move.b` per iteration (not
  `andi.l #255`; cheap); `(x & 0xFF) * 320` → shifts + add, or one `muls.w` in 16 bits (no
  `__mulsi3`); bitfield tests compile to `tst.b` / `and.w` (fine to read); small structs
  (`struct {short x, y;}`) return in `d0` (`-freg-struct-return`), larger ones through a hidden
  pointer: pass an out-pointer; `p == 0` costs `cmp.w #0,%a0`. The real traps are the 32-bit
  division and the loop forms above.

## 10. How to measure

- **Clock-dependent runtime checks (headless verified, Yoshi damage):**
  `RT_CYCLES` has no interrupt clock. Its `rt_ticks()` must use the PC virtual
  clock, `((u32)rt_frame * RT_FRAME_TICKS2) >> 1`, with16-bit wrap. Reading the
  hardware tick global freezes countdowns while movement/screen-only probes
  can still pass. Compare timer fields and expiry boundaries PC/TI.
- **Static image windows (datasheet-cycle measured, Yoshi):** dispatch the
  horizontal shift once per plane instead of calling a row helper100 times.
  An overlapping32-bit read needs only even alignment on MC68000. At offset7,
  shifting right9 and taking the low word beats left7 followed by clearing/
  swapping. Pre-render tiny HUD panels and group tear pixels into one masked
  sprite. Five actors, six eggs, detached baby and HUD peak202398 cycles,
  below210000; pixel-equivalent TileMap cold/cache refresh peaked253604.
  These are compiled headless measurements, not TiEmu hardware verification.

0. **First choice: `tools/bin/ti-cycles`** (**verified**, `tools/m68kbench/test/cyctest.c`: `nop`
   4, `lsl.l #8` 24, `lsl.w #7` 20, `movem.l` of 10 registers 92, `mulu.w #$FFFF` 70 (+4 for the
   immediate), `mulu.w #0` 38, `move.b (a0,d1.w),d0` 14, `divu.w` 140: the datasheet, where TiEmu
   gives ~12 for any `movem` and ignores the 2n of shifts). It runs the `.89z` on the PC under
   Musashi, headless, in a fraction of a second, reports the cycles of the zones marked with
   `tools/m68kbench/bench.h` and writes the virtual screen from memory as a PNG (`BENCH_SHOT`), so a
   benchmark also checks the picture (a checksum per scenario: an optimisation must not change
   it). Runtime games: `make cycles` / `make xcheck` (the VAT with `--file`, saves, the AMS
   fonts and key scripts are emulated; **verified**: FFA's 18 scenarios, life, flappy and the demo
   give the PC's checksums). Limits: no I/O ports (build a `-DBENCH` variant without grayscale, keyboard and
   interrupts); AMS ROM calls are emulated with an estimated cost, listed apart; no wait states
   (the TI-89 has none). Example: `games/mode7/` (`make bench`, `tools/bench.py`).
1. Put the candidate code in a copy of `experiments/bench/bench.c` (volatile inputs, volatile
   sink, loop-carried dependencies so GCC cannot hoist or fold the work; beware strength
   reduction, which turned `(la + i) * lb` into additions until XOR was used).
2. Build it with the same flags as the game, run it with `ti-run`, read the screen with `ti-shot`.
3. Confirm with the assembly (`-S`) that the loop contains what you think.
   If the hot path is `movem`- or shift-heavy (ExtGraph, sprite and scroll routines, hand asm), add a
   datasheet cycle count: TiEmu undercounts those instructions (§1).
4. Keep the result here (mark it **verified**) if it is a general lesson.
5. **Fine timer for profiling on a real calculator** (**verified** in TiEmu, HW2 and HW3,
   `experiments/hwsync/hwsync.c`). Clearing bits 5–4 of port `0x600015` makes the int-5 counter
   `0x600017` count at OSC2/32: 16,074 counts/s measured, ~16,384 nominal, about 61 µs or 730 cycles
   per count. Redirect int 5 to `DUMMY_HANDLER`, then read the byte before and after the code. The
   counter reloads from the start value (`PRG_getStart()`) after 255, so account for that. Restore
   both `0x600015` and the start value afterwards. This is the only way to time `movem`- and
   shift-heavy code correctly, since TiEmu undercounts them (§1).

## 11. When to reach for assembly

Full guide (recipes, register convention, 68000 cycle table, idioms, porting kernel-era ASM):
`ti68k-asm.md`. Measured: a Mode 7 inner loop went from 4,315 cycles (C) to 2,674 (asm), 1.6×.

Only after measuring that a specific C routine is the bottleneck and after the options above
(better algorithm, tables, pointers, ExtGraph routine, inline `asm` for one instruction). Then
**ask the user first** (project rule). Candidates: inner rendering loops (span filling, texture
mapping), bulk copies not covered by ExtGraph.

### Text in a dialogue box (verified, TiEmu, FFA scenario 53, 2026-09-27)

- 72 characters of `draw_text` (6x8 font, both planes) redrawn every frame: ~125k cycles.
- The same page drawn once into a plane-format RAM buffer (only the characters the
  typewriter adds), then shown as 5 opaque 32x31 sprites: still ~75k (ExtGraph shifts the
  rows); copied byte by byte into both planes at a byte-aligned x: the whole box 116k (the
  frame, the name tag and the copy). Frame 281k -> 218k. Draw static text once; place it on
  a byte boundary so it can be copied without shifts.

### Sparse scenery mutations (measured with ti-cycles, Minish Woods M3)

- For an immutable scene plus persistent changes, store XOR deltas between
  original and replacement planes. Draw each changed patch once after the
  base scene and before actors. An XOR sprite avoids masked read/merge work;
  a restored state needs only its change flags, with no mutable world bitmap.
- Discard unchanged rows offline and deduplicate identical patches. For the
  70% forest view, sixteen pre-shifted 32-bit forms of the small deltas fit in
  a 54964-byte archived bank together with action art. Word-aligned interior
  drawing is two plain long XORs per row in C; clipped patches use ExtGraph.
- Restore foreground over the actual outlined silhouette bounds, rather than
  a padded 32/64-pixel sprite canvas. This matters when transparent margins
  overlap dense canopy. The 70% dense replay fell from 391118 cycles with
  masked patches to 331056 after XOR deltas, pre-shifts and tighter bounds.
  All 53 bushes cut, every attack pose and 32 camera offsets were checked;
  PC/TI screens agree. These are datasheet-cycle measurements, not a TiEmu
  hardware timing claim; no new assembly was authored.
