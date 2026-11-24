#!/usr/bin/env python3
"""Build a private width-capable core candidate while preserving the ABI97 release.

Reuse unchanged cold programs/assets from an explicit baseline source group;
rebuild all29 matching images and the actual packed battle entry. This is a
build/envelope check, not CPU, installed-RAM or full-chapter acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
sys.path.insert(0,str(HERE.parent/'tools'))
import chapter_packages as packages


def run(args,cwd,log):
    p=subprocess.run(list(map(str,args)),cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    log.write_text(p.stdout)
    if p.returncode:print(p.stdout[-2500:]);p.check_returncode()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline',type=Path,default=HERE/'x/chapter1-ready/package-source.89g')
    parser.add_argument('--out',type=Path,default=HERE/'x/wide-core')
    args=parser.parse_args();baseline=args.baseline.resolve();out=args.out.resolve()
    assert not baseline.is_relative_to(out),'Never overwrite the baseline proof tree'
    out.mkdir(parents=True,exist_ok=True)
    mods=out/'modules';native=out/'native'
    mods.mkdir(exist_ok=True);native.mkdir(exist_ok=True)
    run([sys.executable,HERE.parent/'tools/module.py',mods/'ffbm.bin',HERE/'battle.c',29,
         '--second',mods/'ffbn.bin','--third',mods/'ffbo.bin','--fourth',mods/'ffbp.bin',
         '--companion',27,'--companion-peer',28],ROOT,out/'modules-build.log')
    wrapper=native/'bridge.c'
    wrapper.write_text('#include "'+str(HERE/'native_battle.c')+'"\n')
    run([ROOT/'tools/bin/ti-cc','-DFFT_MODULE_HEADER="'+str(mods/'modules.h')+'"',
         '-o','fftbat','-pack','ffbcore',wrapper,ROOT/'runtime/core/rt_core.c',
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'],native,native/'build.log')
    source=packages.read_group(baseline)
    for name in ('ffbm','ffbn','ffbo','ffbp'):
        old=source[name]
        source[name]=packages.Variable(name,old.kind,old.attr,(mods/(name+'.bin')).read_bytes()+b'\0dat\0\xf8')
    for name,suffix in (('fftbat','.89z'),('ffbcore','.89y')):
        v=packages.read_group(native/(name+suffix))[name]
        source[name]=packages.Variable(name,v.kind,3,v.payload)
    packed=source['ffbcore'].payload
    assert packed[2:4]==b'TP'
    size=struct.unpack_from('<H',packed)[0];assert size<=64936
    core=json.loads((HERE/'packages/core.json').read_text())
    chapter=json.loads((HERE/'packages/chapter-01.json').read_text())
    report,_,_=packages.build(source,core,[chapter],out/'packages')
    (out/'source.89g').write_bytes(packages.group_bytes(list(source.values())))
    result=dict(candidate_only=True,performance_accepted=False,hardware_ram_measured=False,
                baseline_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),
                core_key=report['core_key'],core_transport=report['core']['bytes'],
                chapter_transport=report['chapters'][0]['bytes'],ttpack_input=size,ttpack_headroom=64936-size,
                rebuilt=['fftbat','ffbcore','ffbm','ffbn','ffbo','ffbp'],
                source_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in
                    [HERE.parent/n for n in ('fft.c','fft.h','module.h','core_limits.h','view_format.h',
                                             'chapter_files.inc','scene_buffer.h','scene_buffer.inc','bootstrap_files.inc',
                                             'unit_blit.inc','monster_ko.inc','save.inc')]})
    (out/'build.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__':main()
