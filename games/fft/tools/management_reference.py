#!/usr/bin/env python3
"""Reproduce the original deployment/status references using our existing boot door.

No manual save and no RAM pokes beyond oracle.boot's documented event skip.
Captures are reference PS1 pixels, never TI screenshots or redistributable assets.
"""
from pathlib import Path
import argparse
import hashlib
import json
import numpy as np

import oracle
import extract
import battle

HERE=Path(__file__).resolve().parents[1]
OUT=HERE/'x/management'


def main():
    ap=argparse.ArgumentParser(__doc__);ap.add_argument('disc')
    ap.add_argument('--icons-only',action='store_true');args=ap.parse_args()
    OUT.mkdir(parents=True,exist_ok=True)
    p=oracle.PSX(args.disc)
    if args.icons_only:
        p.load(str(OUT/'deployment.state'))
    else:oracle.boot(p)
    disc,files=extract.disc_files(args.disc);raw=disc.read(*files['EVENT/ITEM.BIN'])
    d=battle.Disc(args.disc)
    vram=np.frombuffer(bytes(p.vram),'<u2').reshape(512,1024)
    sheet=np.frombuffer(raw[:32768],'<u2').reshape(256,64)
    checks=[]
    for item in (1,0x13,0x9d,0xba,0xd0):
        rec=d.at(battle.ITEMS+12*item,12);sprite=rec[1]
        x=(sprite%15)*4;y=32+(sprite//15)*16
        assert np.array_equal(vram[256+y:272+y,896+x:900+x],sheet[y:y+16,x:x+4])
        checks.append(dict(item=item,source_sprite=sprite,palette=rec[0],matched_pixels=256))
    assert vram[254:256,640:768].tobytes()==raw[32768:]
    (OUT/'icon-source-check.json').write_text(json.dumps(dict(
        scope='Five selected original icons and all item palettes match loaded PS1 VRAM; the full atlas may contain overwritten map-name pixels.',
        matched_icon_pixels=1280,matched_palette_bytes=512,checks=checks,
        source_sha256=hashlib.sha256(raw).hexdigest()),indent=2)+'\n')
    if args.icons_only:
        print('1280 icon pixels and 512 palette bytes equal the original PS1 VRAM');return
    p.image().save(OUT/'original-deployment.png')
    p.save(str(OUT/'deployment.state'))
    sequence=[('deployment-clean','CROSS:45'),('status','SQUARE:60'),('deployment-search','CROSS:30 TRIANGLE:60')]
    for label,keys in sequence:
        values=oracle.taps(keys);oracle.run(p,len(values)+1,values)
        p.image().save(OUT/f'original-{label}.png')
    report=dict(entry='games/fft/tools/oracle.py:boot, new game with documented event skip',
                capture_scope='Deployment and original equipment/status panel; not a world-map Formation run.',
                source_sha256={n:hashlib.sha256((HERE/'tools'/n).read_bytes()).hexdigest()
                               for n in ('oracle.py','management_reference.py')},sequence=sequence)
    (OUT/'reference.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
