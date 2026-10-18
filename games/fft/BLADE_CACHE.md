# Single-facing portraits and bounded blade cache — 2026-10-17

Active native camera data is now **158,468 bytes**, down from 174,860 after the first
[storage cleanup](STORAGE_CLEANUP.md), and from 399,328 in the original inventory.
This follow-up saves **16,392 additional bytes**: 2,088 from portraits and 14,304 from
blade compression. These are raw bank payloads, excluding variable/archive headers.

## Portraits

`ffti` stores six 348-byte portrait sprites instead of twelve. Status and both HUD
sides use the same original facing; no mirrored copy is generated in RAM. Portrait
colour, transparency and outline remain intact. The left HUD portrait intentionally
changes orientation; the target HUD and Status keep their prior source facing.
`ffti` falls from 18,372 to **16,284 bytes**.

## Blade format and memory bound

`fftw` now contains:

| Component | Bytes |
|---|---:|
| Raw armed body poses, read directly from Flash | 19,968 |
| Forty-eight logical-pose to unique-frame byte indices | 48 |
| Forty-one big-endian compressed-stream offsets | 82 |
| Forty independently ZX0-compressed blade images | 2,894 |
| **Total** | **22,992** |

Every decoded blade is the same 432-byte, 32×36, three-plane image as before. All 48
logical pictures retain their original horizontal anchors and existing scenery
cover/water clipping. Armed bodies and ordinary character sprites remain read in
place, with no whole-bank expansion.

There are **three fixed 432-byte cache slots**, indexed by swing phase. Three byte
keys identify each slot's unique frame; zero means invalid. Including structure
alignment, full tactical Work grows by **1,300 bytes**, from 43,192 to **44,492**.
There is no allocator, eviction list, unbounded growth or per-character cache.
The slots are replaced when another weapon/facing needs that swing phase and are
invalidated during `game_init` before a reloaded bank can be consumed.

The compact camera review never draws these separate blades. Its cache fields and
helpers are compiled out, so compact Work remains **22,762 bytes**, and its optional
perspective geometry cache remains 20,284. The full tactical view+scene+Work total
is 114,252 bytes, before executable, grayscale, stack and AMS. Headless allocation
does not establish physical AMS heap headroom; no new emulator run was performed.

## Preparation and measured CPU

After the frame's ordinary HUD work, the wind-up prepares at most one missing
blade phase, using the existing nonnegative spare-credit heuristic and charging 25k
credits. Actual decoding is below 24k cycles. This leaves the first visible blade
frame free to build its scenery-cover mask. Credits are conservative estimates;
the hard bound is checked with the full native frame replays below.

A one-slot design failed the crowded/fast-forward cases: held acceleration can
skip directly to any of the three blade phases, leaving a decode on the expensive
first cover frame. Three resident phases keep those cases warm without changing
HUD construction order or skipping attack animation. No extra per-frame work is
required once the appropriate slots are ready.

`make blade-cache-check` compares the actual cache against the preserved raw
dictionary on PC and the native 68000. Across all 48 logical poses, the measured
worst cold lookup is **23,018 cycles / 1.918 ms** and worst warm lookup **344 cycles
/ 0.029 ms**, including lookup/measurement overhead, at 12 MHz. These are datasheet
CPU counts, excluding grayscale, presentation and waits; no TiEmu/FPS claim follows.

Native ordinary frame peaks after integration:

| Replay / fixture | Peak cycles |
|---|---:|
| Crowded sword attack, orientation 3 | 355,524 |
| Crowded dagger attack, orientation 3 | 354,286 |
| Full 16,000-frame AI battle | 350,092 |
| Held fast-forward, 6,000 frames | 355,730 |
| Defeat, 4,000 frames | 359,688 |

All remain below 360,000 cycles. The ordinary battle still wins after 15,403 frames
and 80 turns. Held acceleration to Ramza's first turn takes 496 frames versus 495
before; the normal 987-frame opening is unchanged. This is the existing budget-based
fast-forward policy responding to preparation work, not changed CT/combat rules.

## Verification and reproduction

```sh
make test pc ti cycles CFLAGS_GAME=-DFFT_MEASURE
make check-blows events-check check-interface
make storage-cleanup-check blade-cache-check
make camera-prefetch-check camera-prefetch-filter-check camera-config
make storage-audit
```

Run native builds sequentially: GCC4TI writes temporary `.s`/`.o` files beside shared
input sources. Concurrent native builds can delete or overwrite their intermediates.

Checks include all 48 blade pictures, cold/warm replacement and initialization,
unchanged tactical State during cache operations, 1,520 font alignments and 510
original names. All 137 preserved game images match outside the intentionally
changed left portrait rectangle (`x=0..18`, `y=71..99`); 120 have changes there,
17 are entirely identical. Cache compression causes no other image differences.

The native frame suites compare 88 attack/replay screens, 101 banner/loot/replay
screens and 177 UI screens, including water/occlusion and all four attack views.
Camera tests retain 484 interactive PC/TI frames and 34 cold/prepared pictures.
Evidence is local under `x/storage/blade-cache/checked.json`, `cleanup-checked.json`,
`x/events/checked.json`, `x/config/checked.json` and `x/prefetch/checked.json`.

Rebuild and send the executable and current UI/weapon banks together. Old binaries
expect both portrait facings and a raw blade dictionary and cannot read these layouts.

## Font-shift explanation

The former font stored each glyph at all 16 bit alignments relative to a 16-pixel
word. This avoided variable shifts when a letter started between word boundaries,
but multiplied its stored artwork 16 times (and used four bytes per row). The current
font retains only 95×5 original row bytes, shifts them when rebuilding a HUD part,
then reuses the existing panel cache. Its position and typography are unchanged.
