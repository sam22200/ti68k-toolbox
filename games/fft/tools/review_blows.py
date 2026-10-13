#!/usr/bin/env python3
"""A frame sheet of milestone 25 from the real headless PC renderer (local only)."""
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw

OUT = Path('x'); OUT.mkdir(exist_ok=True)
FRAMES = [5, 12, 13, 14, 15, 16, 18, 24, 26, 28, 40]
ROWS = [(17, 'Sword / S'), (18, 'Sword / W'), (19, 'Sword / N'), (20, 'Sword / E'),
        (21, 'Dagger / S'), (22, 'Dagger / W'), (23, 'Dagger / N'), (24, 'Dagger / E'),
        (25, 'Unarmed')]
sheet = Image.new('RGB', (len(FRAMES) * 320, len(ROWS) * 225 + 22), 'white')
draw = ImageDraw.Draw(sheet)
for col, frame in enumerate(FRAMES): draw.text((col * 320 + 5, 5), 'frame %d' % frame, fill='black')
for row, (scenario, name) in enumerate(ROWS):
    for col, frame in enumerate(FRAMES):
        path = OUT / ('m25_%d_%d.png' % (scenario, frame))
        subprocess.run(['./fft_pc', '--headless', '--scenario', str(scenario), '--frames', str(frame),
                        '--scale', '2', '--shot', str(path)], check=True, stdout=subprocess.DEVNULL)
        sheet.paste(Image.open(path), (col * 320, row * 225 + 42))
    draw.text((5, row * 225 + 25), name, fill='black')
sheet.save(OUT / 'm25_blows.png')
print(OUT / 'm25_blows.png')
