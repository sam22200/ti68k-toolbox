#!/usr/bin/env python3
"""Every changed current-map jump edge must agree with original instructions."""
import json,sys,tempfile
from pathlib import Path
from check_flow import HERE,ROOT,data_links
from check import run
sys.path.insert(0,str(HERE.parent/'tools'))
import jump_surface_oracle
import surface_data as surface


def main():
    out=HERE/'x/core-jump/graph';out.mkdir(parents=True,exist_ok=True)
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'graph',HERE/'check_jump_graph.c',
         ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
    original=surface.Original(surface.battle.Disc(str(surface.core_audit.DEFAULT_DISC)))
    original.write(0x8018f4f0,0x801d6000);original.uc.mem_write(0x1d001b,b'\x06')
    results=[]
    for label,scenario in [('orbonne',101),('gariland',201)]:
        with tempfile.TemporaryDirectory(prefix='fft-jump-graph-')as name:
            data=Path(name);data_links(data);rows=run([out/'graph',scenario],data)
        changed=[]
        for row in rows.splitlines():
            jump,origin,target,old,now,side,height,column=row.split()
            col=bytes.fromhex(column);original.bind(col)
            for layer in range(2):original.uc.mem_write(0x1d6000+layer*256,bytes((col[4*layer+3]&128,)))
            expected=1-original.call('horizontal_jump',0,int(side),int(side)^2,int(height))
            assert expected==int(now),(label,row,expected)
            changed.append(dict(jump=int(jump),origin=int(origin),target=int(target),before=int(old),after=int(now),
                                column=column,side=int(side),height=int(height),original=expected))
        results.append(dict(map=label,changed_edges=len(changed),removed=sum(not r['after']for r in changed),
                            added=sum(r['after']for r in changed),edges=changed))
        print(label,':',len(changed),'changed jump edges agree with original MIPS',flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')


if __name__=='__main__':main()
