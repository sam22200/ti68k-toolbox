#!/usr/bin/env python3
"""PAL aim lock/unlock and egg rebound probes after a real capture/conversion."""
import json
from reference import OUT,ROM
from actors_reference import prepare
from art import SNES,PAD

def sample(snes,f):
    actors=[]
    for off in range(0,96,4):
        state=snes.read(0x700f00+off,2)
        ident=snes.read(0x701360+off,2)
        if state and ident in (0x23,0x24,0x25):
            actors.append({'slot':off,'state':state,'id':ident,
                'x':snes.read(0x7010e2+off,2),'y':snes.read(0x701182+off,2),
                'vx':snes.read(0x701220+off,2),'vy':snes.read(0x701222+off,2)})
    return {'frame':f,'aim':snes.read(0x7000de,2),'angle':snes.read(0x7000ee,2),
        'locked':snes.read(0x7000ea,2),'eggs':actors}

def main():
    directory=OUT/'items'; directory.mkdir(exist_ok=True)
    s=SNES(ROM); results={}
    try:
        for name,presses in [('lock',{65:'A',80:'L',96:'R',120:'A'}),
                             ('down',{65:'A',74:'A'}),('diagonal',{65:'A',82:'A'})]:
            prepare(s,True); rows=[]
            for f in range(300):
                buttons=['Y'] if f==0 else ['DOWN'] if 20<=f<60 else [presses[f]] if f in presses else []
                s.pressed=sum(1<<PAD[b] for b in buttons); s.lib.retro_run()
                rows.append(sample(s,f))
            results[name]=rows
            print(name,[r for f,r in enumerate(rows) if f>=65 and
                       (r['locked']!=rows[f-1]['locked'] or
                        [(e['id'],bool(e['vx'])) for e in r['eggs']] !=
                        [(e['id'],bool(e['vx'])) for e in rows[f-1]['eggs']])])
    finally:s.close()
    assert len({r['angle'] for r in results['lock'][81:96]})==1
    assert len({r['angle'] for r in results['lock'][97:110]})>1
    for name in ('down','diagonal'):
        rows=results[name]
        f=next(f for f,r in enumerate(rows) if any(e['id']==0x24 for e in r['eggs']))
        before=next(e for e in rows[f-1]['eggs'] if e['id']==0x25)
        after=next(e for e in rows[f]['eggs'] if e['id']==0x24)
        vy=(after['vy']^32768)-32768
        assert before['vy']>0 and vy<0 and abs(vy+before['vy'])<=1
    (directory/'aim.json').write_text(json.dumps({'source':json.loads((OUT/'start.json').read_text()),'cases':results},indent=2)+'\n')

if __name__=='__main__':main()
