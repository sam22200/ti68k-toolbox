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
        changed=[];walk=[r for r in rows.splitlines() if r.startswith('walk ')]
        # Walk edges also differ from the legacy no-surface graph since the source step/surface
        # migration; core-step and core-surface check those against the original rules.
        for row in (r for r in rows.splitlines() if not r.startswith('walk ')):
            jump,origin,target,old,now,side,height,dh,steep,column=row.split()
            col=bytes.fromhex(column);original.bind(col)
            for layer in range(2):original.uc.mem_write(0x1d6000+layer*256,bytes((col[4*layer+3]&128,)))
            expected=1-original.call('horizontal_jump',0,int(side),int(side)^2,int(height))
            # battle_move_propagate_destination: a horizontal jump never lands higher (return 8)
            # and stays within Jump x2 (return 9).
            if int(dh)>0 or -int(dh)>2*int(jump):expected=0
            if int(steep):expected=0   # leaving/entering a steep panel uphill or flat (return 3)
            assert expected==int(now),(label,row,expected)
            changed.append(dict(jump=int(jump),origin=int(origin),target=int(target),before=int(old),after=int(now),
                                column=column,side=int(side),height=int(height),landing=int(dh),steep=int(steep),original=expected))
        results.append(dict(map=label,walk_edges_changed=len(walk),changed_edges=len(changed),removed=sum(not r['after']for r in changed),
                            added=sum(r['after']for r in changed),edges=changed))
        print(label,':',len(changed),'changed jump edges agree with original MIPS;',len(walk),'walk edges (core-step/surface)',flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')


if __name__=='__main__':main()
