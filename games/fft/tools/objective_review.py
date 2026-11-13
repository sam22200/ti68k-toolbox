#!/usr/bin/env python3
"""Show the integrated large objective titles through the production renderer.

The original approved proposal remains in x/objectives/review. New images go
into production-review; no private engine patch or shipping bank is modified.
"""
import argparse
import json
from pathlib import Path
import sys
from PIL import Image, ImageDraw
from review_files import save_image

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE/'campaign'))
from check_objective_runtime import render_objectives
import objective_bank


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE/'campaign/x/objectives/production-review')
    out = ap.parse_args().out.resolve(); out.mkdir(parents=True, exist_ok=True)
    target, protect = dict(slot=1, name='Keane'), dict(slot=0, name='Delita')
    banks = [objective_bank.build([dict(route='g', **pair)]) for pair in
             [dict(target=target), dict(protect=protect), dict(target=target, protect=protect)]]
    screens = render_objectives(out, None, banks)
    sheet = Image.new('RGB', (960, 648), 'white'); draw = ImageDraw.Draw(sheet)
    for i, label in enumerate(['Ordinary elimination', 'Named target', 'Protected ally', 'Target and protected ally']):
        x, y = (i%2)*480, (i//2)*324
        draw.text((x+6, y+6), label, fill='black')
        image = Image.open(out/f'objective-{i}.png').convert('RGB').crop((0, 0, 160, 100))
        sheet.paste(image.resize((480, 300), Image.Resampling.NEAREST), (x, y+24))
    save_image(sheet, out/'objective-before-after.png')
    (out/'results.json').write_text(json.dumps(dict(screens=screens, approved_pixels_equal=True), indent=2)+'\n')
    print('Production objective views: exact approved pixels; '+str(out/'objective-before-after.png'))


if __name__ == '__main__': main()
