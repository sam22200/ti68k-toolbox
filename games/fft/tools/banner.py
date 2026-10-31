#!/usr/bin/env python3
"""The banners' font (fft.c bn_*): Lato Black's outlines, sampled offline.

Inkscape turns each glyph into SVG paths (text to path); the curves are flattened and simplified
into contours of byte points, the cap height 64 units (y + 8: the overshoots and '!' stay in a
byte). strip_bytes() reproduces the original C vector rasterizer exactly (even-odd,
2 x 2 samples a pixel, DIVS rounding). Its four small text strips are copied from the
archived UI bank through a runtime wipe. raster() remains a Python visual preview.

font_bytes() -> (bytes for ffti, advances): per glyph of CHARS, its offset (u16) in the table,
then [advance, contours, (points, x y ...) per contour].
"""
import sys
import struct

FONT = 'Lato Black'
TTF = '/usr/share/fonts/truetype/lato/Lato-Black.ttf'
CHARS = 'ABCDEFILMNORSTVY!'
CAP = 64                                # units of the cap height
Y0 = 8                                  # added to y (overshoots above the cap line)
SPACE = 22                              # a space's advance, units

# Only these small text strips are stored, never full-screen scenery. Integer sampling
# matches the former on-calculator vector fill (including DIVS truncation toward zero).
LINES = [('BATTLE START', 12, 1), ('DEFEAT ALL ENEMIES!', 9, 17),
         ('VICTORY!', 18, 4), ('DEFEAT', 18, 4)]


def strip_bytes(glyphs=None):
    g = glyphs or outlines()
    records, widths = [], []
    for text, cap, top in LINES:
        width = sum((g[c][0] if c != ' ' else SPACE) * cap // CAP + 1 for c in text)
        left = (160 - width) // 2
        assert left > 0 and left + width < 160
        h = (CAP + 2 * Y0) * cap // CAP + 1
        # Full word-aligned strip, including the one-pixel outline.
        x0, x1 = (left - 1) & ~15, min(160, (left + width + 16) & ~15)
        rows, stride = h + 2, (x1 - x0) // 8
        image = [[0] * 160 for _ in range(rows)]
        ox = left
        for c in text:
            advance = (g[c][0] if c != ' ' else SPACE) * cap // CAP + 1
            if c != ' ':
                edges = []
                yo = -(Y0 * cap // 16) & 3
                for contour in g[c][1]:
                    pts = [(x * cap // 16, y * cap // 16 + yo) for x, y in contour]
                    edges += [(x0, y0, x1, y1) for (x0, y0), (x1, y1)
                              in zip(pts, pts[1:] + pts[:1]) if y0 != y1]
                cov = [[0] * 32 for _ in range(h)]
                for j in range(2 * h):
                    yc = 2 * j + 1
                    def trunc(n, d):
                        return (abs(n) // abs(d)) * (-1 if (n < 0) != (d < 0) else 1)
                    xs = sorted(xa + trunc((yc - ya) * (xb - xa), yb - ya)
                                for xa, ya, xb, yb in edges if (ya <= yc) != (yb <= yc))
                    assert len(xs) <= 8
                    for xa, xb in zip(xs[::2], xs[1::2]):
                        for k in range(max(0, xa >> 1), min(64, xb >> 1)):
                            cov[j >> 1][k >> 1] += 1
                for y, row in enumerate(cov):
                    for x, v in enumerate(row):
                        if v and ox + x < 160:
                            image[y + 1][ox + x] |= 1 if v == 1 else 2 if v < 4 else 3
            ox += advance
        mask = [[any(image[yy][xx] for yy in range(max(0, y-1), min(rows, y+2))
                     for xx in range(max(0, x-1), min(160, x+2))) for x in range(160)]
                for y in range(rows)]
        body = bytearray([x0, top - 1, rows, stride])
        for plane in (1, 2, 0):
            for y in range(rows):
                for x in range(x0, x1, 8):
                    body.append(sum(0x80 >> k for k in range(8)
                                    if image[y][x+k] & plane) if plane else
                                sum(0x80 >> k for k in range(8) if mask[y][x+k]))
        records.append(body); widths.append(width)
    data, offsets = bytearray(), []
    for record in records:
        offsets.append(2 * len(records) + len(data)); data += record
    return struct.pack('>4H', *offsets) + data, widths


# Keep the reviewed banner API; the offline SVG converter is shared.
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'lib'))
from svgfont import _parse, _simplify, outlines as _font_outlines


def outlines():
    return _font_outlines(TTF, FONT, CHARS, CAP, Y0)


def font_bytes():
    g = outlines()
    body, offs = bytearray(), []
    for c in CHARS:
        adv, conts = g[c]
        offs.append(len(body))
        body += bytes([adv, len(conts)])
        for cont in conts:
            body.append(len(cont))
            for x, y in cont:
                body += bytes([x, y])
    head = bytearray()
    for o in offs:
        o += 2 * len(offs)
        head += bytes([o >> 8, o & 255])
    return bytes(head + body), {c: g[c][0] for c in CHARS}


def raster(g, text, cap):
    """The calculator's fill, in Python: rows of greys 0..3 (None: transparent), 2 x 2 samples a
    pixel, even-odd; cap: the cap height in pixels."""
    adv = {c: g[c][0] for c in g}
    w = sum((adv[c] if c != ' ' else SPACE) * cap // CAP + 1 for c in text) + 4
    h = (CAP + 2 * Y0) * cap // CAP + 2
    cov = [[0] * w for _ in range(h)]
    ox = 2 * 4
    for c in text:
        if c == ' ':
            ox += 4 * (SPACE * cap // CAP + 1); continue
        a, conts = g[c]
        # quarter-pixel units: points scaled once
        edges = []
        for cont in conts:
            yo = -(Y0 * cap // 16) & 3         # the cap line on a pixel's edge
            p = [(ox + x * cap // 16, y * cap // 16 + yo) for x, y in cont]
            for (x0, y0), (x1, y1) in zip(p, p[1:] + p[:1]):
                if y0 != y1: edges.append((x0, y0, x1, y1))
        for j in range(2 * h):                 # sample rows, centres 2j + 1 quarter units
            yc = 2 * j + 1
            xs = sorted(x0 + (yc - y0) * (x1 - x0) // (y1 - y0) for x0, y0, x1, y1 in edges
                        if min(y0, y1) <= yc < max(y0, y1))
            for xa, xb in zip(xs[::2], xs[1::2]):
                for k in range(xa // 2, xb // 2):   # sample columns, centres 2k + 1
                    if 0 <= k // 2 < w: cov[j // 2][k // 2] += 1
        ox += 4 * (a * cap // CAP + 1)
    grey = [[min(3, (v * 3 + 2) // 4) for v in row] for row in cov]
    mask = [[any(grey[yy][xx] for yy in range(max(0, y - 1), min(h, y + 2)) for xx in range(max(0, x - 1), min(w, x + 2)))
             for x in range(w)] for y in range(h)]
    return [[grey[y][x] if mask[y][x] else None for x in range(w)] for y in range(h)]


if __name__ == '__main__':
    from PIL import Image
    g = outlines()
    data, _ = font_bytes()
    print('font: %d bytes, %d points' % (len(data), sum(len(c) for _, cs in g.values() for c in cs)))
    rows = []
    for text, cap in (('BATTLE START', 12), ('DEFEAT ALL ENEMIES!', 9), ('VICTORY!', 18), ('DEFEAT', 18)):
        rows += raster(g, text, cap) + [[None] * 4]
    w = max(len(r) for r in rows)
    im = Image.new('RGB', (w, len(rows)), (150, 150, 150))
    pal = [(255, 255, 255), (170, 170, 170), (85, 85, 85), (0, 0, 0)]
    for y, r in enumerate(rows):
        for x, v in enumerate(r):
            if v is not None: im.putpixel((x, y), pal[v])
    im.resize((w * 4, len(rows) * 4), Image.NEAREST).save(sys.argv[1] if len(sys.argv) > 1 else 'banner.png')
