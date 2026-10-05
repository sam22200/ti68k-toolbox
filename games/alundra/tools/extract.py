#!/usr/bin/env python3
"""Alundra's art for the TI room, taken from the disc (decision: the graphics come from the game).

Runs the local disc headless (ti-port-ps1 psxrun.py) from a save state on the ship's deck:
- the player's poses: each key sequence (walk, stop, jump per direction) is played and the
  player's GPU packets (textured quads with CLUT 192,497) are read from RAM every frame; each new
  pose is composed from VRAM (two parts: body and legs), scaled by 0.48 (42 px tall -> 20) with a
  vote per target pixel over the source area, its 16 colours mapped to 4 greys, outlined in white;
- the scenery: four 16 x 16 cells of a screen of the village of Inoa (grass, the stones of a
  retaining wall, cobbles), two greys each by a luminance percentile.

Writes gfx.h (C arrays, ExtGraph sprite layout) and review sheets in x/. Commercial data:
gfx.h and x/ stay local (.gitignore); this script is what the repository keeps.

usage: tools/extract.py DISC.cue DECK.state INOA.state   (RE_NOTES.md § Tools: the ship's deck
       for the poses, the village of Inoa reached from a memory card save for the scenery)
"""
import os, sys, struct
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '../../../.claude/skills/ti-port-ps1/scripts'))
import psxrun

CLUT, TPAGE = (192, 497), (320, 256)          # the player's palette and texture page (4-bit)
SHADOW = (448, 256, 152, 88, 24, 16)          # his shadow: page x, y, u, v, w, h (4-bit, semi-transparent)
# the shadow's sizes: on the ground (24 x 16 scaled) and high in a jump (the game: 22 x 14 at the apex)
SHADOW_SIZES = ((13, 8), (12, 7))
SCALE = 22 / 42                                # 42 px tall standing -> 22 (the user's size B)
SW, SH, AY = 16, 25, 23                        # sprite 16 x 25, feet on row 23
WW, WX = 32, 16                                # scaled first into 32 columns, feet at column 16,
                                               # then cut to the 16 around the pose (offset kept)
IDLE = (21, 5, 2)                              # breathing: stand, in, out (steps at 32 fps; the
                                               # game: 40, 10, 4 frames at 60 Hz)
# palette index -> grey (0 white .. 3 black): gold hair and skin light, blues and browns dark
GREY = [0, 3, 2, 1, 3, 2, 1, 0, 2, 1, 0, 3, 2, 1, 0, 0]
# per direction: the walk (6 images after the stand), then the stop, then a jump
DIRS = {'down': 'DOWN', 'up': 'UP', 'right': 'RIGHT'}
# scenery cells of the village's screen (column, row of 16 px) and luminance percentile; the
# screen: INOA.state, DOWN+LEFT held 150 frames (a grass terrace above a stone retaining wall)
CELLS = {'floor': ((6, 4), 25), 'face': ((5, 12), 35), 'walltop': ((11, 11), 35), 'wallface': ((14, 11), 35)}
VILLAGE_WALK = 150


def sxy(v):
    x, y = v & 0xffff, v >> 16
    return (x - 0x10000 if x >= 0x8000 else x, y - 0x10000 if y >= 0x8000 else y)


def player_quads(ram):
    """the player's parts in each of the two packet buffers, in drawing order"""
    w = struct.unpack('<%dI' % (len(ram) // 4), ram)
    bufs = {}
    for i in range(1, len(w) - 9):
        if (w[i] >> 24) in (0x2c, 0x2d, 0x2e, 0x2f) and 8 <= (w[i - 1] >> 24) <= 10:
            cl = w[i + 2] >> 16
            if ((cl & 63) * 16, cl >> 6) != CLUT:
                continue
            p = [sxy(w[i + k]) for k in (1, 3, 5, 7)]
            uv = [(w[i + k] & 0xff, (w[i + k] >> 8) & 0xff) for k in (2, 4, 6, 8)]
            bufs.setdefault(i & ~0x3ff, []).append((i, p, uv))
    out = []
    for parts in bufs.values():
        parts.sort(key=lambda t: -t[0])        # the later packet is linked first: drawn first
        out.append([(p, uv) for _, p, uv in parts])
    return out


def pose_key(parts):
    """the pose relative to its legs (the part reaching lowest): feet = its bottom centre"""
    legs = max(parts, key=lambda t: max(y for _, y in t[0]))
    ax = (legs[0][0][0] + legs[0][1][0]) / 2
    ay = max(y for _, y in legs[0])
    return tuple((tuple(uv), tuple((x - ax, y - ay) for x, y in p)) for p, uv in parts)


def compose(vram, key):
    """the pose drawn from VRAM: palette indices, -1 transparent; feet at (32, 60)"""
    img = np.full((64, 64), -1, int)
    tx, ty = TPAGE
    for uv, p in key:
        (x0, y0), (x1, _), (_, y1) = p[0], p[1], p[2]
        (u0, v0), (u1, _), (_, v1) = uv[0], uv[1], uv[2]
        for y in range(int(y0), int(y1)):
            t = v0 + (y - int(y0)) * (v1 - v0) // int(y1 - y0)
            for x in range(int(min(x0, x1)), int(max(x0, x1))):
                s = int((x - x0) * (u1 - u0) / (x1 - x0)) + u0
                k = (vram[ty + t, tx + s // 4] >> ((s & 3) * 4)) & 15
                if vram[CLUT[1], CLUT[0] + k]:
                    img[y + 60, x + 32] = k
    return img


def shrink(src):
    """vote per target pixel over its source area, anchored at the feet; grey or -1"""
    out = np.full((SH, WW), -1, int)
    g = np.where(src >= 0, np.take(GREY, np.maximum(src, 0)), 4)
    for j in range(SH):
        for i in range(WW):
            x0, x1 = 32 + (i - WX) / SCALE, 32 + (i + 1 - WX) / SCALE
            y0, y1 = 60 + (j - AY) / SCALE, 60 + (j + 1 - AY) / SCALE
            acc = np.zeros(5)
            for y in range(max(0, int(y0)), min(64, int(np.ceil(y1)))):
                wy = min(y + 1, y1) - max(y, y0)
                for x in range(max(0, int(x0)), min(64, int(np.ceil(x1)))):
                    acc[g[y, x]] += wy * (min(x + 1, x1) - max(x, x0))
            if acc[:4].sum() >= acc.sum() / 2 and acc.sum():
                out[j, i] = int(np.argmax(acc[:4]))
    out[0] = -1                                # room for the outline
    return out


def window(img):
    """the 16 columns around the pose (outline included) and their offset from the feet; a
    pose wider than 16 loses its outermost columns on both sides"""
    xs = np.nonzero((img >= 0).any(0))[0]
    left = int(round((xs[0] + xs[-1] + 1) / 2 - SW / 2))
    left = max(0, min(WW - SW, left))
    return img[:, left:left + SW], left - WX


def outlined(s):
    """white outline (mask dilated by one pixel): grey 0 where the body is not"""
    body = s >= 0
    o = body.copy()
    o[1:] |= body[:-1]; o[:-1] |= body[1:]; o[:, 1:] |= body[:, :-1]; o[:, :-1] |= body[:, 1:]
    r = np.where(body, s, np.where(o, 0, -1))
    return r


def run_seq(psx, state, seq):
    psx.load(state)
    poses = []
    for keys, n in seq:
        psx.pressed = 0
        for k in keys.split():
            psx.pressed |= 1 << psxrun.PAD[k]
        for _ in range(n):
            psx.run()
            frame = []
            for parts in player_quads(bytes(psx.ram)):
                frame.append(pose_key(parts))
            poses.append(frame)
    return poses


def first_new(frames, start, seen):
    """poses in order of first appearance from frame `start` on, not in `seen`"""
    out = []
    for f in frames[start:]:
        for k in f:
            if k not in seen and k not in out:
                out.append(k)
    return out


def sprite_rows(img, w):
    """grey image (-1 transparent) -> light, dark, mask rows (MSB = left pixel)"""
    L, D, M = [], [], []
    for row in img:
        l = d = m = 0
        for x, g in enumerate(row):
            bit = 1 << (w - 1 - x)
            if g < 0: m |= bit
            else:
                if g & 1: l |= bit
                if g & 2: d |= bit
        L.append(l); D.append(d); M.append(m)
    return L, D, M


def shadow_masks(vram):
    """the shadow's texture (non-zero texels) area-scaled to each size: rows, MSB = left"""
    tx, ty, u0, v0, w, h = SHADOW
    tex = np.array([[(vram[ty + v0 + t, tx + (u0 + u) // 4] >> (((u0 + u) & 3) * 4)) & 15 != 0
                     for u in range(w)] for t in range(h)], float)
    out = []
    for sw, sh in SHADOW_SIZES:
        img = np.array(Image.fromarray((tex * 255).astype(np.uint8)).resize((sw, sh), Image.BOX)) >= 128
        out.append([int(''.join('1' if b else '0' for b in r).ljust(16, '0'), 2) for r in img])
    return out


def cell(screen, c, pct):
    i, j = c
    rgb = screen[j * 16:j * 16 + 16, i * 16:i * 16 + 16].astype(float)
    lum = 0.299 * rgb[..., 0] + 0.587 * rgb[..., 1] + 0.114 * rgb[..., 2]
    return (lum < np.percentile(lum, pct)).astype(int)       # 1 = line, 0 = base


def main():
    disc, state, village = sys.argv[1], sys.argv[2], sys.argv[3]
    out = os.fdopen(os.dup(1), 'w', buffering=1)
    os.dup2(os.open(os.devnull, os.O_WRONLY), 1)          # the core prints on stdout
    psx = psxrun.PSX(disc)
    psx.load(state); psx.run()
    idle = run_seq(psx, state, [('', 3)])[-1]
    stand0 = set(idle)
    frames = {}
    for d, key in DIRS.items():
        walk = run_seq(psx, state, [(key, 75), ('', 20)])
        w = first_new(walk[:75], 3, stand0)
        stand = idle[0] if d == 'down' else walk[-1][0]
        jump = run_seq(psx, state, [(key, 4), (key + ' CROSS', 2), (key, 30)])
        j = first_new(jump, 4, set(w) | {stand})[:2]
        rest = run_seq(psx, state, [(key, 12), ('', 80)])
        stand = rest[12 + 6][0]                          # the stand after a walk (40 frames)
        b = first_new(rest, 12, set(w) | {stand} | set(j))[:2]
        frames[d] = [stand] + w[:6] + j + b
        out.write('%s: %d walk, %d jump, %d breathing\n' % (d, len(w), len(j), len(b)))
    vram = np.frombuffer(bytes(psx.vram), '<u2').reshape(512, 1024)
    wide = {d: [outlined(shrink(compose(vram, k))) for k in ks] for d, ks in frames.items()}
    wide['left'] = [im[:, ::-1] for im in wide['right']]     # mirrored about the feet (16 | 16)
    imgs, ox = {}, {}
    for d, ims in wide.items():
        cut = [window(im) for im in ims]
        imgs[d] = [c for c, _ in cut]
        ox[d] = [o for _, o in cut]
    psx.load(village)
    psx.pressed = 1 << psxrun.PAD['DOWN'] | 1 << psxrun.PAD['LEFT']
    for _ in range(VILLAGE_WALK + 2):
        psx.run()
    screen = np.array(psx.image().convert('RGB'))
    os.makedirs('x', exist_ok=True)
    order = ['down', 'up', 'left', 'right']
    # review sheet: every image on light grey, x4
    pal = np.array([[255] * 3, [170] * 3, [85] * 3, [0] * 3, [170] * 3], np.uint8)
    rows = [np.concatenate([pal[np.where(im < 0, 4, im)] for im in imgs[d]], 1) for d in order]
    sheet = np.concatenate(rows, 0)
    Image.fromarray(sheet).resize((sheet.shape[1] * 4, sheet.shape[0] * 4), Image.NEAREST).save('x/hero.png')
    Image.fromarray(screen).save('x/village.png')
    with open('gfx.h', 'w') as f:
        f.write('// Generated by tools/extract.py from the local disc: do not edit, do not commit.\n')
        f.write('#define HERO_SW %d\n#define HERO_SH %d\n#define HERO_AY %d\n' % (SW, SH, AY))
        f.write('#define HERO_FRAMES %d  // per direction: stand, walk x 6, jump (take-off, air), breathing x 2\n' % len(imgs['down']))
        f.write('#define IDLE_STAND %d\n#define IDLE_IN %d\n#define IDLE_OUT %d\n' % IDLE)
        f.write('// directions: 0 down, 1 up, 2 left, 3 right\n')
        f.write('static const u16 hero_gfx[4][%d][3][%d] = {\n' % (len(imgs['down']), SH))
        for d in order:
            f.write('  {\n')
            for im in imgs[d]:
                f.write('    {%s},\n' % ', '.join('{%s}' % ','.join('0x%04x' % v for v in r) for r in sprite_rows(im, SW)))
            f.write('  },\n')
        f.write('};\n')
        f.write('// x of each image left column relative to the feet: the game own placement\n')
        f.write('static const s8 hero_ox[4][%d] = {%s};\n' % (len(imgs['down']),
                ', '.join('{%s}' % ','.join(str(v) for v in ox[d]) for d in order)))
        f.write('// the shadow (darkens the floor one grey), %d x %d and %d x %d, centred on the feet\n'
                % (SHADOW_SIZES[0] + SHADOW_SIZES[1]))
        for k, (m, (sw, sh)) in enumerate(zip(shadow_masks(vram), SHADOW_SIZES)):
            f.write('#define SHADOW%d_W %d\n#define SHADOW%d_H %d\n#define SHADOW%d_N %d\n'
                    % (k, sw, k, sh, k, sum(bin(v).count('1') for v in m)))
            f.write('static const u16 shadow%d[%d] = {%s};\n' % (k, sh, ','.join('0x%04x' % v for v in m)))
        for name, (c, pct) in CELLS.items():
            t = cell(screen, c, pct)
            f.write('static const u8 tex_%s[16][2] = {' % name)
            f.write(','.join('{0x%02x,0x%02x}' % tuple(int(''.join(map(str, r[k:k + 8])), 2) for k in (0, 8)) for r in t))
            f.write('};\n')
    out.write('gfx.h: %d hero images, %d textures\n' % (4 * len(imgs['down']), len(CELLS)))


if __name__ == '__main__':
    main()
