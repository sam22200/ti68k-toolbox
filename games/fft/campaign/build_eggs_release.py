#!/usr/bin/env python3
"""Build changed FSV7 core programs; reuse all chapter art/content from prior source.

Overlays must already be exported to active data/ by module.py. This does not
run a broad test sweep, re-extract maps/scenes or overwrite historical releases.
"""
import hashlib
import argparse
import json
from pathlib import Path
from check_flow import HERE,ROOT,run
import chapter_packages as packages


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--only',nargs='+',choices=['fftbat','fftscn','fftdep','fftmap','fftmng','ffttitle','fftstory'],help='rebuild selected changed entries; reuse other private binaries')
    args=ap.parse_args()
    out=HERE/'x/eggs';packed=out/'packed';packed.mkdir(parents=True,exist_ok=True)
    if args.only:
        required=['fftstory.89z']+[n+'.89z'for n in ('fftbat','fftscn','fftdep','fftmap','fftmng','ffttitle')]+[n+'.89y'for n in ('ffbcore','ffscore','ffpccore','ffwmcore','ffmgcore','ffttcore')]
        if any(not(packed/n).is_file()for n in required):raise ValueError('--only requires an existing complete private build')
    core=ROOT/'runtime/core/rt_core.c';unpack=ROOT/'lib/unpack68k.s';tile=ROOT/'tools/extgraph/lib/tilemap.a'
    for program,companion,source,extra in [
        ('fftbat','ffbcore','native_battle.c',[core,unpack,tile]),
        ('fftscn','ffscore','native_scene.c',[core,unpack,tile]),
        ('fftdep','ffpccore','native_deploy.c',[core,tile]),
        ('fftmap','ffwmcore','native_worldmap.c',[core,unpack,tile]),
        ('fftmng','ffmgcore','native_manage.c',[core,tile]),
        ('ffttitle','ffttcore','native_title.c',[core,unpack,tile])]:
        if args.only and program not in args.only:continue
        log=run([ROOT/'tools/bin/ti-cc','-o',program,'-pack',companion,HERE/source,*extra],packed)
        (packed/(program+'.log')).write_text(log);print(program+': '+log.strip()[-600:],flush=True)
    if not args.only or 'fftstory' in args.only:
        (packed/'fftstory.log').write_text(run([ROOT/'tools/bin/ti-cc','-o','fftstory',HERE/'launcher.c'],packed))
    source=packages.read_group(HERE/'x/story-choices/release/source.89g')
    changed=[]
    for file in sorted(packed.glob('*.89[zy]')):
        variable=next(iter(packages.read_group(file).values()))
        source[variable.name]=packages.Variable(variable.name,variable.kind,3,variable.payload,variable.folder)
        changed.append(variable.name)
    # The supervisor stays in RAM, while packed entries and their data archive.
    variable=source['fftstory'];source['fftstory']=packages.Variable(variable.name,variable.kind,0,variable.payload)
    for name in ('ffbm','ffbn','ffbo','ffwb','ffeg'):
        path=(HERE/'data'/(name+'.bin')) if name.startswith('ffb') else out/(name+'.bin')
        source[name]=packages.Variable(name,0x1c,3,path.read_bytes()+packages.resources.TRAILER);changed.append(name)
    release=out/'release';release.mkdir(exist_ok=True)
    (release/'source.89g').write_bytes(packages.group_bytes(source.values()))
    spec=json.loads((HERE/'packages/core.json').read_text())
    chapter=json.loads((HERE/'packages/chapter-01.json').read_text())
    report,_,_=packages.build(source,spec,[chapter],release)
    shipping=out/'shipping';shipping.mkdir(exist_ok=True)
    for group in ('fft-core.89g','chapter-01.89g'):
        for variable in packages.read_group(release/group).values():
            (shipping/(variable.name+'.89y' if variable.kind==0x1c else variable.name+'.89z')).write_bytes(
                packages.group_bytes([variable],single=True))
            if variable.kind==0x1c and variable.payload.endswith(packages.resources.TRAILER):
                (shipping/(variable.name+'.bin')).write_bytes(variable.raw)
    (out/'release-build.json').write_text(json.dumps(dict(changed=changed,report=report,
        source_sha256=hashlib.sha256((release/'source.89g').read_bytes()).hexdigest()),indent=2)+'\n')
    print(json.dumps(dict(core_key=report['core_key'],core_bytes=report['core']['bytes'],chapter_bytes=report['chapters'][0]['bytes']),indent=2))


if __name__=='__main__':main()
