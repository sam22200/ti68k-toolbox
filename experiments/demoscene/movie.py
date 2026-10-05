#!/usr/bin/env python3
"""GIF of one or more ti-cycles movies in a grid, each played at its real speed on the calculator.

A movie is a directory written by `ti-cycles --png DIR/f%d.png ... > DIR/log.txt` on a program
that, per frame, does BENCH_VALUE(cycles of the frame) then BENCH_SHOT (gamefx.c, voxel.c
-DMOVIE). Same clock as sidebyside.py: at time t a panel shows the last frame its program would
have finished by t at 12 MHz (cycles x 1.1 for the grayscale driver); a movie shorter than the
longest one loops.

  movie.py OUT.gif TITLE DIR LABEL [DIR LABEL ...] [--cols N]
"""
import sys
from PIL import Image, ImageDraw, ImageFont
from sidebyside import HZ, FPS, SCALE, load, panel


def main():
    a = sys.argv[1:]
    cols = 3
    if '--cols' in a:
        i = a.index('--cols')
        cols = int(a[i + 1])
        del a[i:i + 2]
    out, title, rest = a[0], a[1], a[2:]
    movies = []
    for d, lab in zip(rest[::2], rest[1::2]):
        v, f = load(d, 0)
        t, e = 0.0, []
        for c in v:
            t += c / HZ
            e.append(t)
        movies.append((v, f, e, lab))
    cols = min(cols, len(movies))
    rows = (len(movies) + cols - 1) // cols
    total = max(m[2][-1] for m in movies)
    font = ImageFont.load_default()
    pw, ph = 160 * SCALE, 100 * SCALE
    W, H = cols * (pw + 10) + 10, 22 + rows * (ph + 40)
    frames = []
    for k in range(int(total * FPS)):
        t = k / FPS
        img = Image.new('L', (W, H), 0x30)
        d = ImageDraw.Draw(img)
        d.text((10, 6), title, fill=0xFF, font=font)
        for n, (v, f, e, lab) in enumerate(movies):
            tm, i = t % e[-1], 0
            while i + 1 < len(e) and e[i + 1] <= tm:
                i += 1
            x, y = 10 + (n % cols) * (pw + 10), 22 + (n // cols) * (ph + 40)
            img.paste(panel(f[i], 160), (x, y))
            avg = sum(v) / len(v)
            d.text((x, y + ph + 4), lab, fill=0xFF, font=font)
            d.text((x, y + ph + 18), '%dk cycles/frame, ~%.0f fps' % (avg / 1000, HZ / avg),
                   fill=0xC0, font=font)
        d.text((W - 70, 6), 't = %4.1f s' % t, fill=0xC0, font=font)
        frames.append(img.convert('P'))
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=1000 // FPS, loop=0,
                   optimize=True)
    print(out, len(frames), 'frames, %.1f s' % total)


main()
