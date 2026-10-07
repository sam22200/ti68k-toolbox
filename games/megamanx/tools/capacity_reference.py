#!/usr/bin/env python3
"""Original buster allocation, charge presses and full-slot controlled probes."""
from reference import *
from measure import row
from combat_reference import actors

def sample(s):
    r=row(s);r['shots']=actors(s,0x1228,0x1428);return r

def main():
    directory=OUT/'capacity';directory.mkdir(exist_ok=True)
    s=SNES(ROM);cases=[]
    definitions=[('rapid',{f:(['Y'] if not f%2 else []) for f in range(60)},70),
                 ('three_medium',{0:['Y'],1:[],2:['Y'],3:[],4:['Y'],35:[]},75),
                 ('three_large',{0:['Y'],1:[],2:['Y'],3:[],4:['Y'],105:[]},145)]
    try:
        for name,keys,count in definitions:
            s.load(OUT/'start.state');initial=sample(s);held=[];samples=[]
            for f in range(count):
                if f in keys:held=keys[f]
                run(s,held);samples.append(sample(s))
            cases.append(dict(name=name,initial=initial,keys=keys,samples=samples,isolation='Natural inputs only.'))
            print(name,'max active',max(len(r['shots']) for r in samples),'releases',[(f,[(p['slot'],p['kind']) for p in r['shots']]) for f,r in enumerate(samples) if any(p['kind'] for p in r['shots'])][:2],flush=True)
        s.load(OUT/'start.state')
        for f in range(5):run(s,['Y'] if not f%2 else [])
        assert len(actors(s,0x1228,0x1428))==3
        s.save(directory/'three.state')
        for hold in (31,101):
            s.load(directory/'three.state');samples=[]
            # Only projectile current/previous X is kept on-screen. Native
            # motion, charge, slot allocation and hero state remain untouched.
            for f in range(hold+7):
                for p in actors(s,0x1228,0x1428):
                    for offset in (5,0x22):s.write(0x7e0000+p['slot']+offset,200,2)
                run(s,['Y'] if f<hold-1 else []);samples.append(sample(s))
            cases.append(dict(name=f'anchored_{hold}',hold=hold,samples=samples,isolation='All existing shot current/previous X kept at200 before each update; no other RAM writes.'))
            assert all(len(r['shots'])==3 and not any(p['kind'] for p in r['shots']) for r in samples)
            assert samples[hold-1]['charge_state']==0
            print('anchored release',hold,[(p['slot'],p['kind']) for p in samples[hold-1]['shots']],flush=True)
        (directory/'reference.json').write_text(json.dumps(dict(cases=cases),indent=2)+'\n')
    finally:s.close()
if __name__=='__main__':main()
