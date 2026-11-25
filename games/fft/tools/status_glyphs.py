#!/usr/bin/env python3
"""One pictogram per status, in the solid style, plus the animated charge star.

Style (proposal B, accepted): the glyph is a 5x5 body with a one-pixel outline of
the opposite value, so it reads on any scenery. A negative status is a black body
with a white halo, a positive one a white body with a black outline, and charging
is the original's five-point star animated through its five fill states, as
EVENT/FRAME.BIN draws it.

Symbols follow the original where it is legible at this size: "..." for Silence,
a heart for Charm, a square inside a square for Stop. Statuses with no symbol
that survives five pixels fall back to the bare triangle, down for a penalty and
up for a boon, so the polarity is never lost.

usage: status_glyphs.py --sheet      a contact sheet of every glyph
       status_glyphs.py --demo       an ally and an enemy cycling their statuses
"""
import argparse
from pathlib import Path

import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parents[1]
SHADE = {(0, 0): 0, (1, 0): 1, (0, 1): 2, (1, 1): 3}
GREY = [216, 152, 88, 24]
VALUE = {'w': (0, 0), 'g': (1, 0), 'd': (0, 1), 'k': (1, 1)}
UNIT_SH, UNIT_W = 26, 16

# name: (polarity, 5x5 shape). '#' body, 'o' a hole in the outline's value.
NEGATIVE = {
    'Poison':    ('...##', '...##', '.##..', '###..', '.##..'),      # two rising bubbles
    'Dark':      ('..#..', '.###.', '#o#o#', '.###.', '..#..'),      # a shut eye
    'Silence':   ('.....', '#.#.#', '#.#.#', '.....', '.....'),      # the original's "..."
    'Sleep':     ('#####', '...#.', '..#..', '.#...', '#####'),      # Z
    'Stop':      ('#####', '#ooo#', '#o#o#', '#ooo#', '#####'),      # the original's square
    'Slow':      ('#####', '.###.', '.###.', '..#..', '..#..'),      # a thick down arrow
    'Confusion': ('.###.', '...#.', '..##.', '.....', '..#..'),      # ?
    'Berserk':   ('#...#', '##.##', '#o#o#'),                        # angry eyes, brows down
    'Petrify':   ('#####', '#####', '##o##', '#####', '#####'),      # a cracked block
    'DontMove':  ('#...#', '.#.#.', '..#..', '.#.#.', '#...#'),      # X
    'DontAct':   ('.....', '#####', '#####', '.....', '.....'),      # a bar
    'Charm':     ('.#.#.', '#####', '#####', '.###.', '..#..'),      # the original's heart
    'Frog':      ('.#.#.', '#####', '#o#o#', '#####', '#####'),      # a frog head, eyes on top
    'Chicken':   ('..#..', '#.#.#', '.###.', '..#..', '..#..'),      # a bird track, leg over it
    'Undead':    ('.###.', '#o#o#', '#####', '#.#.#', '#.#.#'),      # a skull
    'Sentence':  ('#####', '..#..', '..#..', '..#..', '#####'),      # an hourglass frame
}
POSITIVE = {
    'Haste':     ('..#..', '.###.', '#####', '..#..', '..#..'),      # an up arrow
    'Regen':     ('..#..', '..#..', '#####', '..#..', '..#..'),      # a cross
    'Protect':   ('.###.', '#####', '#####', '.###.', '..#..'),      # a shield
    'Shell':     ('..#..', '.###.', '#####', '#####', '#####'),      # a dome
    'Reflect':   ('#...#', '##.##', '#####', '##.##', '#...#'),      # a mirror
    'Float':     ('..#..', '.###.', '#####', '.....', '#####'),      # above a line
    'Faith':     ('..#..', '.###.', '..#..', '..#..', '.###.'),
    'Innocent':  ('.###.', '#ooo#', '.###.'),                        # a halo, a flat ring
    'Transparent': ('#...#', '.....', '#...#', '.....', '#...#'),
    'Defend':    ('#####', '#ooo#', '#####', '.###.', '..#..'),      # a shield with a band
    'Reraise':   ('#...#', '##.##', '#####', '.#.#.'),               # angel wings
}
# The original's five fill states, read off FRAME.BIN's star row.
# The original twinkles its charge star rather than filling it like a gauge: its
# five frames alternate solid and hollow. Both states have to read on any scenery,
# so the twinkle swaps the two polarities instead of emptying one of them -- a
# hollow star drawn in the halo's own value would be white inside a white halo,
# which is invisible.
STAR = ('..#..',
        '#####',
        '.###.',
        '.#.#.',
        '#...#')
CHARGE = [('k', 'w'), ('k', 'w'), ('w', 'k'), ('w', 'k'), ('k', 'w')]


def charge_frame(k):
    body, outline = CHARGE[k % len(CHARGE)]
    return render(STAR, body, outline)


FALLBACK_NEG = ('#####', '#####', ' ### ', ' ### ', '  #  ')
FALLBACK_POS = ('  #  ', ' ### ', ' ### ', '#####', '#####')


def dilate(g):
    h, w = len(g), len(g[0])
    out = [[False] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            if g[y][x]:
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        if 0 <= y + dy < h and 0 <= x + dx < w:
                            out[y + dy][x + dx] = True
    return out


def render(shape, body, outline):
    g = [[c in '#o' for c in row] for row in shape]
    o = [[c == 'o' for c in row] for row in shape]
    h, w = len(g), len(g[0])
    pad = [[False] * (w + 2) for _ in range(h + 2)]
    sym = [[False] * (w + 2) for _ in range(h + 2)]
    for y in range(h):
        for x in range(w):
            pad[y + 1][x + 1] = g[y][x]
            sym[y + 1][x + 1] = o[y][x]
    halo = dilate(pad)
    img = np.zeros((h + 2, w + 2), np.uint8)
    a = np.zeros((h + 2, w + 2), bool)
    for y in range(h + 2):
        for x in range(w + 2):
            if sym[y][x] or (not pad[y][x] and halo[y][x]):
                img[y, x] = SHADE[VALUE[outline]]; a[y, x] = True
            elif pad[y][x]:
                img[y, x] = SHADE[VALUE[body]]; a[y, x] = True
    return img, a


def glyph(name):
    if name in NEGATIVE:
        return render(NEGATIVE[name], 'k', 'w')
    if name in POSITIVE:
        return render(POSITIVE[name], 'w', 'k')
    raise KeyError(name)


def blit(dst, src, alpha, x0, y0):
    for y in range(src.shape[0]):
        for x in range(src.shape[1]):
            if alpha[y, x] and 0 <= y0 + y < dst.shape[0] and 0 <= x0 + x < dst.shape[1]:
                dst[y0 + y, x0 + x] = src[y, x]


FONT = {c: r for c, r in zip(
    'abcdefghijklmnopqrstuvwxyz0123456789 .',
    [('010','101','111','101','101'),('110','101','110','101','110'),('011','100','100','100','011'),
     ('110','101','101','101','110'),('111','100','110','100','111'),('111','100','110','100','100'),
     ('011','100','101','101','011'),('101','101','111','101','101'),('111','010','010','010','111'),
     ('001','001','001','101','010'),('101','110','100','110','101'),('100','100','100','100','111'),
     ('101','111','111','101','101'),('101','111','111','111','101'),('010','101','101','101','010'),
     ('110','101','110','100','100'),('010','101','101','011','001'),('110','101','110','101','101'),
     ('011','100','010','001','110'),('111','010','010','010','010'),('101','101','101','101','011'),
     ('101','101','101','101','010'),('101','101','111','111','101'),('101','101','010','101','101'),
     ('101','101','010','010','010'),('111','001','010','100','111'),('111','101','101','101','111'),
     ('010','110','010','010','111'),('111','001','111','100','111'),('111','001','011','001','111'),
     ('101','101','111','001','001'),('111','100','111','001','111'),('011','100','111','101','111'),
     ('111','001','010','010','010'),('111','101','111','101','111'),('111','101','111','001','110'),
     ('000','000','000','000','000'),('000','000','000','000','010')])}


def text(dst, s, x0, y0, shade=3):
    for ch in s.lower():
        g = FONT.get(ch, FONT[' '])
        for y in range(5):
            for x in range(3):
                if g[y][x] == '1' and 0 <= y0 + y < dst.shape[0] and 0 <= x0 + x < dst.shape[1]:
                    dst[y0 + y, x0 + x] = shade
        x0 += 4
    return x0


def unit_frame(path, index=0):
    raw = np.frombuffer(Path(path).read_bytes(), '>u2')
    f = raw[index * 4 * UNIT_SH:(index + 1) * 4 * UNIT_SH]
    light, dark, mask = f[:UNIT_SH], f[UNIT_SH:2 * UNIT_SH], f[2 * UNIT_SH:3 * UNIT_SH]
    img = np.zeros((UNIT_SH, UNIT_W), np.uint8)
    a = np.zeros((UNIT_SH, UNIT_W), bool)
    for r in range(UNIT_SH):
        for x in range(UNIT_W):
            bit = 1 << (15 - x)
            if mask[r] & bit:
                continue
            img[r, x] = SHADE[(1 if light[r] & bit else 0, 1 if dark[r] & bit else 0)]
            a[r, x] = True
    return img, a


def sheet(out, scale):
    names = list(NEGATIVE) + list(POSITIVE)
    cols, cw, ch = 8, 22, 20
    rows = (len(names) + 5 + cols - 1) // cols
    W, H = cols * cw, rows * ch + 8
    canvas = np.full((H, W), 1, np.uint8)
    for i, name in enumerate(names + ['Charge%d' % k for k in range(5)]):
        if name.startswith('Charge'):
            g, a = charge_frame(int(name[-1]))
        else:
            g, a = glyph(name)
        x0, y0 = (i % cols) * cw, (i // cols) * ch
        blit(canvas, g, a, x0 + (cw - g.shape[1]) // 2, y0 + 2)
        text(canvas, name[:5], x0 + 1, y0 + 12, 3)
    rgb = np.array(GREY, np.uint8)[canvas]
    Image.fromarray(rgb).resize((W * scale, H * scale), Image.NEAREST).save(out)
    print('wrote', out, '%dx%d' % (W, H), len(names), 'glyphs + 5 charge frames')


def demo(out, units, scale):
    body, body_a = unit_frame(units)
    dia, dia_a = render(('..#..', '.###.', '#####', '.###.', '..#..'), 'k', 'w')
    show = ['Poison', 'Silence', 'Sleep', 'Stop', 'Haste', 'Protect', 'Regen', 'Charge']
    W, H = 60, 46
    frames = []
    for step, name in enumerate(show):
        for sub in range(5 if name == 'Charge' else 1):
            canvas = np.full((H, W), 1, np.uint8)
            g, a = (charge_frame(sub) if name == 'Charge' else glyph(name))
            feet = H - 12
            for half, enemy in ((0, False), (1, True)):
                ux = 6 + half * 28
                blit(canvas, body, body_a, ux, feet - UNIT_SH)
                top = feet - UNIT_SH - 2
                if enemy:
                    total = dia.shape[1] + g.shape[1] - 2
                    bx = ux + UNIT_W // 2 - total // 2
                    blit(canvas, dia, dia_a, bx, top - dia.shape[0])
                    blit(canvas, g, a, bx + dia.shape[1] - 1, top - g.shape[0])
                else:
                    blit(canvas, g, a, ux + UNIT_W // 2 - g.shape[1] // 2, top - g.shape[0])
            text(canvas, name[:8], 2, H - 7, 3)
            rgb = np.array(GREY, np.uint8)[canvas]
            frames.append(Image.fromarray(rgb).resize((W * scale, H * scale), Image.NEAREST))
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=300, loop=0)
    print('wrote', out, len(frames), 'frames')


def charge_gif(out, units, scale, hold=200):
    """The charge star twinkling above an ally and an enemy, two cycles.

    Each state holds the same fraction of a second as in game, where the picture
    advances on st.tick (about thirty frames a second).
    """
    body, body_a = unit_frame(units)
    dia, dia_a = render(('..#..', '.###.', '#####', '.###.', '..#..'), 'k', 'w')
    W, H = 60, 50
    frames = []
    for k in range(len(CHARGE) * 2):
        canvas = np.full((H, W), 1, np.uint8)
        g, a = charge_frame(k)
        feet = H - 10
        for half, enemy in ((0, False), (1, True)):
            ux = 6 + half * 28
            blit(canvas, body, body_a, ux, feet - UNIT_SH)
            top = feet - UNIT_SH - 2
            if enemy:
                total = dia.shape[1] + g.shape[1] - 2
                bx = ux + UNIT_W // 2 - total // 2
                blit(canvas, dia, dia_a, bx, top - dia.shape[0])
                blit(canvas, g, a, bx + dia.shape[1] - 1, top - g.shape[0])
            else:
                blit(canvas, g, a, ux + UNIT_W // 2 - g.shape[1] // 2, top - g.shape[0])
        rgb = np.array(GREY, np.uint8)[canvas]
        frames.append(Image.fromarray(rgb).resize((W * scale, H * scale), Image.NEAREST))
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=hold, loop=0)
    print('wrote', out, len(frames), 'frames at %dms' % hold)


# SID of each glyph (status_policy.h). A status with no glyph falls back to the
# bare triangle of its polarity, so a picture is never lost.
SID = {
    'Poison': 24, 'Dark': 10, 'Silence': 12, 'Sleep': 35, 'Stop': 30, 'Slow': 29,
    'Confusion': 11, 'Berserk': 20, 'Petrify': 8, 'DontMove': 36, 'DontAct': 37,
    'Charm': 34, 'Frog': 22, 'Chicken': 21, 'Undead': 3, 'Sentence': 39,
    'Haste': 28, 'Regen': 25, 'Protect': 26, 'Shell': 27, 'Reflect': 38,
    'Float': 17, 'Faith': 32, 'Innocent': 33, 'Transparent': 19, 'Defend': 6,
    'Reraise': 2,
}
CHARGING = (4, 5, 7)            # Charging, Jump, Perform: the twinkling star
CENTRE = 7                      # bit of a 16-pixel row, as art.h foe_gfx centres


def rows16(img, a):
    """One glyph as the engine's light, dark and mask rows, centred like foe_gfx."""
    h, w = img.shape
    x0 = CENTRE - w // 2
    out = []
    for y in range(h):
        lig = dark = msk = 0
        for x in range(16):
            bit = 1 << (15 - x)
            gx = x - x0
            if 0 <= gx < w and a[y, gx]:
                v = img[y, gx]
                if v & 1:
                    lig |= bit
                if v & 2:
                    dark |= bit
            else:
                msk |= bit          # transparent, mask bit set
        out.append((lig, dark, msk))
    return out


PICTO_H = 7                     # the tallest glyph; every one is stored this tall


def header(out):
    """status_art.h: one flat blob of pictograms plus an index.

    The chapter executable sits at TTPack's size limit, so the glyphs have to be
    able to live in the archive tail (tools/contour_assets.py) and be read in
    place. One `u16` blob with an offset per pictogram moves there as a block;
    thirty-two named arrays behind a table of pointers could not, since a pointer
    is an address the archive cannot hold.
    """
    names = list(NEGATIVE) + list(POSITIVE)
    lines = ['// Generated by tools/status_glyphs.py: one pictogram per status, drawn above a',
             '// unit. Same plane convention as art.h foe_gfx. Never edit by hand.',
             '#ifndef FFT_STATUS_ART_H', '#define FFT_STATUS_ART_H', '']
    order, blob = [], []
    for name in names + ['charge%d' % k for k in range(len(CHARGE))]:
        if name.startswith('charge'):
            r = rows16(*charge_frame(int(name[6:])))
        else:
            r = rows16(*glyph(name))
        assert len(r) <= PICTO_H, (name, len(r))
        # Every pictogram is stored PICTO_H rows tall, padded at the top with
        # transparent rows. The engine places them by their bottom edge, so the
        # padding costs nothing on screen and buys a fixed stride: no offset
        # table and no height table in an executable that has no room for either.
        r = [(0, 0, 0xFFFF)] * (PICTO_H - len(r)) + list(r)
        order.append(name)
        for k in range(3):
            blob.extend(x[k] for x in r)
    lines += ['#define PICTO_COUNT %d' % len(names),
              '#define PICTO_CHARGE_STATES %d' % len(CHARGE),
              '#define PICTO_N (PICTO_COUNT + PICTO_CHARGE_STATES)',
              '#define PICTO_H %d                 // every pictogram, padded at the top' % PICTO_H,
              '#define PICTO_STRIDE (3 * PICTO_H) // its three planes, light then dark then mask',
              '',
              'static const u16 picto_blob[%d] = {' % len(blob)]
    for i in range(0, len(blob), 12):
        lines.append('    ' + ' '.join('0x%04X,' % v for v in blob[i:i + 12]))
    lines.append('};')
    lines += ['', '// status id -> pictogram index, 0xFF when the status has none',
              'static const u8 picto_of_sid[40] = {']
    table = ['0xFF'] * 40
    for i, name in enumerate(order[:len(names)]):
        if name in SID:
            table[SID[name]] = str(i)
    for sid in CHARGING:
        table[sid] = 'PICTO_COUNT'          # the charge star, animated
    lines.append('    ' + ', '.join(table))
    lines += ['};', '', '#endif', '']
    Path(out).write_text('\n'.join(lines))
    print('wrote', out, len(names), 'glyphs +', len(CHARGE), 'charge states,',
          2 * len(blob), 'bytes of picture')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--header', action='store_true')
    ap.add_argument('--sheet', action='store_true')
    ap.add_argument('--demo', action='store_true')
    ap.add_argument('--charge', action='store_true')
    ap.add_argument('--units', type=Path, default=HERE / 'fftu.bin')
    ap.add_argument('--dir', type=Path, default=HERE / 'campaign/x/status-picto')
    ap.add_argument('--scale', type=int, default=6)
    args = ap.parse_args()
    args.dir.mkdir(parents=True, exist_ok=True)
    if args.header:
        header(HERE / 'status_art.h')
        return
    if args.charge:
        charge_gif(args.dir / 'charge.gif', args.units, args.scale)
        return
    if args.sheet or not args.demo:
        sheet(args.dir / 'glyphs.png', args.scale)
    if args.demo or not args.sheet:
        demo(args.dir / 'demo.gif', args.units, args.scale)


if __name__ == '__main__':
    main()
