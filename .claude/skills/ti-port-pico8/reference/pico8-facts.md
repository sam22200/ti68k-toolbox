# PICO-8 facts for a TI-89 port

What a port depends on: the machine, the number semantics, the memory map, the API, the cart
formats, where carts and ports are, and the pitfalls. Researched 2026-10-01 from the official
manual v0.2.7 [M] (`https://www.lexaloffle.com/dl/docs/pico-8_manual.html`), the PICO-8 fandom
wiki [W:Page] (`https://pico-8.fandom.com/wiki/<Page>`, blocks curl: use a browser), the
sources of zepto8/z8lua, ccleste, fake-08 and the C# Celeste Classic. Lines marked
**verified** were run here (z8lua, shrinko8, `p8trace.py`, `test_p8num.sh`, ti-cycles).

## Contents

1. Hardware spec and program contract
2. Number type
3. Memory map
4. Graphics API semantics
5. Input
6. Audio
7. Cartridge formats, tools, where carts are
8. Lua dialect specifics
9. Existing ports, emulators and transpilers
10. Porting pitfalls

---------------------------------------------------------------------------------------------------
## 1. Hardware spec and program contract

| Item | Value | Source |
|---|---|---|
| Display | 128x128, fixed 16-colour palette (4 bits/pixel) | [M] Specifications |
| Input | 6-button controllers (L R U D O X), up to 8 players (`btn(b, pl)`, pl 0..7) | [M] |
| Cart | "32k data encoded as png files" (0x8000 bytes: 0x4300 data + 0x3d00 code) | [M], [W:P8PNGFileFormat] |
| Sound | 4 channels, 64 SFX, 64 music patterns | [M] |
| Code | max 8192 tokens; char limit 65535 (shrinko8 reports 26589 chars = 41 %); compressed code < 15360 bytes for .p8.png/.rom (not enforced for .p8) | [M] Code Limits, §1.5 |
| CPU | "4M vm insts/sec"; virtual 8 MHz (8,388,608 = 2^23 cycles/s), ~2 cycles per Lua VM instruction | [M] CPU, [W:CPU] |
| Per frame | 279,620 virtual cycles at 30 fps, 139,810 at 60 fps | [W:CPU] |
| Sprites | 128 8x8 sprites + 128 shared with the lower half of the map (256 total, 128x128 sheet) | [M] |
| Map | 128x32 tiles + 128x32 shared (128x64 if the lower sprite sheet is unused) | [M] |
| Base RAM | 64 KiB addressable (peek/poke); Lua RAM 2 MB (code + variables, not addressable) | [M] Memory |

The ~2 cycles/VM-instruction model and the per-call costs in [W:CPU] (cycles are virtual units):
- local read 0, global/upvalue read 2, `+ -` 1, `* / % \` 2, bitwise ops 1, compare 2 (+2 outside an if),
  table access 2, `#` 2, function call 4 + 1 per argument, return 2, numeric `for` 7 + 2n, `while` 2 + 4n,
  `..` 4, `@`/`%`/`$` peek operators 1.
- "System" costs: `cls()` 2048; `spr()` 2 per pixel of the rectangle (transparent included, 128 for 8x8);
  `sspr()` 2 per destination pixel; `rectfill` 2*max(1, flr(pixels/16)); `map()` 2*max(1, n*64) for n
  non-zero cells drawn (~128/tile, like spr); `print` 4 + 16 per character; `circfill` 2*n*flr((n+9)/4);
  `line` 2*ceil(pixels/2); `sfx`/`music` 32. (Measured on old versions 0.1.11/0.1.12, marked "out of date".)
- Use: a cart at 100 % of the PICO-8 CPU does ~140k VM instructions per 30 fps frame. Native C on the
  68000 runs far fewer high-level operations than that per frame, so profile the target cart (`stat(1)`
  in PICO-8) before choosing it. Celeste is light.

### Program structure [M §5]
- All code tabs are concatenated left to right and run once at load (top-level statements run first),
  then `_init()` once, then the loop.
- `_update()`: called once per update at 30 fps. `_draw()`: "called once per visible frame".
- If `_draw` cannot finish in time, PICO-8 drops to 15 fps and calls `_update()` twice per visible
  frame (so `_draw` is skipped every other update).
- `_update60()` defined instead of `_update()`: both `_update60` and `_draw` run at 60 fps, with half
  the CPU per frame. Slow hosts or web players may run at 30 fps (or 15), calling `_update60` several
  times per `_draw`.
- `btnp` state is reset at the start of each `_update`/`_update60`; `time()`/`t()` counts update calls
  (1/30 s or 1/60 s steps), not real time: several calls in one frame return the same value.
- `flip()`: the custom main loop pattern `::_:: cls() ... flip() goto _` (tweetcarts, old carts). Not
  needed when `_draw`/`_update` exist. If a frame ends without `flip()` and `_draw` is not running,
  the back buffer is copied to the screen anyway.
- `#include file.lua` / `cart.p8:1` is flattened into the cart when it is saved as .p8.png.

### Palette (screen palette indices 0..15 = system colours 0..15) [W:Palette]
| # | name | hex | | # | name | hex |
|---|---|---|---|---|---|---|
| 0 | black | #000000 | | 8 | red | #FF004D |
| 1 | dark-blue | #1D2B53 | | 9 | orange | #FFA300 |
| 2 | dark-purple | #7E2553 | | 10 | yellow | #FFEC27 |
| 3 | dark-green | #008751 | | 11 | green | #00E436 |
| 4 | brown | #AB5236 | | 12 | blue | #29ADFF |
| 5 | dark-grey | #5F574F | | 13 | lavender (manual: indigo) | #83769C |
| 6 | light-grey | #C2C3C7 | | 14 | pink | #FF77A8 |
| 7 | white | #FFF1E8 | | 15 | light-peach (manual: peach) | #FFCCAA |

Undocumented "secret" colours 128..143 (also -16..-1), only reachable through the screen palette
`pal(c, 128+i, 1)` or poking 0x5f10..0x5f1f; zep allows their use but they are not official:
| # | name | hex | | # | name | hex |
|---|---|---|---|---|---|---|
| 128 | brownish-black | #291814 | | 136 | dark-red | #BE1250 |
| 129 | darker-blue | #111D35 | | 137 | dark-orange | #FF6C24 |
| 130 | darker-purple | #422136 | | 138 | lime-green | #A8E72E |
| 131 | blue-green | #125359 | | 139 | medium-green | #00B543 |
| 132 | dark-brown | #742F29 | | 140 | true-blue | #065AB5 |
| 133 | darker-grey | #49333B | | 141 | mauve | #754665 |
| 134 | medium-grey | #A28879 | | 142 | dark-peach | #FF6E59 |
| 135 | light-yellow | #F3EF7D | | 143 | peach | #FF9D81 |
System indices are masked with 0x8f (16..127 alias 0..15, 144..255 alias 128..143).

Rec.601 luma of the base 16 (computed here; a first guess for a 4-grey mapping, 0=white..3=black
to be decided per game, never global):
0:0 1:43 2:69 3:88 4:105 5:88 6:195 7:244 8:85 9:172 10:219 11:140 12:143 13:126 14:165 15:215.
Pitfall: red(8) = 85, dark-green(3) = 88, dark-grey(5) = 88: hue-only contrasts disappear in greys.
Map per game (palette table + hand overrides) and check sprite readability (CLAUDE.md visibility rule).

---------------------------------------------------------------------------------------------------
## 2. Number type (the most important thing to get right)

- One number type: signed 16.16 fixed point in 32 bits, two's complement. Range -32768.0
  (0x8000.0000) .. 32767.99998 (0x7fff.ffff), step 1/65536 ≈ 0.0000153 [M], [W:Lua].
- Decimal literals: z8lua **truncates** them (**verified**: `0.6` = 0x0.9999, `0.21` = 0x0.35c2,
  `-0.21` = -0x0.35c2 = 0xffff.ca3e); the wiki says "nearest". The port compares against z8lua, so
  C constants take the bits z8lua prints (`tostr(v, true)`). Hex/binary literals may have
  fractions (`0x11.4` = 17.25, `0b10001.01`).
- Overflow wraps (32-bit integer arithmetic): a counter incremented once per frame overflows after
  ~18 min at 30 fps [M Quirks]. `128*128+128*128` overflows [W:Lua].
- Multiply: `(int64)a*b >> 16` (arithmetic shift, i.e. floor of the exact product) [z8lua fix32.h].
  Pitfall: ccleste's fixed-point mode uses `/ 65536` (truncation toward zero), which differs from
  PICO-8 for negative products by 1 ulp.
- Divide: `(int64)a * 65536 / b` (C truncation toward zero: **verified** `-0x0.0001 / 2 = 0`
  while `-0x0.0001 * 0.5 = -0x0.0001`, so `x / 2` is not `x * 0.5` nor `x >> 1` for negatives), special case `x / 1 == x`; if `b == 0` or the result
  does not fit: 0x7fff.ffff when the signs of a and b agree (also 0/0), 0x8000.0001 (= -0x7fff.ffff)
  otherwise [M Types], [z8lua]. ccleste returns ±1 for /0 (wrong).
- `%`: result always in [0, |b|) ("PICO-8 always returns positive values"; uses abs(b)); `x % 0 == 0`
  since 0.2.5f [z8lua]. So `-1 % 8 == 7` (Lua floor-mod; C `%` gives -1).
- `a \ b` = `flr(a / b)` (integer division with floor): `9\2 == 4`, `-9\2 == -5` [M].
- `^` = power (right associative, priority above unary minus: `-2^2 == -4`). Expensive in PICO-8.
  zepto8 computes it with double `pow`. Rarely used in game logic.
- `flr(x)` = `x & 0xffff0000` (floor, also for negatives: `flr(-2.3) == -3`, `flr(-5.2) == -6`);
  `ceil(x) = -flr(-x)`; `x & -1` and `x \ 1` are flr idioms in golfed code [W:Flr].
- `abs(0x8000) == 0x7fff.ffff` (0.2.3+, saturates) [z8lua].
- `sgn(x)` = 1 if x >= 0 else -1. **`sgn(0) == 1`** [M Quirks], [W:Sgn]. Celeste avoids it and
  defines its own `sign(v)` returning 0 for 0. The C# Celeste emulator maps `sign` to `Math.Sign`.
- `min/max/mid(x,y,z)`: `mid` returns the middle value, argument order irrelevant (`mid(7,5,10) == 7`).
  Missing args are 0 (`mid(x, lo)` clamps against 0).
- `sqrt`: bitwise integer square root of `x<<16` (exact algorithm in z8lua `pico8_sqrt`), 0 for x <= 0.
- `rnd(x)`: 0 <= n < x with fraction; `rnd()` = `rnd(1)`; `rnd(tbl)` returns a random element
  `tbl[1..#tbl]`; negative x behaves as unsigned (`rnd(-1)` = `rnd(0xffff.ffff)`) [W:Rnd].
  Exact algorithm (decompiled; identical in zepto8 vm.cpp and ccleste celeste.c; state mirrored at
  0x5f44..0x5f4b):
  ```c
  uint32_t a, b;                       /* PRNG state */
  void step(void) { a = ((a >> 16) | (a << 16)) + b; b += a; }
  int32_t rnd_bits(uint32_t range_bits) { /* range_bits = raw 16.16 bits of x, 0x10000 for rnd() */
      step(); return range_bits ? a % range_bits : 0; }   /* result is raw 16.16 bits */
  void srand(int32_t seed_bits) {       /* raw bits of the argument */
      seed_bits &= 0x7fffffff;          /* PICO-8 drops the MSB */
      b = seed_bits ? seed_bits : 0xdeadbeef;
      a = b ^ 0xbead29ba;
      for (int i = 0; i < 32; i++) step(); }
  ```
  Seed is randomised at cart start [M]. Implemented in `scripts/p8shim.lua` and
  `assets/p8num.h` (start state `srand(0)` in both; **verified** equal over 2,306 values, and
  `srand(0)` gives ccleste's `0x60009755`). On the 68000, `rnd(n)` for an integer n is exact with
  one `divu.w`: `a % (n << 16) = ((a >> 16) % n) << 16 | (a & 0xffff)` (`p8_rndi`); a
  fractional range needs a 32-bit modulo (`__umodsi3`, slow: cosmetic use only).
- Trig uses turns (1.0 = full circle). `cos(x)` normal; `sin(x)` is inverted: `sin(0.25) == -1`,
  `sin(0.75) == 1`, `sin(0.125) == -0.7071` [M], [W:Sin]. Angles therefore run clockwise on screen
  (y down). Exact PICO-8 values: z8lua `sin_helper` uses a 4096-entry `uint16_t` correction table
  (8 KB) and rounds the last 2 bits; `cos(x) = sin_helper(x - 0.25)`. ccleste uses a 4098-entry int32
  table (16 KB). A 1024-entry int16 quarter table is enough if bit-exactness is not needed.
  **Verified z8lua bug**: `cos(0)` = 1.1055 and `sin(0.75)` = 1.1055 (the table is read one past
  its end at the quarter turns); `p8shim.lua` clamps to ±1.
- `atan2(dx, dy)` returns 0..1, **x first** (unlike C `atan2(y, x)`), and with the inverted y:
  `atan2(1,0)=0`, `atan2(0,-1)=0.25`, `atan2(-1,0)=0.5`, `atan2(0,1)=0.75`, `atan2(1,1)=0.875`,
  `atan2(0,0)=0.25` [W:Atan2]. So `cos(atan2(dx,dy))*r, sin(atan2(dx,dy))*r` points toward (dx,dy).
  Exact algorithm in z8lua `pico8_atan2` (2049-entry `uint16_t` table, 4 KB).
- Bitwise: `band bor bxor bnot shl shr lshr rotl rotr` and operators `& | ^^ (or ~) ~ << >> >>> <<> >><`,
  acting on all 32 bits (fraction included). `shr`/`>>` arithmetic, `lshr`/`>>>` logical
  (`-2 >>> 1 == 32767`). Shift by n >= 32 gives 0 (`<<`, `>>>`) or the sign fill (`>>` clamps to 31);
  negative n shifts the other way; rotations use n & 31 [z8lua]. Function forms treat non-numbers as 0,
  operator forms raise an error [M].
- `tostr(v, flags)`: 0x1 hex `0x0011.0000`, 0x2 as 32-bit integer; numbers print rounded to 4
  decimals (`32767.99999` prints as 32768.0). `tostr(nil) == "[nil]"`, `tostr() == ""`.
  `tonum(s, flags)`: 0x1 hex without prefix, 0x2 signed 32-bit integer >> 16, 0x4 return 0 on failure;
  `tonum("hoge")` returns nothing; out-of-range values wrap [M], [W:Tonum]. Arithmetic coerces strings
  (`2+"3" == 5`).
- Number to string conversion matters for score displays: decimal, at most 4 fraction digits,
  trailing zeros removed (`1/3` prints `0.3333`).

---------------------------------------------------------------------------------------------------
## 3. Memory map (base RAM, 64 KiB) [M Memory], [W:Memory]

| Range | Size | Content |
|---|---|---|
| 0x0000-0x0fff | 4 KiB | sprite sheet rows 0-63 (sprites 0-127) |
| 0x1000-0x1fff | 4 KiB | sprite sheet rows 64-127 (sprites 128-255) = map rows 32-63 (shared) |
| 0x2000-0x2fff | 4 KiB | map rows 0-31 (128 bytes per row, 1 byte per tile) |
| 0x3000-0x30ff | 256 B | sprite flags, 1 byte per sprite (bit0 red=1 ... bit7 peach=128) |
| 0x3100-0x31ff | 256 B | music: 64 patterns x 4 bytes (bit6 = channel off, bits0-5 = sfx; bit7 of bytes 0/1/2 = loop start / loop end / stop) |
| 0x3200-0x42ff | 4352 B | 64 SFX x 68 bytes (32 notes x 2 bytes + editor/filter byte, speed, loop start, loop end) |
| 0x4300-0x55ff | 4864 B | general use (not loaded from cart; survives `load()`) |
| 0x5600-0x5dff | 2 KiB | general use / custom font (0.2.2+, P8SCII `\014`) |
| 0x5e00-0x5eff | 256 B | persistent cart data (64 numbers) once `cartdata(id)` is called |
| 0x5f00-0x5f3f | 64 B | draw state |
| 0x5f40-0x5f7f | 64 B | hardware state |
| 0x5f80-0x5fff | 128 B | GPIO |
| 0x6000-0x7fff | 8 KiB | screen, 128x128 4 bpp |
| 0x8000-0xffff | 32 KiB | general use / extended map (0.2.4+) / extra sprite sheets (0.2.6+) |

Pixel format (sprite sheet and screen): 2 pixels per byte, **low nibble = left (even) pixel**, high
nibble = right pixel; rows of 64 bytes. Sprite n starts at `512*(n\16) + 4*(n%16)`; next row +64.
Screen pixel (x,y) is at `0x6000 + y*64 + x\2`.

Draw state, the addresses carts actually poke:
- 0x5f00-0x5f0f draw palette (`pal(c0,c1)`; bit 0x10 of an entry = transparent, `palt`).
- 0x5f10-0x5f1f screen (display) palette (`pal(c0,c1,1)`): fades, secret colours.
- 0x5f20-0x5f23 clip x0,y0,x1,y1 (exclusive end; `clip()` clamps to 0..128).
- 0x5f24 print left margin; 0x5f25 pen colour (low nibble; high nibble = fill-pattern 2nd colour);
  0x5f26/27 print cursor x,y.
- 0x5f28-0x5f2b camera x,y as int16 little-endian.
- 0x5f2c screen mode: 0 normal, 1 64x128 stretch, 2 128x64, **3 = 64x64 (2x zoom)**, 5/6/7 mirror,
  129/130/131 flip, 133/134/135 rotate. A 64x64 cart (`poke(0x5f2c,3)`) fits 160x100 easily.
- 0x5f2d devkit mouse/keyboard (flags 1 enable, 2 mouse buttons as btn 4..6, 4 pointer lock).
- 0x5f2e palette persistence bits; 0x5f30 = 1 suppresses next pause menu.
- 0x5f31-0x5f32 fill pattern (16 bits, LSB first), 0x5f33 fill flags (1 transparent, 2 sprites, 4 global).
- 0x5f34: bit0 = colour arguments carry fill pattern bits (0x1000.0000 etc.), bit1 = inversion.
- 0x5f35 line endpoint invalid; 0x5f3c-0x5f3f last line endpoint.
- 0x5f36 misc: 2 circle .5 radius, 4 no auto newline in print, 8 sprite 0 drawn by map/tline,
  16 defaults for sget/mget/pget at 0x5f59/0x5f5a/0x5f5b, 64 no print scrolling, 128 print wrap.
- 0x5f37 = 1: do not reload cart ROM after leaving editors (editor only).
- 0x5f38-0x5f3b tline wrap masks and offsets.
Hardware state:
- 0x5f40-0x5f43 audio effects per channel; 0x5f44-0x5f4b PRNG state (see §2).
- 0x5f4c-0x5f53 current button bitmask for players 0..7 (0x5f4c = `btn() & 0x3f`).
- 0x5f54 GFX mapping (0x00 default, 0x60 = use screen as sprite sheet); 0x5f55 SCREEN (0x60 default,
  0x00 = draw into the sprite sheet); 0x5f56 MAP base (0x20 default, 0x10..0x2f or >= 0x80);
  0x5f57 map width (0 = 256, default 128).
- 0x5f5c btnp initial delay, 0x5f5d repeat delay (frames at 30 fps; 0 = default 15/4; 255 = never).
- 0x5f5f-0x5f7f high-colour / per-scanline palette modes (rare, ignore).

API a port has to emulate when the cart uses memory tricks:
- `peek(a,[n])` (n <= 8192 results), `poke(a, v1, v2, ...)`, `peek2/poke2` (16-bit LE, value
  0xffff.0000 bits), `peek4/poke4` (32-bit LE, the full 16.16 value), operators `@a %a $a`;
  `memcpy(dst, src, len)` (overlap allowed), `memset(dst, val, len)`, `reload(dst, src, len, [file])`
  (from cart ROM), `cstore(...)`; `cartdata(id)`, `dget(i)`, `dset(i, v)` (i 0..63).
- Typical uses: copying map/gfx chunks (level decompression into 0x2000), screen effects reading
  0x6000 (`memcpy(0x6000, 0x8000, 0x2000)` back buffers, screen shake by copying rows), palette fades
  via 0x5f10, `poke(0x5f2c,3)`, `poke(0x5f5c,255)`, saving to 0x5e00. A port that keeps a real 64 KiB
  `uint8_t ram[0x10000]` with these exact layouts makes all of this work; a port that converts assets
  to TI formats (pre-shifted 2-plane sprites) must detect and rewrite every `peek/poke/memcpy`.
- 64 KiB is ~1/3 of the TI's ~190 KB free RAM; the gfx+map+flags part (0x0000-0x30ff, 12.25 KiB) is
  the only part most carts need.

---------------------------------------------------------------------------------------------------
## 4. Graphics API semantics [M Graphics], [W pages]

All drawing goes through the draw state: camera offset, draw palette, transparency, clip rect, pen
colour, fill pattern. Reset when the program runs or by `reset()`. Coordinates are 16.16 numbers
converted to integers by taking the integer part of the fixed value, i.e. **floor**
(zepto8 converts with `m_bits >> 16`), then the camera is subtracted (`x -= camera.x`).
- `cls([col])`: fills with col (default 0) **and resets the clip rect**. Cursor goes to 0,0.
- `camera([x, y])`: offset -x,-y for all subsequent draws; `camera()` resets. Stored as int16
  (floored). Screen shake = `camera(rnd(4)-2, rnd(4)-2)` then `camera()` before the HUD.
- `clip(x, y, w, h, [clip_previous])`, `clip()` resets.
- `pset(x,y,[c])`, `pget(x,y)` (0 out of bounds), `sget/sset` on the sprite sheet, `color(c)`
  (default 6), `cursor(x,y,[c])`.
- `line(x0,y0,[x1,y1,[c]])`: missing end point = continue from the last end point; `line()` breaks.
- `rect/rectfill(x0,y0,x1,y1,[c])`: corners inclusive, any order. `rrect/rrectfill(x,y,w,h,r,[c])` (0.2.6+).
- `circ/circfill(x,y,r,[c])`: r < 0 draws nothing; `oval/ovalfill(x0,y0,x1,y1,[c])`.
- `spr(n, x, y, [w, h], [flip_x], [flip_y])`: n 0..255 (cost 0 if out of range), w/h in sprites
  (fractions allowed: `spr(1,x,y,0.5,0.5)` draws the top-left 4x4), flips are booleans. Colour 0 is
  transparent by default. Pixels go through the draw palette, then transparency is tested on the
  palette entry.
- `sspr(sx, sy, sw, sh, dx, dy, [dw, dh], [flip_x], [flip_y])`: stretch blit in sheet pixels (scaling,
  rotation-like effects, big titles).
- `map(cel_x, cel_y, [sx, sy], [cel_w, cel_h], [layers])` (old name `mapdraw`): draws tiles to screen
  pixels sx,sy (camera applies). Tile 0 is not drawn (`poke(0x5f36,8)` to draw it). `cel_w/h` default
  to the whole map. Layers: "only sprites with matching flags are drawn, e.g. 0x5 = flag 0 and 2"
  [M]. Implementations: zepto8 and fake-08 draw a tile when `(fget(tile) & layers) != 0` (any bit);
  the wiki [W:Map] says "flags set for every bit". Ambiguous; carts almost always pass a single bit
  (Celeste: `map(...,4)` bg, `2` terrain, `8` fg), where both readings agree. Test multi-bit masks in
  PICO-8 before relying on either.
- `mget(x,y)` (0 out of bounds), `mset(x,y,v)`; map rows >= 32 read 0x1000 (shared with sprites 128+).
- `fget(n,[f])`, `fset(n,[f],v)`: 8 flags per sprite; without f the whole byte (`fset(2, 1|2|8)`).
- `pal(c0, c1, [p])`: p=0 draw palette (affects later draws only), p=1 screen palette (whole frame,
  applied at flip: fades, flashes), p=2 secondary palette for fill patterns. `pal()` resets all
  palettes and transparency; `pal(p)` resets one; `pal(tbl, [p])` sets several (keys mod 16; 1-based
  table so colour 0 is the last entry of a 16-element list).
- `palt(c, [t])`: transparency per colour for spr/sspr/map/tline; `palt()` = only colour 0
  transparent; `palt(bitfield16)` sets all (bit 15 = colour 0).
- `fillp(p)`: 4x4 pattern, bit 15 = top-left, reading order (row 0 = 0x8000,0x4000,0x2000,0x1000 ...
  row 3 = 8,4,2,1). Checkerboard: `fillp(0b0011001111001100)` [M]. Set bits use the second colour (high nibble of the colour argument:
  `circfill(64,64,20,0x4e)` = brown/pink); fractional bits: 0b0.100 transparency (set bits not drawn),
  0b0.010 apply to sprites, 0b0.001 secondary palette global. Applies to circ, circfill, rect,
  rectfill, oval, ovalfill, pset, line. Default 0 (solid). On a 4-grey screen fill patterns are useful
  (dithered fades, shadows) and map directly to TI pixel masks.
- `tline(x0,y0,x1,y1,mx,my,[mdx,mdy],[layers])`: textured line sampling the map (Mode-7 floors,
  rotations, raycasters). Expensive per pixel; rewrite with the project's Mode-7 code if present.
- `print(str, x, y, [c])` / `print(str, [c])` / shorthand `?"text"` on one line. Returns the right-most
  x. Without coordinates it appends a newline and scrolls below y=122. Built-in font: glyphs 3x5 in a
  4x6 cell (4 px advance, 6 px line: 32 columns x 21 lines); characters >= 128 (P8SCII glyphs, e.g.
  the button symbols) are 8 px wide (7x5) [W:Print]. Letters typed in the editor are stored lowercase in .p8
  and shown as PICO-8's uppercase glyphs; uppercase in a .p8 file shows as the small "puny" font. P8SCII control codes `\n \f<c> \#<c> \^...` change colour, background,
  cursor, wide/tall text [M Appendix A]. The PICO-8 font as data: zepto8 and ccleste (`data/font.bmp`)
  ship one. The TI AMS 4x6 small font (`F_4x6`) has the same cell size: a direct substitute.
- `flip()`: present the back buffer and wait for the next frame (custom loops only).

Draw order = call order; no sprite priorities. A port must preserve the exact sequence (Celeste:
clouds, bg map layer 4, platforms, terrain layer 2, objects, fg layer 8, particles, HUD).

---------------------------------------------------------------------------------------------------
## 5. Input [M Input], [W:Btnp]

- `btn(b, [pl])`: b 0 left, 1 right, 2 up, 3 down, 4 O, 5 X; pl 0..7. `btn()` with no args = bitfield,
  P0 bits 0..5, P1 bits 8..13. Bit 6 = pause (`btn(6)`).
- Keyboard P0: arrows, O = Z / C / N, X = X / V / M. P1: S F E D, O = LShift, X = Tab / W / Q / A.
- Glyph globals: in the editor Shift-L/R/U/D/O/X type ⬅️ ➡️ ⬆️ ⬇️ 🅾️ ❎, predefined single-character
  globals equal to 0..5 (`btnp(⬅️)`); in .p8 text they are UTF-8 emoji. A translator must map these
  identifiers.
- `btnp(b, [pl])`: true on the frame the button goes down, then repeats: after 15 frames held, true
  every 4 frames (at 30 fps; doubled at 60 fps). zepto8 logic with i = frames held (1 on the press
  frame): `i == 1 || (delay != 255 && i > delay && (i - delay - 1) % rate == 0)`, delay = @0x5f5c or 15,
  rate = @0x5f5d or 4. `poke(0x5f5c,255)` = no repeat.
- Player 2 is rare on BBS games; on the TI there is one keyboard: map P0 to arrows + two keys
  (e.g. 2nd/alpha or shift/diamond, or F1/F2), drop or share P1.

---------------------------------------------------------------------------------------------------
## 6. Audio

- `sfx(n, [channel], [offset], [length])`: n 0..63, channel 0..3 (-1 auto, -2 stop this sfx on all);
  n = -1 stop, -2 release loop. `music(n, [fade_ms], [channel_mask])`: pattern 0..63, -1 stop.
  `stat(46..57)` report playback state (some carts sync gameplay or cutscenes to music via
  `stat(54)`/`stat(56)`: those need a fake clock).
- 22050 Hz synth, 8 waveforms + custom, 8 effects. Note duration = speed x 183 ticks (≈1/120 s).
- TI-89: no sound hardware in practice. Make `sfx`/`music` no-ops; return plausible `stat()` values
  (pattern advance from a frame counter if the game waits on music); replace gameplay-relevant
  sounds by visual cues (flash, small screen shake, icon). Celeste: 25 `sfx(`, 9 `music(` calls, none
  gameplay-critical.

---------------------------------------------------------------------------------------------------
## 7. Cartridge formats, tools, where carts are

### .p8 text format [W:P8FileFormat]
```
pico-8 cartridge // http://www.pico-8.com
version N             (integer that grows with releases; Celeste's cart says "version 5")
__lua__
...code (lowercase; glyphs as UTF-8)...
__gfx__   128 lines x 128 hex digits; one digit per pixel in pixel order (left pixel first)
__gff__   2 lines x 256 hex digits: 256 flag bytes, normal byte order (MSB nibble first)
__label__ 128 x 128 like __gfx__ (0-f and g-v for the 32 colours)
__map__   32 lines x 256 hex digits: 128 bytes per line, map rows 0-31, MSB nibble first
__sfx__   64 lines x 168 digits (4 header bytes + 32 notes x 5 digits: pitch 2, wave 1, vol 1, fx 1)
__music__ 64 lines "FF AABBCCDD" (flags byte; 0x41.. = channel silent)
```
- Order written by PICO-8: header, lua, gfx, gff, label, map, sfx, music (shrinko8 writes
  lua, gfx, map, gff, sfx, music, label; parsers must key on the `__xxx__` lines, not order).
- Missing sections or trailing lines = default (zeros); empty sections are omitted.
- __gfx__ to memory: for each pair of digits "ab" (pixel a left, pixel b right) the byte is
  `(b << 4) | a` (first digit = low nibble) [W:P8FileFormat]. Gfx row r is 64 bytes at `r*64`.
  The lower map half (rows 32..63) is always stored in __gfx__ rows 64..127, even when used as map:
  decode those gfx rows to bytes first; map row 32+k = the 128 bytes at `0x1000 + k*128`, i.e. gfx
  rows 64+2k and 65+2k, each tile byte = (second digit << 4) | first digit of its digit pair.
- __map__ bytes are plain hex pairs (first digit = high nibble).
- __gff__ bytes plain hex pairs (first digit = high nibble).

### .p8.png [W:P8PNGFileFormat]
- 160x205 RGBA PNG; each cart byte = 2 low bits of each channel, order A R G B (A holds bits 7-6),
  pixels left to right, top to bottom: 32,800 (0x8020) bytes.
- 0x0000-0x42ff: gfx, map, flags, music, sfx exactly as RAM.
- 0x4300-0x7fff: code. `\0pxa` header = new format (0.2.0+): bytes 4-5 decompressed length (big
  endian), 6-7 compressed length + 8; bit stream LSB first with move-to-front literals and LZ back
  references (offset 5/10/15 bits, length 3 + 3-bit groups). `:c:\0` = old format (pre-0.2.0):
  table of 59 common characters + LZ pairs `offset = (b0-0x3c)*16 + (b1&0xf)`, `length = (b1>>4)+2`.
  Otherwise plain ASCII up to the first 0.
- 0x8000 version id, 0x8001-0x8003 PICO-8 version, 0x8004 platform, 0x8006-0x8019 SHA1 (checked).
- Official C decompressor released by Lexaloffle: `https://github.com/dansanderson/lexaloffle`.

### Conversion tools (verified here: shrinko8 converted the Celeste png in < 1 s)
- shrinko8 (Python 3 + Pillow, MIT, active 2026): `https://github.com/thisismypassport/shrinko8`
  - `python shrinko8.py in.p8.png out.p8` (any of p8, png, rom, tiny-rom, lua, clip, url, js, pod,
    label, spritesheet), `--count` (tokens/chars/compressed), `--lint`, `--unminify` (readable code
    from minified carts: most BBS carts are not minified, but big ones are),
    `--extra-output sheet.png spritesheet` (128x128 sprite sheet PNG), `--format lua` to stdout.
  - Working command here: `tools/pyenv/bin/python shrinko8.py cart.p8.png cart.p8 --count`.
- picotool (Python 3, MIT, 2024): `https://github.com/dansanderson/picotool`: `p8tool stats`,
  `p8tool listlua cart.p8.png`, `p8tool writep8`, `printast` (a Lua AST of PICO-8 syntax, useful
  for a translator), `p8tool build`.
- PICO-8 itself: `pico8 foo.p8 -export foo.p8.png` (and the reverse); `load #id` / Splore to fetch
  BBS carts (needs a license).
- pico8-to-lua (Lua, Zlib): `https://github.com/benwiley4000/pico8-to-lua`: rewrites `+=`, `!=`,
  `if (c) stmt`, `?`, etc. to standard Lua, so a stock Lua parser can read the code.

### Where carts are
- Lexaloffle BBS: thread `https://www.lexaloffle.com/bbs/?tid=<tid>` (Celeste: tid=2145), carts
  category 7. Cart PNG URL (from the BBS page's own `get_cart_url` JS):
  - numeric cart id: `https://www.lexaloffle.com/bbs/cposts/<floor(id/10000)>/<id>.p8.png`
    (Celeste: `.../cposts/1/15133.p8.png`, verified HTTP 200, 160x205 PNG);
  - named cart id (newer carts, e.g. `mygame-3`): `https://www.lexaloffle.com/bbs/cposts/<first 2 chars of id>/<id>.p8.png`;
  - also `https://www.lexaloffle.com/bbs/get_cart.php?cat=7&play_src=0&lid=<id>` (verified 200 image/png).
- Splore (in PICO-8) browses the same BBS. Backup threads exist (`https://www.lexaloffle.com/bbs/?tid=31544`).
- Licences: carts can be tagged CC4-BY-NC-SA on submission; **untagged carts ("No License", e.g.
  Celeste) stay all rights reserved**, only Lexaloffle may distribute them on the BBS (zep,
  `https://www.lexaloffle.com/bbs/?tid=2184`). CC4-BY-NC-SA allows a non-commercial port with
  attribution under the same licence. For this repo: keep untagged carts and their derived assets
  local only (like `sources/`), commit only our engine code, or pick CC-tagged carts.

---------------------------------------------------------------------------------------------------
## 8. Lua dialect specifics a translator must handle

PICO-8 Lua = Lua 5.2 core without the standard library, numbers replaced by 16.16 fixed [M].
Syntax extensions (parsed by PICO-8's pre-processor, single-line only):
- Compound assignment `+= -= *= /= \= %= ^= ..= |= &= ^^= <<= >>= >>>= <<>= >><=` (whole statement on
  one line; there is no `~=` compound because `~=` is "not equal").
- `!=` = `~=`.
- Short if / while: `if (cond) stmt1 stmt2` (parentheses required, rest of the line is the body,
  optional `else` on the same line); `while (cond) stmt`.
- `?expr, x, y, c` = `print(...)` at the start of a line.
- `\` integer division, `@ % $` peek prefix operators, binary literals `0b1010.1`, `0x11.4`.
- `#include` (flattened in saved carts). Comments `//` are NOT valid; `--` and `--[[ ]]`.
- `goto` and labels (`::_::`) are Lua 5.2 features used by loops.
Semantics:
- Globals by default; any undeclared assignment creates a global (shrinko8 `--lint` lists them).
  `_ENV` tricks exist in token-golfed carts (`local _ENV = obj`).
- Upvalues and closures are everywhere: `foreach(objects, function(o) ... end)` (Celeste: 62
  anonymous `function(`), callbacks stored in tables, `menuitem` callbacks.
- Tables: 1-based sequences (`{11,12,13}[1] == 11`), `[0]=` allowed but ignored by `#`, `all`,
  `foreach`, `add`, `del`. Unset keys read nil. `#t` undefined with holes.
- `add(t, v, [i])` appends or inserts, returns v; `del(t, v)` removes the first element equal to v and
  shifts the rest (returns it); `deli(t, [i])` removes by index (last if omitted); `count(t, [v])`.
- `all(t)` iterates values in order; `foreach(t, f)` calls f(v). Both tolerate `del()` of the current
  element during iteration ([M] example deletes inside `for item in all(a)`): a C port iterating a
  pool must handle removal while iterating (iterate backwards or mark-and-sweep).
- `pairs(t)` order is not guaranteed (hash order); `next`. Code depending on pairs order is rare but
  breaks determinism tests.
- nil: arithmetic on nil is a runtime error; `x == nil` / `x ~= nil` checks are common for optional
  fields (Celeste `type.if_not_fruit ~= nil`); `and/or` as ternary (`a and b or c`, wrong when b is
  false/nil); only false and nil are falsy (**0 is true**).
- Strings: interned, immutable, 1-based. `sub(s, i, [j])` inclusive (`sub(s,5,9)`), `sub(s,i,true)`
  = one character; `#s`; `..` concatenation (numbers coerced); `ord(s,[i],[n])`, `chr(...)`,
  `split(s, [sep or width], [convert_numbers=true])` (`split("1,,2,") = {1,"",2,""}`), `tostr`,
  `tonum`, `type`. Carts store level data and sprite tables in long strings decoded at `_init`
  (`split`, `ord`, hex strings): the translator can pre-decode these at build time.
- Metatables (`setmetatable`, `__index` for classes, `__add` vectors), `rawget/rawset/rawequal/rawlen`,
  varargs `...`, `select`.
- Coroutines: `cocreate(f)`, `coresume(c, ...)` (returns true / false+error; errors inside do not stop
  the cart), `costatus(c)` ("running", "suspended", "dead"), `yield()`. Used for cutscenes, dialogue,
  animations, intro sequences. In C: rewrite as explicit state machines or protothreads
  (switch-based, local state in a struct).
- Typical object pattern (Celeste): a `types` table of type tables `{tile=18, init=function(this)...,
  update=function(this)..., draw=function(this)...}`; `init_object(type,x,y)` builds an object table
  with `type`, x, y, hitbox, `spd`, `rem` sub-pixel remainders, calls `type.init`, `add(objects,obj)`;
  `destroy_object` does `del(objects,obj)`; `_update` does `foreach(objects, update_object)`. Map
  tiles whose sprite number equals `type.tile` spawn objects at room load. In C: `enum type`, a struct
  with the union of all fields used by any type, a fixed pool (ccleste: `MAX_OBJECTS 30`), a switch or
  function-pointer table per type.
- `menuitem(i, label, cb)` pause menu (up to 5): map to an F-key menu or drop.
- `stat(x)`: 0 memory, 1 CPU, 6 param string, 7 fps, 46..57 audio, 80..95 time of day, 30..39 devkit.
- `load("#cart", breadcrumb, params)`: multi-cart games (chained carts) need several data files.
- `printh` debug output: ignore.

---------------------------------------------------------------------------------------------------
## 9. Existing ports, emulators and transpilers

### Celeste Classic ports (the reference case)
| Project | URL | Lang | Notes / reusable |
|---|---|---|---|
| ccleste | https://github.com/lemon32767/ccleste (archived 2023) | C (C++ for fixed point) | `celeste.c` (2032 lines) = line-by-line hand port of the Lua; `celeste.h` exposes `Celeste_P8_init/update/draw`, `Celeste_P8_set_call_func`, `Celeste_P8_set_rndseed`, save/load state. All PICO-8 calls go through one variadic callback `int (*)(CELESTE_P8_CALLBACK_TYPE, ...)` with 14 calls: MUSIC, SPR, BTN, SFX, PAL, PAL_RESET, CIRCFILL, PRINT, RECTFILL, LINE, MGET, CAMERA, FGET, MAP. No dynamic allocation, global state. Floats by default; `make USE_FIXEDP=1` (C++) replaces `float` by a 16.16 struct with PICO-8-decompiled `/`, `sin`, `rnd`, `srand`. `tilemap.h` = map data; `data/gfx.bmp`, `font.bmp`, sounds. Frontend `sdl12main.c` (1089 lines) implements the callbacks (note the `map` mask hack: flag index = mask-1 except mask 4). TAS input format "0,0,3,5,..." (one input bitfield per frame) = a ready-made deterministic test script. GitHub detects no licence file; the game belongs to Maddy Thorson & Noel Berry. **Most reusable starting point for a TI Celeste: keep celeste.c, write a TI frontend for the 14 callbacks, replace floats by int32 16.16 in C (no C++ in GCC4TI path).** |
| NoelFB/Celeste `Source/PICO-8` | https://github.com/NoelFB/Celeste (local `sources/celeste`) | C# | `Classic.cs` (official 1:1 C# port in the full game), `Emulator.cs` (spr/map/pal/print/rnd over XNA). MIT for code only, not the game/assets. Its `sign` uses `Math.Sign` (0 for 0) and `sin` = `Math.Sin((1-a)*2π)`. |
| CEleste (TI-84 Plus CE) | https://github.com/commandz0/CEleste | C++ (CE toolchain) | MIT. A calculator precedent (eZ80 48 MHz, 320x240 colour): uses the full screen without scaling, `make gfx` asset step, practice mode. Also Yoshilest https://github.com/YoshiHack/Yoshilest. |
| Celeste-Classic-GBA | https://github.com/JeffRuLz/Celeste-Classic-GBA | C (devkitPro) | 240x160 screen, "almost a direct conversion with minor graphical alterations"; maxmod sound data (ccleste reuses its wavs). Fork: https://github.com/LinUwUxCat/Another-Celeste-GBA |
| Game Boy | https://github.com/demma98/gb-celeste (https://joshop.itch.io/celeste-game-boy) | SM83 asm (~8000 lines) | all levels and music on DMG: proof that a 4-shade 160x144 version works. WIP GBC: https://github.com/matanui159/celeste-gameboy |
| Amiga | Paweł Juen Nowak (https://www.indieretronews.com/2026/03/celeste-rage-inducing-platformer-for.html) | ? | 68020 minimum, smooth at 50 fps on a 50 MHz 68060, frame-skip variant playable on a 25 MHz 68030; chunky buffer + C2P (the costly part). Lesson for 68000: do not emulate a 4 bpp chunky screen and convert; draw straight into planes. |
| Playdate | https://hteumeuleu.itch.io/celeste | Lua | 1-bit 400x240 conversion (art redone in black/white). |

### PICO-8 runtimes / emulators (source for exact semantics)
| Project | URL | Lang / licence | Reusable |
|---|---|---|---|
| zepto8 + z8lua | https://github.com/samhocevar/zepto8, https://github.com/samhocevar/z8lua | C++ | Most exact reference: `z8lua/fix32.h` (mul, div, mod, shifts, abs, floor), `lpico8lib.c` (sin/cos/atan2/sqrt/sgn/mid/tostr with decompiled tables in `trigtables.h`: sin 4096 x u16, atan 2049 x u16), `src/pico8/vm.cpp` (rnd/srand/btnp), `src/pico8/gfx.cpp` (spr, map with layers, palettes, fill patterns). fix32.h is WTFPL. |
| fake-08 | https://github.com/jtothebell/fake-08 | C++ | Runs .p8/.p8.png on 3DS, Switch, Vita, Wii U, Miyoo and other homebrew; `source/graphics.cpp` has a full software rasteriser over a 4 bpp buffer (`map` uses `(fget(cell) & layer)` any-bit). Shows what a full Lua-on-device approach costs (needs a Lua VM, MBs of RAM): not viable on a 12 MHz 68000. |
| tac08 | https://github.com/0xcafed00d/tac08 | C, MIT | Runtime for .p8 text carts; easy-to-read API implementation. |
| picolove | https://github.com/picolove/picolove | Lua/LÖVE, Zlib | Lua implementation of the API: a readable spec of each call. |

### Transpilers / AOT compilers
| Project | URL | Notes |
|---|---|---|
| luacretro | https://github.com/thediad/luacretro | JS compiler: "a PICO-8-flavored, statically-typed Lua subset ahead-of-time compiled to C" (no heap, no closures, fixed-point numbers); per-target capability table `CAPS` in `compiler/emit.js` (GameTank, GBA, Genesis, NES, C64). The closest existing thing to a PICO-8→C translator; a TI-89 target would be a new CAPS entry plus a runtime. |
| mdlua (md_lua_sdk) | https://github.com/monteslu/md_lua_sdk | MIT. Same front end for the Mega Drive (68000 via SGDK, m68k-gcc): 16.16 numbers with "integral values kept in fast 32-bit integers". Static dialect: no tables, closures, metatables, coroutines, string concatenation, nil, goto; conditions must be boolean. Also `mdlua pico8 cart.p8.png` for unmodified carts through a full Lua runtime, but "the Lua heap is ... roughly 10 KB, so only small carts fit". **The only 68000 PICO-8-flavoured toolchain found; its generated C and fixed-point runtime are worth reading.** Siblings: gba_lua_sdk, neslua, c64lua, gametank_lua_sdk. |
| PICO-C | https://github.com/BryanHaley/PICO-C | C-like language → PICO-8 Lua (wrong direction). |
No general, unrestricted "any PICO-8 cart → C" transpiler was found. Real carts use closures, dynamic
tables and strings, so a port is a semi-manual translation (ccleste style), helped by tools
(shrinko8 lint/unminify, picotool AST, pico8-to-lua) for parsing and inventory.

---------------------------------------------------------------------------------------------------
## 10. Porting pitfalls (and what to do on the TI)

1. **Exact fixed point.** Game physics depends on 16.16 rounding (sub-pixel accumulators like
   Celeste's `rem.x`, speeds 0.2, gravity 0.21, friction via `appr`). Floats give tiny divergences
   (ccleste README: negligible for play, visible for TAS). On the TI: `int32_t` 16.16 everywhere a
   value can be fractional, `int16_t` where the translator proves it integer; multiply = floor of
   the 64-bit product >> 16 (on 68000: `muls.w` when both fit 16 bits, constant multiplies as shifts/
   adds: `x*0.5` → `x>>1` (same floor semantics; but `x/2` truncates: `p8_div2k`), `*0.2` →
   reciprocal constant), never `__divsi3` in loops. Check generated asm for `__mulsi3`/`__divsi3`
   (CLAUDE.md rule): GCC4TI compiles a 16.16 product written on 16-bit halves in C into three
   `__mulsi3` calls (**verified**); `p8num.h` uses inline `muls.w`/`mulu.w` (~380 cycles).
2. **Division corner cases**: x/0 = ±0x7fff.ffff (not a trap: the 68000 `divs` by 0 raises a CPU
   exception); `%` is floor-mod and never negative; `\` floors (C `/` truncates toward 0:
   `-9/2 == -4` in C, `-9\2 == -5` in PICO-8). Use `>>` for power-of-two `\` (arithmetic shift floors).
3. **sgn(0) == 1.** A port mapping `sgn` to a C sign function returning 0 changes behaviour
   (e.g. facing direction or knockback at rest). Check each game's own `sign` helpers (Celeste has one).
4. **flr of negatives / coordinates.** Draw calls floor their coordinates (fixed bits >> 16);
   C `(int)x` truncates toward zero, wrong for x in (-1, 0) and all negative fractions: off-by-one
   sprites entering from the left/top and at camera offsets. In 16.16 C use `x >> 16`
   (arithmetic shift on the 68000 with GCC = floor). ccleste's `int(_x)` casts on floats have this bug.
5. **Integer vs fractional coordinates.** Objects keep fractional x/y; collision helpers often
   `flr` them, drawing floors implicitly, `mget(x/8, y/8)` floors too. Keep the same order of
   operations (flr after the add, not before).
6. **Update/draw split and frame rate.** PICO-8 may skip `_draw` (15 fps mode), but many carts mutate
   state inside `_draw` (Celeste moves clouds and particles in `_draw`). Call `_update` and `_draw`
   1:1 and never skip a draw that mutates state; skip only pure rendering.
   `_update60` games: at 30 fps on the TI either run 2 updates per drawn frame (logic cost x2) or
   rescale every speed/timer by 2 (inexact, changes feel). Prefer 30 fps carts.
7. **Map layers.** `map(...,layers)`: tile 0 never drawn, any-bit vs all-bits ambiguity (§4); the
   lower map half aliases sprites 128-255 (a cart using big maps has only 128 sprites). Celeste draws
   the same 16x16 room three times with masks 4, 2, 8 (bg, terrain, fg): on the TI, pre-split the
   room into layer tile maps at build time (or one combined map plus an fg list) and use the
   ExtGraph TileMap engine instead of three per-tile passes.
8. **Palette swaps as effects**: `pal(a,b)` recolouring (Celeste: hair colour per dash count
   `pal(8, 12)` etc., flashes, fruit), `pal(c,c2,1)` full-screen fades, `pal()` resets. With 4 greys,
   a swap between colours that map to the same grey is invisible: pick gameplay-relevant swaps and
   give them distinct greys or outline/pattern changes (e.g. hair black vs white, dashed vs not).
   Screen-palette fades → grey ramp tables per fade step.
9. **Screen shake via camera**: `camera(rnd(4)-2, rnd(4)-2)` jitters everything drawn after it;
   HUD drawn after `camera()` stays still. On the TI shaking a TileMap means changing the scroll
   origin (cheap) plus offset sprites; keep the reset before the HUD.
10. **Random determinism**: the seed is random at start; carts using `srand(n)` for procedural
    levels need the exact PRNG (§2) to reproduce PICO-8 layouts; tests (`game_scenario(n)`) need a
    fixed seed. Note `rnd(x)` returns fractions: `flr(rnd(6))+1` patterns.
11. **Screen size**: 128x128 vs 160x100. Width fits (16 px margins), height loses 28 rows. Options:
    vertical camera scrolling within a room (Celeste rooms are exactly 16x16 tiles = 128x128),
    a 100-row window following the player, or redesigned rooms; 64x64-mode carts
    (`poke(0x5f2c,3)`) fit at 1:1 or x1.5. Scaling 128→100 vertically (0.78) distorts 8x8 tiles:
    avoid. Fonts: AMS 4x6 matches PICO-8 metrics.
12. **Colour 0 transparency and sprite 0**: colour 0 is transparent in spr/map by default, but
    `rectfill(...,0)`/`cls(0)` draw black. Map tile 0 = empty. Colour 0 may also be made opaque
    (`palt(0,false)`) and another colour transparent: keep a per-call transparency set at
    conversion time (masks for pre-shifted TI sprites).
13. **Draw order and overdraw**: no z-buffer; order of foreach over `objects` defines layering (and
    `del` during iteration changes the order of later objects). Preserve list order.
14. **Fill patterns**: `fillp` bit order is top-left = bit 15; the second colour is the high nibble;
    transparency bit 0b0.1. Pre-compute TI plane masks per pattern.
15. **Strings and print**: number printing format (4 decimals), `print` returning the width, P8SCII
    control codes, lowercase/uppercase glyph swap; P8SCII glyphs >= 128 (button icons) need TI
    replacements.
16. **Memory tricks**: carts that `memcpy` into 0x6000 (screen) or treat the screen as a sprite sheet
    (0x5f54/0x5f55) or decompress levels into 0x2000/0x1000 need either an emulated RAM or a
    case-by-case rewrite. Grep each cart for `peek|poke|memcpy|memset|reload|@|%0x|\$` before choosing it.
17. **Token-golfed code**: heavy carts are minified or use `_ENV` and long data strings; run
    `shrinko8 --unminify` and pre-decode data strings at build time.
18. **btnp timing**: repeat counts are in update frames (15/4 at 30 fps); keep the same counter in
    the TI input layer and expose 0x5f5c/0x5f5d if poked.
19. **Overflowing timers**: `t()`/frame counters wrap at 32768 (18 min at 30 fps); a C `int32` 16.16
    counter wraps identically, an `int16` integer counter too, but a `long` integer counter does not:
    match the wrap only if the game relies on it.
20. **Audio-synchronised logic**: `stat(54..57)` or waiting for `music` to end (title screens,
    cutscenes): emulate with a frame-based fake clock since `sfx/music` become no-ops.

### Sizes for a TI port of a typical cart
- Sprite sheet as TI data: 256 sprites x 8 rows x (2 planes + 1 mask) bytes = 6 KiB unshifted
  (ExtGraph pre-shifted versions cost more: see `experiments/tilemap/`), map 4-8 KiB, flags 256 B.
- Code: Celeste's 5887 tokens became 2032 lines of C in ccleste. The AMS 2.xx TI-89 program limit is
  24,576 bytes; the Titanium has none (CLAUDE.md): keep gfx/map in a data file if size gets close.
