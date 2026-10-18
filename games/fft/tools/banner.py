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
import os
import re
import subprocess
import sys
import tempfile
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


def _parse(d):
    """SVG path data -> contours (lists of (x, y)), curves flattened."""
    toks = re.findall(r'[MmLlHhVvCcQqZzSsTt]|-?(?:\d+\.?\d*|\.\d+)(?:e-?\d+)?', d)
    out, cur, i, cmd = [], [], 0, None
    x = y = sx = sy = 0.0
    def num():
        nonlocal i
        i += 1
        return float(toks[i - 1])
    while i < len(toks):
        if re.match(r'[A-Za-z]', toks[i]):
            cmd = toks[i]; i += 1
            if cmd in 'Zz':
                if cur: out.append(cur)
                cur = []; x, y = sx, sy
                continue
        rel = cmd.islower()
        c = cmd.upper()
        if c == 'M':
            nx, ny = num(), num()
            if rel: nx += x; ny += y
            if cur: out.append(cur)
            x, y = sx, sy = nx, ny; cur = [(x, y)]
            cmd = 'l' if rel else 'L'
        elif c == 'L':
            nx, ny = num(), num()
            if rel: nx += x; ny += y
            x, y = nx, ny; cur.append((x, y))
        elif c == 'H':
            nx = num(); x = nx + x if rel else nx; cur.append((x, y))
        elif c == 'V':
            ny = num(); y = ny + y if rel else ny; cur.append((x, y))
        elif c in 'CQ':
            n = 3 if c == 'C' else 2
            p = [(num(), num()) for _ in range(n)]
            if rel: p = [(a + x, b + y) for a, b in p]
            pts = [(x, y)] + p
            for k in range(1, 9):
                t = k / 8
                q = pts
                while len(q) > 1:
                    q = [((1 - t) * a[0] + t * b[0], (1 - t) * a[1] + t * b[1]) for a, b in zip(q, q[1:])]
                cur.append(q[0])
            x, y = pts[-1]
        else:
            raise ValueError('path command ' + cmd)
    if cur: out.append(cur)
    return out


def _simplify(pts, tol):
    """Douglas-Peucker on a closed contour."""
    def dp(p):
        if len(p) < 3: return p
        (ax, ay), (bx, by) = p[0], p[-1]
        dx, dy = bx - ax, by - ay
        n = (dx * dx + dy * dy) ** .5 or 1
        k, dmax = 0, -1
        for j in range(1, len(p) - 1):
            d = abs((p[j][0] - ax) * dy - (p[j][1] - ay) * dx) / n
            if d > dmax: k, dmax = j, d
        if dmax <= tol: return [p[0], p[-1]]
        return dp(p[:k + 1])[:-1] + dp(p[k:])
    far = max(range(len(pts)), key=lambda j: (pts[j][0] - pts[0][0]) ** 2 + (pts[j][1] - pts[0][1]) ** 2)
    a = dp(pts[:far + 1] + [])
    b = dp(pts[far:] + [pts[0]])
    return a[:-1] + b[:-1]


def outlines():
    """{char: (advance, [contour [(x, y) bytes]])}, the cap height CAP units."""
    from PIL import ImageFont
    size = 100
    pil = ImageFont.truetype(TTF, size)
    svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="4000" height="200">']
    for k, c in enumerate(CHARS):
        svg.append('<text id="g%d" x="%d" y="150" font-family="%s" font-weight="900" font-size="%d">%s</text>'
                   % (k, 200 * k + 20, FONT, size, c.replace('&', '&amp;')))
    svg.append('</svg>')
    with tempfile.TemporaryDirectory() as tmp:
        src, dst = os.path.join(tmp, 'in.svg'), os.path.join(tmp, 'out.svg')
        open(src, 'w').write('\n'.join(svg))
        subprocess.run(['inkscape', src, '--export-text-to-path', '--export-plain-svg',
                        '--export-filename=' + dst], check=True, capture_output=True)
        out = open(dst).read()
    glyphs = {}
    for k, c in enumerate(CHARS):
        m = re.search(r'<g[^>]*id="g%d"[^>]*>(.*?)</g>' % k, out, re.S) or \
            re.search(r'<path[^>]*id="g%d"[^>]*/>' % k, out, re.S)
        ds = re.findall(r'\sd="([^"]+)"', m.group(0))
        glyphs[c] = [p for d in ds for p in _parse(d)]
    e = [p for cont in glyphs['E'] for p in cont]
    top, base = min(p[1] for p in e), max(p[1] for p in e)
    s = CAP / (base - top)
    res = {}
    for k, c in enumerate(CHARS):
        x0 = 200 * k + 20
        conts = []
        for cont in glyphs[c]:
            pts = [((x - x0) * s, (y - top) * s + Y0) for x, y in cont]
            pts = _simplify(pts, 0.7)
            q = []
            for x, y in pts:
                p = (max(0, min(255, round(x))), max(0, min(255, round(y))))
                if not q or q[-1] != p: q.append(p)
            if len(q) > 2: conts.append(q)
        res[c] = (round(pil.getlength(c) * s * size / size), conts)
    return res


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
