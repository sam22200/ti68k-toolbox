#!/usr/bin/env python3
"""Five contrast proposals for all four accepted town speakers; current look as control.

Only palette quantization changes. Source faces, nearest17x27 sampling, outline,
record layout, shared dialogue frame and scene remain identical. PC review only.
"""
from pathlib import Path
import hashlib
import json
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageChops

import review_town_services as review
import town_service_assets as assets
import interface
import extract

HERE, ROOT = review.HERE, review.ROOT
PROPOSALS = (
    ('A', 'Natural', 'Absolute source luminance; existing7/16/22 thresholds.'),
    ('B', 'Balanced', 'Normalized palette, gamma1.35, uniform four-grey quantization.'),
    ('C', 'Deeper', 'Normalized palette, gamma1.85, deeper midtones.'),
    ('D', 'Crisp', 'Gamma1.35 with1.5x contrast around the midtone.'),
    ('E', 'Dithered', 'Gamma1.35, ordered2x2 dither in the midtones only.'),
)


def treatment(face, colors, variant):
    small = np.asarray(Image.fromarray(face).resize((17, 27), Image.Resampling.NEAREST))
    return assets.face_greys(small, colors, variant)


def main():
    previous = HERE / 'x/town-services-review'
    out = HERE / 'x/town-portrait-contrast'
    out.mkdir(parents=True, exist_ok=True)
    # Reuse the preceding source/layout study and its scene, with no regeneration.
    protected = sorted(set((HERE / 'data').glob('*.bin')) |
                       set((HERE / 'data').glob('*.89y')) |
                       set((HERE / 'data').glob('*.sav')) |
                       {HERE / 'x/check/ffsave_won.sav'} |
                       {p for p in previous.iterdir() if p.suffix in ('.png', '.gif', '.json', '.bin', '.txt')})
    before = {str(p.relative_to(ROOT)): review.fingerprint(p) for p in protected}
    for name in ('ffsk.bin', 'bar-face.bin', 'bar-planes.bin', 'rumor.txt'):
        (out / name).write_bytes((previous / name).read_bytes())
    cue = ROOT / 'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    disc, files = extract.disc_files(str(cue))
    raw = disc.read(*files['EVENT/WLDFACE.BIN'])
    records = []
    for variant in range(6):
        for role, index in assets.SPEAKERS.items():
            face, colors = assets.face_source(raw, index)
            grey = np.full((29, 32), -1, dtype=int)
            grey[1:28, 1:18] = treatment(face, colors, variant)
            interface.outline(grey)
            record = bytes(interface.planes(grey, 32))
            control = assets.portrait(raw, index)
            assert len(record) == 348 and record[232:] == control[232:]
            if variant == 0: assert record == control
            records.append(record)
    (out / 'portraits.bin').write_bytes(b''.join(records))
    common = [HERE / 'preview_town_services.c', ROOT / 'runtime/core/rt_core.c',
              ROOT / 'runtime/platform-sw/rt_sw.c']
    normal = None
    for sanitize in (False, True):
        exe = out / ('preview-san' if sanitize else 'preview')
        flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie'] if sanitize else []
        review.run(['cc', '-std=gnu99', '-O1', '-g', '-Wall', '-Wextra', '-Wno-unused-parameter',
                    *flags, '-o', exe, *common])
        log = review.run([exe, out, HERE / 'x/check/ffsave_won.sav', '--portraits'],
                         previous / 'data', os.environ.copy())
        assert '24 shared-renderer screens; Save unchanged' in log
        hashes = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob('portrait-*.png')}
        assert len(hashes) == 24
        if sanitize: assert normal == hashes
        else: normal = hashes
        (out / ('sanitizers.log' if sanitize else 'pc.log')).write_text(log)
    roles = ('Shopkeeper', 'Guildmaster', 'Fur Merchant', 'Bar owner')
    labels = [('0', 'Previous contrast')] + [(a, b) for a, b, _ in PROPOSALS]
    cellw, cellh = 180, 344
    sheet = Image.new('RGB', (6 * cellw, 4 * cellh + 28), '#e9e6df')
    draw = ImageDraw.Draw(sheet)
    draw.text((12, 8), 'SAME FOUR FACES - five contrast proposals; 6x portrait and actual 1x screen', fill='black')
    for speaker, role in enumerate(roles):
        control = Image.open(out / ('portrait-0-%u.png' % speaker)).convert('RGB')
        for variant, (letter, label) in enumerate(labels):
            im = Image.open(out / ('portrait-%u-%u.png' % (variant, speaker))).convert('RGB')
            if variant:
                box = ImageChops.difference(control, im).getbbox()
                assert box and box[0] >= 1 and box[1] >= 1 and box[2] <= 21 and box[3] <= 32
            x, y = variant * cellw, 28 + speaker * cellh
            draw.text((x + 8, y + 5), '%s - %s' % (letter, label), fill='black')
            draw.text((x + 8, y + 20), role, fill='black')
            sheet.paste(im.crop((1, 1, 22, 33)).resize((126, 192), Image.Resampling.NEAREST), (x + 26, y + 36))
            sheet.paste(im, (x + 10, y + 238))
        sheet.crop((0, 28 + speaker * cellh, 6 * cellw, 28 + (speaker + 1) * cellh)).save(
            out / ('contrast-%u.png' % (speaker + 1)))
    sheet.save(out / 'portraits-five-contrasts.png')
    after = {str(p.relative_to(ROOT)): review.fingerprint(p) for p in protected}
    assert before == after
    (out / 'preserved.json').write_text(json.dumps(before, indent=2) + '\n')
    result = dict(identity_approval='accepted', contrast_approval='pending',
                  user_message='yup valid but i dont like the contrast of the portrait can you make the 4 porteait in 5 dofferents ways',
                  proposals=[dict(letter=a, label=b, method=c) for a, b, c in PROPOSALS],
                  speakers=assets.SPEAKERS, source_sha256=hashlib.sha256(raw).hexdigest(),
                  same_masks=True, control_records_exact=True, only_face_pixels_change=True,
                  pc_sanitizers_equal=True, pc_screens=24, protected_files=len(before),
                  shipping_changed=False, native_run=False,
                  screen_sha256=normal,
                  picture=review.fingerprint(out / 'portraits-five-contrasts.png'))
    (out / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Four accepted faces, five new treatments plus exact control;24 PC/sanitizer screens agree.')
    print('%d input banks, saves and preceding review files unchanged.' % len(before))


if __name__ == '__main__':
    main()
