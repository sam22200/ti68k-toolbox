#!/usr/bin/env python3
"""Rebuild only the changed management/deployment children; reuse the egg release."""
import hashlib
import json
from check_flow import HERE,ROOT,run
import chapter_packages as packages
import monster_formation_bank as art


def main():
    out=HERE/'x/monster-formation';packed=out/'packed';packed.mkdir(parents=True,exist_ok=True)
    raw=art.from_review((HERE/'x/monster-family-review/ffmr.bin').read_bytes())
    for program,companion,source in [('fftmng','ffmgcore','native_manage.c'),('fftdep','ffpccore','native_deploy.c')]:
        log=run([ROOT/'tools/bin/ti-cc','-o',program,'-pack',companion,HERE/source,
                 ROOT/'runtime/core/rt_core.c',ROOT/'tools/extgraph/lib/tilemap.a'],packed)
        (packed/(program+'.log')).write_text(log);print(log.strip(),flush=True)
    source=packages.read_group(HERE/'x/eggs/release/source.89g')
    for path in packed.glob('*.89[zy]'):
        v=next(iter(packages.read_group(path).values()));source[v.name]=packages.Variable(v.name,v.kind,3,v.payload,v.folder)
    source['ffma']=packages.Variable('ffma',0x1c,3,raw+packages.resources.TRAILER)
    release=out/'release';release.mkdir(exist_ok=True)
    (release/'source.89g').write_bytes(packages.group_bytes(source.values()))
    spec=json.loads((HERE/'packages/core.json').read_text());chapter=json.loads((HERE/'packages/chapter-01.json').read_text())
    report,_,_=packages.build(source,spec,[chapter],release)
    shipping=out/'shipping';shipping.mkdir(exist_ok=True)
    for group in ('fft-core.89g','chapter-01.89g'):
        for v in packages.read_group(release/group).values():
            (shipping/(v.name+('.89y'if v.kind==0x1c else'.89z'))).write_bytes(packages.group_bytes([v],single=True))
            if v.kind==0x1c and v.payload.endswith(packages.resources.TRAILER):(shipping/(v.name+'.bin')).write_bytes(v.raw)
    proof=dict(report=report,source_sha256=hashlib.sha256((release/'source.89g').read_bytes()).hexdigest())
    (out/'release-build.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(dict(core_key=report['core_key'],core_bytes=report['core']['bytes'],chapter_bytes=report['chapters'][0]['bytes']),indent=2))


if __name__=='__main__':main()
