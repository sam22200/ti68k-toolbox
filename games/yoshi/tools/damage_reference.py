#!/usr/bin/env python3
"""Measure PAL damage, Baby Mario and rescue; keep original data local."""
import json
from reference import OUT, ROM
from actors_reference import prepare
from snesrun import SNES, PAD

DIRECTORY = OUT / 'damage'


def sample(s, frame):
    result = {'frame': frame}
    for name, address, signed in (
        ('x',0x70008c,False),('y',0x700090,False),('vx',0x7000b4,True),
        ('vy',0x7000aa,True),('invincible',0x7001d6,False),('baby',0x7001b2,False),
        ('bx',0x7010e2,False),('by',0x701182,False),('bvx',0x701220,True),
        ('bvy',0x701222,True),('stars',0x7e03b6,False),('clock',0x7e0392,False),
        ('recharge',0x7e0394,False)):
        result[name] = s.read(address,2,signed=signed)
    result['phase'] = s.read(0x7019d6)
    result['xsub'] = s.read(0x7010e1)
    result['ysub'] = s.read(0x701181)
    return result


def snapshot(s, name):
    s.image().save(DIRECTORY / f'{name}.png')
    for kind, data in zip(('vram','cgram','oam','regs'),s.ppu()):
        (DIRECTORY / f'{name}.{kind}').write_bytes(data)


def main():
    DIRECTORY.mkdir(parents=True,exist_ok=True)
    s = SNES(ROM)
    cases = []
    try:
        prepare(s,True)
        # Disclosed one-time pose and tutorial flag; the hit is a natural
        # collision with the real first Shy Guy. No per-frame interventions.
        s.write(0x7e0372,s.read(0x7e0372,2)|128,2)
        for address,value in ((0x70008c,490),(0x70008a,0),(0x7001d6,0)):
            s.write(address,value,2)
        s.save(DIRECTORY/'contact.state')
        rows=[]
        for frame in range(700):
            s.pressed=0; s.lib.retro_run()
            rows.append(sample(s,frame))
            if frame in (60,64,68,72): snapshot(s,f'cry{frame}')
            if frame == 80: s.save(DIRECTORY/'rescue.state')
        assert rows[0]['invincible']==160 and rows[0]['baby']==0x8000
        assert rows[1]['baby']==0 and rows[1]['bvx']==-435 and rows[1]['bvy']==-1162
        assert all(r['clock']==r['frame']%4 for r in rows[1:437])
        assert rows[437]['stars']==0 and rows[438]['phase']==2
        cases.append({'name':'contact','samples':rows})
        for name,dx,dy,buttons in (('touch',0,-8,0),('tongue',-36,-12,1<<PAD['Y'])):
            s.load(DIRECTORY/'rescue.state')
            bx,by=s.read(0x7010e2,2),s.read(0x701182,2)
            for address,value in ((0x70008c,bx+dx),(0x700090,by+dy),
                                  (0x7000b4,0),(0x7000aa,0),(0x7000a8,0),
                                  (0x70008a,0),(0x70008e,0),(0x7000c4,0)):
                s.write(address,value,2)
            rows=[]
            for frame in range(150):
                s.pressed=buttons if frame<8 else 0; s.lib.retro_run()
                rows.append(sample(s,frame))
            assert any(r['baby']&0x8000 for r in rows),name
            cases.append({'name':name,'samples':rows})
            first=next(r['frame'] for r in rows if r['baby']&0x8000)
            print(name,'reattached at frame',first,'stars',rows[first]['stars'])
    finally: s.close()
    metadata=json.loads((OUT/'start.json').read_text())
    metadata.update({'interventions':'prepare(capture=True), one-time contact pose/tutorial flag; rescue uses one-time airborne player pose',
                     'cases':cases})
    (DIRECTORY/'reference.json').write_text(json.dumps(metadata,indent=2)+'\n')
    from pathlib import Path
    rows=cases[0]['samples'][1:25]
    Path('generated/damage_ref.h').write_text(
        '/* Local PAL launch reference, generated and ignored. */\n'
        'static const struct { u16 x,y; s16 vx,vy; u8 xs,ys,phase; } baby_launch[] = {\n'+
        ''.join('{%(bx)d,%(by)d,%(bvx)d,%(bvy)d,%(xsub)d,%(ysub)d,%(phase)d},\n'%v for v in rows)+'};\n')
    print('PAL damage: 700 contact frames; two 150-frame rescue cases.')


if __name__=='__main__': main()
