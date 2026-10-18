#!/usr/bin/env python3
"""PC art review at 32 game ticks/s, sampled every two ticks; never a TI FPS claim."""
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw

OUT = Path('x/events')


def panel(scenario, frame, label):
    source = Image.open(OUT / 'frames' / ('%u-%03u.png' % (scenario, frame)))
    result = Image.new('RGB', (480, 324), 'white')
    result.paste(source.resize((480, 300), Image.Resampling.NEAREST), (0, 24))
    ImageDraw.Draw(result).text((8, 6), label, fill='black')
    return result


def main():
    frames = OUT / 'frames'; frames.mkdir(parents=True, exist_ok=True)
    subprocess.run([OUT / 'test-events', frames], check=True)
    for name, scenes in [('banners', [(0, 'BATTLE START'), (59, 'VICTORY!'), (60, 'DEFEAT')]),
                         ('loot', [(69, 'KO > crystal'), (70, 'KO > chest'), (71, 'Queued transformations')])]:
        images = []
        for frame in range(2, 161, 2):
            result = Image.new('RGB', (480 * len(scenes), 324), 'white')
            for i, (scenario, label) in enumerate(scenes):
                result.paste(panel(scenario, frame, label + ' / PC preview'), (480 * i, 0))
            images.append(result)
        images[0].save(OUT / (name + '-pc.gif'), save_all=True, append_images=images[1:],
                       # GIF delays use 10 ms units: average exactly 62.5 ms/sample.
                       duration=[60, 60, 60, 70] * (len(images) // 4), loop=0, disposal=2)
        print(OUT / (name + '-pc.gif'))
    sheet = Image.new('RGB', (480 * 3, 324 * 2), 'white')
    for i, (scenario, frame, label) in enumerate([(0, 60, 'Objective'), (59, 60, 'Complete victory'),
        (60, 60, 'Defeat'), (69, 6, 'KO flash'), (69, 18, 'Dissolve'), (69, 28, 'Rotating crystal')]):
        sheet.paste(panel(scenario, frame, label), (i % 3 * 480, i // 3 * 324))
    sheet.save(OUT / 'review.png'); print(OUT / 'review.png')


if __name__ == '__main__':
    main()
