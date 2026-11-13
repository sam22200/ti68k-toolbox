#!/usr/bin/env python3
"""PC objective-banner proposal using existing Lato strips, font, map and actors.

Only a temporary engine copy changes focus highlighting. No production source,
shipping bank, descriptor, ABI, save or TI binary is changed by this review.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw
import banner
from review_files import save_image

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE/'campaign'))
from check_flow import data_links


def records(packet):
    offsets = struct.unpack_from('>4H', packet)
    result = []
    for off in offsets:
        _, _, rows, stride = packet[off:off+4]
        result.append(packet[off:off+4+3*rows*stride])
    return result


def objective_packet(original, texts, glyphs):
    def fit(text):
        for cap in (12, 11, 10):
            width = sum((glyphs[c][0] if c != ' ' else banner.SPACE)*cap//banner.CAP+1 for c in text)
            if width <= 152:
                return cap
        raise ValueError('Goal name requires a deliberate layout, not tiny text')
    caps = [fit(text) for text in texts]
    lines = banner.LINES
    try:
        banner.LINES = [(texts[0], caps[0], 4), (texts[1], caps[1], 26),
                        ('VICTORY!', 18, 4), ('DEFEAT', 18, 4)]
        packet, widths = banner.strip_bytes(glyphs)
    finally:
        banner.LINES = lines
    parts = records(original)
    parts[:2] = records(packet)[:2]  # preserve the reviewed victory/defeat strips exactly
    out, offsets = bytearray(), []
    for part in parts:
        offsets.append(8+len(out))
        out += part
    return struct.pack('>4H', *offsets)+out, dict(widths=widths[:2], caps=caps)


def run(command, cwd, log):
    result = subprocess.run(list(map(str, command)), cwd=cwd, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.write_text(result.stdout)
    result.check_returncode()
    return result.stdout


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE/'campaign/x/objectives/review')
    out = ap.parse_args().out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    ui = (HERE/'campaign/data/ffcu.bin').read_bytes()
    at = int(re.search(r'#define UIC_BANNERS (\d+)', (HERE/'ui_common.h').read_text())[1])
    original = ui[at:]
    engine = (HERE/'fft.c').read_text()
    old = 'st.banner == BN_START && st.unit[i].team == TEAM_ENEMY && st.unit[i].hp'
    assert engine.count(old) == 1
    patched = engine.replace(old, 'st.banner == BN_START && (review_focus & ((u16)1 << i)) && st.unit[i].hp')
    images, logs, widths = [], [], []
    with tempfile.TemporaryDirectory(prefix='fft-objective-review-') as name:
        base = Path(name)
        shadow = base/'games/fft'
        (shadow/'campaign').mkdir(parents=True)
        for p in HERE.iterdir():
            if p.name != 'campaign':
                (shadow/p.name).symlink_to(p, target_is_directory=p.is_dir())
        for p in (HERE/'campaign').iterdir():
            (shadow/'campaign'/p.name).symlink_to(p, target_is_directory=p.is_dir())
        for p in ('runtime', 'lib', 'tools'):
            (base/p).symlink_to(ROOT/p, target_is_directory=True)
        (shadow/'fft.c').unlink()
        (shadow/'fft.c').write_text(patched)
        binary = out/'review-pc'
        run(['cc', '-std=gnu99', '-O1', '-g', '-w', '-o', binary,
             shadow/'campaign/review_objectives.c', ROOT/'runtime/core/rt_core.c',
             ROOT/'runtime/platform-sw/rt_sw.c'], base, out/'build.log')
        data = base/'data'
        data.mkdir()
        data_links(data)
        names = run([binary, 4, 'unused'], data, out/'names.log')
        names = dict(line.split(':', 1) for line in names.splitlines())
        target, protect = names['target'].upper(), names['protect'].upper()
        texts = [(f'DEFEAT {target}!', ''),
                 (f'PROTECT {protect}!', 'DEFEAT ALL FOES!'),
                 (f'DEFEAT {target}!', f'PROTECT {protect}!')]
        chars = ''.join(sorted(set(banner.CHARS+''.join(t for pair in texts for t in pair)) - {' '}))
        glyphs = banner._font_outlines(banner.TTF, banner.FONT, chars, banner.CAP, banner.Y0)
        for kind in range(4):
            if kind:
                packet, width = objective_packet(original, texts[kind-1], glyphs)
                (data/'review.bin').write_bytes(packet)
                widths.append(width)
            path = out/f'objective-{kind}.png'
            logs.append(run([binary, kind, path], data, out/f'case-{kind}.log'))
            images.append(Image.open(path).convert('RGB').crop((0, 0, 160, 100)))
    labels = ['Current: defeat all enemies', 'Proposal: named target only',
              'Proposal: protect an ally', 'Proposal: target plus protected ally']
    sheet = Image.new('RGB', (960, 648), 'white')
    draw = ImageDraw.Draw(sheet)
    for i, (label, image) in enumerate(zip(labels, images)):
        x, y = (i % 2)*480, (i//2)*324
        draw.text((x+6, y+6), label, fill='black')
        sheet.paste(image.resize((480, 300), Image.Resampling.NEAREST), (x, y+24))
    save_image(sheet, out/'objective-before-after.png')
    report = dict(scope='PC visual proposal only; private focus-highlight patch, existing Lato rasterizer/result strips. '
                        'No production rules/format/art integration or native/performance claim.',
                  source_sha256=hashlib.sha256(engine.encode()).hexdigest(),
                  shared_ui_sha256=hashlib.sha256(ui).hexdigest(), widths=widths,
                  logs=logs, names=names, style='Direct named goals at title-size cap12, fitting down to10 only when necessary; no small name labels', approval='pending')
    (out/'results.json').write_text(json.dumps(report, indent=2)+'\n')
    print('Four PC objective views ready; production untouched; approval pending')


if __name__ == '__main__':
    main()
