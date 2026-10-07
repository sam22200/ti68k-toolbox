#!/usr/bin/env python3
"""Original roller timelines on the compiled TI engine, one update per frame."""
import json
from reference import GAME,OUT
from check_original_ti import execute
from enemy_header import observations
def main():
    cases=json.loads((OUT/'enemy/reference.json').read_text())['cases'];results=[];total=0
    directory=GAME/'x/original';directory.mkdir(exist_ok=True)
    for case in cases:
        name=case['name'];translate={'B':'A','Y':'B'}
        keys=directory/f'enemy-{name}.txt'
        keys.write_text('\n'.join(str(f)+' '+' '.join(translate.get(k,k) for k in values) for f,values in case['keys'].items())+'\n')
        scenario=17 if name=='charged_release' else 18 if name=='medium_release' else 16
        rows=execute(scenario,keys,len(case['samples']));count=0
        for f,active,x,y,xs,vx,hp,phase,fuse,player_hp in observations(case):
            ti=rows[f]
            got=[ti[56]!=0,ti[52],ti[53],ti[54],ti[62],ti[55],ti[57],ti[64],ti[17]]
            assert got[0]==active and got[-1]==player_hp and (not active or got[1:8]==[x,y,xs,vx&65535,hp,phase,fuse]),(name,f,got,[active,x,y,xs,vx,hp,phase,fuse,player_hp])
            count+=1
        results.append(dict(case=name,samples=count));total+=count;print(name,count,'natural enemy observations pass on68000',flush=True)
    report=dict(samples=total,cases=results,fields='Active enemy X/Y/fraction/VX/HP/phase/fuse and player HP; source culling differs from native viewport, explicit deaths checked.')
    (GAME/'x/enemy-ti.json').write_text(json.dumps(report,indent=2)+'\n');print(total,'original enemy observations pass')
if __name__=='__main__':main()
