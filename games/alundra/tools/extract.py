#!/usr/bin/env python3
"""Alundra's art for the TI room, taken from the disc (decision: the graphics come from the game).

Runs the local disc headless (ti-port-ps1 psxrun.py) from a save state on the ship's deck:
- the player's poses: each key sequence (walk, stop, jump per direction) is played and the
  player's GPU packets (textured quads with CLUT 192,497) are read from RAM every frame; each new
  pose is composed from VRAM (two parts: body and legs), scaled by 0.48 (42 px tall -> 20) with a
  vote per target pixel over the source area, its 16 colours mapped to 4 greys, outlined in white;
- the scenery: four 16 x 16 cells of a screen of the village of Inoa (grass, the stones of a
  retaining wall, cobbles), two greys each by a luminance percentile.
- the village of Inoa (milestone 9): its height map read from RAM, resampled to TI tiles (a
  level = 16 px of the game), stairs, walls; its image pasted from the game's screens, the
  player and the NPCs removed, scaled, 4 greys: alvil0.bin, alvil1.bin (the TI data files).

Writes gfx.h (C arrays, ExtGraph sprite layout) and review sheets in x/. Commercial data:
gfx.h and x/ stay local (.gitignore); this script is what the repository keeps.

usage: tools/extract.py DISC.cue DECK.state INOA.state [N/D]   (RE_NOTES.md § Tools: the ship's
       deck for the poses, the village of Inoa reached from a memory card save for the scenery;
       N/D the game's scale on the TI, default 22/42: everything else follows from it)
"""
import os, sys, struct
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '../../../.claude/skills/ti-port-ps1/scripts'))
import psxrun
import psxgpu

CLUT, TPAGE = (192, 497), (320, 256)          # the player's palette and texture page (4-bit)
SHADOW = (448, 256, 152, 88, 24, 16)          # his shadow: page x, y, u, v, w, h (4-bit, semi-transparent)
# the scale: 22/42 = Alundra 42 px tall standing -> 22 (the user's size B); a parameter (Makefile
# SCALE): the sprite, the shadow, the speeds and the levels all follow from it
SCALE_ND = tuple(int(v) for v in (sys.argv[4] if len(sys.argv) > 4 else '22/42').split('/'))
SCALE = SCALE_ND[0] / SCALE_ND[1]
HEIGHT = round(42 * SCALE)                     # standing height on the TI (22)
SW = 16 if SCALE <= 0.6 else 32                # sprite width (ExtGraph: 16 or 32)
SH, AY = HEIGHT + 3, HEIGHT + 1                # sprite 16 x 25, feet on row 23
WW, WX = 2 * SW, SW                            # scaled first into 32 columns, feet at column 16,
                                               # then cut to the 16 around the pose (offset kept)
# the shadow's sizes: on the ground (24 x 16 scaled) and high in a jump (the game: 22 x 14 at the apex)
SHADOW_SIZES = ((round(24 * SCALE), round(16 * SCALE)), (round(22 * SCALE), round(14 * SCALE)))
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
# the village of Inoa (milestone 9): the game's own image of it on the TI, the collision from its
# height map (RE_NOTES.md § Addresses: pointer at 801ac6c4, cells of 24 x 16 px, the height in
# 16 px units in byte 3, the slope type in byte 2, flags 0x41 = wall) resampled to TI tiles of
# 16 / SCALE px of the game (30.5 at 22/42), the most common cell under each; a level = 16 px
# of the game, as in the game (Experiment 11)
MAP_PTR, MAP_COL = 0x801ac6c4, 0x80137990
V_X0, V_X1, V_ROW0, V_ROW1 = 0, 1248, 11, 57   # the village: x px, map rows (outside = walls)
V_BASE = 1                                     # the lowest ground (level 0), 16-px units
V_START = (732, 424, 10)                       # the player in INOA.state (leaving the house): x, y, height
# the village's image: the player teleported over a grid (V_GRID), the other objects (NPCs)
# moved out of the map, each settled screen pasted where its camera is (the player's position
# minus his feet on screen, read from the GPU packets), in (x, y - z + V_OY); several screens
# per pixel, the median kept (the player and what moves vanish)
V_GRID, V_OY, V_K = ((60, 1248, 110), (150, 960, 70)), 400, 6
V_FILL, V_TRIES = 40, ((0, 40), (0, 120), (0, 200), (-60, 120), (60, 120), (0, -40))
V_PCT = (18, 45, 72)                           # 4 greys: luminance percentiles (the grass apart)
V_PAVE = 95                                    # paving: grey and brighter than this on average
OBJS, OBJ_SIZE = 0x801ac6f8, 0x294             # the objects (0 = the player): +0x114 x
OT_N = 1100                                    # ordering-table entries (964 used, 16 per map row)

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


def sc(v):
    """alundra.h SC(): a size set at the reference scale 22/42, at SCALE"""
    n, d = SCALE_ND
    return (v * 42 * n + d * 11) // (d * 22)


def depth_rows(psx):
    """per screen pixel, the map row (16 px) of the scenery drawn on top in the frame just run
    (the GPU log, psxgpu.py: its ordering-table entry = 16 x row + 5 or 6; the player's is 16 x
    his row + 10, so whatever a row in front of his draws hides him), -1 where nothing is; the
    player and what is semi-transparent (shadows) left out"""
    prims = psxgpu.parse(*psxgpu.frame_log(psx))
    ots = [p['addr'] for p in prims if p['kind'] == 'ot' and p['addr'] < 0x1f0000]
    if not ots:
        return None
    base = min(ots)
    vram = np.frombuffer(bytes(psx.vram), '<u2').reshape(512, 1024)
    clut = (CLUT[1] << 6) | (CLUT[0] >> 4)
    _, own = psxgpu.replay(prims, vram, psxgpu.area(prims),
                           skip=lambda p: p['semi'] or (p['kind'] == 'poly' and p['clut'] == clut))
    ok = (own >= base) & (own < base + 4 * OT_N)
    return np.where(ok, ((own - base) >> 2) >> 4, -1)


def village_image(psx, state):
    """the village as the game draws it, without the player or the NPCs (V_GRID, median), and
    the map row of what each pixel shows (depth_rows, the median too)"""
    H, W = 1400, 1300
    samp = np.zeros((H, W, V_K, 3), np.uint8)
    ksamp = np.zeros((H, W, V_K), np.int16)
    cnt = np.zeros((H, W), np.uint8)

    def step():
        for k in range(1, 64):                 # every other object out of the map
            o = OBJS + k * OBJ_SIZE
            if psx.read(o + 0x114, 4):
                psx.write(o + 0x114, 3000 << 16, 4)
        psx.run()

    def cam():
        px, py, pz = (psx.read(a, 4) / 65536 for a in (0x801ac80c, 0x801ac810, 0x801ac814))
        q = player_quads(bytes(psx.ram))
        if len(q) != 2:
            return None
        f = []
        for b in q:
            legs = max(b, key=lambda t: max(v for _, v in t[0]))
            f.append(((legs[0][0][0] + legs[0][1][0]) / 2, max(v for _, v in legs[0])))
        if f[0] != f[1]:                       # both packet buffers: the player at rest
            return None
        return int(round(px - f[0][0])), int(round(py - pz - f[0][1])) + V_OY
    def shot(x, y):
        """the player dropped at (x, y): the settled screen pasted; False if none"""
        psx.load(state); psx.run()
        psx.write(0x801ac80c, x << 16, 4)
        psx.write(0x801ac810, y << 16, 4)
        psx.write(0x801ac814, 0x100 << 16, 4)
        for _ in range(100):
            step()
        c1 = cam(); step(); c2 = cam()
        psx.lib.retro_tiport_gplog_reset()
        step(); c3 = cam()
        if not c1 or c1 != c2 or c2 != c3:     # the camera still moving: skipped
            return False
        rows = depth_rows(psx)
        if rows is None:
            return False
        img = np.array(psx.image().convert('RGB'))[40:232, 8:312]   # the HUD left out
        rows = rows[40:232, 8:312]
        ys, xs = c3[1] + 40, c3[0] + 8
        Y0, X0 = max(0, ys), max(0, xs)
        Y1, X1 = min(H, ys + img.shape[0]), min(W, xs + img.shape[1])
        if Y1 <= Y0 or X1 <= X0:
            return False
        sub = img[Y0 - ys:Y1 - ys, X0 - xs:X1 - xs]
        c = cnt[Y0:Y1, X0:X1]
        yy, xx = np.nonzero(c < V_K)
        samp[Y0 + yy, X0 + xx, c[yy, xx]] = sub[yy, xx]
        ksamp[Y0 + yy, X0 + xx, c[yy, xx]] = rows[Y0 - ys:Y1 - ys, X0 - xs:X1 - xs][yy, xx]
        c[yy, xx] += 1
        return True
    for y in range(*V_GRID[1]):
        for x in range(*V_GRID[0]):
            shot(x, y)
    # the holes (a drop into a wall, a camera still moving): points of the village not seen
    # yet, each tried from players around it (the camera shows him about 120 px from the top)
    Ys = np.nonzero(cnt.any(1))[0]
    for Y in range(Ys[0] + 20, Ys[-1], V_FILL):
        for X in range(16, V_X1 - 8, V_FILL):
            for dx, dy in V_TRIES:
                if cnt[Y, X]:
                    break
                shot(min(V_X1 - 12, max(12, X + dx)), Y - V_OY + dy)
    med = np.zeros((H, W, 3), np.uint8)
    key = np.full((H, W), -1, np.int16)
    for k in range(1, V_K + 1):
        m = cnt == k
        if m.any():
            med[m] = np.median(samp[m][:, :k], axis=1).astype(np.uint8)
            key[m] = np.median(ksamp[m][:, :k], axis=1).astype(np.int16)
    return med, key


def village_tiles(psx):
    """the village as TI tiles: level (0-63) | 0x80 wall | 0x40 stair, a level = 16 px of the
    game (alundra.h § A tile)"""
    m = psx.read(MAP_PTR, 4)
    tile = 16 / SCALE
    w, h = int(np.ceil((V_X1 - V_X0) / tile)), int(np.ceil((V_ROW1 + 1 - V_ROW0) * 16 / tile))
    cache = {}

    def cell(x, y):
        c, r = psx.read(MAP_COL + 2 * x, 2), y >> 4
        if (c, r) not in cache:
            a = m + 0x604 + c * 8 + r * 0x1a0
            f, t, ht = psx.read(a, 2), psx.read(a + 2), psx.read(a + 3)
            cache[c, r] = 'W' if f & 0x41 or ht < V_BASE or not V_ROW0 <= r <= V_ROW1 else \
                ('S', ht) if t & 3 else ht
        return cache[c, r]
    grid = []
    for j in range(h):
        row = []
        for i in range(w):
            count = {}
            for sy in range(8):
                for sx in range(8):
                    x = int(V_X0 + (i + (sx + .5) / 8) * tile)
                    y = int(V_ROW0 * 16 + (j + (sy + .5) / 8) * tile)
                    k = cell(x, y) if x < V_X1 else 'W'
                    count[k] = count.get(k, 0) + 1
            row.append(max(count, key=count.get))
        grid.append(row)
    out = [[0x80 | 3] * w for _ in range(h)]
    for j in range(1, h - 1):                  # the exits closed: walls all round
        for i in range(1, w - 1):
            v = grid[j][i]
            # every cell the game does not flag is a floor at its own height, the houses'
            # roofs too (OBSERVED: dropped on one, the player stands and walks on it); its
            # slope cells (stairs, roofs) let the feet follow the floor
            if isinstance(v, int):
                out[j][i] = min(63, v - V_BASE)
            elif v != 'W':
                out[j][i] = 0x40 | min(63, v[1] - V_BASE)
    # walls: their level is the height drawn (the image's own), at least 3 above the floor near
    for j in range(h):
        for i in range(w):
            if out[j][i] & 0x80:
                v = grid[j][i]
                near = [out[b][a] & 63 for a, b in ((i - 1, j), (i + 1, j), (i, j - 1), (i, j + 1))
                        if 0 <= a < w and 0 <= b < h and not out[b][a] & 0x80]
                hv = v[1] if isinstance(v, tuple) else v if isinstance(v, int) else 0
                out[j][i] = 0x80 | min(63, max(hv - V_BASE, (max(near) if near else 0) + 3))
    sx, sy = round((V_START[0] - V_X0) * SCALE), round((V_START[1] - V_ROW0 * 16) * SCALE)
    i, j = min(((i, j) for j in range(h) for i in range(w) if out[j][i] == V_START[2] - V_BASE),
               key=lambda p: (p[0] * 16 + 8 - sx) ** 2 + (p[1] * 16 + 8 - sy) ** 2)
    return out, (i * 16 + 8, j * 16 + 8)


def level_px(h):
    """alundra.h LEVEL_PX(): screen px of h levels"""
    return (h * sc(134)) >> 4


def picture_top(tiles):
    """the image row of tile row 0: room above for the highest floor and a player on it"""
    return max(0, max(level_px(v & 63) - j * 16 for j, r in enumerate(tiles) for v in r
                      if not v & 0x80)) + HEIGHT + 4


def village_picture(canvas, tiles, top, floor):
    """the image on the TI: the village's image scaled by SCALE (an area average), 4 greys
    (luminance percentiles; the grass a flat light grey, the paving white, lone pixels gone,
    for the TI's low-contrast LCD), as two
    planes (light, dark: 1 bit per pixel, MSB left); row 0 of the tiles at image row `top`;
    floor: the pixels showing a terrace's floor (terrace_floor: only they may be paving)"""
    from scipy import ndimage
    h, w = len(tiles), len(tiles[0])
    iw, ih = w * 16, top + h * 16
    small = np.array(Image.fromarray(canvas).resize(
        (round(canvas.shape[1] * SCALE), round(canvas.shape[0] * SCALE)), Image.BOX)).astype(float)
    # TI image row v = top + (y - z - V_ROW0 * 16) * SCALE, z the TI's (level 0 = the game's
    # height V_BASE): canvas row (y - z - V_BASE * 16 + V_OY)
    off = round(((V_ROW0 - V_BASE) * 16 + V_OY) * SCALE) - top
    rgb = np.zeros((ih, iw, 3))
    for v in range(ih):
        r = v + off
        if 0 <= r < small.shape[0]:
            n = min(iw, small.shape[1])
            rgb[v, :n] = small[r, :n]
    L = rgb @ [0.299, 0.587, 0.114]
    valid = L > 3
    # the grass (olive, saturated, dark, smooth; its shadows too) one flat light grey: the
    # hero (dark, white outline) reads on it; foliage is greener, lighter and textured
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    mx, mn = rgb.max(-1), rgb.min(-1)
    lt = (mx + mn) / 510
    sat = (mx - mn) / np.maximum(np.where(lt < 0.5, mx + mn, 510 - mx - mn), 1)
    hue = np.degrees(np.arctan2(np.sqrt(3) * (g - b), 2 * r - g - b)) % 360
    sd = np.sqrt(np.maximum(ndimage.uniform_filter(L * L, 5) - ndimage.uniform_filter(L, 5) ** 2, 0))
    grass = (hue > 38) & (hue < 62) & (sat > 0.4) & (lt < 0.3) & (sd < 14)
    grass = ndimage.binary_opening(ndimage.binary_closing(grass, iterations=2), iterations=2)
    t = [np.percentile(L[valid & ~grass], q) for q in V_PCT]
    q = 3 - np.digitize(L, t)                   # 0 white .. 3 black
    q[grass] = 1
    # the paving (grey, bright on average, on a terrace's floor: not the slate roofs) white,
    # its joints light grey where clearly darker
    pave = floor & ~grass & (sat < 0.3) & (ndimage.uniform_filter(L, 5) > V_PAVE)
    q[pave] = np.where(L[pave] < ndimage.uniform_filter(L, 7)[pave] - 22, 1, 0)
    # lone pixels (no neighbour of the same grey) take the commonest grey around
    same = sum((np.roll(np.roll(q, dy, 0), dx, 1) == q).astype(int)
               for dy in (-1, 0, 1) for dx in (-1, 0, 1)) - 1
    best, bc = np.zeros(q.shape, int), np.full(q.shape, -1.)
    for k in range(4):
        c = ndimage.uniform_filter((q == k).astype(float), 3)
        best[c > bc], bc[c > bc] = k, c[c > bc]
    q = np.where(same <= 1, best, q)
    q[~valid] = 0
    planes = []
    for bit in (1, 2):                          # light = grey & 1, dark = grey >> 1
        p = ((q & bit) != 0).astype(np.uint8)
        planes.append(np.packbits(p, axis=1).tobytes())
    return planes, top, iw // 8, ih, q


def ti_rows(key, top, iwb, ih):
    """the map row of what each TI image pixel shows (-1: nothing), sampled at its centre"""
    off = round(((V_ROW0 - V_BASE) * 16 + V_OY) * SCALE) - top
    vv, uu = np.mgrid[0:ih, 0:iwb * 8]
    ys = np.clip(((vv + off + 0.5) / SCALE).astype(int), 0, key.shape[0] - 1)
    xs = np.clip(((uu + 0.5) / SCALE).astype(int), 0, key.shape[1] - 1)
    return key[ys, xs].astype(int)


def terrace_floor(psx, rows):
    """per TI image pixel: whether what it shows is a terrace's flat floor (an unflagged flat
    map cell at one of the common heights: not a roof, a wall or an object), from its map row
    and its x"""
    m = psx.read(MAP_PTR, 4)
    col = np.array([psx.read(MAP_COL + 2 * x, 2) for x in range(V_X1)])
    cells = {}
    for c in set(col.tolist()):
        for r in range(V_ROW0, V_ROW1 + 1):
            a = m + 0x604 + c * 8 + r * 0x1a0
            cells[c, r] = (psx.read(a, 2) & 0x41, psx.read(a + 2) & 3, psx.read(a + 3))
    flat = [h for (f, t, h) in (cells[c, r] for c in col.tolist() for r in range(V_ROW0, V_ROW1 + 1))
            if not f and not t and h >= V_BASE]
    n = np.bincount(flat)
    terr = set(np.nonzero(n >= len(flat) * 0.05)[0].tolist())
    ih, iw = rows.shape
    xs = np.minimum(((np.arange(iw) + 0.5) / SCALE).astype(int), V_X1 - 1)
    out = np.zeros(rows.shape, bool)
    for v in range(ih):
        for u in range(iw):
            k = rows[v, u]
            if V_ROW0 <= k <= V_ROW1:
                f, t, h = cells[int(col[xs[u]]), int(k)]
                out[v, u] = not f and not t and h in terr
    return out


def village_depth(rows, iwb, ih):
    """what hides the player, per image byte (8 pixels): a threshold T (the player is behind
    when his y / 2 < T, TI px) and the mask of its pixels (MSB left). A pixel of map row k
    hides him when his row (his y / 16 rounded, the game's) is above k: y < 16 k - 8; a byte
    keeps its frontmost row (its other pixels never hide); sampled at each TI pixel's centre"""
    k = rows
    T = np.where(k >= 0, np.round((16 * k - 8 - V_ROW0 * 16) * SCALE / 2), 0).clip(0, 255).astype(int)
    tb = T.reshape(ih, iwb, 8)
    tmax = tb.max(2)
    bits = (tb == tmax[..., None]) & (tmax[..., None] > 0)
    mask = np.packbits(bits.reshape(ih, iwb * 8).astype(np.uint8), axis=1)
    return tmax.astype(np.uint8).tobytes(), mask.tobytes(), T


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
    os.makedirs('x', exist_ok=True)
    if os.environ.get('REUSE') and os.path.exists('x/village_key.npy'):   # the last capture
        canvas = np.array(Image.open('x/village_full.png').convert('RGB'))
        vkey = np.load('x/village_key.npy')
    else:
        canvas, vkey = village_image(psx, village)
        Image.fromarray(canvas).save('x/village_full.png')
        np.save('x/village_key.npy', vkey)
    psx.load(village); psx.run()
    vil, vstart = village_tiles(psx)
    vtop = picture_top(vil)
    viwb, vih = len(vil[0]) * 2, vtop + len(vil) * 16
    rows = ti_rows(vkey, vtop, viwb, vih)
    planes, vtop, viwb, vih, vq = village_picture(canvas, vil, vtop, terrace_floor(psx, rows))
    dkey, dmask, dT = village_depth(rows, viwb, vih)
    for k, pl in enumerate(planes + [dkey, dmask]):   # the TI data variables (Makefile: .89y)
        open('alvil%d.bin' % k, 'wb').write(pl + bytes(2))   # (the view copy reads a word past a row)
    Image.fromarray((dT * 255 // max(1, dT.max())).astype(np.uint8)).save('x/village_depth.png')
    Image.fromarray(np.array([255, 170, 85, 0], np.uint8)[vq]).save('x/village_ti.png')
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
        f.write('#ifndef GFX_H\n#define GFX_H\n')
        f.write('#define SCALE_N %d\n#define SCALE_D %d   // the scale on the TI\n' % SCALE_ND)
        f.write('#define HERO_SW %d\n#define HERO_SH %d\n#define HERO_AY %d\n' % (SW, SH, AY))
        f.write('typedef u%d hero_row;\n' % SW)
        f.write('#define HERO_FRAMES %d  // per direction: stand, walk x 6, jump (take-off, air), breathing x 2\n' % len(imgs['down']))
        f.write('#define IDLE_STAND %d\n#define IDLE_IN %d\n#define IDLE_OUT %d\n' % IDLE)
        f.write('// directions: 0 down, 1 up, 2 left, 3 right\n')
        f.write('static const hero_row hero_gfx[4][%d][3][%d] = {\n' % (len(imgs['down']), SH))
        for d in order:
            f.write('  {\n')
            for im in imgs[d]:
                f.write('    {%s},\n' % ', '.join('{%s}' % ','.join('0x%x' % v for v in r) for r in sprite_rows(im, SW)))
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
        f.write('// the village of Inoa: %d x %d tiles, level | 0x80 wall | 0x40 stair; its image in\n'
                '// alvil0.bin (light) and alvil1.bin (dark): %d bytes x %d rows, tile row 0 at row %d\n'
                % (len(vil[0]), len(vil), viwb, vih, vtop))
        f.write('#define VILLAGE_W %d\n#define VILLAGE_H %d\n#define VILLAGE_TOP %d\n' % (len(vil[0]), len(vil), vtop))
        f.write('#define VILLAGE_IWB %d\n#define VILLAGE_IH %d\n' % (viwb, vih))
        f.write('#define VILLAGE_SX %d\n#define VILLAGE_SY %d\n' % vstart)
        f.write('static const u8 village[%d] = {\n%s};\n' % (len(vil) * len(vil[0]), ''.join(
                '  %s,\n' % ','.join('0x%02x' % v for v in r) for r in vil)))
        f.write('#endif\n')
    out.write(''.join(''.join('#' if v & 0x80 else '/' if v & 0x40 else '%x' % v for v in r) + '\n' for r in vil))
    out.write('village: top %d, image %d x %d, start %s\n' % (vtop, viwb * 8, vih, vstart))
    out.write('gfx.h: %d hero images, %d textures\n' % (4 * len(imgs['down']), len(CELLS)))


if __name__ == '__main__':
    main()
