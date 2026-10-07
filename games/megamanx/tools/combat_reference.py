#!/usr/bin/env python3
"""Exact charge boundaries and unmodified Highway enemy/contact timelines."""
from reference import *
from measure import row

def actors(s,start,end):
    values=[]
    for a in range(start,end,64):
        if s.ram[a]:
            values.append(dict(slot=a,active=s.ram[a],state=s.ram[a+2],kind=s.ram[a+10],
                x=s.read(0x7e0000+a+5,2),y=s.read(0x7e0000+a+8,2),
                hp=s.ram[a+0x27],vx=s.read(0x7e0000+a+0x1a,2,True),
                vy=s.read(0x7e0000+a+0x1c,2,True),xs=s.ram[a+4],ys=s.ram[a+7]))
    return values

def main():
    directory=OUT/'combat';directory.mkdir(exist_ok=True)
    s=SNES(ROM);results={'charge':[],'natural':[],'shoot_enemy':[]}
    try:
        for hold in list(range(30,46))+list(range(97,111)):
            s.load(OUT/'start.state')
            for f in range(hold):run(s,['Y'])
            run(s,[])
            p=row(s);p['hold']=hold;p['shots']=actors(s,0x1228,0x1428)
            results['charge'].append(p)
        s.load(OUT/'start.state')
        previous=16
        for f in range(600):
            run(s,['RIGHT'] if f<287 else [])
            p=row(s);p['frame']=f;p['enemies']=actors(s,0xe68,0x1228)
            results['natural'].append(p)
            if f in (160,200,240,260,285,286,300,320,360,440):
                s.save(directory/f'natural-{f}.state');save_scene(s,f'combat/natural-{f}')
            if p['hp']!=previous:print('natural damage',f,previous,'->',p['hp']);previous=p['hp']
        s.load(OUT/'start.state')
        # Move until the first enemy becomes visible, then stand and fire.
        for f in range(430):
            buttons=['RIGHT'] if f<260 else (['Y'] if (f-260)%12<1 else [])
            run(s,buttons)
            p=row(s);p['frame']=f;p['enemies']=actors(s,0xe68,0x1228);p['shots']=actors(s,0x1228,0x1428)
            results['shoot_enemy'].append(p)
            if f in (260,270,280,290,300):
                s.save(directory/f'shoot-{f}.state');save_scene(s,f'combat/shoot-{f}')
        (directory/'reference.json').write_text(json.dumps(results,indent=2)+'\n')
        print('charge boundaries:',[(p['hold'],[q['kind'] for q in p['shots'] if q['x']<180]) for p in results['charge']])
    finally:s.close()
if __name__=='__main__':main()
