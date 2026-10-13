#!/usr/bin/env python3
"""Milestone 26 contact sheet from the actual native framebuffer; local only."""
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw

CASES = [(26, 40, 'Turn / MENU'), (27, 40, 'Act submenu'), (28, 40, 'Chemist / Items'),
         (29, 40, 'Attack target'), (30, 40, 'Potion target'), (35, 40, 'Hover recruit'),
         (31, 40, 'Status / stats'), (32, 40, 'Status / equipment'), (40, 40, 'Chemist / abilities'),
         (33, 40, 'Enemy status'), (34, 40, 'Water / height'), (38, 40, 'Turn order'),
         (36, 5, 'Walk / bands hidden'), (37, 18, 'Blow / bands hidden'), (39, 40, 'Quit / default No')]
OUT = Path('x'); OUT.mkdir(exist_ok=True)
sheet = Image.new('RGB', (3 * 640, 5 * 428), 'white')
draw = ImageDraw.Draw(sheet)
for i, (scenario, frames, name) in enumerate(CASES):
    path = OUT / ('m26_%d.png' % scenario)
    subprocess.run(['./fft_pc', '--headless', '--scenario', str(scenario), '--frames', str(frames),
                    '--scale', '4', '--shot', str(path)], check=True, stdout=subprocess.DEVNULL)
    x, y = i % 3 * 640, i // 3 * 428
    draw.text((x + 5, y + 5), name, fill='black'); sheet.paste(Image.open(path), (x, y + 23))
sheet.save(OUT / 'm26_interface.png'); print(OUT / 'm26_interface.png')
