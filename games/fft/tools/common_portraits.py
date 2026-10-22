#!/usr/bin/env python3
"""The common dialogue portraits (ffcp), for every known speaker of every chapter's scenes.

usage: common_portraits.py DISC.cue
Portraits are converted exactly as the battle ones (tools/interface.py: the SPR's portrait
quad, 17x27, four greys, white outline, 32x29 light/dark/mask planes). Layout, big-endian:
  "FCP1", u16 offset of the portraits, u8 names, then per name: u8 portrait, the speaker's
  name as the scripts write it, NUL; even-aligned, the portraits (348 bytes each).
Generic speakers (a cadet, a knight) are left out: they keep the name-only box (CLAUDE.md,
dialogue portraits).
Written into opening/data and campaign/data.
"""
from pathlib import Path
import sys

import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import extract    # noqa: E402
import interface  # noqa: E402

PORTRAITS = [('Ramza', 'RAMUZA2'), ('Delita', 'DILY2'), ('Agrias', 'AGURI'),
             ('Gafgarion', 'H61'), ('Ovelia', 'HIME'), ('Simon', 'SIMON')]
# Script names of the same characters (Orbonne's chapel names them before they introduce
# themselves; the priest at Orbonne is Simon).
ALIASES = {'Princess Ovelia': 'Ovelia', 'Female Knight': 'Agrias', 'Black knight': 'Gafgarion',
           'Priest': 'Simon'}


def portrait(disc, files, spr_name):
    spr = disc.read(*files['BATTLE/%s.SPR' % spr_name])
    pal = np.frombuffer(spr[8 * 32:9 * 32], '<u2').astype(int)
    packed = np.frombuffer(spr[0x8200:0x9200], np.uint8).reshape(32, 128)
    sheet = np.empty((32, 256), np.uint8)
    sheet[:, ::2] = packed & 15; sheet[:, 1::2] = packed >> 4
    source = np.rot90(sheet[:, 80:128])
    scaled = np.array(Image.fromarray(source).resize((17, 27), Image.Resampling.NEAREST))
    im = np.full((29, 32), -1, dtype=int)
    im[1:28, 1:18] = interface.greys(scaled, pal)
    interface.outline(im)
    return bytes(interface.planes(im, 32))


def main():
    disc, files = extract.disc_files(sys.argv[1])
    faces = [portrait(disc, files, spr) for _, spr in PORTRAITS]
    ramza = (HERE / 'opening/data/ffoi.bin').read_bytes()[:348]   # its battle portrait 0
    assert faces[0] == ramza, 'portrait conversion differs from the battle bank'
    index = {name: i for i, (name, _) in enumerate(PORTRAITS)}
    names = list(index.items()) + [(alias, index[name]) for alias, name in ALIASES.items()]
    out = bytearray(b'FCP1') + bytes(2) + bytes([len(names)])   # u16: the portraits' offset
    for name, i in names:
        out += bytes([i]) + name.encode('ascii') + b'\0'
    if len(out) & 1:
        out.append(0)
    out[4:6] = len(out).to_bytes(2, 'big')
    out += b''.join(faces)
    for chapter in ('opening', 'campaign'):
        (HERE / chapter / 'data/ffcp.bin').write_bytes(out)
    print('ffcp: %d portraits, %d names, %d bytes' % (len(faces), len(names), len(out)))


if __name__ == '__main__':
    main()
