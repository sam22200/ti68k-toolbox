#!/usr/bin/env python3
"""Show an FMB1 body bank: every picture it holds, its animations, its cost.

Decodes the packed bank itself, so what is shown is what the game draws. Writes
a contact sheet of every picture, animated GIFs at the bank's own rhythm (front
and mirrored, as the renderer flips them), and a report of how close the
pictures are to each other: how many could be dropped, and what that saves.

    monster_body_review.py --bank games/fft/campaign/data/ffmb.bin --out x/monster-body/review
"""
import argparse
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image, ImageDraw

import monster_body_bank as bank
from monster_appearance import REPRESENTATIVE_JOBS

HERE = Path(__file__).resolve().parents[1]
POSE_LABEL = ('Idle', 'Walk', 'Attack', 'Hurt', 'Dead')
PAD = 3                                   # room around a body on the review canvas


def read(raw):
    """-> families[(jobs, {anim: [(picture index, wait)]})], pictures[(gray, ax, ay)]."""
    fams, slots = struct.unpack_from('>HH', raw, 4)
    base = bank.HEADER+4*fams+4*bank.ANIMS*fams
    offsets = struct.unpack_from(f'>{slots+1}H', raw, base)
    waits = raw[base+2*(slots+1):base+2*(slots+1)+slots]
    order, pictures = {}, []
    for start in offsets[:-1]:
        if start in order:
            continue
        h, wb, ay, ax = raw[start:start+4]
        n = h*wb
        planes = [np.unpackbits(np.frombuffer(raw[start+4+k*n:start+4+(k+1)*n], np.uint8)
                                ).reshape(wb//2, h, 16).transpose(1, 0, 2).reshape(h, 8*wb)
                  for k in range(4)]
        planes = [a.astype(int) for a in planes]
        gray = np.where(planes[2] == 1, -1, planes[0]+2*planes[1])
        order[start] = len(pictures)
        pictures.append((gray, ax, ay))
    families = []
    for f in range(fams):
        jobs = [bank.JOB0+i for i in range(bank.JOBS) if raw[8+i] == f]
        anims = {}
        for a in range(bank.ANIMS):
            first, count, _ = struct.unpack_from('>HBB', raw, bank.HEADER+4*fams+4*(bank.ANIMS*f+a))
            anims[a] = [(order[offsets[first+k]], waits[first+k]) for k in range(count)]
        families.append((jobs, anims))
    return families, pictures


def on_canvas(picture, shape=(56, 112)):
    """A picture placed by its feet on a common canvas, so two can be compared."""
    gray, ax, ay = picture
    out = np.full(shape, -1)
    y0, x0 = shape[0]-12-ay, shape[1]//2-ax
    assert y0 >= 0 and x0 >= 0 and y0+gray.shape[0] <= shape[0] and x0+gray.shape[1] <= shape[1]
    out[y0:y0+gray.shape[0], x0:x0+gray.shape[1]] = gray
    return out


def image_of(picture, scale=1, back=170):
    gray, _, _ = picture
    pixels = np.full(gray.shape, back, np.uint8)
    body = gray >= 0
    pixels[body] = (255-85*gray[body]).astype(np.uint8)
    im = Image.fromarray(pixels)
    return im.resize((im.width*scale, im.height*scale), Image.Resampling.NEAREST) if scale > 1 else im


def distances(pictures):
    """Pixels that differ between every pair, placed on the same feet."""
    boxes = np.stack([on_canvas(p) for p in pictures])
    n = len(pictures)
    d = np.zeros((n, n), int)
    for i in range(n):
        d[i] = (boxes != boxes[i]).reshape(n, -1).sum(1)
    return d


def picture_bytes(picture):
    gray, _, _ = picture
    return 4+4*gray.shape[0]*(gray.shape[1]//8)


def closeness(families, pictures, limits):
    """What dropping pictures within D differing pixels of another would save."""
    d = distances(pictures)
    sizes = [picture_bytes(p) for p in pictures]
    nearest = sorted(int(min(d[i, k] for k in range(len(pictures)) if k != i))
                     for i in range(len(pictures)))
    rows = []
    for limit in limits:
        keep, replace = [], {}
        for i in range(len(pictures)):
            match = next((k for k in keep if d[i, k] <= limit), None)
            if match is None:
                keep.append(i)
            replace[i] = match if match is not None else i
        slots = sum(len(v) for _, anims in families for v in anims.values())
        merged = 0                       # neighbours in a sequence that become one slot
        for _, anims in families:
            for steps in anims.values():
                merged += sum(1 for a, b in zip(steps, steps[1:])
                              if replace[a[0]] == replace[b[0]])
        rows.append(dict(limit=limit, pictures=len(keep), dropped=len(pictures)-len(keep),
                         picture_bytes=sum(sizes[i] for i in keep),
                         slots=slots, slots_merged=merged))
    return rows, nearest


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--bank', type=Path, default=HERE/'campaign/data/ffmb.bin')
    ap.add_argument('--out', type=Path, default=HERE/'campaign/x/monster-body/review')
    ap.add_argument('--names', type=Path, help='families.json from monster_body_assets.py')
    ap.add_argument('--scale', type=int, default=3)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    raw = args.bank.read_bytes()
    bank.validate(raw)
    families, pictures = read(raw)
    names = {}
    if args.names:
        names = {row['jobs'][0]: row['name'] for row in json.loads(args.names.read_text())['families']}

    # 1. every picture the bank holds: one row per pose, front then back.
    scale = args.scale
    cell = (40*scale, 32*scale)
    width = max(len(anims[a])+len(anims[a+1]) for _, anims in families for a in range(0, 10, 2))
    sheet = Image.new('RGB', (150+width*cell[0], 30+len(families)*5*(cell[1]+14)), 'white')
    draw = ImageDraw.Draw(sheet)
    draw.text((8, 8), f'{args.bank.name}: every picture of every pose, front then back, '
                      f'{scale}x nearest, placed on its feet', fill='black')
    y = 30
    for jobs, anims in families:
        draw.text((8, y+2), names.get(jobs[0], f'job {jobs[0]}'), fill='black')
        draw.text((8, y+14), f'jobs {jobs[0]}-{jobs[-1]}', fill='gray')
        for pose in range(5):
            draw.text((8, y+28+pose*(cell[1]+14)-28+28), '', fill='black')
            draw.text((90, y+pose*(cell[1]+14)+cell[1]//2), POSE_LABEL[pose], fill='gray')
            x = 150
            for side in range(2):
                for picture, wait in anims[2*pose+side]:
                    body = image_of(pictures[picture], scale).convert('RGB')
                    sheet.paste(body, (x+(cell[0]-body.width)//2,
                                       y+pose*(cell[1]+14)+cell[1]-body.height))
                    draw.text((x+2, y+pose*(cell[1]+14)+cell[1]+1),
                              f'{"fb"[side]}{wait}', fill='gray')
                    x += cell[0]
                x += cell[0]//3
        y += 5*(cell[1]+14)
    sheet.save(args.out/'pictures.png')

    # 2. the animations, at the bank's own rhythm, front and mirrored.
    pages = (len(families)+7)//8
    for page in range(pages):
        group = families[8*page:8*page+8]
        frames, cw, ch = [], 48*args.scale, 56*args.scale
        for pose in range(5):
            total = max(sum(w for _, w in anims[2*pose]) for _, anims in group)
            for tick in range(total):
                image = Image.new('RGB', (40+len(group)*(2*cw+10), 40+ch), 'white')
                draw = ImageDraw.Draw(image)
                draw.text((8, 6), f'{POSE_LABEL[pose]} - original facing and its mirror, '
                                  f'{args.scale}x, the bank\'s own waits', fill='black')
                for col, (jobs, anims) in enumerate(group):
                    steps = anims[2*pose]
                    loop = sum(w for _, w in steps)
                    k, t = 0, tick % loop
                    for k, (_, w) in enumerate(steps):
                        if t < w:
                            break
                        t -= w
                    body = image_of(pictures[steps[k][0]], args.scale).convert('RGB')
                    x = 20+col*(2*cw+10)
                    draw.text((x, 24), names.get(jobs[0], str(jobs[0]))[:14], fill='black')
                    for mirror in range(2):
                        shown = body.transpose(Image.Transpose.FLIP_LEFT_RIGHT) if mirror else body
                        image.paste(shown, (x+mirror*cw+(cw-shown.width)//2, 40+ch-shown.height))
                frames.append(image)
        frames[0].save(args.out/f'animations-{page+1}.gif', save_all=True,
                       append_images=frames[1:], duration=33, loop=0, disposal=2, optimize=False)

    report = dict(bank=str(args.bank), bytes=len(raw), families=len(families),
                  jobs=sum(len(j) for j, _ in families),
                  slots=sum(len(v) for _, anims in families for v in anims.values()),
                  pictures=len(pictures),
                  picture_bytes=sum(picture_bytes(p) for p in pictures),
                  **dict(zip(('closeness', 'nearest_neighbour_pixels'),
                            closeness(families, pictures, (0, 1, 2, 4, 8, 16, 32, 64)))))
    (args.out/'review.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k: v for k, v in report.items()
                      if k not in ('closeness', 'nearest_neighbour_pixels')}))
    near = report['nearest_neighbour_pixels']
    print('pixels to the closest other picture: min %d, median %d, max %d'
          % (near[0], near[len(near)//2], near[-1]))
    print(' limit  pictures  dropped  picture bytes  slots merged')
    for row in report['closeness']:
        print('%6d %9d %8d %14d %13d' % (row['limit'], row['pictures'], row['dropped'],
                                         row['picture_bytes'], row['slots_merged']))


if __name__ == '__main__':
    main()
