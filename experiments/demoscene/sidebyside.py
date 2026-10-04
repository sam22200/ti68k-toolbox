#!/usr/bin/env python3
"""Side-by-side GIF of two ti-cycles movies played at their real speed on the calculator.

A movie is a directory written by `ti-cycles --png DIR/f%d.png ... > DIR/log.txt` on a program
that, per frame, does BENCH_VALUE(cycles of the frame) then BENCH_SHOT (voxel.c -DMOVIE,
games/mode7 bench scenario 5). The GIF runs on a wall clock: at time t each panel shows the last
frame its program would have finished by t at 12 MHz (cycles x 1.1 for the grayscale driver, the
same for both), so the slower side drops frames and finishes later.

  sidebyside.py OUT.gif TITLE DIR_A LABEL_A DIR_B LABEL_B [--skip N] [--width W]
--skip N: ignore the first N BENCH_VALUE lines of the logs (other values printed before).
--width W: the view's width in pixels (Mode 7: 128).
"""
import sys
from PIL import Image, ImageDraw, ImageFont

HZ = 12e6 / 1.1                 # 12 MHz, ~9 % taken by the grayscale driver
FPS = 20                        # GIF frames per second of wall time
SCALE = 2
GREYS = (0xD8, 0x98, 0x58, 0x18)  # white, light, dark, black on a lighter LCD-like ramp


def load(d, skip):
    vals = [int(l.split()[1]) for l in open(d + '/log.txt') if l.startswith('value:')][skip:]
    frames = [Image.open('%s/f%d.png' % (d, i)).convert('L') for i in range(len(vals))]
    return vals, frames


def panel(img, width):
    v = img.crop((0, 0, width, 100))
    # the PNG's 4 levels (white..black) -> GREYS
    levels = sorted(set(v.getdata()), reverse=True)
    lut = {l: GREYS[min(i, 3)] for i, l in enumerate(levels)} if len(levels) <= 4 else None
    if lut:
        v = v.point(lambda p: lut.get(p, p))
    return v.resize((width * SCALE, 100 * SCALE), Image.NEAREST)


def main():
    a = sys.argv[1:]
    skip = int(a[a.index('--skip') + 1]) if '--skip' in a else 0
    width = int(a[a.index('--width') + 1]) if '--width' in a else 160
    out, title, da, la, db, lb = a[:6]
    va, fa = load(da, skip)
    vb, fb = load(db, skip)
    ends = []
    for v in (va, vb):
        t, e = 0.0, []
        for c in v:
            t += c / HZ
            e.append(t)
        ends.append(e)
    total = max(ends[0][-1], ends[1][-1]) + 1.0
    font = ImageFont.load_default()
    pw, ph = width * SCALE, 100 * SCALE
    W, H = pw * 2 + 30, ph + 70
    out_frames = []
    n = int(total * FPS)
    for k in range(n):
        t = k / FPS
        img = Image.new('L', (W, H), 0x30)
        d = ImageDraw.Draw(img)
        d.text((10, 6), title, fill=0xFF, font=font)
        for side, (v, f, e, lab) in enumerate(((va, fa, ends[0], la), (vb, fb, ends[1], lb))):
            i = 0
            while i + 1 < len(e) and e[i + 1] <= t:
                i += 1
            shown = i if e[0] <= t else 0
            x = 10 + side * (pw + 10)
            img.paste(panel(f[shown], width), (x, 22))
            avg = sum(v) / len(v)
            fps = HZ / avg
            done = 'done' if t >= e[-1] else 'frame %d/%d' % (shown + 1, len(v))
            d.text((x, 26 + ph), lab, fill=0xFF, font=font)
            d.text((x, 40 + ph), '%dk cycles/frame, ~%.1f fps   %s' % (avg / 1000, fps, done),
                   fill=0xC0, font=font)
        d.text((W - 70, 6), 't = %4.1f s' % t, fill=0xC0, font=font)
        out_frames.append(img.convert('P'))
    out_frames[0].save(out, save_all=True, append_images=out_frames[1:], duration=1000 // FPS,
                       loop=0, optimize=True)
    print(out, n, 'frames, %.1f s' % total)


main()
