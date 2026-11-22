#!/usr/bin/env python3
"""Reproducible PC-only town portraits and Bar A/B proposals; shipping stays untouched."""
from pathlib import Path
import hashlib
import json
import os
import struct
import subprocess
import sys

from PIL import Image, ImageDraw, ImageChops
import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE.parent / 'tools'))
import extract
import management_data as md
import shop_scene
import town_service_assets as assets


def run(args, cwd=HERE, env=None):
    p = subprocess.run(list(map(str, args)), cwd=cwd, env=env, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode:
        print(p.stdout)
        p.check_returncode()
    return p.stdout


def fingerprint(path):
    return dict(sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                mtime_ns=path.stat().st_mtime_ns)


def article(raw):
    """Strict decode of the uncompressed first WLDMES entry for the review only."""
    end = raw.index(254)
    text, i = [], 0
    punct = dict(md.PAGED)
    punct[2064] = '"'  # FONT.BIN's quotation glyph, not an unknown '?' placeholder.
    while i < end:
        c = raw[i]; i += 1
        if c < 10: text.append(chr(c + 48))
        elif c < 36: text.append(chr(c + 55))
        elif c < 62: text.append(chr(c - 36 + 97))
        elif c in (0xFA, 0xF8, 0xFD): text.append(' ')
        elif 0xD0 <= c <= 0xDF:
            g = (c & 15) * 208 + raw[i]; i += 1
            if g not in punct: raise ValueError('unknown rumor glyph %d' % g)
            text.append(punct[g])
        elif c in md.SIGNS: text.append(md.SIGNS[c])
        else: raise ValueError('unexpected rumor code %02x' % c)
    return ' '.join(''.join(text).split())


def sheet(out, filename, columns, rows):
    scale, cellw, cellh = 4, 664, 442
    canvas = Image.new('RGB', (columns * cellw, len(rows) * cellh), '#ece9e1')
    d = ImageDraw.Draw(canvas)
    for y, row in enumerate(rows):
        for x, (name, title) in enumerate(row):
            d.text((x * cellw + 12, y * cellh + 8), title, fill='black')
            im = Image.open(out / (name + '.png')).convert('RGB')
            assert im.size == (160, 100)
            canvas.paste(im.resize((160 * scale, 100 * scale), Image.Resampling.NEAREST),
                         (x * cellw + 12, y * cellh + 28))
    canvas.save(out / filename)


def main():
    out = HERE / 'x/town-services-review'
    out.mkdir(parents=True, exist_ok=True)
    protected = sorted(set((HERE / 'data').glob('*.bin')) |
                       set((HERE / 'data').glob('*.89y')) |
                       set((HERE / 'data').glob('*.sav')) |
                       {HERE / 'x/check/ffsave_won.sav'})
    before = {str(p.relative_to(ROOT)): fingerprint(p) for p in protected}
    cue = ROOT / 'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    disc, files = extract.disc_files(str(cue))
    faces = disc.read(*files['EVENT/WLDFACE.BIN'])
    background = disc.read(*files['WORLD/WLDBK.BIN'])
    messages = disc.read(*files['WORLD/WLDMES.BIN'])
    current = (HERE / 'data/ffsk.bin').read_bytes()
    corrected = assets.corrected_rooms(current, faces)
    (out / 'ffsk.bin').write_bytes(corrected)
    (out / 'bar-face.bin').write_bytes(assets.portrait(faces, assets.SPEAKERS['bar']))
    rgb = assets.bar_source(background)
    Image.fromarray((rgb * 255 // 31).astype('uint8')).save(out / 'bar-original.png')
    (out / 'bar-planes.bin').write_bytes(shop_scene.planes(assets.bar_greys(rgb)))
    (out / 'rumor.txt').write_text(article(messages))
    # Existing room records, names, stock, saves and the runtime are not regenerated.
    room_start = struct.unpack_from('>H', current, 12)[0]
    assert len(corrected) == len(current) and corrected[room_start:] == current[room_start:]
    # Independent source contact sheet beside the rendered four-grey portraits.
    contact = Image.new('RGB', (4 * 144, 172), 'white'); draw = ImageDraw.Draw(contact)
    for i, (role, index) in enumerate(assets.SPEAKERS.items()):
        px, pal = assets.face_source(faces, index)
        rgb_face = np.stack([(pal[px] >> bit & 31) * 255 // 31 for bit in (0, 5, 10)], 2).astype('uint8')
        rgb_face[px == 0] = 0
        contact.paste(Image.fromarray(rgb_face).resize((96, 144), Image.Resampling.NEAREST), (i * 144 + 20, 22))
        draw.text((i * 144 + 8, 5), '%s: face %d' % (role, index), fill='black')
    contact.save(out / 'speakers-source.png')
    common = [HERE / 'preview_town_services.c', ROOT / 'runtime/core/rt_core.c',
              ROOT / 'runtime/platform-sw/rt_sw.c']
    data = out / 'data'
    data.mkdir(exist_ok=True)
    for bank in (HERE / 'data').glob('*.bin'):
        target = data / bank.name
        if not target.exists(): target.symlink_to(bank)
    (data / 'ffsave.sav').write_bytes((HERE / 'x/check/ffsave_won.sav').read_bytes())
    normal_log = ''
    screen_hashes = {}
    for sanitize in (False, True):
        exe = out / ('preview-san' if sanitize else 'preview')
        flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie'] if sanitize else []
        run(['cc', '-std=gnu99', '-O1', '-g', '-Wall', '-Wextra', '-Wno-unused-parameter',
             *flags, '-o', exe, *common])
        log = run([exe, out, HERE / 'x/check/ffsave_won.sav'], data, os.environ.copy())
        assert 'Save unchanged' in log
        pictures = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                    for p in out.glob('*.png') if p.name.startswith(
                        ('store-', 'office-', 'fur-', 'bar-home', 'bar-hand-', 'a-list',
                         'b-list', 'a-read', 'b-read')) and p.name != 'a-read-2.png'}
        if sanitize:
            assert log == normal_log and pictures == screen_hashes
        else:
            normal_log = log; screen_hashes = pictures
        (out / ('sanitizers.log' if sanitize else 'pc.log')).write_text(log)
    sheet(out, 'portraits-before-after.png', 2, [
        [(name + '-before', label + ' - CURRENT'),
         (name + '-after', label + ' - CORRECT SOURCE PORTRAIT')]
        for name, label in [('store', 'Shop'), ('office', 'Soldier Office'), ('fur', 'Fur Shop')]])
    for name in ('store', 'office', 'fur'):
        old = Image.open(out / (name + '-before.png')).convert('RGB')
        new = Image.open(out / (name + '-after.png')).convert('RGB')
        box = ImageChops.difference(old, new).getbbox()
        assert box and box[0] >= 1 and box[1] >= 1 and box[2] <= 21 and box[3] <= 32
    sheet(out, 'bar-options.png', 2, [
        [('bar-home', 'COMMON - Bar / Chapter I'), ('bar-home', 'COMMON - original room and owner')],
        [('a-list', 'A - list over the room'), ('b-list', 'B - centered list')],
        [('a-read', 'A - wide reader / fewer pages'), ('b-read', 'B - dialogue over the room')],
        [('a-read', 'A - entire sample fits one page'), ('b-read-2', 'B - second page')]])
    hand = [Image.open(out / ('bar-hand-%d.png' % i)).convert('RGB').resize(
        (640, 400), Image.Resampling.NEAREST) for i in range(8)]
    hand[0].save(out / 'bar-hand.gif', save_all=True, append_images=hand[1:], duration=133, loop=0)
    after = {str(p.relative_to(ROOT)): fingerprint(p) for p in protected}
    assert before == after, 'a protected bank/save changed'
    (out / 'preserved.json').write_text(json.dumps(before, indent=2) + '\n')
    result = dict(approval='pending', shipping_changed=False, native_run=False,
                  speakers=assets.SPEAKERS, unchanged_room_bytes=len(current) - room_start,
                  protected_files=len(before), pc_and_sanitizers_equal=True,
                  pc_screens=len(screen_hashes), glove_frames=8,
                  portrait_changes_confined_to_face=True,
                  source_sha256={name: hashlib.sha256(raw).hexdigest() for name, raw in [
                      ('EVENT/WLDFACE.BIN', faces), ('WORLD/WLDBK.BIN', background),
                      ('WORLD/WLDMES.BIN', messages)]},
                  scope='Portrait correction and source Bar A/B presentation only; no rumor gates/save integration.',
                  sample_titles='Short descriptive review labels; not yet extracted original title IDs.',
                  pictures={name: fingerprint(out / name) for name in [
                      'portraits-before-after.png', 'bar-options.png', 'bar-hand.gif',
                      'speakers-source.png', 'bar-original.png']},
                  screen_sha256=screen_hashes)
    (out / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
    print(normal_log.strip())
    print('Portraits and Bar A/B ready; %d protected banks/saves unchanged.' % len(before))


if __name__ == '__main__':
    main()
