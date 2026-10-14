#!/usr/bin/env python3
"""Our own art for the FFT tech demo (no disc data) -> art.h (committed).

usage: art.py art.h
Greys: 0 white, 1 light, 2 dark, 3 black (light plane = bit 0, dark plane = bit 1).
The scenery is the game's own (tools/extract.py: the map's textured mesh drawn per view);
and so are the units (tools/units.py -> units.h); this file holds the rest:
- The units' shadow (drawn into their frames by tools/units.py).
- The cursor and the reach marker (24x12 diamonds); the diamond above enemies (team at a glance);
  enemy markers and ground shapes; a native ring/crossbar/point active-unit pointer.
- The 4 solid greys of the rotation frames (8x8 polygon patterns); each face's grey comes from
  the views (extract.py, map.h).
"""
import sys


def diamond_rows():
    """Column span of each of the 12 rows of the 24x12 diamond (tiles with neighbours at
    (+-12, +-6) exactly)."""
    out = []
    for r in range(12):
        k = r if r < 6 else 11 - r
        out.append((10 - 2 * k, 14 + 2 * k))   # [x0, x1)
    return out


def plane_rows(pix, w, h, x0=0):
    """pix[y][x] grey or None -> light, dark, mask as u32 rows (bit 31 = column x0)."""
    L, D, M = [], [], []
    for y in range(h):
        l = d = m = 0
        for x in range(w):
            v = pix[y][x]
            if v is None:
                continue
            bit = 1 << (31 - x - x0)
            m |= bit
            if v & 1:
                l |= bit
            if v & 2:
                d |= bit
        L.append(l); D.append(d); M.append(m)
    return L, D, M


def overlay_hi():
    """The reach marker on a 24x12 tile: a white diamond with a dark grey edge 2 px a row, laid
    on the neighbours' own edges (the diamonds overlap by 2 px): the reach reads as a white area
    ruled by a grid (chosen on a sheet of styles: the earlier inset black and white rings, a
    light grey centre with a black edge, the edge alone in black or white)."""
    rows = diamond_rows()
    pix = [[None] * 24 for _ in range(12)]
    for y, (x0, x1) in enumerate(rows):
        for x in range(x0, x1):
            pix[y][x] = 2 if x < x0 + 2 or x >= x1 - 2 or y in (0, 11) else 0
    return pix


def cursor():
    """The cursor: a thick black line (4 px per row) with a white one inside (the user's pick
    of three: 1, 2 or 4 px per row)."""
    rows = diamond_rows()
    pix = [[None] * 24 for _ in range(12)]
    for y, (x0, x1) in enumerate(rows):
        for x in [*range(x0, min(x0 + 6, x1)), *range(max(x1 - 6, x0), x1)]:
            pix[y][x] = 3 if x < x0 + 4 or x >= x1 - 4 else 0   # 4 black, 2 white inside
    return pix


FOE = """
.......000......
......00300.....
.....0033300....
.....0333330....
.....0033300....
......00300.....
.......000......
"""                                     # above an enemy's head: a black diamond, white border

ACTIVE = """
................
.......33.......
......3003......
......3003......
.......33.......
.....333333.....
.....311113.....
......3333......
......3023......
......3023......
......3003......
.......33.......
................
"""                                     # ring, crossbar and blade; source rows before white outline

HAND = """
..................
......3333........
....3300003.......
...3000033333333..
..330000000000003.
.3010000000000003.
.3000000000111133.
.30100000001333...
.30100000013......
.31110000133......
.3133111113.......
..313311133.......
.....33333........
..................
"""                                     # FFA title-screen glove: short index, rounded thumb, shaded palm

def outlined_pattern(pattern, width=16):
    pix = [[None if c == '.' else int(c) for c in row.ljust(width, '.')] for row in pattern.strip('\n').split('\n')]
    assert all(len(row) == width for row in pix)
    solid = [[p is not None for p in row] for row in pix]
    for y, row in enumerate(pix):
        for x, p in enumerate(row):
            if p is None and any(solid[yy][xx] for yy in range(max(0, y - 1), min(len(pix), y + 2))
                                for xx in range(max(0, x - 1), min(width, x + 2))):
                row[x] = 0
    return pix

def active_marker():
    return outlined_pattern(ACTIVE)

SHADOW = """
....222222222.
...22222222222
....222222222.
"""                                     # 16 wide, the units' anchor at column 8 (units.py)

# Markers on sloped tiles: the tile's four corners lifted (height units of 6 px; view corners
# top, right, bottom, left), its outline drawn from them: the reach tile (white, its edge dark
# grey up to 0.9 px inside) and the cursor (black 0-3 px, white 4-5 px), as on the flat
# diamond, by the distance to the edges (normal, so that the near-horizontal edges of a tilted tile get a line).
# The shapes: a ramp (two adjacent corners up) or a single corner up, by 1 or 2 units. Box: 24
# columns from the tile's left, rows from 2 units above its lowest corner.
SLOPES = [(a, b, d) for d in (1, 2) for a, b in ((0, 1), (1, 2), (2, 3), (3, 0))] + \
         [(a, None, d) for d in (1, 2) for a in range(4)]


def slope_marker(lift, cur):
    import math
    v = [(12, 12 - 6 * lift[0]), (25, 18 - 6 * lift[1]), (12, 24 - 6 * lift[2]), (-1, 18 - 6 * lift[3])]
    pix = [[None] * 24 for _ in range(26)]
    for y in range(26):
        for x in range(24):
            px, py = x + .5, y + .5
            d, inside = 99, True
            for k in range(4):
                (x0, y0), (x1, y1) = v[k], v[(k + 1) % 4]
                ex, ey = x1 - x0, y1 - y0
                cr = (ex * (py - y0) - ey * (px - x0)) / math.hypot(ex, ey)   # > 0: inside (clockwise)
                if cr < 0:
                    inside = False
                d = min(d, cr)
            if not inside:
                continue
            if cur:
                pix[y][x] = 3 if d < 1.79 else 0 if d < 2.68 else None
            else:
                pix[y][x] = 2 if d < .9 else 0
    return pix


# 8x8 polygon patterns (the rotation frames): rows of (light, dark) bytes,
# written doubled as 16-bit words
def pattern(f):
    L, D = [], []
    for y in range(8):
        l = d = 0
        for x in range(8):
            v = f(x, y)
            if v & 1: l |= 0x80 >> x
            if v & 2: d |= 0x80 >> x
        L.append(l); D.append(d)
    return L, D


def main():
    out = open(sys.argv[1], 'w')
    w = out.write
    w('// Generated by tools/art.py (our own art, committed). See that file for the layout.\n')

    def arr(name, rows, fmt='0x%08lXUL'):
        w('static const u32 %s[%d] = { %s };\n' % (name, len(rows), ', '.join(fmt % r for r in rows)))

    L, D, M = plane_rows(overlay_hi(), 24, 12)
    arr('hi_l', L); arr('hi_d', D); arr('hi_m', M)
    L, D, M = plane_rows(cursor(), 24, 12)
    arr('cur_l', L); arr('cur_d', D); arr('cur_m', M)
    # rotation frames (half resolution): solid greys (checkerboards between two greys were
    # tried: the user preferred solid faces)
    foe = [[None if c == '.' else int(c) for c in l] for l in FOE.strip('\n').split('\n')]
    L, D, M = plane_rows(foe, 16, len(foe))
    w('#define FOE_H %d\n' % len(foe))
    w('static const u16 foe_gfx[3][FOE_H] = { { %s }, { %s }, { %s } };\n' % tuple(', '.join('0x%04lX' % (v >> 16) for v in P) for P in (L, D, [~m & 0xFFFF0000 for m in M])))
    active = active_marker()
    L, D, M = plane_rows(active, 16, len(active))
    w('#define ACTIVE_H %d\n' % len(active))
    w('static const u16 active_gfx[3][ACTIVE_H] = { { %s }, { %s }, { %s } };\n' %
      tuple(', '.join('0x%04lX' % (v >> 16) for v in P) for P in (L, D, [~m & 0xFFFF0000 for m in M])))
    hand = outlined_pattern(HAND, 32)
    L, D, M = plane_rows(hand, 32, len(hand))
    w('#define HAND_H %d\n' % len(hand))
    w('static const u32 hand_gfx[3][HAND_H] = { { %s }, { %s }, { %s } };\n' %
      tuple(', '.join('0x%08lX' % v for v in P) for P in (L, D, [~m & 0xFFFFFFFF for m in M])))
    keys, tops, rows, data = [], [], [], {n: [] for n in ('sl_l', 'sl_d', 'sl_m', 'sc_l', 'sc_d', 'sc_m')}
    for a, b, d in SLOPES:
        lift = [0] * 4
        lift[a] = d
        if b is not None:
            lift[b] = d
        rp, cp = slope_marker(lift, False), slope_marker(lift, True)
        used = [y for y in range(26) if any(c is not None for c in rp[y] + cp[y])]
        if not used:                    # seen edge-on (the front corner 2 up): the flat marker
            continue
        y0, y1 = used[0], used[-1] + 1
        keys.append(sum(l << (2 * k) for k, l in enumerate(lift))); tops.append(y0); rows.append(y1 - y0)
        for pre, pix in (('sl', rp), ('sc', cp)):
            L, D, M = plane_rows(pix[y0:y1], 24, y1 - y0)
            data[pre + '_l'] += L; data[pre + '_d'] += D; data[pre + '_m'] += M
    w('#define NSLOPE %d\n' % len(keys))
    w('static const u8 slope_key[NSLOPE] = { %s };   // corner lifts, 2 bits each (top right bottom left)\n' % ', '.join(map(str, keys)))
    w('static const u8 slope_top[NSLOPE] = { %s };   // first row below 2 units over the lowest corner\n' % ', '.join(map(str, tops)))
    w('static const u8 slope_rows[NSLOPE] = { %s };\n' % ', '.join(map(str, rows)))
    w('static const u8 slope_off[NSLOPE] = { %s };   // its first row in sl_* and sc_*\n' % ', '.join(str(sum(rows[:k])) for k in range(len(rows))))
    for n, r in data.items():
        arr(n, r)
    w('#define SHADES 4\n')
    w('static const u16 turn_pat[SHADES][2][8] = {\n')
    for k in range(4):
        L, D = pattern(lambda x, y: k)
        w('    { { %s }, { %s } },\n' % (', '.join('0x%04X' % (v << 8 | v) for v in L), ', '.join('0x%04X' % (v << 8 | v) for v in D)))
    w('};\n')
    out.close()


if __name__ == '__main__':
    main()
