# Shared white font

**Accepted/integrated2026-10-24:** the user's "good" accepts the comparison.
Default flow is now FQF2; `FFT_LEGACY_SERIF` selects historical FQF1 controls.
The portable implementation is embedded by `flow_draw.inc` in each consumer;
the standalone API checker links the same `.c` normally. Scene/results native
checks and35 title-B cases pass; all packed children/new group build. See
[QA_FOLLOWUP_2026-10-24.md](QA_FOLLOWUP_2026-10-24.md) for current package size
and the new QA batch. The earlier candidate notes below record the review.
Shared-font Decision markers are cleared. Title A/B remains separate.

2026-10-24. The earlier472686-byte group is running in the user's TiEmu. That
session belongs to the user and must not receive automated keys/restarts.
The group SHA256 is `ae1a291ee1a737293215db5ca35017a10c63842acdcd7faf8b03c38c1ab8ee18`.

## Implemented, pending visual verdict

`lib/bitmap_font.c/.h` is a portable string renderer; `lib/bitmap_font.py` builds
its shared glyph bank offline. `FFT_SHARED_SERIF` enables private FQF2 flow-bank
consumers in chapter/location overlays, Orbonne, results/money and isolated
title B. Title B uses a private FTB2 bank. The normal build still uses FQF1/FTL1
and the existing images. No current campaign bank, launcher or package was
replaced. Do not mix either candidate bank with the old consumer.

The candidate keeps DejaVu Serif Bold at the existing point sizes8/9/10/11/12/15,
proportional advances, kerning, shadow and the left-to-right grey/white reveal.
Each letter is rasterized once at an integer origin; whole-line FreeType
hinting is not reproduced exactly. Orbonne changes more visibly because the
previous title came from the PS1's already-rasterized MAPTITLE image. Money uses
the same glyph bank with8px advance. Main menu/file-choice layout stays shared.

The archive contains only the glyph repertoire required by the current strings,
plus spaces/question marks and money digits. Future location names must extend
the generator input, rather than expecting a complete six-size ASCII font.
Text remains strings, not pictures; a new heading reusing these glyphs costs
its text record instead of another960-byte strip.

## Data accounting, not final module/package cost

| Private candidate data | Before | After | Delta |
|---|---:|---:|---:|
| Flow bank, including all unchanged nontext sections |13126|9736|−3390|
| Isolated title B bank |1820|1196|−624|
| Combined |14946|10932|−4014|

The shared font is6556 bytes of the new flow bank, including all six faces,
116 glyph records and kerning/directories. The13 title strings occupy326 bytes.
The source Orbonne strip is still present, unused, in the scene bank; no savings
are claimed for removing it. The legacy BATTLE START/VICTORY/DEFEAT strips still
use their existing offline SVG-outline converter and are outside this candidate.
Font code is linked into each consumer; its final packed-module costs and frame
costs have not been measured. Do not describe the4014-byte data saving as a
measured campaign-group saving.

## Reproduce and review

```
make -C games/fft/campaign shared-font-check
make -C games/fft/campaign shared-font-review
```

The first target checks6602 bounds/layout/fallback assertions and37 rendered
screens on PC with Address/UndefinedBehavior sanitizers and the actual68000
binary. Both produce `EBD63473`. Logs and machine-readable reports are under
`x/shared-font/`. This proves the font API, not full native chapter packaging.
The ordinary campaign `make test` also passes with the default renderer.

The review target builds the actual scene, battle-result and title C renderers
twice, using private data/saves. Before is left and shared glyphs are right, at
the same scale and logical animation steps. `x/shared-font/comparison.png` and
six GIFs are marked Decision in the gallery, including the GIF-only listing.
Playback cadence is illustrative, not measured TI timing. Superseded
`x/white-titles/` GIFs retain their files but no longer carry Decision markers.

Per the project's visual gate, await approval before optimising, benchmarking
or replacing the active consumers/banks. After acceptance: finish the native
consumer packaging checks, measure code/per-frame costs, rebuild all shared
modules, then prepare a new group. Do not restart the user's playtest to send it.
Title A versus B is still a separate choice; font approval does not select B.
