# Shared bitmap font

`bitmap_font.py` makes a BMF1 bank from a host font and a per-size character
repertoire. `bitmap_font.c/.h` measure and draw ordinary strings on the Portable
Game Runtime. The bank is read in place; drawing allocates no heap, parses no
SVG and performs no scaling. Fractional advances and kerning are precomputed
in26.6 fixed point; only the final sprite coordinates are rounded down.

```python
from bitmap_font import build
bank, report = build([(10, 'ABC 0123456789'), (15, '68k')])
```

```c
BfFont font;
BfLayout layout = {0, 24, 160, sweep, 0};
if (bf_open(&font, bank, bytes, face_id))
    bf_draw(&font, "ABC", &layout);
```

`left/top/width` describe a centered line, with the top ink pixel at `top+1`.
`sweep` is a horizontal reveal position relative to `left`, with dark/light/white
phases trailing its edge.255 fully exposes a normal160px line. `cell=0` selects
proportional layout;1..32 selects a fixed pixel advance. Text is limited to128
characters and256px of advance. Unsupported ASCII/control bytes use `?`.
Generate a larger repertoire when adding text; absent letters are not silently
available from the original TTF on the calculator.

## Format

All16-bit values are big endian. The8-byte header contains `BMF1`, face count,
reserved byte, and total bank length. Each8-byte face directory record contains
glyph-directory offset, kerning offset, pair count, source point size, reserved
byte. The glyph directory has95 absolute offsets for ASCII32..126; zero selects
the question-mark fallback.

A glyph has an8-byte header: advance26.6, signed x/y baseline bearings, ink width,
height, two reserved bytes. Width is at most32 and height at most24. Each8px
column then stores `height` light bytes, dark bytes and transparent-mask bytes.
The last column is padded with transparency. Space has no ink. Kerning records
are sorted `(ASCII_pair_u16, signed_delta_26_6)`, four bytes each; supported deltas
are within four pixels. Rendering uses ordinary8px masked sprites; the reveal
edge needs at most72 bytes of temporary plane rows.

## FFT candidate and validation

`make -C games/fft/campaign shared-font-check` generates a private bank and checks
truncation, bounds, fallback, empty/overlong strings, fixed/proportional layout
and37 real sprite-plane screens under PC sanitizers and the68000 runner.
Current accumulated visible-plane hash: `EBD63473`;6602 assertions pass on both.
`shared-font-review` renders six before/after GIFs from actual game C code.

The first FFT bank is6556 bytes for six point sizes and116 glyph records,
including per-face fallbacks/spaces. Its strings add326 bytes. This is not the
compiled engine or total package cost. Dynamic per-letter hinting differs from
the old whole-line images; the user accepted the comparison on2026-10-24.
FFT now uses the glyph bank, with `FFT_LEGACY_SERIF` for historical controls. See
`games/fft/campaign/SHARED_FONT.md` for exact data accounting and integration limits.

The separate `svgfont.py` remains the offline vector-outline converter used by
legacy FFT banners. It is not called by this bitmap engine.
