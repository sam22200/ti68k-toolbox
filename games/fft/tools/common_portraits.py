#!/usr/bin/env python3
"""Shared FCP2 identity bank: exact dialogue/battle/management portraits and names.

usage: common_portraits.py DISC.cue
The seven-byte prefix retains u16 portrait offset and the speaker-alias count;
alias records remain (u8 portrait ID, NUL-terminated speaker name). Three byte
ID directories select Gariland, Orbonne and management pictures. Five u16 words
immediately before the even-aligned portrait data hold those three offsets,
the510-entry generic-name directory offset and the unique portrait count.
Portraits remain unchanged32x29 light/dark/mask records (348 bytes). Generic
name offsets are relative to their own directory. Records stay in Flash.
Written identically into opening/data and campaign/data.
"""
from pathlib import Path
import sys,struct

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
    import units,opening_assets,management_data
    from identity_assets import write_changed
    disc, files = extract.disc_files(sys.argv[1])
    faces = [portrait(disc, files, spr) for _, spr in PORTRAITS]
    def store(spr):
        raw=portrait(disc,files,spr)
        if raw not in faces:faces.append(raw)
        return faces.index(raw)
    gariland=[store(spr)for _,spr,_ in units.CHARS]
    orbonne=[store(opening_assets.CHARS[i][1])for i in (0,2,3,4,5,6,7,8)]
    management=[]
    for k in range(management_data.CAREERS+1):
        for gender in ('MW'if k<management_data.CAREERS else ' '):
            spr=management_data.SHEETS[k]+'_'+gender if k<management_data.CAREERS else 'RAMUZA'
            if 'BATTLE/'+spr+'.SPR'not in files:spr=spr.replace('_W','_M')if gender=='W'else spr.replace('_M','_W')
            management.append(store(spr))
    assert len(faces)<=255
    index = {name: i for i, (name, _) in enumerate(PORTRAITS)}
    names = list(index.items()) + [(alias, index[name]) for alias, name in ALIASES.items()]
    out = bytearray(b'FCP2') + bytes(2) + bytes([len(names)])
    for name, i in names:
        out += bytes([i]) + name.encode('ascii') + b'\0'
    directories=[]
    for ids in (gariland,orbonne,management):directories.append(len(out));out+=bytes(ids)
    if len(out)&1:out.append(0)
    meta=len(out);out+=bytes(10)
    out[4:6] = len(out).to_bytes(2, 'big')
    out += b''.join(faces)
    directories.append(len(out));names_at=len(out);out+=bytes(1020);canonical={}
    small=disc.read(*files['EVENT/SMALL.OUT'])
    entries=small[0xec+struct.unpack_from('<I',small,0x70)[0]:].replace(b'\xff',b'\xfe').split(b'\xfe')
    people=[interface.decode(raw)for first in (256,512)for raw in entries[first:first+255]]
    for i,name in enumerate(people):
        raw=name.encode('ascii')+b'\0'
        if raw not in canonical:canonical[raw]=len(out)-names_at;out+=raw
        struct.pack_into('>H',out,names_at+2*i,canonical[raw])
    struct.pack_into('>5H',out,meta,*directories,len(faces))
    for chapter in ('opening', 'campaign'):
        write_changed(HERE/chapter/'data/ffcp.bin',out)
        from identity_assets import compact,MARK
        h=HERE/chapter/'data/interface.h'
        if h.exists()and MARK in h.read_text():compact(chapter)
    print('ffcp: %d unique portraits, %d aliases,510 generic names,%d bytes'%(len(faces),len(names),len(out)))


if __name__ == '__main__':
    main()
