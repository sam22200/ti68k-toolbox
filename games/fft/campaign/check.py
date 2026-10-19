#!/usr/bin/env python3
"""Actual 68000 Academy/Gariland pixels and portable state; no emulator timings."""
from pathlib import Path
import subprocess,re,json,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];OUT=HERE/'x/check'
def run(args,cwd=HERE/'data'):
    p=subprocess.run(list(map(str,args)),cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if p.returncode:print(p.stdout);p.check_returncode()
    return p.stdout
def main():
    OUT.mkdir(parents=True,exist_ok=True)
    empty=OUT/'none.keys';empty.write_text('0\n')
    advance=OUT/'advance.keys';advance.write_text(''.join(f'{f} ENTER\n{f+1}\n' for f in range(30,1001,15)))
    rotate=OUT/'rotate.keys';rotate.write_text('12 F5\n13\n50 F1\n51\n')
    fast=OUT/'fast.keys';fast.write_text('0 A\n')
    status=OUT/'status.keys';status.write_text('1 UP\n2\n3 UP\n4\n5 ENTER\n6\n30 B\n31\n')
    results=[]
    for name,flags,banks,cases in (
        ('academy',[],['ffac'],[('text',0,90,empty),('complete',0,1000,advance)]),
        ('gariland',['-DCHECK_GAR'],['ffgv0','ffgv1','ffgv2','ffgv3','fftu','fftw','ffgi'],
         [('manual',61,90,empty),('rotations',61,100,rotate),('ai',16,9500,fast),('status',61,90,status)])):
        if '--status-only' in sys.argv:
            cases=[c for c in cases if c[0]=='status']
            if not cases:continue
        if '--academy-only' in sys.argv and name!='academy':continue
        common=[HERE/'check_native.c',ROOT/'runtime/core/rt_core.c']
        run(['cc','-std=gnu99','-O1',*flags,'-o',OUT/name,*common,ROOT/'runtime/platform-sw/rt_sw.c'])
        run([ROOT/'tools/bin/ti-cc','-o',OUT/(name+'c'),'-DRT_CYCLES',*flags,*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        for case,scenario,frames,keys in cases:
            pc=run([OUT/name,scenario,frames,keys,OUT/(name+'-'+case+'-pc.png')])
            command=[ROOT/'tools/bin/ti-cycles','--max',10000,'--arg',scenario,'--frames',frames,'--keys',keys,'--png',OUT/(name+'-'+case+'-ti.png')]
            for bank in banks:command+=['--file',HERE/'data'/(bank+'.89y')]
            ti=run([*command,OUT/(name+'c.89z')]);(OUT/(name+'-'+case+'.log')).write_text(ti)
            executed=int(re.search(r'executed (\d+)',pc)[1]);checksum=re.search(r'checksum (\w+)',pc)[1];state=int(re.search(r'state (\w+)',pc)[1],16)
            assert checksum.lower()==re.search(r'checksum\s+(\w+)',ti)[1].lower(),(name,case,'pixels')
            values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
            assert values[-3:]==[executed,0xacad0001,state],(name,case,'state',values,pc)
            row=dict(chapter=name,case=case,executed_frames=executed,checksum=checksum,state_hash=state)
            results.append(row);print(json.dumps(row),flush=True)
    (OUT/('status.json' if '--status-only' in sys.argv else 'academy.json' if '--academy-only' in sys.argv else 'checked.json')).write_text(json.dumps(results,indent=2)+'\n')
    print('Campaign: %d actual-68000 pixels/state replays passed'%len(results))
if __name__=='__main__':main()
