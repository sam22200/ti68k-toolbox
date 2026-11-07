#!/usr/bin/env python3
"""Actual compact geometry/search on PC, sanitizers and68000; no UI or assets."""
import argparse
import json
import re
from pathlib import Path
from check import HERE,ROOT,run


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'x/core-grid')
    out=ap.parse_args().out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=HERE/'check_tactical_grid.c'
    run(['cc','-std=gnu99','-O1','-g','-Wall','-Wextra','-o',out/'check',source])
    pc=run([out/'check'])
    run(['cc','-std=gnu99','-O1','-g','-fsanitize=address,undefined',
         '-fno-omit-frame-pointer','-o',out/'sanitized',source])
    sanitized=run([out/'sanitized']);assert sanitized==pc
    run([ROOT/'tools/bin/ti-cc','-o',out/'gridc',source])
    native=run([ROOT/'tools/bin/ti-cycles','--max',20000,out/'gridc.89z'])
    (out/'native.log').write_text(native);(out/'pc.log').write_text(pc)
    match=re.search(r'grid (\w+) failures 0 checks (\d+)',pc);assert match,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(match[1],16),0,int(match[2])],(pc,values)
    assert '_main returned'in native
    zone=next(line for line in native.splitlines()if re.match(r'^4\s+Compact node search',line))
    report=dict(checks=int(match[2]),hash=match[1],native_search_bytes=values[-5],
                native_grid_bytes=values[-4],native_search_zone=zone.strip(),
                scope='Compact identity, explicit layers/via links; fast costs1..3 and wide costs1..6, FIFO ties, yield/restart and all256 IDs. Long-jump actor spans/runtime graph hookup are not certified here.')
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    print(pc.strip());print(zone.strip())


if __name__=='__main__':main()
