#!/usr/bin/env python3
"""PC-only visibility strategy review; never invokes any TI validation tool."""
import hashlib
import json
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw
from perspective_demo import gif

HERE = Path(__file__).resolve().parents[1]
OUT = HERE/'x/fixed-review'
MODES = ['auto', 'solid', 'dotted']
VIEWS = ['close-front', 'close-back', 'wide-front', 'wide-back']


def run(args):
    return subprocess.check_output([str(a) for a in args], cwd=HERE, text=True)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    binary = OUT/'capture-pc'
    run(['cc', '-std=gnu99', '-O1', '-DFFT_EXPLORATION', '-DFFT_FIXED',
         '-o', binary, 'fft.c', 'tools/fixed_review_capture.c',
         '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c'])
    images, hashes = {}, {}
    tones = {216: 255, 152: 170, 88: 85, 24: 0}
    for view, name in enumerate(VIEWS):
        for mode, style in enumerate(MODES):
            folder = OUT/(name+'-'+style); folder.mkdir(exist_ok=True)
            log = run([binary, view, mode, folder]); (folder/'pc.log').write_text(log)
            assert len(log.splitlines()) == 32
            hashes[name+'-'+style] = log.splitlines()
            images[name, style] = [Image.open(folder/f'{n:03d}.png').convert('L')
                                   .point([tones.get(x, x) for x in range(256)]).convert('RGB')
                                   for n in range(32)]
    # The user's two keep-solid examples must match fully solid world pixels, excluding HUD.
    for name in ['close-front', 'wide-back']:
        for a, b in zip(images[name, 'auto'], images[name, 'solid']):
            assert a.crop((0, 8, 160, 93)).tobytes() == b.crop((0, 8, 160, 93)).tobytes(), name
    assert images['wide-front', 'auto'][8].crop((0, 8, 160, 93)).tobytes() != images['wide-front', 'solid'][8].crop((0, 8, 160, 93)).tobytes()

    comparison = []
    selected = ['close-front', 'wide-front', 'wide-back']
    for n in range(32):
        im = Image.new('RGB', (640, 660), 'white'); draw = ImageDraw.Draw(im)
        for row, view in enumerate(selected):
            for col, mode in enumerate(['solid', 'auto']):
                x, y = col*320, row*220
                draw.text((x+4, y+4), view.upper()+' / '+mode.upper(), fill='black')
                im.paste(images[view, mode][n].resize((320, 200), Image.Resampling.NEAREST), (x, y+20))
        comparison.append(im)
    gif(OUT/'solid-auto.gif', comparison, [100]*32)
    comparison[8].save(OUT/'solid-auto.png')

    modes = []
    for n in range(32):
        im = Image.new('RGB', (960, 660), 'white'); draw = ImageDraw.Draw(im)
        for row, view in enumerate(selected):
            for col, mode in enumerate(['auto', 'solid', 'dotted']):
                x, y = col*320, row*220
                draw.text((x+4, y+4), view.upper()+' / '+mode.upper(), fill='black')
                im.paste(images[view, mode][n].resize((320, 200), Image.Resampling.NEAREST), (x, y+20))
        modes.append(im)
    gif(OUT/'three-modes.gif', modes, [100]*32)
    modes[8].save(OUT/'three-modes.png')
    manifest = dict(platform='Portable C on PC only; no TI build, ti-cycles, xcheck or TiEmu.',
                    timing='Uniform 10 FPS for visual review, not a performance measurement.',
                    source_frames=384, binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                    sources={name:hashlib.sha256((HERE/name).read_bytes()).hexdigest()
                             for name in ['fft.c', 'fft.h', 'perspective.inc', 'fixed.inc']},
                    regression='Close/front and wide/back automatic world pixels equal solid; wide/front changes.',
                    checksums=hashes)
    (OUT/'manifest-pc.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('384 PC frames; annotated keep-solid examples preserved; foreground cutaway differs. No TI validation.')


if __name__ == '__main__':
    main()
