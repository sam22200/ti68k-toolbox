#!/usr/bin/env python3
"""FFA sprites: ASCII art -> gfx.h (ExtGraph layout: light, dark, mask; mask bit 1 = transparent).
Art chars: '.' transparent, 'w' white, 'l' light grey, 'd' dark grey, 'b' black.
Main sprites get a 1-pixel white outline (shape dilated in 8 directions) for visibility.
Run: python3 tools/gfx.py > gfx.h   (from games/ffa)"""

LEVEL = {'w': 0, 'l': 1, 'd': 2, 'b': 3}

HEAD_DOWN = [                    # 14 wide; hero art is 14x21 inside a 16x24 outlined sprite
    "....bbbbbb....",
    "...bddddddb...",
    "..bddddddddb..",
    ".bdddlddlddddb",
    ".bddlllllldddb",
    ".bdlwwllwwldb.",
    ".bdlbwllwbldb.",
    "..blllllllb...",
    "...bllllbbb...",
]
HEAD_UP = [
    "....bbbbbb....",
    "...bddddddb...",
    "..bddddddddb..",
    ".bddddddddddb.",
    ".bddddddddddb.",
    ".bddddddddddb.",
    ".bdddddddddb..",
    "..bddddddddb..",
    "...bbddddbb...",
]
BODY_DOWN = [
    "..bbbdddbbb...",
    ".bwwbddddbwwb.",
    "bwwlbdwwdblwwb",
    "bwlbddwwddblwb",
    "blbbdddddddbbl",
    "bllbddddddbllb",
    ".bbbbbbbbbbbb.",
]
BODY_UP = [
    "..bbbdddbbb...",
    ".bwwbddddbwwb.",
    "bwwlbddddblwwb",
    "bwlbdddddddlwb",
    "blbbdddddddbbl",
    "bllbddddddbllb",
    ".bbbbbbbbbbbb.",
]
LEGS_FRONT = [                   # stand, left step, right step
    ["..bdddbbdddb..", "..bddb..bddb..", "..bddb..bddb..", "..bbbb..bbbb..", ".bbbbb..bbbbb."],
    ["..bdddbbdddb..", "..bddb..bddb..", "..bddb..bbbb..", "..bbbb..bbbbb.", ".bbbbb........"],
    ["..bdddbbdddb..", "..bddb..bddb..", "..bbbb..bddb..", ".bbbbb..bbbb..", "........bbbbb."],
]
SIDE = [                         # facing right: head and body
    "....bbbbbb....",
    "...bddddddb...",
    "..bdddddddbb..",
    ".bddddddddddb.",
    ".bdddddddlwwb.",
    ".bddddddlwbwb.",
    "..bdddddlwwwwb",
    "..bdddddlwwwb.",
    "...bdddlllbb..",
    "....bbbdbb....",
    "...bwwbddb....",
    "...bwlbdddb...",
    "...blbddddb...",
    "...bbdddddb...",
    "...bdddddddb..",
    "...bbbbbbbbb..",
]
LEGS_SIDE = [
    ["....bdddb.....", "....bddb......", "....bddb......", "....bbbb......", "....bbbbb....."],
    ["...bddddb.....", "..bddbbddb....", ".bddb..bddb...", ".bbbb..bbbb...", "bbbbb..bbbbb.."],
    ["....bdddb.....", "....bdddb.....", "....bddbb.....", "....bbbbb.....", "....bbbbbb...."],
]

# NPCs: (head down, head up, body down, body up, legs [stand, step A, step B]); 14 columns each
KING = (
    ["...b..bb..b...", "...bwbwwbwb...", "..bwwwwwwwwb..", "..bbbbbbbbbb..", ".bllwwwwwwllb.",
     ".blwbwwwwbwlb.", ".bllwwwwwwllb.", "..blwwbbwwlb..", "...bwwwwwwb..."],
    ["...b..bb..b...", "...bwbwwbwb...", "..bwwwwwwwwb..", "..bbbbbbbbbb..", ".bllllllllllb.",
     ".bllllllllllb.", ".bllllllllllb.", "..blllllllb...", "...blllllb...."],
    ["..bbbwwwwbbb..", ".bddbwllwbddb.", "bdddbwllwbdddb", "bwddbwllwbddwb", "bwddbwllwbddwb",
     "bdddbwllwbdddb", "bdddbwllwbdddb"],
    ["..bbbddddbbb..", ".bddddddddddb.", "bddddddddddddb", "bwddddddddddwb", "bwddddddddddwb",
     "bddddddddddddb", "bddddddddddddb"],
    [["bdddbwllwbdddb", "bdddbwllwbdddb", ".bddbwllwbddb.", ".bbbbbbbbbbbb.", "..bb......bb.."],
     ["bdddbwllwbdddb", "bdddbwllwbdddb", ".bddbwllwbddb.", ".bbbbbbbbbbbb.", "..bb.........."],
     ["bdddbwllwbdddb", "bdddbwllwbdddb", ".bddbwllwbddb.", ".bbbbbbbbbbbb.", "..........bb.."]],
)
# Parts for the other NPCs: heads (down view; the up view fills the face with the hair level),
# bodies, legs. 14 columns each.
HEADS = {
    'bald_beard': ("l", ["....bbbbbb....", "...bllllllb...", "..bllwwwwllb..", "..blwwwwwwlb..",
                         ".bdlwbwwbwldb.", ".bdlwwwwwwldb.", ".bdllwwwwlldb.", "..bwwwwwwwwb..",
                         "...bwwwwwwb..."]),
    'long_hair': ("d", ["....bbbbbb....", "...bddddddb...", "..bddddddddb..", ".bddwwwwwwddb.",
                        ".bdwbwwwwbwdb.", ".bdwwwwwwwwdb.", ".bddwwbbwwddb.", ".bdd.bwwb.ddb.",
                        ".bdd..bb..ddb."]),
    'light_hair': ("l", ["....bbbbbb....", "...bllllllb...", "..bllllllllb..", ".bllldlldllllb",
                         ".blldwwwwwdllb", ".bldwwwwwwdlb.", ".bldbwwwwbdlb.", "..bdwwwwwwdb..",
                         "...bwwbbwwb..."]),
    'helmet': ("l", ["....bbbbbb....", "...bllllllb...", "..bllwllllb...", "..bbbbbbbbbb..",
                     "..bdwwwwwwdb..", "..bdbwwwwbdb..", "..bdwwwwwwdb..", "...bwwbbwwb...",
                     "....bwwwwb...."]),
    'cap': ("d", ["..............", "....bbbbbb....", "...bddddddb...", "..bbbbbbbbbb..",
                  "..bwwwwwwwwb..", "..bwbwwwwbwb..", "..bwwwwwwwwb..", "...bwwbbwwb...",
                  "....bwwwwb...."]),
    'wild': ("d", ["...bb.bb.bb...", "..bddbddbddb..", ".bddddddddddb.", ".bddwwwwwwddb.",
                   ".bdwbwwwwbwdb.", ".bdwwwwwwwwdb.", ".bddwbbbbwddb.", "..bdwwwwwwdb..",
                   "...bdddddb...."]),
}
BODIES = {
    'robe': ["..bbbddddbbb..", ".bddbddddbddb.", "bdddbllllbdddb", "bwddbddddbddwb", "bwddbddddbddwb",
             "bdddbddddbdddb", "bdddbddddbdddb"],
    'dress': ["..bbbllllbbb..", ".bllbwwwwbllb.", "blllbwwwwblllb", "bwllbwwwwbllwb", "bwlllwwwwlllwb",
              "blllllwwlllllb", "bllllllllllllb"],
    'armor': ["..bbbbbbbbbb..", ".blbllllllblb.", "bllbldlldlbllb", "bwlbllllllblwb", "bwbbbbbbbbbbwb",
              "bllbldlldlbllb", ".bbbllllllbbb."],
    'tunic': ["..bbbwwwwbbb..", ".bwwbwwwwbwwb.", "bwwlbwwwwblwwb", "bwlbwwwwwwblwb", "blbbbbbbbbbbbl",
              "bllbddddddbllb", ".bbbbbbbbbbbb."],
    'rags': ["..bbbddddbbb..", ".bddbdlldbddb.", "bddlbldddblddb", "bwdbddldddbdwb", "blbbdddddddbbl",
             "bllbdldddldbllb"[:14], ".bbbbbbbbbbbb."],
}
HEM = [["bdddddddddddddb"[:14], "bddddddddddddb", ".bddddddddddb.", ".bbbbbbbbbbbb.", "..bb......bb.."],
       ["bddddddddddddb", "bddddddddddddb", ".bddddddddddb.", ".bbbbbbbbbbbb.", "..bb.........."],
       ["bddddddddddddb", "bddddddddddddb", ".bddddddddddb.", ".bbbbbbbbbbbb.", "..........bb.."]]
HEM_L = [[r.replace('d', 'l') for r in f] for f in HEM]


def head_up(hair, rows):
    out = []
    for y, r in enumerate(rows):
        out.append(r if y < 2 else ''.join(hair if c in 'wlbd' and 0 < x < 13 and r[x - 1:x + 2].count('.') == 0 else c
                                            for x, c in enumerate(r)))
    return out


def person(head, body, legs):
    hair, hd = HEADS[head]
    b = BODIES[body]
    return (hd, head_up(hair, hd), b, b, legs)


NPCS = [('KING', KING),
        ('OLEN', person('bald_beard', 'robe', HEM)),
        ('JESS', person('long_hair', 'dress', HEM_L)),
        ('LARC', person('light_hair', 'armor', LEGS_FRONT)),
        ('VILLAGER', person('cap', 'tunic', LEGS_FRONT)),
        ('SOLDIER', person('helmet', 'armor', LEGS_FRONT)),
        ('SELLER', person('cap', 'robe', HEM)),
        ('PRISONER', person('wild', 'rags', LEGS_FRONT))]



def grid(rows, w, h, ox=0, oy=0):
    g = [[None] * w for _ in range(h)]
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c != '.':
                g[oy + y][ox + x] = LEVEL[c]
    return g


def outlined(g, level=0):
    h, w = len(g), len(g[0])
    out = [r[:] for r in g]
    for y in range(h):
        for x in range(w):
            if g[y][x] is None and any(0 <= y + j < h and 0 <= x + i < w and g[y + j][x + i] is not None
                                       for j in (-1, 0, 1) for i in (-1, 0, 1)):
                out[y][x] = level
    return out


def mirror(g):
    return [r[::-1] for r in g]


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
    for k, part in enumerate(('light', 'dark', 'mask')):
        out.append(f"static const {ctype} {name}_{part}[{len(sprites)}][{h}] = {{")
        for s in sprites:
            out.append("    { " + ", ".join(fmt % v for v in s[k]) + " },")
        out.append("};")


def main():
    out = ["// Generated by tools/gfx.py: do not edit. ExtGraph sprite rows: light, dark, mask (1 = transparent)."]
    frames = []                               # dir (down, up, left, right) x (stand, step A, step B)
    for head, body in ((HEAD_DOWN, BODY_DOWN), (HEAD_UP, BODY_UP)):
        for legs in LEGS_FRONT:
            frames.append(outlined(grid(head + body + legs, 16, 24, 1, 2)))
    side = [outlined(grid(SIDE + legs, 16, 24, 1, 2)) for legs in LEGS_SIDE]
    frames += [mirror(g) for g in side] + side
    out.append("#define HERO_FRAMES %d" % len(frames))
    emit("hero", [planes(g, 16) for g in frames], 16, 24, "u16", out)
    npc = []                                  # per NPC: down x 3, up x 3
    for k, (name, (hd, hu, bd, bu, legs)) in enumerate(NPCS):
        out.append("#define SPR_%s %d" % (name, k))
        for head, body in ((hd, bd), (hu, bu)):
            for l in legs:
                npc.append(outlined(grid(head + body + l, 16, 24, 1, 2)))
    out.append("#define NPC_KINDS %d" % len(NPCS))
    emit("npc", [planes(g, 16) for g in npc], 16, 24, "u16", out)
    import sys
    if '--ids' in sys.argv:                   # sprite ids only (gfx_ids.h, for every source file)
        out = ["// Generated by tools/gfx.py --ids: do not edit."] + [l for l in out if l.startswith('#define')]
    print("\n".join(out))


main()
