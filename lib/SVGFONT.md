# Shared offline outline font converter

`svgfont.py` factors out the converter formerly private to
`games/fft/tools/banner.py`. It runs on the host, using Inkscape's text-to-path
export and Pillow's advances. It is an SVG outline subset, not a general SVG
runtime or an on-calculator XML parser.

```python
from svgfont import outlines

glyphs = outlines(ttf_path, "DejaVu Serif", characters_including_E, weight=700)
# {character: (advance, contours_of_byte_coordinate_pairs)}
```

Defaults preserve FFT's reviewed Lato Black cap64/y-origin8 quantization.
Callers own packing and rendering. New faces require review, particularly
negative bearings, descenders, hinting at small sizes and the font family
actually resolved by Inkscape. Font artwork/generated banks remain local.

FFT's old converter wrapper and its consumers keep their existing API. The
extracted converter produces the same696-byte vector alphabet and all4128
bytes of the four reviewed banner strips. Hash evidence is under
`games/fft/campaign/x/cost-study/banner-shared-check.json`.

For fast dynamic text at a small set of sizes, use glyph bitmaps generated on
the host, a shared archive bank and ordinary strings. Retain vector outlines
only when arbitrary heights justify a bounded rasterization step. Reveal the
composed cached line with a moving horizontal clip/brightness edge; do not
rasterize vector paths every animation frame. FFT's proposal and size scope
are documented in `games/fft/campaign/UI_SIZE_STUDY.md`.

The portable bitmap implementation now lives in `bitmap_font.c/.h`, with its
offline builder in `bitmap_font.py`; see `BITMAP_FONT.md`. FFT's shared glyph
renderer was accepted and integrated2026-10-24. Existing legacy banners still
use this SVG converter unchanged.
