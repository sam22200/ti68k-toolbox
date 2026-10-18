#!/usr/bin/env python3
"""Run actual cache code against the preserved, uncompressed blade dictionary."""
from pathlib import Path
import json
import re
import subprocess
import sys

HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[1]
OUT=HERE/'x/storage/blade-cache'


def run(args,cwd=HERE):
    return subprocess.check_output(list(map(str,args)),cwd=cwd,text=True)


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    raw=(HERE/'x/storage/before-blade-cache/fftw.bin').read_bytes()[20016:]
    assert len(raw)==40*432
    (OUT/'bladeraw.bin').write_bytes(raw)
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','bladeraw.bin','bladeraw'],OUT)
    run(['cc','-std=gnu99','-O1','-g','-o',OUT/'test','test_blade_cache.c',
         '../../runtime/core/rt_core.c','../../runtime/platform-sw/rt_sw.c'])
    # rt_file reads the native archive banks and the test-only raw fixture from one directory.
    import shutil
    for name in ('fftv0','fftv1','fftv2','fftv3','fftu','fftw','ffti'):
        for suffix in ('.bin','.89y'):
            shutil.copy2(HERE/(name+suffix),OUT/(name+suffix))
    print(run([OUT/'test'],OUT))
    run([ROOT/'tools/bin/ti-cc','-o',OUT/'bladechk','-DRT_CYCLES','test_blade_cache.c',
         '../../runtime/core/rt_core.c','../../lib/unpack68k.s','../../tools/extgraph/lib/tilemap.a'])
    args=[ROOT/'tools/bin/ti-cycles','--max','10000']
    for file in sorted(OUT.glob('*.89y')): args+=['--file',file]
    log=run([*args,OUT/'bladechk.89z'],OUT); (OUT/'cycles.log').write_text(log)
    values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
    assert 0xb3000001 in values and '_main returned' in log
    cases=[]
    for i,value in enumerate(values):
        if value>>24==0xb2:
            cases.append(dict(pose=value&255,cold_cycles=values[i+1],hit_cycles=values[i+2]))
    assert len(cases)==48 and max(c['hit_cycles'] for c in cases)<1000
    work=next(v&0xffffff for v in values if v>>24==0xb1)
    report=dict(checked_poses=48,cache_frame_bytes=432,cache_slots=3,cache_image_bytes=1296,native_work_bytes=work,
                native_work_delta=work-43192,
                worst_cold_cycles=max(c['cold_cycles'] for c in cases),
                worst_hit_cycles=max(c['hit_cycles'] for c in cases),cases=cases,
                measurement='Datasheet 68000 cycles, actual lookup plus markers; no emulator, grayscale or frame wait.')
    (OUT/'checked.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='cases'},indent=2))


if __name__=='__main__': main()
