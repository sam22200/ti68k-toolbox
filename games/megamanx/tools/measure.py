#!/usr/bin/env python3
"""Movement and combat timelines on the local cold-booted Highway state."""
import json
from reference import *

FIELDS={'x':(X,2,False),'y':(Y,2,False),'vx':(VX,2,True),'vy':(VY,2,True),
        'xs':(0x7e0bac,1,False),'ys':(0x7e0baf,1,False),
        'state':(0x7e0baa,1,False),'substate':(0x7e0bab,1,False),
        'ground':(0x7e0bd3,1,False),'hp':(0x7e0bcf,1,False),
        'facing':(0x7e0bb9,1,False),'charge':(0x7e0bff,1,False),
        'charge_state':(0x7e0c00,1,False),'charge_tier':(0x7e0c03,1,False)}

def row(s):return {k:s.read(*v) for k,v in FIELDS.items()}

def suppress(s):
    # Disclosed isolation only: enemy/projectile pools. No anchoring of player,
    # camera, collision, motion fields or timing during the measurements.
    for a in range(0xe68,0x1628,64):s.ram[a]=0

def main():
    directory=OUT/'measure';directory.mkdir(exist_ok=True)
    s=SNES(ROM);cases=[]
    definitions=[('idle',90,{}),('right',90,{0:['RIGHT']}),('left',60,{0:['LEFT']}),
                 ('release',90,{0:['RIGHT'],35:[]}),
                 ('turn',90,{0:['RIGHT'],35:['LEFT'],60:[]}),
                 ('jump_1',90,{0:['B'],1:[]}),('jump_10',90,{0:['B'],10:[]}),
                 ('jump_held',90,{0:['B'],60:[]}),
                 ('run_jump',90,{0:['RIGHT','B'],35:['RIGHT'],60:[]}),
                 ('run_then_jump',90,{0:['RIGHT'],20:['RIGHT','B'],45:['RIGHT'],70:[]})]
    try:
        for name,count,keys in definitions:
            s.load(OUT/'start.state');held=[];samples=[]
            for f in range(count):
                if f in keys:held=keys[f]
                suppress(s);run(s,held);samples.append(row(s))
            cases.append({'name':name,'frames':count,'keys':keys,'samples':samples})
        s.load(OUT/'start.state')
        for f in range(480):suppress(s);run(s,['RIGHT'])
        s.save(OUT/'wall.state');save_scene(s,'wall')
        wall_initial=row(s);walls=[]
        for name,keys in [('slide',{0:['RIGHT']}),('kick_short',{0:['RIGHT','B'],2:['RIGHT']}),
                          ('kick_held',{0:['RIGHT','B'],35:['RIGHT']})]:
            s.load(OUT/'wall.state');held=[];samples=[]
            for f in range(90):
                if f in keys:held=keys[f]
                suppress(s);run(s,held);samples.append(row(s))
            walls.append({'name':name,'keys':keys,'samples':samples})
        shots=[]
        for hold in (1,20,21,22,23,24,25,30,45,80,90,92,93,94,95,96,110):
            s.load(OUT/'start.state');samples=[]
            for f in range(hold+35):
                run(s,['Y'] if f<hold else [])
                p=row(s);p['shots']=[]
                for a in range(0x1228,0x1428,64):
                    if s.ram[a]:p['shots'].append({'slot':a,'state':s.ram[a+2],'kind':s.ram[a+10],
                        'x':s.read(0x7e0000+a+5,2),'y':s.read(0x7e0000+a+8,2),
                        'vx':s.read(0x7e0000+a+0x1a,2,True),'xs':s.ram[a+4]})
                samples.append(p)
            shots.append({'hold':hold,'samples':samples})
            print('charge hold',hold,'release',samples[hold]['shots'])
        s.load(OUT/'start.state');damage=[];hit=None
        for f in range(450):
            run(s,['RIGHT'] if f<287 else [])
            damage.append(row(s))
            if hit is None and damage[-1]['hp']!=16:
                hit=f;s.save(OUT/'hit.state');save_scene(s,'hit')
        assert hit==286
        results={'source':json.loads((OUT/'start.json').read_text()),
            'isolation':'Before each movement/wall frame, clear primary states of enemy/projectile pools E68..1628; no player/timing anchoring.',
            'movement':cases,'wall_initial':wall_initial,'walls':walls,'shots':shots,'damage':damage,'first_hit':hit}
        (directory/'reference.json').write_text(json.dumps(results,indent=2)+'\n')
        gen=GAME/'generated';gen.mkdir(exist_ok=True)
        lines=['/* Generated from local USA ROM; ignored. */',
            'typedef struct { unsigned short x,y; short vx,vy; unsigned char xs,ys,state; } MotionSample;']
        for case in cases+[dict(c,name='wall_'+c['name']) for c in walls]:
            lines.append('static const MotionSample ref_'+case['name']+'[]={')
            for p in case['samples']:lines.append('{'+','.join(str(p[k]) for k in ('x','y','vx','vy','xs','ys','state'))+'},')
            lines.append('};')
        (gen/'motion_ref.h').write_text('\n'.join(lines)+'\n')
        print('Movement:',sum(c['frames'] for c in cases),'states; wall probes3; natural hit at',hit)
    finally:s.close()

if __name__=='__main__':main()
