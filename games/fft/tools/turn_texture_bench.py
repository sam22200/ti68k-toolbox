#!/usr/bin/env python3
"""Native textured rotation: same angles, PC/TI parity, texture-off geometry control."""
import json
from pathlib import Path
import shutil
import subprocess

import turn_audit as audit
import camera_cuts as demo
from turn_texture_assets import prepare

HERE=Path(__file__).resolve().parents[1]
OUT=HERE/'x/turn-texture'


def stage(data,assets):
    data.mkdir(parents=True,exist_ok=True)
    for p in (HERE/'x/tiemu-camera/data').iterdir():
        if p.suffix in ['.bin','.89y']: shutil.copy2(p,data/p.name)
    for p in assets.glob('*.bin'):
        shutil.copy2(p,data/p.name)
        subprocess.run([str(audit.ROOT/'tools/gcc4ti-bin/bin/ttbin2oth'),'-89','dat',p.name,p.stem],cwd=data,check=True,stdout=subprocess.DEVNULL)
        variable=data/(p.stem+'.89y'); raw=bytearray(variable.read_bytes()); raw[0x49]=3; variable.write_bytes(raw)


def main():
    assets=OUT/'assets'; manifest=prepare(assets)
    flags=audit.FLAGS.copy()
    for name,extra in [('flat',['-DFFT_TURN_TEXTURE','-DFFT_TURN_TEXTURE_FLAT']),('textured',['-DFFT_TURN_TEXTURE'])]:
        audit.OUT=OUT/name; audit.FLAGS=flags+extra
        stage(audit.OUT/'data',assets); audit.main()
    a=json.loads((OUT/'flat/headless.json').read_text()); b=json.loads((OUT/'textured/headless.json').read_text())
    for x,y in zip(a['cases'],b['cases']): assert x['frames']==y['frames']==4
    # Both variants must retain exactly the same full-resolution settled views.
    left=(OUT/'flat/pc.csv').read_text().splitlines(); right=(OUT/'textured/pc.csv').read_text().splitlines()
    assert all(x==y for x,y in zip(left,right) if x.split(',')[1]=='4')
    result=dict(assets=manifest,scope='Same native mesh/angles/depth/actors, flat versus affine textured; each variant matches 32 PC/TI frames. Cold destination included.',
        cases=[dict(case=x['case'],flat_cpu_ms=x['cpu_only_ms_at_12mhz'],textured_cpu_ms=y['cpu_only_ms_at_12mhz'],
                    texture_delta_cpu_ms=y['cpu_only_ms_at_12mhz']-x['cpu_only_ms_at_12mhz']) for x,y in zip(a['cases'],b['cases'])])
    (OUT/'comparison.json').write_text(json.dumps(result,indent=2)+'\n')
    demo.OUT=OUT/'demo'; demo.FLAGS+=['-DFFT_TURN_TEXTURE']; stage(demo.OUT/'data',assets); demo.main()
    print(json.dumps(result,indent=2))


if __name__=='__main__': main()
