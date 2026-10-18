#!/usr/bin/env python3
"""Verify actual auto-battle UI, tactical signatures and native frame costs."""
import json
from pathlib import Path
import re
import subprocess

HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[1]
OUT=HERE/'x/auto-battle'


def run(args):
    return subprocess.check_output(list(map(str,args)),cwd=HERE,text=True)


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    common=['../../runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-o',OUT/'oracle','tools/auto_battle_check.c',
         *common,'../../runtime/platform-sw/rt_sw.c'])
    run([ROOT/'tools/bin/ti-cc','-o',OUT/'autocheck','-DRT_CYCLES','-DFFT_MEASURE',
         'tools/auto_battle_check.c',*common,'../../lib/unpack68k.s','../../tools/extgraph/lib/tilemap.a'])
    base=[ROOT/'tools/bin/ti-cycles','--max',10000]
    for name in ('fftv0','fftv1','fftv2','fftv3','fftu','fftw','ffti'):base+=['--file',name+'.89y']
    cancel=OUT/'cancel.txt';cancel.write_text((HERE/'keys/auto-unit.txt').read_text()+'14 ESC\n15\n')
    records=[]
    cases=[('submenu',9,HERE/'keys/auto-unit.txt'),('thinking',30,HERE/'keys/auto-unit.txt'),
           ('cancel',80,cancel),('unit',1200,HERE/'keys/auto-unit.txt'),
           ('party',16000,HERE/'keys/auto-party.txt')]
    for name,frames,keys in cases:
        pc=run([OUT/'oracle',10,frames,keys,OUT/(name+'.png')])
        ti=run([*base,'--arg',10,'--frames',frames,'--keys',keys,OUT/'autocheck.89z'])
        (OUT/(name+'.log')).write_text(ti)
        assert '_main returned' in ti
        assert re.search(r'checksum (\w+)',pc)[1].lower()==re.search(r'checksum (\w+)',ti)[1].lower(),name
        expected=[int(s,16) for s in re.findall(r'STATE,(\w+)',pc)]
        values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
        at=values.index(0xAB700001)
        state_size,work_size=values[at+1:at+3]
        assert values[at+3:]==expected,(name,values[at+3:],expected)
        costs=values[:2*frames]
        assert len(costs)==2*frames
        ordinary=[c for m,c in zip(costs[::2],costs[1::2]) if not m>>31]
        peak=max(ordinary)
        assert peak<=360000,(name,peak)
        if name=='submenu':assert expected[3]>>24==14
        if name=='cancel':assert expected[2]==0 and (expected[3]>>16)&255==0
        if name=='unit':assert expected[2]==1<<6
        if name=='party':assert expected[3]&255 in (1,2)
        records.append(dict(case=name,frames=frames,ordinary_peak_cycles=peak,
                            native_state_bytes=state_size,native_work_bytes=work_size,
                            state_summary=expected,checksum=re.search(r'checksum (\w+)',pc)[1]))
    report=dict(checks=records,matched_pc_ti_checkpoints=len(records),
                native_program_file_bytes=(HERE/'fft.89z').stat().st_size,
                measurement='MC68000 datasheet cycles; actual render/update loop, no emulator or grayscale wait.')
    (OUT/'checked.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
