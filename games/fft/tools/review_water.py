#!/usr/bin/env python3
"""Actual native 160x100 water scenes, enlarged without filtering; local output."""
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw


def main():
    out = Path('x'); out.mkdir(exist_ok=True)
    cases = [(41 + r, 20, 'six sheets / view %d' % r) for r in range(4)]
    cases += [(45, 3, 'entry / airborne'), (45, 12, 'entry / landed'),
              (46, 3, 'wading'), (47, 3, 'leaving / hop'), (48, 14, 'sword in water'),
              (49, 24, 'KO in water'), (51, 20, 'critical'), (52, 14, 'throw in water')]
    sheet = Image.new('RGB', (3 * 640, 4 * 428), 'white'); text = ImageDraw.Draw(sheet)
    for k, (scenario, frames, label) in enumerate(cases):
        shot = out / ('m27_%d_%d.png' % (scenario, frames))
        subprocess.run(['./fft_pc', '--headless', '--scenario', str(scenario), '--frames', str(frames),
                        '--shot', str(shot)], check=True, stdout=subprocess.DEVNULL)
        x, y = k % 3 * 640, k // 3 * 428
        text.text((x + 8, y + 8), '%d, frame %d: %s' % (scenario, frames, label), fill='black')
        sheet.paste(Image.open(shot), (x, y + 28))
    sheet.save(out / 'm27_water.png')
    print(out / 'm27_water.png')


if __name__ == '__main__': main()
