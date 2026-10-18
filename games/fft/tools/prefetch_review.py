#!/usr/bin/env python3
"""Measure the actual TI idle slices and compare cold/prepared renderings to the PC."""
import csv
import hashlib
import json
import re
import shutil
import config_review as config
import turn_audit as audit

OUT=audit.HERE/'x/prefetch'

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    data=OUT/'data'; data.mkdir(exist_ok=True)
    retired=OUT/'retired-banks'; retired.mkdir(exist_ok=True)
    for p in data.iterdir():
        if p.suffix in ('.bin','.89y') and p.stem not in config.NATIVE_BANKS+('ffttex','fftiso'):
            shutil.move(str(p),str(retired/p.name))
    for p in (config.OUT/'data').iterdir():
        if p.suffix in ('.bin','.89y'): shutil.copy2(p,data/p.name)
    flags=config.FLAGS
    common=['../../runtime/core/rt_core.c','../../lib/unpack68k.s','../../tools/extgraph/lib/tilemap.a']
    audit.run(['cc','-std=gnu99','-O1','-g',*flags,'-o',OUT/'oracle','tools/prefetch_check.c',
               '../../runtime/core/rt_core.c','../../runtime/platform-sw/rt_sw.c'])
    pc=audit.run([OUT/'oracle'],data); (OUT/'pc.csv').write_text(pc)
    expected=[r[2].lower() for r in csv.reader(pc.splitlines()) if r[0]=='shot']
    assert len(expected)==34 and all(expected[i]==expected[i+1] for i in range(0,34,2))
    audit.run([audit.ROOT/'tools/bin/ti-cc','-o',OUT/'prefchk',*flags,'-DRT_CYCLES',
               'tools/prefetch_check.c',*common])
    audit.run([audit.ROOT/'tools/bin/ti-cc','-o',OUT/'prefemu',*flags,
               'tools/prefetch_tiemu.c',*common])
    args=[audit.ROOT/'tools/bin/ti-cycles','--max',2000000]
    for p in sorted(data.glob('*.89y')): args+=['--file',p]
    log=audit.run([*args,OUT/'prefchk.89z'],data); (OUT/'cycles.log').write_text(log)
    actual=re.findall(r'shot \d+ checksum (\w+)',log)
    assert [v.lower() for v in actual]==expected and '_main returned' in log
    values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
    assert 0x7b000001 in values and not any(v>>24==0x7f for v in values)
    cases=[]
    for i,v in enumerate(values):
        if v>>24!=0x78: continue
        n=v&255; frames,total,worst,prepared=values[i+1:i+5]
        cold=values[i+6]
        cases.append(dict(case=n,facing=n>>2,view='wide' if (n>>1)&1 else 'close',
                          scenery='filled' if n&1 else 'textured',idle_frames=frames,
                          idle_total_cycles=total,idle_worst_cycles=worst,
                          entry_prepared_cycles=prepared,entry_cold_cycles=cold,
                          prepared_cpu_ms=prepared/12000,cold_cpu_ms=cold/12000))
    assert len(cases)==16
    memory=next(v&0xffffff for v in values if v>>24==0x7a)
    report=dict(scope='Datasheet-counted native 68000, CPU only; no grayscale, presenter or frame waits. Not TiEmu/hardware timing.',
                checked_frames=len(expected),checks='Cold/prepared equality, four facings, both zooms and appearances, live moved actor/pose, preserved ISO buffers/state, input yield, stale-view rejection and allocation fallback.',
                extra_heap_bytes=memory,cases=cases,
                bank_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(data.glob('*.89y'))},
                compact_work_bytes=next(v&0xffffff for v in values if v>>24==0x7c),
                timer_binary_sha256=hashlib.sha256((OUT/'prefemu.89z').read_bytes()).hexdigest(),
                source_sha256={name:hashlib.sha256((audit.HERE/name).read_bytes()).hexdigest() for name in
                    ('fft.c','perspective.inc','fixed.inc','prefetch.inc','tools/prefetch_check.c')})
    (OUT/'checked.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__': main()
