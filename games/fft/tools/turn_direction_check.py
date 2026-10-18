#!/usr/bin/env python3
"""Independent geometric direction checks and a labelled PC diagnostic sheet."""
from PIL import Image, ImageDraw, ImageFont
import turn_audit as audit


def main():
    out = audit.HERE / 'x/turn-direction'
    out.mkdir(parents=True, exist_ok=True)
    data = audit.HERE / 'x/config/data'
    assert (data/'fftlook.bin').exists(), 'Run make camera-config-ti first.'
    audit.run(['cc', '-std=gnu99', '-O1', '-g', *audit.FLAGS,
               '-DFFT_TURN_LOWPOLY_NATIVE', '-o', out/'check', 'tools/turn_direction_check.c',
               '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c', '-lm'])
    log = audit.run([out/'check', out], data)
    (out/'check.log').write_text(log)
    markers = {}
    for line in log.splitlines():
        if line.startswith('MARK,'):
            _, name, x, y = line.split(',')
            markers[name] = (int(x), int(y))
    sheet = Image.new('RGB', (7*320, 2*255+32), '#eeeeee')
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 16)
    for case in range(2):
        names = ['source'] + [f'low-{step}' for step in range(5)] + ['target']
        labels = ['A / ISO', 'A / LOW POLY', '22.5 deg', '45 deg', '67.5 deg', 'B / LOW POLY', 'B / ISO']
        for col, (name, label) in enumerate(zip(names, labels)):
            key = f'{case}-{name}'
            x, y = col*320, case*255+28
            image = Image.open(out/f'{key}.png').convert('RGB').resize((320,200), Image.Resampling.NEAREST)
            sheet.paste(image, (x,y))
            mx,my = markers[key]; mx=x+mx*2;my=y+my*2
            draw.ellipse((mx-7,my-7,mx+7,my+7), outline='#e00040', width=3)
            draw.text((x+6,y-24), f'{"F1" if case else "F5"}: {label}', fill='black', font=font)
    draw.text((8, sheet.height-27), 'PC diagnostic, not timed footage. Red ring: same enemy. Endpoint low-poly views are diagnostic only.', fill='black', font=font)
    sheet.save(out/'direction-proof.png')
    print(log.splitlines()[-1])
    print(out/'direction-proof.png')


if __name__ == '__main__':
    main()
