# Reach cache and grouped background patches

Measured on 2026-10-19 with the actual TI binary under `ti-cycles`, campaign
Gariland, scenario 61. MC68000 datasheet cycles at nominal 12 MHz; grayscale
interrupts, frame waits, packed startup and real AMS overhead are excluded.
This is not a TiEmu or physical-calculator FPS measurement.

## What was already cached

Reach tiles are composed into the two scene colour planes when the range or
its style changes. Ordinary cursor movement does not redraw the complete reach.
The aligned viewport cache keeps these pixels, while each frame restores the
background and draws animated actors, cursor, water and UI. Thus the expensive
reach operation is an opening/style-change spike, not recurring idle work.

The existing removal path restores changed scene rows over budgeted updates.
A full-screen dirty-rectangle presenter would be a separate change: both
grayscale buffers, sprite overlaps, water and transient UI would need tracking.
The ordinary cached background copy alone does not justify that complexity.

## Changes

- Keep exact reach-membership bits and the visible style (Move/aim, normal/over
  scenery). If an invalidation changes only costs or other nonvisual state,
  retain the completed scene without erasing/redrawing its reach.
- Only use this comparison after the previous scene has completed. A pending
  row restoration cannot qualify; changing ranges during cancellation still
  finishes restoration before composing the new range.
- Compose a changed range in the original order, then patch its bounding union
  into the screen cache once. Previously every tile separately patched its box,
  repeatedly shifting/copying overlapping regions.

This is not a per-tile removal algorithm: a genuinely changed set still uses
the existing clear/recompose path. Depth tests, hidden dashes, slope markers,
world coordinates, input behavior and all-at-once appearance are preserved.

## Before/after

Same keys and camera positions, six native pixel/state checkpoints identical.

| Frame/event | Before cycles | After cycles | CPU time before -> after |
|---|---:|---:|---:|
| 20: open Move | 1,615,940 | 1,477,518 | 134.7 -> 123.1 ms |
| 81: change overlay style | 1,439,150 | 1,300,528 | 119.9 -> 108.4 ms |
| 130: reopen Move | 1,434,920 | 1,296,514 | 119.6 -> 108.0 ms |
| 175: open small attack range | 284,098 | 286,272 | 23.7 -> 23.9 ms |
| 220: small attack overlay | 372,654 | 374,812 | 31.1 -> 31.2 ms |
| Warm range, frames 22–44 mean | 169,924 | 169,924 | 14.2 -> 14.2 ms |

Large-range event frames improve by 8.6–9.6%; tiny ranges pay about 0.18 ms
extra for bookkeeping/patch setup. Do not describe the unchanged warm-frame
cost as a general FPS gain. The initial ring-building zone itself previously
peaked at 1,169,146 cycles (97.4 ms), within the larger opening frame.

Native Work grows by **20 bytes** in both chapters (Orbonne: 35,520 -> 35,540).
No new art or lookup bank. The instrumented Gariland container grows by 294
bytes; the packed opening/Gariland cores by 256/278 bytes. The rebuilt
23-variable group was 354,487 bytes (+534); the subsequent title-skip guard
brings it to354,475 bytes. It has not replaced the user's live
TiEmu session and has not received a new AMS heap-lifetime check.

## Reproduce

Use existing local-disc banks; serialize GCC4TI commands:

```sh
make -C games/fft rings-test rings-variant-test
make -C games/fft/opening check
make -C games/fft/campaign check bundle
tools/pyenv/bin/python games/fft/tools/profile_reach.py
```

`profile_reach.py --baseline PATH.89z` also runs a previously captured
instrumented campaign-check binary, checking the final picture and state.
The local pre-change control is `campaign/x/ring-diff/ringbase.89z`;
`ab-checkpoints.json` additionally records six before/after checkpoints.
Fresh results and the exact key script go to `campaign/x/ring-diff/`.

The focused PC test compares 240 entire scenes with clean reconstruction:
four views in legacy Gariland, corrected campaign Gariland and Orbonne;
membership additions/removals, unchanged costs, styles, empty sets, overlapping
markers and a new range interrupting removal. The screen cache is independently
compared against an uncached viewport. Native checks pass thirteen opening and
seven campaign replays, including active-range rotations and cancellation.
