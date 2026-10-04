#!/usr/bin/env python3
"""Metal Gear Solid (GBC) -> the TI port's data, from the local ROM run under PyBoy.

Milestone 1 = VR Training, Sneaking, No weapon, Practice, Lv.01. Everything is measured on the
running ROM (no ROM data format decoded yet): the game is driven from power-on to the level by a
fixed key script, then the level's BG map (VRAM, CGB attributes and palettes), its metatile map
and collision (WRAM bank 5, D000: 16 entries of 2 bytes per row, id + solidity nibble), the
start, the goal box (WRAM bank 2, D000/D002), the guard's patrol (run 1,300 frames, Snake out of
sight) and the sprite frames (OAM + VRAM per frame, per direction and animation step).

Output (generated, local: commercial ROM): level.h (map, collision, measured numbers), gfx.h
(offsets into the data file), mgsdat.bin / mgsdat.be.bin (tiles and sprites, PC / TI byte
order), x/*.png (review sheets).
Run: tools/pyenv/bin/python games/mgs/tools/extract.py [ROM]   (from the repository root)
"""
import hashlib
import os
import struct
import sys

from PIL import Image
from pyboy import PyBoy

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.dirname(HERE)
ROM = sys.argv[1] if len(sys.argv) > 1 else os.path.join(GAME, '../../roms/gb/Metal_Gear_Solid_.gbc')
X = os.path.join(GAME, 'x')

# The CGB colours of Lv.01 -> 4 greys (0 white, 1 light, 2 dark, 3 black), chosen for 160x100 in
# 4 greys: floor light with white grid lines, shadows and wall tops dark, outside black.
BG_GREY = {
    (0, 0, 32): 3, (0, 24, 65): 2, (0, 49, 106): 2, (0, 74, 131): 2,              # VR outside
    (49, 16, 0): 2, (65, 24, 0): 2, (82, 41, 0): 2, (98, 57, 0): 2, (98, 49, 0): 2,  # wall tops
    (115, 74, 0): 2, (156, 82, 0): 1, (213, 123, 0): 1, (255, 222, 0): 0,         # wall faces
    (222, 82, 0): 1, (255, 139, 16): 0, (205, 41, 0): 2,                          # floor, grid, shade
}
SPR_GREY = {
    (24, 24, 57): 3, (106, 106, 222): 2, (205, 205, 255): 1,                      # Snake
    (16, 49, 32): 3, (0, 205, 123): 1, (189, 74, 57): 2,                          # guard
    (115, 98, 90): 2,                                                             # shadows
}

pb = PyBoy(ROM, window='null', sound_emulated=False)
pb.set_emulation_speed(0)
m = pb.memory


def w(a):
    return m[a] | m[a + 1] << 8


def setw(a, v):
    m[a] = v & 255
    m[a + 1] = v >> 8


def tick(n=1):
    for _ in range(n):
        pb.tick()


def press(b, hold=4, after=8):
    pb.button_press(b)
    tick(hold)
    pb.button_release(b)
    tick(after)


def snap():
    import io
    f = io.BytesIO()
    pb.save_state(f)
    return f.getvalue()


def load(s):
    import io
    pb.load_state(io.BytesIO(s))


# ---------------------------------------------------------------- power-on -> Lv.01
def boot():
    tick(600)
    for _ in range(3):                      # logos, PRESS START, menu
        press('start')
        tick(120 if _ < 2 else 60)
    for _ in range(3):                      # NEW GAME -> ... -> VR TRAINING
        press('down', 6, 1)
        tick(20)
    for t in (90, 90, 90, 120):             # SNEAKING MODE, NO WEAPON, PRACTICE, Lv.01
        press('a')
        tick(t)
    press('a')
    tick(400)
    press('a')
    tick(60)
    intro = snap()                          # the intro shows the top of the level (SCY 0)
    tick(200)
    for _ in range(4):                      # the briefing texts
        press('a')
        tick(200 if _ < 3 else 100)
    tick(300)                               # Snake appears
    return intro, snap()


# ---------------------------------------------------------------- CGB video
def palettes(reg):
    raw = []
    for i in range(64):
        m[reg] = i
        raw.append(m[reg + 1])
    out = []
    for p in range(8):
        cs = []
        for c in range(4):
            v = raw[p * 8 + c * 2] | raw[p * 8 + c * 2 + 1] << 8
            cs.append(((v & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31, ((v >> 10) & 31) * 255 // 31))
        out.append(cs)
    return out


def vram(bank):
    return bytes(m[bank, 0x8000:0x9fff]) + bytes([m[bank, 0x9fff]])


def grab():
    return dict(v0=vram(0), v1=vram(1), bgp=palettes(0xff68), obp=palettes(0xff6a), lcdc=m[0xff40],
                scx=m[0xff43], scy=m[0xff42], oam=bytes(m[0xfe00:0xfea0]))


def tilepix(v, n, attr):
    rows = []
    for y in range(8):
        yy = 7 - y if attr & 0x40 else y
        lo, hi = v[n * 16 + 2 * yy], v[n * 16 + 2 * yy + 1]
        row = [((lo >> (7 - x)) & 1) | (((hi >> (7 - x)) & 1) << 1) for x in range(8)]
        rows.append(row[::-1] if attr & 0x20 else row)
    return rows


def render_bg(g):
    base = 0x1c00 if g['lcdc'] & 8 else 0x1800
    img = Image.new('RGB', (256, 256))
    px = img.load()
    for my in range(32):
        for mx in range(32):
            t, a = g['v0'][base + my * 32 + mx], g['v1'][base + my * 32 + mx]
            n = t if g['lcdc'] & 0x10 else (256 + t if t < 128 else t)
            for y, row in enumerate(tilepix(g['v1'] if a & 8 else g['v0'], n, a)):
                for x, c in enumerate(row):
                    px[mx * 8 + x, my * 8 + y] = g['bgp'][a & 7][c]
    return img


def render_obj(g, cx, cy, pals, rx=12):
    """The OAM entries of one object (palettes `pals`, near screen point cx, cy = its feet) drawn
    on a 32x32 canvas with the feet at (16, 26)."""
    img = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    px = img.load()
    for n in reversed(range(40)):
        y, x, t, f = g['oam'][4 * n:4 * n + 4]
        sy, sx = y - 16, x - 8
        if (f & 7) not in pals or abs(sx + 4 - cx) > rx or not -30 <= sy - cy <= 8:
            continue
        for yy, row in enumerate(tilepix(g['v1'] if f & 8 else g['v0'], t, f)):
            for xx, c in enumerate(row):
                X, Y = sx - cx + 16 + xx, sy - cy + 26 + yy
                if c and 0 <= X < 32 and 0 <= Y < 32:
                    px[X, Y] = g['obp'][f & 7][c] + (255,)
    return img


# ---------------------------------------------------------------- level
def level(intro, play):
    load(intro)
    tick()
    gi = grab()
    load(play)
    tick()
    gp = grab()
    img = Image.new('RGB', (160, 240))
    img.paste(render_bg(gi).crop((0, 0, 160, 128)), (0, 0))
    img.paste(render_bg(gp).crop((0, gp['scy'], 160, 240)), (0, gp['scy']))
    cells = [[(m[5, 0xd000 + r * 32 + c * 2], m[5, 0xd001 + r * 32 + c * 2]) for c in range(10)]
             for r in range(15)]
    start = (w(0xc5c2) - 256, w(0xc5c4) - 256)
    goal = (m[2, 0xd000] | m[2, 0xd001] << 8, m[2, 0xd002] | m[2, 0xd003] << 8)
    goal = (goal[0] - 256, goal[1] - 256)
    return img, cells, start, goal


def grey(rgb, table, where):
    if rgb not in table:
        sys.exit('no grey for colour %s (%s): add it to the table' % (rgb, where))
    return table[rgb]


def tiles_of(img, cells):
    tiles, index, tmap = [], {}, []
    for r in range(15):
        row = []
        for c in range(10):
            g = tuple(grey(img.getpixel((c * 16 + x, r * 16 + y)), BG_GREY, 'bg')
                      for y in range(16) for x in range(16))
            if g not in index:
                index[g] = len(tiles)
                tiles.append(g)
            row.append(index[g])
        tmap.append(row)
    return tiles, tmap


# ---------------------------------------------------------------- the guard's patrol
def patrol(play):
    load(play)
    setw(0xc5c2, 256 + 20)                  # Snake in the bottom-left corner: never seen
    setw(0xc5c4, 256 + 200)
    gx = w(0xc712) - 256
    log = []
    for f in range(1300):
        tick()
        log.append((w(0xc714) - 256, m[0xc716], m[0xc719]))
    ys = [y for y, _, _ in log]
    # segments of constant (dir, state): their lengths give the waits and turns
    segs = []
    for f, (y, d, s) in enumerate(log):
        if not segs or tuple(segs[-1][1:3]) != (d, s):
            segs.append([f, d, s, y, 0])
        segs[-1][4] += 1
    return gx, min(ys), max(ys), segs


# ---------------------------------------------------------------- sprites
def snake_frames(play):
    frames = {}
    for keys in (['up'], ['down'], ['left'], ['right'], ['up', 'left'], ['up', 'right'],
                 ['down', 'left'], ['down', 'right']):
        for x0, y0 in ((32, 192), (72, 192), (80, 120), (80, 60)):
            load(play)
            setw(0xc5c2, 256 + x0)
            setw(0xc5c4, 256 + y0)
            tick(2)
            for k in keys:
                pb.button_press(k)
            for _ in range(70):
                tick()
                g = grab()
                key = (m[0xc5c6], m[0xc5cb] if m[0xc5c0] else 99)
                if key not in frames:
                    frames[key] = render_obj(g, m[0xc5d8] - g['scx'], m[0xc5d9] - g['scy'], {0, 6})
            for k in keys:
                pb.button_release(k)
            tick()
            g = grab()
            key = (m[0xc5c6], 99)
            if key not in frames:
                frames[key] = render_obj(g, m[0xc5d8] - g['scx'], m[0xc5d9] - g['scy'], {0, 6})
    return frames


def guard_frames(right_state):
    """Every frame of the patrol, in time order per (dir, state); the camera must show the guard."""
    load(right_state)
    setw(0xc5c2, 256 + 140)                 # out of every cone, the camera stays up
    setw(0xc5c4, 256 + 48)
    seq, prev = [], None
    for f in range(1300):
        tick()
        cur = (w(0xc712) - 256, w(0xc714) - 256, m[0xc716], m[0xc719])
        if prev:
            g = grab()
            sy = prev[1] - g['scy']
            if sy - 26 >= 0 and sy + 6 < 128:
                seq.append((prev[2], prev[3], render_obj(g, prev[0] - g['scx'], sy, {1, 6}, 8)))
        prev = cur
    return seq


def right_state(play):
    """The camera at the top of the level (Snake walked up the corridor, then waits there)."""
    load(play)
    setw(0xc5c2, 256 + 72)
    setw(0xc5c4, 256 + 140)
    tick(2)
    # wait for the guard at the top first, then walk up behind it
    for _ in range(2000):
        tick()
        if m[0xc719] == 0 and w(0xc714) - 256 == 32:
            break
    pb.button_press('up')
    for _ in range(200):
        tick()
        if w(0xc5c4) - 256 <= 66 or m[0xc6d0] != 0xff:
            break
    pb.button_release('up')
    tick()
    if m[0xc6d0] != 0xff:
        sys.exit('seen while setting the camera up')
    return snap()


def grey_pixels(img, outline=True):
    """32x32 RGBA (feet at 16, 26) -> {(x, y) relative to the feet: grey}, outlined in white."""
    lvl = {}
    for y in range(32):
        for x in range(32):
            p = img.getpixel((x, y))
            if p[3]:
                lvl[(x - 16, y - 26)] = grey(p[:3], SPR_GREY, 'sprite')
    if outline:
        for (x, y) in list(lvl):
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    lvl.setdefault((x + dx, y + dy), 0)
    return lvl


def window(frames, h):
    """The 16 x h box (relative to the feet) holding most pixels of every frame."""
    best = None
    for oy in range(-32, 8):
        for ox in range(-16, 1):
            n = sum(1 for f in frames for (x, y) in f if not (ox <= x < ox + 16 and oy <= y < oy + h))
            if best is None or n < best[0]:
                best = (n, ox, oy)
    return best


def to_sprite(lvl, ox, oy, h):
    light, dark, mask = [0] * h, [0] * h, [0xffff] * h
    for (x, y), v in lvl.items():
        x, y = x - ox, y - oy
        if 0 <= x < 16 and 0 <= y < h:
            b = 0x8000 >> x
            mask[y] &= ~b & 0xffff
            if v & 1:
                light[y] |= b
            if v & 2:
                dark[y] |= b
    return light, dark, mask


SHADES = [(255, 255, 255), (170, 170, 170), (85, 85, 85), (0, 0, 0)]


def preview(spr):
    light, dark, mask = spr
    h = len(mask)
    im = Image.new('RGB', (16, h), (255, 0, 255))
    for y in range(h):
        for x in range(16):
            b = 0x8000 >> x
            if not mask[y] & b:
                im.putpixel((x, y), SHADES[(1 if light[y] & b else 0) + (2 if dark[y] & b else 0)])
    return im


def sheet(images, cols, path, scale=3):
    if not images:
        return
    wd, ht = images[0].size
    s = Image.new('RGB', ((wd + 2) * cols, (ht + 2) * ((len(images) + cols - 1) // cols)), (255, 0, 255))
    for i, im in enumerate(images):
        s.paste(im, ((i % cols) * (wd + 2), (i // cols) * (ht + 2)))
    s.resize((s.width * scale, s.height * scale), Image.NEAREST).save(path)


def main():
    os.makedirs(X, exist_ok=True)
    intro, play = boot()
    img, cells, start, goal = level(intro, play)
    img.save(os.path.join(X, 'level1.png'))
    tiles, tmap = tiles_of(img, cells)
    gx, gy0, gy1, segs = patrol(play)
    top = right_state(play)

    # Snake: 8 directions x (stand + walk steps 0, 2, 4, 6, 8, 10: the GB's 11-step cycle is
    # shown in pairs) -> 7 images per direction
    sf = snake_frames(play)
    steps = [99, 0, 2, 4, 6, 8, 10]
    snake = []
    for d in range(8):
        for s in steps:
            if (d, s) not in sf:
                sys.exit('snake frame missing: dir %d step %d' % (d, s))
            snake.append(grey_pixels(sf[(d, s)]))
    # the guard: the walk cycles down (dir 4) and up (dir 0), one image for right (2) and the
    # diagonals (1, 3): the first image seen of each
    gseq = guard_frames(top)

    def cycle(d, s):
        hs, ims = [], []
        for dd, ss, im in gseq:
            if (dd, ss) == (d, s):
                h = hashlib.md5(im.tobytes()).hexdigest()
                if hs and h == hs[-1]:
                    continue
                if h in hs:
                    break
                hs.append(h)
                ims.append(im)
        return ims
    walk_down, walk_up = cycle(4, 1), cycle(0, 4)
    still = {}
    for dd, ss, im in gseq:
        still.setdefault(dd, im)
    if not walk_down or not walk_up or any(d not in still for d in (0, 1, 2, 3, 4)):
        sys.exit('guard frames missing: %s' % sorted(still))
    guard_imgs = walk_down + walk_up + [still[d] for d in (0, 1, 2, 3, 4)]
    guard = [grey_pixels(im) for im in guard_imgs]
    H = 28
    sclip, sox, soy = window(snake, H)
    gclip, gox, goy = window(guard, H)
    snake = [to_sprite(f, sox, soy, H) for f in snake]
    guard = [to_sprite(f, gox, goy, H) for f in guard]
    clipped = sclip + gclip
    sheet([preview(s) for s in snake], 7, os.path.join(X, 'snake.png'))
    sheet([preview(s) for s in guard], 8, os.path.join(X, 'guard.png'))
    tile_imgs = []
    for t in tiles:
        im = Image.new('RGB', (16, 16))
        im.putdata([SHADES[v] for v in t])
        tile_imgs.append(im)
    big = Image.new('RGB', (160, 240))
    for r in range(15):
        for c in range(10):
            big.paste(tile_imgs[tmap[r][c]], (c * 16, r * 16))
    big.resize((480, 720), Image.NEAREST).save(os.path.join(X, 'level1_grey.png'))

    # ---- data file: tiles (ExtGraph TileMap order: 16 (dark, light) u16 pairs), then sprites
    words = []
    for t in tiles:
        for y in range(16):
            dk = lt = 0
            for x in range(16):
                v = t[y * 16 + x]
                if v & 2:
                    dk |= 0x8000 >> x
                if v & 1:
                    lt |= 0x8000 >> x
            words += [dk, lt]
    off_snake = len(words) * 2
    for s in snake:
        words += s[0] + s[1] + s[2]
    off_guard = len(words) * 2
    for s in guard:
        words += s[0] + s[1] + s[2]
    for name, fmt in (('mgsdat.bin', '<'), ('mgsdat.be.bin', '>')):
        open(os.path.join(GAME, name), 'wb').write(struct.pack(fmt + '%dH' % len(words), *words))

    with open(os.path.join(GAME, 'gfx.h'), 'w') as f:
        f.write('// Generated by tools/extract.py from the local ROM: do not edit, do not commit.\n')
        f.write('#define GFX_NTILES %d\n#define GFX_SNAKE %d      // byte offsets in mgsdat\n'
                '#define GFX_GUARD %d\n' % (len(tiles), off_snake, off_guard))
        f.write('#define SNAKE_STEPS %d     // per direction: stand, then the walk steps\n' % len(steps))
        f.write('#define GUARD_WALK_DOWN %d\n#define GUARD_WALK_UP %d\n'
                '#define GUARD_STILL %d      // then still images for dirs 0..4\n'
                % (len(walk_down), len(walk_up), len(walk_down) + len(walk_up)))
        f.write('#define GFX_BYTES %d\n' % (len(words) * 2))
        f.write('#define SPR_H %d          // sprites 16 x SPR_H, top-left = feet + (OX, OY)\n' % H)
        f.write('#define SNAKE_OX %d\n#define SNAKE_OY %d\n#define GUARD_OX %d\n#define GUARD_OY %d\n'
                % (sox, soy, gox, goy))

    def nib(a):                             # solidity: bit 0 TL, 1 TR, 2 BL, 3 BR
        return (8 if a & 0x10 else 0) | (4 if a & 0x20 else 0) | (2 if a & 0x40 else 0) | (1 if a & 0x80 else 0)
    with open(os.path.join(GAME, 'level.h'), 'w') as f:
        f.write('// Generated by tools/extract.py from the local ROM: do not edit, do not commit.\n')
        f.write('// VR Sneaking Lv.01: 10 x 15 cells of 16 px (11 wide for the TileMap engine).\n')
        f.write('static const u8 lv1_map[15 * 11] = {\n')
        for r in range(15):
            f.write('    ' + ', '.join('%d' % v for v in tmap[r] + [tmap[r][0]]) + ',\n')
        f.write('};\n// solid 8x8 quadrants per cell: bit 0 TL, 1 TR, 2 BL, 3 BR\n')
        f.write('static const u8 lv1_solid[15 * 10] = {\n')
        for r in range(15):
            f.write('    ' + ', '.join('%d' % nib(a) for _, a in cells[r]) + ',\n')
        f.write('};\n')
        f.write('#define LV1_START_X %d\n#define LV1_START_Y %d\n' % start)
        f.write('#define LV1_GOAL_X %d\n#define LV1_GOAL_Y %d\n' % goal)
        f.write('#define LV1_GUARD_X %d\n#define LV1_GUARD_TOP %d\n#define LV1_GUARD_BOTTOM %d\n' % (gx, gy0, gy1))
    print('tiles %d, snake %d images, guard %d (%d down, %d up), %d outline pixels clipped, %d bytes'
          % (len(tiles), len(snake), len(guard), len(walk_down), len(walk_up), clipped, len(words) * 2))
    print('start', start, 'goal', goal, 'guard x', gx, 'y', gy0, gy1)
    for s in segs:
        print('patrol frame %4d dir %d state %d y %3d for %3d frames' % tuple(s))


main()
