#!/usr/bin/env python3
"""Compare actual 68000 physics/projectiles directly with original SNES RAM."""
import json,re,subprocess
from reference import GAME,ROOT,OUT
from schema import WORDS
def execute(scenario,keys,count):
    text=subprocess.run([str(ROOT/'tools/bin/ti-cycles'),'--arg',str(scenario),'--frames',str(count),
        '--keys',str(keys),'--file','mmxmap.89y','--file','mmxart.89y','mxref.89z'],cwd=GAME,
        capture_output=True,text=True,check=True).stdout
    values=[int(v)&65535 for v in re.findall(r'^value: (-?\d+)',text,re.M)]
    assert len(values)==count*WORDS+1 and values[-1]==count
    return [values[i*WORDS:(i+1)*WORDS] for i in range(count)]
def main():
    ref=json.loads((OUT/'measure/reference.json').read_text());directory=GAME/'x/original';directory.mkdir(exist_ok=True)
    cases=[dict(c,scenario=101) for c in ref['movement']]
    cases+=[dict(c,name='wall_'+c['name'],scenario=102) for c in ref['walls']]
    cases+=[dict(name='hurt',scenario=107,keys={0:[]},samples=ref['damage'][287:377])]
    frames=0
    for case in cases:
        keys=directory/(case['name']+'.txt')
        translate={'B':'A','Y':'B'}
        keys.write_text(''.join(str(f)+' '+' '.join(translate.get(k,k) for k in held)+'\n' for f,held in case['keys'].items()))
        rows=execute(case['scenario'],keys,len(case['samples']))
        for f,(ti,r) in enumerate(zip(rows,case['samples'])):
            actual=[ti[i] for i in (0,1,5,6,7,8,9)]
            expected=[r[k]&65535 for k in ('x','y','vx','vy','xs','ys','state')]
            assert actual==expected,(case['name'],f,actual,expected)
        frames+=len(rows);print(case['name'],len(rows),'original states match actual 68000',flush=True)
    shot_frames=0
    for hold in (1,45,110):
        case=next(c for c in ref['shots'] if c['hold']==hold)
        keys=directory/f'shot-{hold}.txt';keys.write_text(f'0 B\n{hold}\n')
        count=hold+24 if hold>1 else 24;rows=execute(101,keys,count)
        start=hold if hold>1 else 0
        for f,(ti,r) in enumerate(zip(rows[start:],case['samples'][start:])):
            p=r['shots'][0];actual=[ti[i] for i in (28,29,30,31,32)]
            expected=[p[k]&65535 for k in ('x','y','vx','xs','kind')]
            assert actual==expected,(hold,f,actual,expected)
            assert ti[33]==1
            shot_frames+=1
    (GAME/'x/original-ti.json').write_text(json.dumps(dict(movement_recoil_states=frames,projectile_states=shot_frames),indent=2)+'\n')
    print(frames,'original motion/recoil states and',shot_frames,'projectile states match actual 68000')
if __name__=='__main__':main()
