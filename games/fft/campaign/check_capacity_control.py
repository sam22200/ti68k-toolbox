#!/usr/bin/env python3
"""Hold current data fixed; compare11-slot control with13-capacity render output.

Run check.py first to produce current campaign images. This is a PC visual
regression control; those current images have their own native equality checks.
"""
import hashlib
import json
from pathlib import Path
import tempfile
from check_flow import HERE,ROOT,data_links
from check import run


def main():
    out=HERE/'x/core-capacity/control11';out.mkdir(parents=True,exist_ok=True)
    run(['cc','-std=gnu99','-O1','-w','-DCHECK_GAR','-DFFT_CAPACITY_CONTROL11',
         '-o',out/'control',HERE/'check_native.c',ROOT/'runtime/core/rt_core.c',
         ROOT/'runtime/platform-sw/rt_sw.c'])
    keys=HERE/'x/check'
    cases=[('manual',61,90,'none'),('rotations',61,100,'rotate'),('ai',16,9500,'fast'),
           ('status',61,90,'status'),('reach',61,260,'reach'),
           ('reach-settings',61,200,'settings'),('reach-contour',61,70,'settings'),
           ('reach-menu',61,44,'settings'),('won',16,20000,'won')]
    results=[]
    with tempfile.TemporaryDirectory(prefix='fft-control11-')as name:
        folder=Path(name);data_links(folder)
        for label,scenario,frames,key in cases:
            picture=out/(label+'.png')
            run([out/'control',scenario,frames,keys/(key+'.keys'),picture],folder)
            current=keys/('gariland-'+label+'-pc.png')
            assert picture.read_bytes()==current.read_bytes(),label
            results.append(dict(case=label,identical=True,sha256=hashlib.sha256(picture.read_bytes()).hexdigest()))
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
    print('9 battle pictures identical between fixed11 control and13-capacity engine')


if __name__=='__main__':main()
