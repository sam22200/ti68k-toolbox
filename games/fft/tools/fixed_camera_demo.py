#!/usr/bin/env python3
"""Elevated wide camera and selective cutaway; retain the previous water demo."""
from PIL import Image, ImageDraw
import re
import fixed_water_demo as demo
from perspective_demo import gif, delays


def main(out=None, before=None):
    demo.OUT = out or demo.HERE/'x/fixed-elevated'
    demo.main()
    # Exercise both opacity modes in all four cameras on the real TI binary too.
    keys = demo.OUT/'walls.txt'
    keys.write_text('0\n32 D\n33\n64 F5\n65\n96 D\n97\n128 MODE\n129\n160 D\n161\n192 F5\n193\n224 D\n225\n256\n')
    log = demo.run([demo.ROOT/'tools/bin/ti-cycles', '--arg', 62, '--frames', 256,
                    '--max', 20000, '--keys', keys,
                    *[v for bank in demo.BANKS for v in ['--file', demo.HERE/(bank+'.89y')]],
                    demo.HERE/'fftfcapc.89z'])
    pc = demo.run([demo.OUT/'pc-oracle', 62, keys, 256])
    assert '_main returned' in log
    hashes = re.findall(r'shot \d+ checksum (\w+)', log)
    expected = re.findall(r'^\d+ (\w+)$', pc, re.M)
    assert hashes[:-1] == expected and len(expected) == 256
    (demo.OUT/'walls-cycles.log').write_text(log)
    (demo.OUT/'walls-pc.log').write_text(pc)
    print('256 additional solid/cutaway frames match PC/TI in both zooms/directions.')
    before = before or demo.HERE/'x/fixed-water'
    if not (before/'032.png').exists():
        return
    frames = []
    for n in range(32):
        im = Image.new('RGB', (640, 440), 'white'); draw = ImageDraw.Draw(im)
        for row, (folder, label) in enumerate([(before, 'BEFORE'), (demo.OUT, 'AFTER')]):
            for col, (start, mode) in enumerate([(32, 'CLOSE'), (224, 'WIDE')]):
                x, y = col*320, row*220
                source = Image.open(folder/f'{start+n:03d}.png').convert('RGB').crop((0, 0, 160, 100))
                im.paste(source.resize((320, 200), Image.Resampling.NEAREST), (x, y+20))
                draw.text((x+4, y+4), label+' / '+mode, fill='black')
        frames.append(im)
    gif(demo.OUT/'before-after.gif', frames, delays([1/32]*32))
    frames[8].save(demo.OUT/'before-after.png')


if __name__ == '__main__':
    main()
