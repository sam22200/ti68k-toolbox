# Mode 7 - Demo 2: optimisations of the floor and the 3D

This file covers what was optimised in the decompiled demo, why, how, and how much it saved.
Every change sits in the sources behind `#ifndef ORIGINAL`, marked *optimised* in the comments: `make ORIGINAL=1` still builds the faithful decompilation.

## Method

- **Cycles**: `tools/bin/ti-cycles` runs the benchmark build (`make bench`) on the PC under a 68000 core with the MC68000 datasheet timings.
  - It is verified on `nop`, shifts, `movem`, `mulu`, `divu` and indexed moves (`tools/m68kbench/test/cyctest.c`).
  - TiEmu, by contrast, counts ~12 cycles for any `movem` and ignores the 2 cycles per bit of shifts.
  - There are no wait states on the TI-89; the grayscale driver (~9 % of the CPU on hardware) is left out.
- **Scenarios** (`tools/bench.py`):
  - 0: start line, at rest;
  - 1: start line with 8 distant "mountains", 16 triangles 600-960 world units ahead, covering 9.6 % of the view;
  - 2: full throttle from the start line, 160 frames, so the textures move.
- **Pixels unchanged**: each scenario's last frame is read from memory and checksummed (the 128×100 view).
  - Every step below kept the three checksums of the original.
  - Rows 45-47 are excluded: at the horizon the original samples outside its far texture (its window is narrower than the view) and shows whatever memory lies next to it.
  - The new triangle filler was also compared with the old one on 2,000 random triangles (bench scenario 4): no difference.

## Result

| cycles / frame | original | optimised | gain |
|---|---:|---:|---:|
| scenario 0 (start) | 666,518 | 362,430 | **−46 %** |
| scenario 1 (distant 3D, 10 % of the view) | 1,059,860 | 492,062 | **−54 %** |
| scenario 2 (driving) | 686,896 | 414,938 | **−40 %** |
| ≈ fps at 12 MHz (grayscale included) | 16 / 10 / 16 | 30 / 22 / 26 | |

Per zone, scenario 0 (scenario 2 for the textures while driving):

| zone | original | optimised | gain |
|---|---:|---:|---:|
| Mode 7 far rows (15 × 128 px) | 224,410 | 137,286 | −39 % |
| Mode 7 near rows (20 × 64 texels) + line doubling | 133,176 + 11,802 | 125,404 | −14 % |
| textures, at rest | 111,938 | 14,779 | −87 % |
| textures, driving | 111,938 | 70,907 | −37 % |
| clear the screen | 17,484 | 0 | −100 % |
| sky | 16,756 | 9,614 | −43 % |
| copy to the grayscale planes | 35,452 | 0 | −100 % |
| ship physics and collisions | 11,240 | 4,796 | −57 % |
| 3D, scenario 0 | 93,330 | 59,664 | −36 % |
| 3D, scenario 1 | 486,672 | 189,296 | −61 % |

In order, frame cycles for scenarios 0 / 1 / 2:

| step | scenario 0 | scenario 1 | scenario 2 |
|---|---:|---:|---:|
| original | 666,518 | 1,059,860 | 686,896 |
| 3D: 16-bit projection, `Span2` | 630,826 | 860,168 | 646,553 |
| 3D: scanline loop in asm (`TriSpans`) | 634,076 | 787,484 | 640,221 |
| textures rebuilt only when the window moves | 537,032 | 690,440 | 600,038 |
| Mode 7 loops, one texture block, no clear, sky with `movem` | 420,438 | 573,846 | 473,110 |
| GrayDBuf: no copy to the planes | 385,302 | 539,206 | 437,891 |
| per-row divisions from tables | 376,762 | 530,666 | 429,351 |
| `TriSpans`: one loop per colour | 376,440 | 506,072 | 426,901 |
| near rows: nibble-doubling table | 370,048 | 499,680 | 420,509 |
| physics: `muls.w` | 363,616 | 493,248 | 416,124 |
| a dead `memset` dropped | 362,430 | 492,062 | 414,938 |

---

## Part 1: the Mode 7 floor

### How the original draws it

The view is 128 pixels wide.
- **Far part**: rows 45-59, one texel per pixel.
- **Near part**: rows 60-99, texels of 2×2 pixels (20 rows drawn every other line, then copied by 40 `memcpy`).

Each part samples a 128×128 texture, one byte per texel with value 0-3, rebuilt from the 64×64 tile map around the camera.
Per row: a distance, then a walk in 16.16 fixed point along the row. Per pixel, the original's inner loop (`render.s`, `Mode7Far`):

```asm
	move.w	#\bit, d2            |  8  the pixel's bit
	move.l	d4, d0               |  4
	move.l	d5, d1               |  4
	swap	d0                   |  4  u integer
	swap	d1                   |  4  v integer
	lsl.w	#7, d1               | 20  v * 128
	add.w	d0, d1               |  4
	move.b	(a0, d1.w), d0       | 14  texel 0..3
	beq.b	9f                   | 10  white
	cmpi.w	#1, d0               |  8  ... a compare chain
	beq.b	1f
	cmpi.w	#2, d0
	beq.b	3f
	bra.b	2f
1:	or.b	d2, (a1)             | 12  or into the light plane
	bra.b	9f
2:	or.b	d2, (a1)
3:	or.b	d2, (a3)             | 12  or into the dark plane
9:	add.l	d6, d4               |  8  next texel
	add.l	d7, d5               |  8
```

That is ~88 cycles for a white pixel and ~115 for a coloured one. The screen has to be cleared first, since the loop only ORs.

### 1. Texel index without a shift: one 256-byte-row texture block (−20 cycles per sample)

- **Before**: `lsl.w #7` alone costs 20 cycles (6 + 2 per bit on a real 68000).
- **Change**: both textures now live in one 32 KB block of 128 rows × 256 bytes, the near texture in columns 0-127 and the far one in columns 128-255 (same memory as the original's two 16 KB blocks).
  - The index is `v << 8 | u`: keep `v` as 8.24 (the row set-up's 16.16 value shifted left by 8), take its high word with `swap`, and drop `u`'s integer byte into the low byte:

```asm
	move.l	d5, d1               |  4  v as 8.24
	swap	d1                   |  4  d1.w = v_int << 8 | ...
	move.l	d4, d0               |  4  u as 16.16
	swap	d0                   |  4
	move.b	d0, d1               |  4  d1.w = v_int << 8 | u_int
	move.b	(a0, d1.w), d0       | 14
```

- **Exactness**: same texel for every `u`, `v` inside the texture, because the stepping is the original's (same 16.16 values, same steps).
- **Texture builders**: `BuildTexture8W` / `16W` write rows of 256 bytes.

### 2. Pre-encoded texels shifted into accumulators (no compare chain, no clear)

- **Change**: at load time the tiles are re-encoded (`EncodeTiles`, `floor.c`) so that a texel carries its two plane bits at the top of its byte.
  - Per pixel, two `add.b`/`addx.b` pairs shift them into two byte accumulators (16 cycles for both planes).
  - Every 8 pixels, two `move.b` write the bytes (4.5 cycles per pixel).

```asm
	add.b	d0, d0               |  4  bit 7 into X
	addx.b	d3, d3               |  4  ... into the plane at +0xF00
	add.b	d0, d0               |  4  bit 6
	addx.b	d2, d2               |  4  ... into the plane at the screen
	add.l	d6, d4               |  8
	add.l	d7, d5               |  8
	...                          |     8 times, then:
	move.b	d3, 0xf00(a1)        | 12
	move.b	d2, (a1)+            |  8
```

- **Side effect**: the loop writes whole bytes, so the floor rows no longer need clearing. With the sky rewritten each frame and nothing drawn right of the 128-pixel view, the per-frame `ClearPlanes` goes away (−17k): both screens are cleared once at start-up.
- **Far rows**: ~66 cycles per sample in all, 224k → 137k (−39 %).

### 3. Near rows: 2 bits per texel, a nibble-doubling table, the doubled line written in place

- **Doubling the pixels**: near texels are 2 pixels wide, so each bit must appear twice.
  - The rows accumulate 4 texels (2 bits each, as the far rows).
  - Then a 16-byte table turns each 4-bit nibble into its doubled byte (`abcd` → `aabbccdd`).
- **Doubling the lines**: the byte goes to the row and to the line below it on both planes, which removes the 40 `memcpy` of the line-doubling pass.

```asm
	andi.w	#15, d3
	move.b	(a3, d3.w), d3       | nibble_double
	...
	move.b	d3, 0xf00(a1)        | row, plane at +0xF00
	move.b	d3, 0xf1e(a1)        | the line below
	move.b	d2, 30(a1)
	move.b	d2, (a1)+
```

- **Result**: 145k → 125k (−14 %). The near rows gain less than the far ones: their samples were cheaper to begin with (one per 2 pixels), and the doubled writes stay.

### 4. Per-row divisions from tables (−9k)

- **Before**: each row divides twice (`0x3200 / d5` for the distance, `0x1000 / d5` for the step, `divu.w` = 140 cycles).
- **Change**: the row index `d5` is fixed per row, so the quotients come from four small tables (`far_dist`, `far_step`, `near_dist`, `near_step`, `render.s`). The values are the same.

### 5. Rebuild a texture only when its window moves (−97k at rest, −41k driving)

- **Before**: every frame rebuilt one of the two 16 KB textures from the tile map, alternately, a 112k-cycle copy.
- **Change** (`floor.c`, `UpdateCamera`): the texture's content depends only on the map cell at its top-left corner, so it is rebuilt when that cell changes:

```c
        if (g->map_near != built_near) {        /* optimised: only when the window moved */
            BuildTexture16W(g->map_near, g->tiles16, g->tex_near);
            built_near = g->map_near;
        }
```

- **Result**: at rest nothing is rebuilt; at full speed a texture changes cell every few frames (70k average).
- **Left**: a toroidal texture would update only the new tiles, but it needs a 64 KB block or a mask per sample.

### 6. Sky: 16 bytes per row with `movem` (−7k)

- **Before**: two ROM `memcpy` of 1,350 bytes, whole 30-byte rows.
- **Change**: `SkyCopy` copies only the visible 16 bytes of each of the 45 rows, 16 bytes per `movem.l`.

### 7. Draw into GrayDBuf's hidden planes: no copy (−35k)

- **Before**: the original draws into a 240×128 virtual screen, then copies it to the two grayscale planes every frame (2 × 3,840 bytes with `movem`).
- **Change**: GCC4TI's double buffering has two plane pairs; the program draws into the hidden pair and `GrayDBufToggle()` shows it from the next plane switch.
- **Plane order**: GrayDBuf's pairs are dark first and light at +0xF00, the reverse of the original's virtual screen. Instead of the routines, the data is swapped: the texel codes, the sky order (`SkyCopy` arguments), the ship's plane arrays and the face colours (`DARK_FIRST`, `mode7.h`).

---

## Part 2: the 3D objects

### How the original draws them

- **Per frame**: the cells of a cone in front of the camera, then their faces.
- **Per vertex**: a rotation and a projection in C, with `long` operands.
- **Per face**: `FillTri` sorts the vertices, computes three slopes, and walks the lines in C, calling `HLine` twice per line (once per plane).

### 8. Projection with 16-bit operands (projection −63 % in scenario 1)

GCC compiles `long * long` as a `__mulsi3` call and `long / long` as the ROM's 32-bit division.
The values fit 16 bits:
- 3D coordinates are < 1024 and the sines ≤ 128;
- `side` and `depth` are < 4096;
- the quotient is < 16012 because `depth` > 10.

So one `muls.w` and one `divs.w` give the same results:

```c
    /* before */
    long side = (c * dx + s * dy) >> 7;
    long depth = (c * dy - s * dx) >> 7;
    w->proj[nproj].x = w->cx + (short)(side * focal / depth);
    /* after */
    short side = (muls16(c, dx) + muls16(s, dy)) >> 7;
    short depth = (muls16(c, dy) - muls16(s, dx)) >> 7;
    w->proj[nproj].x = w->cx + divs32_16(muls16(side, focal), depth);
```

The same change in the ship physics and collisions: 11k → 4.8k (the speed stays < 3072 through the throttle curve, and the wall distances are < 4096).

### 9. The scanline loop in asm: `TriSpans` (fill −63 % in scenario 1)

**Before**: per line, the C loop shifts and clamps both edges, then calls `HLine2`, which calls `HLine` for each plane. That is three calls, three sets of pushed arguments, and the address and masks computed twice.

**After**: `FillTri` keeps its set-up (sorting, three `divs.w`) and calls `TriSpans` for each half of the triangle (`render.s`).
- **Per call**:
  - the edges sit in registers;
  - the mask tables' base is in `a6`;
  - the colour picks one of four copies of the loop (a macro), so there is no colour test per line.
- **Per line**:
  - the clamp to 0..127 is one unsigned compare;
  - the first and last words are masked on both planes;
  - the middle words of both planes are written in one loop.
- **Continuing the edges**: the call returns both edges (`fa << 16 | fb`), so the second half continues exactly as the C loop did.

```c
        e = TriSpans(vs, y2, y1, fa, fb, s02, s12, color);
        TriSpans(vs, y1, y0, e >> 16, e, s02, s01, color);
```

- **Result**: fill 381k → 140k in scenario 1 (−63 %); the whole 3D goes 487k → 189k (−61 %).
- **Left**: a line still costs ~400 cycles, mostly the per-line mask look-ups and word counts.

### 10. Small ones

- `VisibleCells` cleared a 256-byte buffer every frame that nothing reads: dropped (−1.2k).
- **Assembler pitfall, met twice here**: GNU as truncates an out-of-range `(d8,pc,Xn)` offset without an error. The mask tables must sit within 127 bytes of the code that indexes them; otherwise use `lea table(pc), An` (16-bit reach), as `TriSpans` does.

---

## What costs the most now (scenario 0, 362k)

| zone | cycles | share |
|---|---:|---:|
| Mode 7 far rows | 137k | 38 % |
| Mode 7 near rows | 125k | 35 % |
| 3D | 60k | 16 % |
| textures, sprite, sky, physics | 39k | 11 % |

The floor is at ~66 cycles per sample, which is about the limit with exact 16.16 stepping. Beyond that, the ideas change the pixels slightly:
- 8.8 stepping in 16-bit words (add.w instead of add.l);
- far rows at half resolution (−65k);
- the precomputed-grid approach of the Mode7 Engine (knowledge base, game techniques §8).
