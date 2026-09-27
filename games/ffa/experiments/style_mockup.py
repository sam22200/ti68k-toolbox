#!/usr/bin/env python3
"""Art style comparison on room 8: each style rendered by tools/art.py, the hero at the start
cell, the 160x100 view (grey and mono) -> style_<name>.png and style_all.png."""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools'))
from PIL import Image
import art
from murparse import Room
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from scale_mockup import hero
U = int(sys.argv[1]) if len(sys.argv) > 1 else 8
CELL = (4, 4) if U == 8 else (5, 4)
r = Room(U)
lay = art.LAYOUTS[U]
views = []
for style in ('slabs', 'planks', 'stone', 'original'):
    if style == 'original':
        src = Image.open(os.path.join(art.HERE, '..', '..', '..', 'ffa_en', 'pictures', 'dec%d.png' % U)).convert('L')
        world = Image.new('RGB', (r.w * 16, r.h * 16))
        world.paste(src.point(lambda v: 0 if v < 128 else 255).convert('RGB').resize((src.width * 16 // 9, src.height * 16 // 9), Image.NEAREST), (16, 16))
    else:
        world = art.to_image(art.render_layout(art.View(r, lay), dict(lay, style=style)))
    hs = hero(16)
    hx, hy = CELL[0] * 16, CELL[1] * 16
    world.paste(hs, (hx - 1, hy + 16 - hs.height + 1), hs)
    cx = min(max(hx + 8 - 80, 0), world.width - 160); cy = min(max(hy + 8 - 50, 0), world.height - 100)
    v = world.crop((cx, cy, cx + 160, cy + 100))
    mono = v.convert('L').point(lambda p: 0 if p < 128 else 255).convert('RGB')
    both = Image.new('RGB', (160, 204), (255, 0, 0)); both.paste(v, (0, 0)); both.paste(mono, (0, 104))
    both.resize((480, 612), Image.NEAREST).save('style_%s.png' % style)
    views.append(both)
    world.resize((world.width * 2, world.height * 2), Image.NEAREST).save('room_%d_%s.png' % (U, style))
sh = Image.new('RGB', (164 * len(views), 204), (255, 0, 0))
for k, v in enumerate(views): sh.paste(v, (k * 164, 0))
sh.resize((sh.width * 2, sh.height * 2), Image.NEAREST).save('style_all.png')
