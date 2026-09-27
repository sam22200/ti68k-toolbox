#!/usr/bin/env python3
"""FFA art pipeline: a room's picture in 4 greys, built from its collision grid and a layout.

Levels: 0 white, 1 light grey, 2 dark grey, 3 black, None = transparent (objects).
- Walls are auto-tiled from the collision grid (3/4 view, Chrono Trigger style): a wall cell
  with a walkable cell below shows its brick face, other wall cells show the dark wall top with
  a lighter rim along the floor.
- Floors and the wall face are procedural 16x16 tiles (STYLES).
- Objects (furniture) are CC0 crops (Pixel-boy's Ninja Adventure tileset, sources/assets, CC0)
  converted to 4 greys with per-object luminance quantiles, or ASCII art; LAYOUTS places them.
render_room(u, style) -> (w*16) x (h*16) level grid; tiles_of(grid) cuts and deduplicates.
"""
import os
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
CC0 = os.path.join(HERE, '..', '..', '..', 'sources', 'assets', 'sp', 'ninja-adventure',
                   'background-elements', 'tileset.png')
PAL = [(255, 255, 255), (170, 170, 170), (85, 85, 85), (0, 0, 0)]
LV = {'.': None, 'w': 0, 'l': 1, 'd': 2, 'b': 3}


def ascii_art(rows):
    return [[LV[c] for c in r] for r in rows]


# ---------------------------------------------------------------- procedural tiles
def tile(fn):
    return [[fn(x, y) for x in range(16)] for y in range(16)]


def floor_wood(x, y):                    # planks 4 px high, staggered joints
    if y % 4 == 3:
        return 2
    if x == (y // 4 * 5 + 3) % 16:
        return 2
    return 0 if (y % 4 == 0 and x % 7 == 1) else 1


def floor_stone(x, y):                   # 8x8 flagstones, offset every other row
    xo = (x + (4 if (y // 8) % 2 else 0)) % 8
    if y % 8 == 7 or xo == 7:
        return 1
    return 0


def floor_dungeon(x, y):                 # rough slabs, dark joints, a few specks
    xo = (x + (5 if (y // 8) % 2 else 0)) % 8
    if y % 8 == 7 or xo == 7:
        return 2
    return 0 if (x * 7 + y * 3) % 23 == 0 else 1


def floor_tiles(x, y):                  # 16x16 light grey slabs, white top-left bevel
    if x == 15 or y == 15:
        return 2
    if x == 0 or y == 0:
        return 0
    return 1


def floor_planks(x, y):                  # light planks 8 px high, dark seams, staggered ends
    if y % 8 == 7:
        return 2
    if x == (0 if (y // 8) % 2 else 8) and True:
        return 2
    return 0 if y % 8 == 0 else 1


def cobble(x, y):                        # courtyard cobbles: small light stones, dark joints
    xo = (x + (3 if (y // 6) % 2 else 0)) % 6
    yo = y % 6
    if yo == 5 or xo == 5:
        return 2
    return 0 if (xo == 0 and yo == 0) else 1


def roof(x, y):                          # keep roof / battlement top: dark stone courses
    if y % 8 == 0 or (x + (4 if (y // 8) % 2 else 0)) % 8 == 0:
        return 2
    return 3


def grass(x, y):                         # short grass: light grey with dark blades
    return 2 if (x * 5 + y * 3) % 11 == 0 or (x * 3 + y * 7) % 13 == 0 else 1


def bricks(x, y, light):                 # wall face: bricks 8x4, mortar, top highlight
    if y >= 14:
        return 3                         # base shadow
    xo = (x + (4 if (y // 4) % 2 else 0)) % 8
    if y % 4 == 3 or xo == 7:
        return 3
    if y % 4 == 0:
        return 1 if light else 2
    return 2 if not light else (1 if (x + y) % 5 else 2)


STYLES = {
    'court': {'floor': tile(cobble), 'face': tile(lambda x, y: bricks(x, y, True)), 'top': tile(roof), 'rim': 3},
    'slabs': {'floor': tile(floor_tiles), 'face': tile(lambda x, y: bricks(x, y, False)), 'top': 3, 'rim': 2},
    'planks': {'floor': tile(floor_planks), 'face': tile(lambda x, y: bricks(x, y, False)), 'top': 3, 'rim': 2},
    # name: floor, wall face, wall top level, rim level
    'wood':  {'floor': tile(floor_wood), 'face': tile(lambda x, y: bricks(x, y, False)), 'top': 3, 'rim': 2},
    'stone': {'floor': tile(floor_stone), 'face': tile(lambda x, y: bricks(x, y, True)), 'top': 2, 'rim': 1},
    'dungeon': {'floor': tile(floor_dungeon), 'face': tile(lambda x, y: bricks(x, y, False)), 'top': 3, 'rim': 2},
}


# ---------------------------------------------------------------- CC0 objects
_cc0 = None


def cc0(x, y, w, h, black=0.12, cut=(0.40, 0.75)):
    """Crop (pixels) of the CC0 tileset -> levels by per-object luminance quantiles: the darkest
    `black` share -> black, then dark grey / light grey / white split at the `cut` quantiles."""
    global _cc0
    if _cc0 is None:
        _cc0 = Image.open(CC0).convert('RGBA')
    im = _cc0.crop((x, y, x + w, y + h))
    px = [[im.getpixel((i, j)) for i in range(w)] for j in range(h)]
    lum = sorted(0.3 * p[0] + 0.59 * p[1] + 0.11 * p[2] for r in px for p in r if p[3] >= 128)
    q = lambda f: lum[min(len(lum) - 1, int(f * len(lum)))]
    t0, t1, t2 = q(black), q(cut[0]), q(cut[1])
    out = []
    for r in px:
        row = []
        for p in r:
            if p[3] < 128:
                row.append(None)
                continue
            L = 0.3 * p[0] + 0.59 * p[1] + 0.11 * p[2]
            row.append(3 if L <= t0 else 2 if L <= t1 else 1 if L <= t2 else 0)
        out.append(row)
    return out


BED = ascii_art([                        # 32x32 (2x2 cells), head at the top
    "..bbbbbbbbbbbbbbbbbbbbbbbbbbbb..",
    ".bddddddddddddddddddddddddddddb.",
    ".bdllllllllllllllllllllllllllldb",
    ".bdlbbbbbbbbbbbbbbbbbbbbbbbbbldb",
    ".bdlbwwwwwwwwwwwbbwwwwwwwwwwbldb",
    ".bdlbwwwwwwwwwwwbbwwwwwwwwwwbldb",
    ".bdlbwwwwwwwwwwlbblwwwwwwwwlbldb",
    ".bdlbllllllllllbbbbllllllllbldb.",
    ".bdlbbbbbbbbbbbbbbbbbbbbbbbbbldb",
    ".bdlbdddddddddddddddddddddddbldb",
    ".bdlbdllllllllllllllllllllldbldb",
    ".bdlbdlwwwwwwwwwwwwwwwwwwwldbldb",
    ".bdlbdllllllllllllllllllllldbldb",
    ".bdlbdddddddddddddddddddddddbldb",
    ".bdlbdllllllllllllllllllllldbldb",
    ".bdlbdlwwwwwwwwwwwwwwwwwwwldbldb",
    ".bdlbdllllllllllllllllllllldbldb",
    ".bdlbdddddddddddddddddddddddbldb",
    ".bdlbdllllllllllllllllllllldbldb",
    ".bdlbdlwwwwwwwwwwwwwwwwwwwldbldb",
    ".bdlbdllllllllllllllllllllldbldb",
    ".bdlbdddddddddddddddddddddddbldb",
    ".bdlbbbbbbbbbbbbbbbbbbbbbbbbbldb",
    ".bdllllllllllllllllllllllllllldb",
    ".bddddddddddddddddddddddddddddb.",
    ".bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb.",
    ".bdb........................bdb.",
    ".bbb........................bbb.",
    "................................",
    "................................",
    "................................",
    "................................",
])

STAIRS_UP = ascii_art([                  # 16x16 in the top wall: steps going up, dark at the top
    "bbbbbbbbbbbbbbbb",
    "bddddddddddddddb",
    "bbbbbbbbbbbbbbbb",
    "bddddddddddddddb",
    "bllllllllllllllb",
    "bbbbbbbbbbbbbbbb",
    "bllllllllllllllb",
    "bllllllllllllllb",
    "bbbbbbbbbbbbbbbb",
    "bwwwwwwwwwwwwwwb",
    "bllllllllllllllb",
    "bbbbbbbbbbbbbbbb",
    "bwwwwwwwwwwwwwwb",
    "bwwwwwwwwwwwwwwb",
    "bllllllllllllllb",
    "bbbbbbbbbbbbbbbb",
])

DOOR = ascii_art([                       # 16x16 closed wooden door in a wall face
    "..bbbbbbbbbbbb..",
    ".bddddddddddddb.",
    "bddlllllllllldb.",
    "bdlddddbddddldb.",
    "bdldlldbdlldldb.",
    "bdldlldbdlldldb.",
    "bdldlldbdlldldb.",
    "bdldlldbdlldldb.",
    "bdldlldbdlwwldb.",
    "bdldlldbdlwbldb.",
    "bdldlldbdlldldb.",
    "bdldlldbdlldldb.",
    "bdldlldbdlldldb.",
    "bdlddddbddddldb.",
    "bddddddddddddddb",
    "bbbbbbbbbbbbbbbb",
])


DESK = ascii_art([                       # 32x32: chair behind, desk top, drawer, legs
    "..........bbbbbbbb..............",
    ".........bddddddddb.............",
    ".........bdllllllddb............",
    ".........bdlwwwwlldb............",
    ".........bdllllllldb............",
    ".........bddddddddb.............",
    "..........bdb..bdb..............",
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
    "bwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwb",
    "bwllllllllllllllllllllllllllllwb",
    "bwllllllllllllllllbbbbbbblllllwb",
    "bwllllllllllllllllbwwwwwblllllwb",
    "bwllllllllllllllllbwwwwwblllllwb",
    "bwllllllllllllllllbbbbbbblllllwb",
    "bllllllllllllllllllllllllllllllb",
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
    "bddddddddddddddddddddddddddddddb",
    "bdlllllllllldbbdddddddddddddddb.",
    "bdlddddddddldbbd..............b.",
    "bdlddbbbdddldbbd..............b.",
    "bdlddddddddldbbd..............b.",
    "bdlllllllllldbbd..............b.",
    "bddddddddddddbbd..............b.",
    "bbbbbbbbbbbbbbbb..............b.",
    "bdb..........bdb..........bdb...",
    "bbb..........bbb..........bbb...",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
])


SHIELD = ascii_art([                     # 16x16 wall decoration: crest shield
    "................",
    "...bbbbbbbbbb...",
    "..bwwwwbbllllb..",
    "..bwwwwbbllllb..",
    "..bwwwwbbllllb..",
    "..bbbbbbbbbbbb..",
    "..bllllbbwwwwb..",
    "..bllllbbwwwwb..",
    "...bllllbwwwb...",
    "...blllbbwwwb...",
    "....bllbbwwb....",
    ".....blbbwb.....",
    "......bbbb......",
    ".......bb.......",
    "................",
    "................",
])

SWORDS = ascii_art([                     # 16x16 wall decoration: crossed swords
    "bb............bb",
    "bwb..........bwb",
    ".bwb........bwb.",
    "..bwb......bwb..",
    "...bwb....bwb...",
    "....bwb..bwb....",
    ".....bwbbwb.....",
    "......bwwb......",
    "......bwwb......",
    ".....bwbbwb.....",
    "...bbbb..bbbb...",
    "...bdb....bdb...",
    "..bddb....bddb..",
    ".bdbb......bbdb.",
    ".bb..........bb.",
    "................",
])

WINDOW = ascii_art([                     # 16x16 arched window in the wall face
    "................",
    ".....bbbbbb.....",
    "...bbwwwwwwbb...",
    "..bwwwwbbwwwwb..",
    "..bwlwwbbwwlwb..",
    "..bwwwwbbwwwwb..",
    "..bbbbbbbbbbbb..",
    "..bwwwwbbwwwwb..",
    "..bwwlwbbwlwwb..",
    "..bwwwwbbwwwwb..",
    "..bwwwwbbwwwwb..",
    "..bbbbbbbbbbbb..",
    ".bllllllllllllb.",
    ".bbbbbbbbbbbbbb.",
    "................",
    "................",
])

TORCH = ascii_art([                      # 16x16 wall torch (flame white/light)
    "......bb........",
    ".....bwwb.......",
    "....bwwlwb......",
    "....bwllwb......",
    ".....blwb.......",
    "....bbbbbb......",
    "....bddddb......",
    ".....bddb.......",
    ".....bddb.......",
    ".....bddb.......",
    "....bbddbb......",
    "....bdbbdb......",
    "....bb..bb......",
    "................",
    "................",
    "................",
])


PILLAR = ascii_art(                      # 16x72: capital, fluted shaft, base (stands on 4 cells)
    ["bbbbbbbbbbbbbbbb", "bwwwwwwwwwwwwwwb", "bllllllllllllllb", "bbbbbbbbbbbbbbbb",
     ".bwllbwllbwlldb.", ".bwllbwllbwlldb."] +
    [".bwllbwllbwlldb."] * 56 +
    ["bbbbbbbbbbbbbbbb", "bwwwwwwwwwwwwwwb", "bllllllllllllllb", "bddddddddddddddb", "bbbbbbbbbbbbbbbb",
     "................", "................", "................", "................", "................"])

THRONE = ascii_art([                     # 32x32 throne against the back wall
    "..........bbbbbbbbbbbb..........",
    ".........bwwwwwwwwwwwwb.........",
    "........bwllllllllllllwb........",
    "........bwlbbbbbbbbbblwb........",
    "........bwlbddddddddblwb........",
    "........bwlbdddwwdddblwb........",
    "........bwlbddwwwwddblwb........",
    "........bwlbdddwwdddblwb........",
    "........bwlbddddddddblwb........",
    "........bwlbddddddddblwb........",
    "........bwlbddddddddblwb........",
    "......bbbwlbddddddddblwbbb......",
    ".....bwwbwlbddddddddblwbwwb.....",
    ".....bwlbwlbddddddddblwblwb.....",
    ".....bwlbbbbbbbbbbbbbbbbblwb....."[:32],
    ".....bwlbwwwwwwwwwwwwwwwblwb....."[:32],
    ".....bwlbllllllllllllllblwb......"[:32],
    ".....bbbbbbbbbbbbbbbbbbbbbbb....."[:32],
    ".....bllbddddddddddddddbllb......"[:32],
    ".....bllbddddddddddddddbllb......"[:32],
    ".....bbbbbbbbbbbbbbbbbbbbbb......"[:32],
    "....bwwwwwwwwwwwwwwwwwwwwwwb....",
    "....bllllllllllllllllllllllb....",
    "....bbbbbbbbbbbbbbbbbbbbbbbb....",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
])

BANNER = ascii_art([                     # 16x32 hanging banner with the Strife crest
    "bbbbbbbbbbbbbbbb",
    "bwwwwwwwwwwwwwwb",
    ".bbbbbbbbbbbbbb.",
    ".bddddddddddddb.",
    ".bdlllllllllldb.",
    ".bdlddddddddldb.",
    ".bdldddwwdddldb.",
    ".bdlddwwwwddldb.",
    ".bdldwwbbwwdldb.",
    ".bdlddwwwwddldb.",
    ".bdldddwwdddldb.",
    ".bdlddddddddldb.",
    ".bdlllllllllldb.",
    ".bddddddddddddb.",
    ".bddddddddddddb.",
    ".bdddddbbddddddb"[:16],
    ".bddddb..bdddddb"[:16],
    ".bdddb....bdddb.",
    ".bddb......bddb.",
    ".bdb........bdb.",
    ".bb..........bb.",
] + ["................"] * 11)


def runner(w, h):                        # royal carpet runner: dark with a light border
    W, H = w * 16, h * 16
    g = []
    for y in range(H):
        r = []
        for x in range(W):
            e = min(x, W - 1 - x)
            if e == 0:
                r.append(3)
            elif e < 3:
                r.append(1)
            elif e == 3:
                r.append(3)
            else:
                r.append(0 if (x + y) % 16 == 0 or (W - x + y) % 16 == 0 else 2)
        g.append(r)
    return g


GATE = ascii_art([                       # 16x16 castle gate: dark arch, portcullis
    "....bbbbbbbb....",
    "..bbddddddddbb..",
    ".bddbbbbbbbbddb.",
    ".bdbbdbbdbbdbdb.",
    "bdbbbdbbdbbdbbdb",
    "bdbdbbbbbbbbbdbd"[:16],
    "bdbbbdbbdbbdbbdb",
    "bdbbbdbbdbbdbbdb",
    "bdbdbbbbbbbbbdbb",
    "bdbbbdbbdbbdbbdb",
    "bdbbbdbbdbbdbbdb",
    "bdbdbbbbbbbbbdbb",
    "bdbbbdbbdbbdbbdb",
    "bdbbbdbbdbbdbbdb",
    "bdbbbbbbbbbbbbdb",
    "bbbbbbbbbbbbbbbb",
])


def carpet(w, h):                        # w x h cells, dark border, light diamond pattern
    W, H = w * 16, h * 16
    g = []
    for y in range(H):
        r = []
        for x in range(W):
            e = min(x, y, W - 1 - x, H - 1 - y)
            if e == 0:
                r.append(3)
            elif e < 3:
                r.append(2)
            elif e == 3:
                r.append(3)
            else:
                r.append(0 if (abs((x - W // 2)) + abs((y - H // 2))) % 8 == 0 else 1)
        g.append(r)
    return g


# ---------------------------------------------------------------- layouts
# (object, cell x, cell y, pixel dx, pixel dy); cells are the logic grid of tools/rooms.py.
def OBJ():
    return {
        'bed': BED,
        'bookcase': cc0(160, 592, 32, 32),
        'shelf': cc0(192, 592, 16, 32),
        'drawers': cc0(64, 592, 32, 32),
        'dresser': cc0(48, 592, 16, 32),
        'wardrobe': cc0(96, 592, 16, 32),
        'desk': DESK,
        'pot': cc0(0, 608, 16, 16),
        'jar': cc0(16, 608, 16, 16),
        'table': cc0(208, 624, 48, 16),
        'chair': cc0(24, 592, 8, 16),
        'stairs': STAIRS_UP,
        'door': DOOR,
        'carpet3x2': carpet(3, 2),
        'shield': SHIELD, 'swords': SWORDS, 'window': WINDOW, 'torch': TORCH,
        'gate': GATE, 'tree': cc0(0, 160, 32, 32), 'pine': cc0(96, 160, 32, 32), 'bush': cc0(0, 336, 16, 16),
        'pillar': PILLAR, 'throne': THRONE, 'banner': BANNER, 'runner5x7': runner(5, 7),
    }


LAYOUTS = {
    5: {'style': 'court', 'fill': [(1, 1, 6, 2, 'grass'), (12, 1, 6, 2, 'grass'), (16, 7, 1, 1, 'grass')],
        'objects': [
        ('tree', 1, 1, -4, -10), ('pine', 3, 1, -2, -8), ('tree', 5, 1, 0, -10),
        ('tree', 12, 1, 0, -10), ('pine', 14, 1, 0, -8), ('tree', 16, 1, -2, -10),
        ('tree', 1, 2, -8, -2), ('tree', 15, 2, 8, -2),
        ('door', 3, 6, 0, 0), ('gate', 9, 6, 0, 0), ('door', 15, 6, 0, 0),
        ('banner', 8, 4, 0, 4), ('banner', 10, 4, 0, 4), ('torch', 7, 6, 4, 0), ('torch', 11, 6, 0, 0),
        ('window', 2, 4, 0, 4), ('window', 16, 4, 0, 4)]},
    6: {'style': 'slabs', 'floor': [(4, 4, 1, 4), (14, 4, 1, 4)], 'objects': [
        ('runner5x7', 7, 3, 0, 0), ('throne', 8, 1, 8, 8), ('banner', 5, 1, 0, 2), ('banner', 12, 1, 0, 2),
        ('torch', 3, 2, 0, 0), ('torch', 7, 2, 4, 0), ('torch', 11, 2, 8, 0), ('window', 15, 1, 0, 4),
        ('stairs', 14, 2, 0, 0), ('pillar', 4, 4, 0, -12), ('pillar', 14, 4, 0, -12)]},
    8: {'style': 'slabs', 'floor': [(2, 2, 2, 2), (15, 2, 2, 2), (2, 6, 2, 2), (11, 6, 2, 2)], 'objects': [
        ('carpet3x2', 5, 4, 0, 0), ('swords', 3, 0, 0, 6), ('torch', 4, 1, 6, 0), ('torch', 6, 1, 4, 0),
        ('shield', 7, 0, 0, 6), ('window', 13, 0, 0, 4), ('shield', 16, 0, 0, 6), ('swords', 12, 0, 0, 6), ('bed', 2, 2, 0, 0), ('stairs', 5, 1, 0, 0),
        ('bookcase', 8, 1, 0, 8), ('door', 10, 5, 0, 0), ('drawers', 11, 2, -8, -8),
        ('bed', 15, 2, 0, 0), ('desk', 2, 6, 0, 0), ('desk', 11, 6, 0, 0),
        ('jar', 7, 7, 0, 0), ('pot', 14, 7, 0, 0)]},
}


# ---------------------------------------------------------------- rendering
def blit(dst, src, X, Y):
    for j, r in enumerate(src):
        for i, v in enumerate(r):
            if v is not None and 0 <= Y + j < len(dst) and 0 <= X + i < len(dst[0]):
                dst[Y + j][X + i] = v


class View:
    """Visual wall map of a room: logic walls and doors are walls, triggers are floor with an
    object on them, the layout's 'floor' rectangles (furniture standing on walls) are floor."""

    def __init__(self, room, layout):
        self.w, self.h = room.w, room.h
        self.wall = [[room.grid[y][x] == 0 or 2 < room.grid[y][x] < 500 for x in range(room.w)]
                     for y in range(room.h)]
        for (x, y, w, h) in layout.get('floor', []):
            for j in range(h):
                for i in range(w):
                    self.wall[y + j][x + i] = False

    def kind(self, x, y):
        return 'wall' if self.wall[y][x] else 'floor'


def render(room, style, layout, px=0, py=0):
    """room: View-like with .w .h and kind(x, y) -> 'wall' | 'floor'; returns levels."""
    S = STYLES[style]
    W, H = room.w * 16, room.h * 16
    g = [[3] * W for _ in range(H)]
    wall = lambda x, y: x < 0 or y < 0 or x >= room.w or y >= room.h or room.kind(x, y) == 'wall'
    for y in range(room.h):
        for x in range(room.w):
            X, Y = x * 16, y * 16
            if not wall(x, y):
                blit(g, S['floor'], X, Y)
            elif not wall(x, y + 1):
                blit(g, S['face'], X, Y)
            elif y + 2 <= room.h and not wall(x, y + 2) and wall(x - 1, y + 1) and wall(x + 1, y + 1):
                blit(g, S['face'], X, Y)             # upper face: walls two tiles tall
                for i in range(16):                  # cornice
                    g[Y][X + i] = 3
                    g[Y + 1][X + i] = 1
                    g[Y + 2][X + i] = 3
                for j in range(14, 16):
                    for i in range(16):
                        g[Y + j][X + i] = 2 if (x * 16 + i + j) % 8 else 3
            else:
                t = S['top']
                if isinstance(t, list):
                    blit(g, t, X, Y)
                else:
                    for j in range(16):
                        for i in range(16):
                            g[Y + j][X + i] = t
                # rim where the top meets floor (left / right / top side) or a face
                for (dx, dy) in ((-1, 0), (1, 0), (0, -1)):
                    if not wall(x + dx, y + dy):
                        for k in range(16):
                            i, j = (0 if dx < 0 else 15, k) if dx else (k, 0)
                            g[Y + j][X + i] = S['rim']
                            i2, j2 = (1 if dx < 0 else 14, k) if dx else (k, 1)
                            g[Y + j2][X + i2] = 3
    return g


FILLS = {'grass': tile(grass), 'cobble': tile(cobble), 'roof': tile(roof)}


def render_layout(room, lay, px=0, py=0):
    g = render(room, lay['style'], [], px, py)
    for (x, y, w, h, name) in lay.get('fill', []):
        for j in range(h):
            for i in range(w):
                blit(g, FILLS[name], (x + i + px) * 16, (y + j + py) * 16)
    objs = OBJ()
    for (name, cx, cy, dx, dy) in lay['objects']:
        blit(g, objs[name], (cx + px) * 16 + dx, (cy + py) * 16 + dy)
    return g


def _unused(g, layout, px, py):
    objs = OBJ()
    for (name, cx, cy, dx, dy) in layout:
        blit(g, objs[name], (cx + px) * 16 + dx, (cy + py) * 16 + dy)
    return g


def to_image(g, scale=1):
    im = Image.new('RGB', (len(g[0]), len(g)))
    for y, r in enumerate(g):
        for x, v in enumerate(r):
            im.putpixel((x, y), PAL[v if v is not None else 0])
    return im.resize((im.width * scale, im.height * scale), Image.NEAREST) if scale > 1 else im


def tiles_of(g, w, h):
    """Cut a level grid into 16x16 tiles in ExtGraph TileMap order (dark, light) row pairs,
    deduplicated. Returns (tiles, map)."""
    tiles, tmap = [], []
    for ty in range(h):
        for tx in range(w):
            rows = []
            for y in range(16):
                d = l = 0
                for x in range(16):
                    v = g[ty * 16 + y][tx * 16 + x] or 0
                    if v & 2:
                        d |= 0x8000 >> x
                    if v & 1:
                        l |= 0x8000 >> x
                rows.append((d, l))
            key = tuple(rows)
            if key not in tiles:
                tiles.append(key)
            tmap.append(tiles.index(key))
    return tiles, tmap
