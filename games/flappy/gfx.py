#!/usr/bin/env python3
"""Flappy sprites: ASCII art -> gfx.h (ExtGraph layout: light, dark, mask; mask bit 1 = transparent).
Art chars: '.' transparent, 'w' white, 'l' light grey, 'd' dark grey, 'b' black.
outline=True adds a 1-pixel white ring (the shape dilated in 8 directions) for visibility.
Run: python3 gfx.py > gfx.h"""
import math

LEVEL = {'w': 0, 'l': 1, 'd': 2, 'b': 3}

BIRD = [                      # facing right, 13x10; wing drawn separately
    "....bbbbbb...",
    "..bbllllwwb..",
    ".bllllllwwwb.",
    "bllllllwwbwb.",
    "blllllllwwwb.",
    "bllllllbbbbbb",
    "bwllllbddddddb",
    ".bwwwwwbbbbbb.",
    "..bbwwwwwb....",
    "....bbbbb.....",
]
WINGS = [                     # overlays at rows 3..7, columns 0..5: up, middle, down
    ["bbbb..", "bwwlb.", "bwwlb.", ".bbb..", "......"],
    ["......", "bbbbb.", "bwwwlb", ".bbbb.", "......"],
    ["......", "......", "bbbbb.", "bwwlb.", ".bbb.."],
]
TILTS = [-45, 0, 45, 90]      # degrees, positive = nose down (90: lossless dive)

DIGITS = {                    # 6x9 bold, white strokes (outlined in black below: 8x11)
    '0': [".wwww.", "wwwwww", "ww..ww", "ww..ww", "ww..ww", "ww..ww", "ww..ww", "wwwwww", ".wwww."],
    '1': ["..ww..", ".www..", "wwww..", "..ww..", "..ww..", "..ww..", "..ww..", "wwwwww", "wwwwww"],
    '2': [".wwww.", "wwwwww", "....ww", "...www", "..www.", ".www..", "www...", "wwwwww", "wwwwww"],
    '3': ["wwwww.", "wwwwww", "....ww", "..wwww", "..wwww", "....ww", "....ww", "wwwwww", "wwwww."],
    '4': ["ww..ww", "ww..ww", "ww..ww", "wwwwww", "wwwwww", "....ww", "....ww", "....ww", "....ww"],
    '5': ["wwwwww", "wwwwww", "ww....", "wwwww.", "wwwwww", "....ww", "....ww", "wwwwww", "wwwww."],
    '6': [".wwww.", "wwwww.", "ww....", "wwwww.", "wwwwww", "ww..ww", "ww..ww", "wwwwww", ".wwww."],
    '7': ["wwwwww", "wwwwww", "....ww", "...www", "..www.", "..ww..", "..ww..", "..ww..", "..ww.."],
    '8': [".wwww.", "wwwwww", "ww..ww", ".wwww.", "wwwwww", "ww..ww", "ww..ww", "wwwwww", ".wwww."],
    '9': [".wwww.", "wwwwww", "ww..ww", "ww..ww", "wwwwww", ".wwwww", "....ww", ".wwwww", ".wwww."],
}

MEDAL = ["..bbbb..", ".bFFFFb.", "bFFSFFFb", "bFSSSFFb", "bFFSFFFb", "bFFFFFFb", ".bFFFFb.", "..bbbb.."]
MEDALS = [('d', 'l'), ('l', 'w'), ('w', 'd'), ('b', 'w')]   # (fill, star): bronze silver gold platinum


def grid(rows, w, h, ox=0, oy=0):
    g = [[None] * w for _ in range(h)]
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c != '.':
                g[oy + y][ox + x] = LEVEL[c]
    return g


def overlay(g, rows, ox, oy):
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c != '.':
                g[oy + y][ox + x] = LEVEL[c]


def scale2x(g):
    """EPX / Scale2x: doubles the size, keeps the pixel-art edges."""
    h, w = len(g), len(g[0])
    at = lambda y, x: g[min(max(y, 0), h - 1)][min(max(x, 0), w - 1)]
    out = [[None] * (2 * w) for _ in range(2 * h)]
    for y in range(h):
        for x in range(w):
            p, a, b, c, d = g[y][x], at(y - 1, x), at(y, x + 1), at(y, x - 1), at(y + 1, x)
            out[2 * y][2 * x] = a if c == a and c != d and a != b else p
            out[2 * y][2 * x + 1] = b if a == b and a != c and b != d else p
            out[2 * y + 1][2 * x] = c if d == c and d != b and c != a else p
            out[2 * y + 1][2 * x + 1] = d if b == d and b != a and d != c else p
    return out


def rotate(g, deg):
    """RotSprite: Scale2x three times (8x), nearest rotation, then the centre sample of each 8x8."""
    h, w = len(g), len(g[0])
    big = scale2x(scale2x(scale2x(g)))
    a = math.radians(deg)
    ca, sa = math.cos(a), math.sin(a)
    cx, cy = w / 2, h / 2
    out = [[None] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            px, py = x + 0.5 - cx, y + 0.5 - cy
            sx, sy = ca * px + sa * py + cx, -sa * px + ca * py + cy
            ix, iy = math.floor(sx * 8), math.floor(sy * 8)
            if 0 <= ix < 8 * w and 0 <= iy < 8 * h:
                out[y][x] = big[iy][ix]
    return out


def outlined(g, level):
    h, w = len(g), len(g[0])
    out = [r[:] for r in g]
    for y in range(h):
        for x in range(w):
            if g[y][x] is None and any(0 <= y + j < h and 0 <= x + i < w and g[y + j][x + i] is not None
                                       for j in (-1, 0, 1) for i in (-1, 0, 1)):
                out[y][x] = level
    return out


def planes(g, width):
    light, dark, mask = [], [], []
    for r in g:
        l = d = m = 0
        for x in range(width):
            v = r[x] if x < len(r) else None
            bit = 1 << (width - 1 - x)
            if v is None:
                m |= bit
            else:
                l |= bit if v & 1 else 0
                d |= bit if v & 2 else 0
        light.append(l); dark.append(d); mask.append(m)
    return light, dark, mask


def emit(name, sprites, width, h, ctype, out):
    fmt = '0x%0' + str(width // 4) + 'X'
    for part in ('light', 'dark', 'mask'):
        out.append(f"static const {ctype} {name}_{part}[{len(sprites)}][{h}] = {{")
        for s in sprites:
            out.append("    { " + ", ".join(fmt % v for v in s[part]) + " },")
        out.append("};")


def main():
    out = ["// Generated by gfx.py: do not edit. ExtGraph sprite rows: light, dark, mask (1 = transparent).",
           "#define BIRD_SX 4                      // sprite origin = hitbox origin - (BIRD_SX, BIRD_SY)",
           "#define BIRD_SY 5"]
    birds = []
    for t in TILTS:
        for wing in WINGS:
            g = grid(BIRD, 16, 16, 1, 3)
            overlay(g, wing, 1, 6)
            g = outlined(rotate(g, t), 0)
            l, d, m = planes(g, 16)
            birds.append({'light': l, 'dark': d, 'mask': m})
    emit("bird", birds, 16, 16, "u16", out)
    digits = []
    for c in "0123456789":
        g = outlined(grid(DIGITS[c], 8, 11, 1, 1), 3)
        l, d, m = planes(g, 8)
        digits.append({'light': l, 'dark': d, 'mask': m})
    emit("digit", digits, 8, 11, "u8", out)
    medals = []
    for fill, star in MEDALS:
        g = outlined(grid([r.replace('F', fill).replace('S', star) for r in MEDAL], 8, 8), 0)
        l, d, m = planes(g, 8)
        medals.append({'light': l, 'dark': d, 'mask': m})
    emit("medal", medals, 8, 8, "u8", out)
    # pipe body row (16 px) and cap (20 px in a 32-px sprite), land band (32 px, period 8)
    body = grid(["blwwllllllddddb".replace("blww", "bllww")], 16, 1)
    l, d, m = planes(body, 16)
    out.append(f"#define PIPE_BODY_L 0x{l[0]:04X}")
    out.append(f"#define PIPE_BODY_D 0x{d[0]:04X}")
    cap = grid(["b" * 20] + ["bllwwlllllllldddddd b".replace(" ", "")] * 4 + ["b" * 20], 32, 6)
    l, d, m = planes(cap, 32)
    emit("cap", [{'light': l, 'dark': d, 'mask': m}], 32, 6, "u32", out)
    land = grid(["ddddllll" * 4] * 4, 32, 4)
    l, d, m = planes(land, 32)
    emit("land", [{'light': l, 'dark': d, 'mask': m}], 32, 4, "u32", out)
    print("\n".join(out))


main()
