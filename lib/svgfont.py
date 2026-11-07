"""Offline SVG outline font conversion shared by TI projects.

Inkscape resolves a host font into paths. Curves are flattened and contours
quantized to byte coordinates; no XML, Inkscape or floating point runs on TI.
This is an outline subset, not a general SVG renderer. Source font/art banks
remain local. Callers own glyph packing and the runtime rasterizer.
"""
import os
import re
import subprocess
import tempfile
from xml.sax.saxutils import escape


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


def outlines(ttf, family, chars, cap=64, y0=8, tolerance=0.7, weight=900):
    """Return byte-coordinate contours and advances; chars must include cap reference E.

    Retains the reviewed banner's quantization/clamping. New faces, especially
    negative bearings or descenders, need visual validation before runtime use.
    The installed Inkscape family must resolve to the same face as ttf.
    """
    from PIL import ImageFont
    TTF, FONT, CHARS, CAP, Y0 = ttf, family, chars, cap, y0
    size = 100
    pil = ImageFont.truetype(TTF, size)
    svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="4000" height="200">']
    for k, c in enumerate(CHARS):
        svg.append('<text id="g%d" x="%d" y="150" font-family="%s" font-weight="%d" font-size="%d">%s</text>'
                   % (k, 200 * k + 20, FONT, weight, size, escape(c)))
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
            pts = _simplify(pts, tolerance)
            q = []
            for x, y in pts:
                p = (max(0, min(255, round(x))), max(0, min(255, round(y))))
                if not q or q[-1] != p: q.append(p)
            if len(q) > 2: conts.append(q)
        res[c] = (round(pil.getlength(c) * s * size / size), conts)
    return res
