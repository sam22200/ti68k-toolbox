#!/usr/bin/env python3
"""Original opening Octoroks: separate story-flag study door and combat traces.

Only initial placement/equipment/actor phase is injected. Replays thereafter
use inputs alone. Commercial pixels and traces remain in ignored fixtures.
"""
import argparse
import json
import struct
import numpy as np
from PIL import Image
from reference import GBA, ROM, OUTPUT, SAVE, PLAYER, ROOM, ORIGIN_X, ORIGIN_Y, boot, digest, raw, sha, rom_bytes
from traversal import GAME

POOL, STRIDE = 0x030015a0, 0x88
DOOR = OUTPUT/'combat.state'
ROM_DATA = rom_bytes()
ENEMY = POOL+7*STRIDE

def setup(g,x=320,y=184,face=2,action=1,timer=200,px=280,py=184):
    g.load(DOOR);place(g,PLAYER,px,py)
    for _ in range(80):g.step([])
    place(g,ENEMY,x,y)
    # Initialize a documented animation phase before the trial, not during it.
    index=face+(4 if action==3 else 0)
    ptr=struct.unpack_from('<I',ROM_DATA,0xca1f8+index*4)[0]
    for o,v,s in ((12,action,1),(14,timer,1),(20,face,1),(21,face*8,1),
                  (88,index,1),(89,1,1),(92,ptr,4)):
        g.write(ENEMY+o,v,s)
    g.step([])

def measure():
    from art import objects, composite, grayscale, sprite_words
    trials=[];poses={};replayed=rgb_pixels=0;timelines={}
    g=GBA(ROM)
    try:
        # Every controlled trial is restored twice, including full video/memory.
        specs=[('walk',f,320,184,280,184,2,100,[],24) for f in range(4)]
        specs += [('contact',f,320,184,320+dx,184+dy,1,200,[],36)
                  for f,(dx,dy) in enumerate(((0,-6),(6,0),(0,6),(-6,0),(6,6),(-6,-6),(12,0),(13,0)))]
        specs += [('sword',f,320+dx,184+dy,320,184,1,200,['A'],24)
                  for f,(dx,dy) in enumerate(((0,-20),(20,0),(0,20),(-20,0)))]
        specs += [('shoot',f,320,184,280,184,3,200,[],32) for f in range(4)]
        for name,face,x,y,px,py,action,timer,keys,n in specs:
            first=None
            for repeat in range(2):
                setup(g,x,y,face&3,action,timer,px,py)
                g.write(PLAYER+20,(face&3)*2)
                initial={'player':sample(g,PLAYER),'enemy':sample(g,ENEMY),'health':g.read(SAVE+0xaa)}
                stream=[];hashes=[]
                for i in range(n):
                    g.step(keys if i==0 else [])
                    stream.append({'player':sample(g,PLAYER),'enemy':sample(g,ENEMY),
                                   'health':g.read(SAVE+0xaa),
                                   'rocks':[sample(g,e) for e in entities(g) if g.read(e+8)==4 and g.read(e+9)==1
                                            and all(abs(v-w)<96*65536 for v,w in zip(xy(g,e),xy(g,ENEMY)))],
                                   'sword':[sample(g,0x030011e8+j*STRIDE) for j in range(7)
                                            if g.read(0x030011e8+j*STRIDE+8)==8 and g.read(0x030011e8+j*STRIDE,4)]})
                    hashes.append(digest(g))
                current=(initial,stream,hashes)
                if not repeat:first=current
                else:assert first==current,(name,face,'replay');replayed+=n
            trials.append({'name':name,'face':face,'initial':initial,'steps':stream,'replay_sha256':sha(json.dumps(hashes).encode())})
        # Capture both complete source animation cycles and all shooting poses.
        for face in range(4):
            for action,n in ((1,34),(3,32)):
                setup(g,320,184,face,action,200,280,184)
                seq=[]
                for i in range(n):
                    prev=sample(g,ENEMY)
                    cx=g.read(ROOM+10,2)-ORIGIN_X;cy=g.read(ROOM+12,2)-ORIGIN_Y
                    anchor=(prev['xy'][0]>>16)-cx,(prev['xy'][1]>>16)-cy
                    flip=g.read(ENEMY+24)&64
                    g.step([]);s=g.snapshot()
                    rgb,owner=composite(s);assert np.array_equal(rgb,np.asarray(g.image()))
                    rgb_pixels+=240*160
                    key=(prev['animation'],prev['frame'],bool(flip))
                    canvas=np.zeros((48,48,4),np.uint8)
                    for o in reversed(objects(s)):
                        if not (prev['tile']<=o['tile']<prev['tile']+64 and o['palette']==(prev['palette']&15)):continue
                        rgba=o['rgba'];yy,xx=np.nonzero(rgba[:,:,3])
                        xx=xx+o['x']-anchor[0]+24;yy=yy+o['y']-anchor[1]+32
                        if not len(xx) or not np.all((xx>=1)&(xx<47)&(yy>=1)&(yy<47)):continue
                        canvas[yy,xx]=rgba[rgba[:,:,3]!=0]
                    assert canvas[:,:,3].any(),('missing enemy',prev,anchor)
                    if key in poses:assert np.array_equal(canvas,poses[key]),('unstable enemy pose',key)
                    else:poses[key]=canvas
                    seq.append(key)
                timelines[(face,action)]=seq
        hurt,hurt_seq,hurt_pixels=hurt_poses(g)
        rgb_pixels+=hurt_pixels;replayed+=2*4*HURT_UPDATES
        keys=sorted(poses);ids={k:i for i,k in enumerate(keys)}
        # Native animation timelines are drawn from checked original updates.
        walk=[[ids[k] for k in timelines[(f,1)][:32]] for f in range(4)]
        shoot=[[ids[k] for k in timelines[(f,3)][:28]] for f in range(4)]
        attack_boxes=[]
        for face in range(4):
            t=next(t for t in trials if t['name']=='sword' and t['face']==face)
            boxes=[]
            for row in t['steps'][:15]:
                sword=[s for s in row['sword'] if s['id']==1]
                assert sword,('no sword hitbox',face)
                b=sword[0]['hitbox'];boxes.append([b[0] if b[0]<128 else b[0]-256,b[1] if b[1]<128 else b[1]-256,b[6],b[7]])
            attack_boxes.append(boxes)
        np.savez_compressed(GAME/'fixtures/source_combat.npz',actors=np.stack([poses[k] for k in keys]),hurt=hurt)
        pack(np.stack([poses[k] for k in keys]),walk,shoot,attack_boxes,hurt,hurt_seq)
        data={'trials':trials,'source_replayed_frames':replayed,'source_rgb_pixels':rgb_pixels,
              'pose_keys':keys,'walk':walk,'shoot':shoot,'sword_boxes':attack_boxes,'hurt':hurt_seq,
              'door_sha256':sha(DOOR.read_bytes()),'core':g.identity,
              'initial_phase_writes':'setup() only; no actor or terrain writes after trial starts'}
        (GAME/'fixtures/combat.json').write_text(json.dumps(data,indent=2)+'\n')
        write_fixtures(data)
        print('Combat:',len(trials),'trials',replayed,'replayed updates',rgb_pixels,'RGB pixels',len(poses),'poses')
    finally:g.close()

HURT_UPDATES=7

def hurt_poses(g):
    """Link's knockback animation (24+facing) after an ordinary contact.

    The source draws it with the damage palette (OBJ 15); the native flash is
    separate, so the same OBJ tiles are decoded with Link's ordinary palette.
    The completed video depicts the previous update's entity pose."""
    from art import entity, composite
    from effects import roll_pixels
    poses=[];keys={};seqs=[];pixels=0
    for face,(dx,dy) in enumerate(((0,-6),(6,0),(0,6),(-6,0))):
        first=None
        for repeat in range(2):
            setup(g,320,184,face,1,200,320+dx,184+dy);g.write(PLAYER+20,face*2)
            seq=[];images=[]
            for i in range(HURT_UPDATES+1):
                prev=entity(g);g.step([])
                if i==0:continue
                assert prev['pose'][0]==24+face,('hurt animation',face,i,prev['pose'])
                snap=g.snapshot();rgb,_=composite(snap)
                assert np.array_equal(rgb,np.asarray(g.image()));pixels+=240*160
                oam=bytearray(snap['oam'])
                for j in range(128):
                    a2=struct.unpack_from('<H',oam,j*8+4)[0]
                    if prev['tile']<=(a2&1023)<prev['tile']+32 and a2>>12==15:
                        struct.pack_into('<H',oam,j*8+4,(a2&0x0fff)|(prev['palette']<<12))
                images.append(roll_pixels({**snap,'oam':bytes(oam)},prev))
                seq.append(tuple(prev['pose']))
            current=(seq,[im.tobytes() for im in images])
            if first is None:first=current
            else:assert first==current,('hurt replay',face)
        ids=[]
        for key,im in zip(first[0],images):
            if key not in keys:keys[key]=len(poses);poses.append(im)
            ids.append(keys[key])
        seqs.append(ids)
    return np.stack(poses),seqs,pixels

def write_fixtures(data):
    chosen=data['trials']
    with (GAME/'fixtures/combat.txt').open('w') as f:
        f.write(str(len(chosen))+'\n')
        for t in chosen:
            initial=t['initial'];p,e=initial['player'],initial['enemy'];kind=('walk','contact','sword','shoot').index(t['name'])
            values=[kind,t['face']&3,len(t['steps']),*[v>>8 for v in p['xy']],*[v>>8 for v in e['xy']],
                    e['action'],e['timer'],initial['health'],p['iframes'],p['recoil'],p['recoil_direction']//4]
            f.write(' '.join(map(str,values))+'\n')
            for i,s in enumerate(t['steps']):
                p,e=s['player'],s['enemy']
                rocks=s['rocks'];r=rocks[0] if rocks else None
                # Bounces and impact debris are explicitly a native adaptation.
                flight=int(bool(r) and r['action']==1 and s['health']==initial['health'])
                f.write(' '.join(map(str,[16 if kind==2 and i==0 else 0,*[v>>8 for v in p['xy']],
                    *[v>>8 for v in e['xy']],s['health'],p['iframes'],p['recoil'],e['hp'],
                    flight,*([v>>8 for v in r['xy']] if r else [0,0]),r['timer'] if flight else 0]))+'\n')

def pack(actors,walk,shoot,boxes,hurt,hurt_seq):
    from art import grayscale,sprite_words
    from effects import packed
    from hud import rock_shift
    rock=rock_shift()
    def array(name,typ,rows):
        def braces(v):return '{'+','.join(braces(x) if isinstance(x,list) else str(x) for x in v)+'}'
        shape=[];v=rows
        while isinstance(v,list):shape.append(len(v));v=v[0]
        return f'static const {typ} {name}'+''.join(f'[{n}]' for n in shape)+'='+braces(rows)+';\n'
    header='/* Generated from source combat frames; commercial data stays local. */\n'
    # Only two entries lie inside the native 720x320 slice.
    positions=[list(struct.unpack_from('<HH',ROM_DATA,0xf4f30+i*16+8)) for i in range(2)]
    assert positions==[[328,56],[280,152]]
    header+=array('enemy_spawn','u16',positions)+array('enemy_walk','u8',walk)+array('enemy_shoot','u8',shoot)
    header+=array('sword_boxes','s8',boxes)
    header+=f'#define HURT_POSES {len(hurt)}\n'+array('hurt_seq','u8',hurt_seq)
    for zoom in (False,True):
        words=[];meta=[]
        for rgba in actors:
            if zoom:rgba=np.asarray(Image.fromarray(rgba).resize((34,34),Image.Resampling.NEAREST));ax,ay=17,23
            else:ax,ay=24,32
            mask=rgba[:,:,3]!=0;level=grayscale(rgba[:,:,:3],True);level[~mask]=0
            mask=np.pad(mask,1);level=np.pad(level,1);expanded=mask.copy()
            for dy in (-1,0,1):
                for dx in (-1,0,1):expanded[1:-1,1:-1]|=mask[1+dy:mask.shape[0]-1+dy,1+dx:mask.shape[1]-1+dx]
            yy,xx=np.nonzero(expanded);x0,x1,y0,y1=int(xx.min()),int(xx.max()+1),int(yy.min()),int(yy.max()+1)
            w=16 if x1-x0<=16 else 32;h=y1-y0
            assert x1-x0<=32,('wide enemy',x1-x0)
            l=np.zeros((h,w),np.uint8);m=np.zeros((h,w),bool)
            l[:,:x1-x0]=level[y0:y1,x0:x1];m[:,:x1-x0]=expanded[y0:y1,x0:x1]
            meta.append([len(words),x0-ax-1,y0-ay-1,w,h]);words+=sprite_words(l,m,w)
        bank='mizfight' if zoom else 'mifight'
        hurt_words=[];hurt_meta=[]
        for rgba in hurt:
            m,p=packed(rgba,zoom);hurt_meta.append([len(hurt_words),*m]);hurt_words+=p
        # Use uniform u32 rows for either width; shift 16-wide rows to high half.
        uniform=[]
        for offset,x,y,w,h in meta:
            uniform += [(v<<(32-w)) | (0xffff if w==16 and j>=2*h else 0)
                        for j,v in enumerate(words[offset:offset+3*h])]
        if zoom:
            assert all(m[3]==16 for m in meta)
            shifted=[];shift_offsets=[];base_size=(len(words)*2+3)&~3
            for offset,x,y,w,h in meta:
                shift_offsets.append(base_size+len(shifted)*4)
                for shift in range(16):
                    for row in range(h):
                        l,d,m=words[offset+row],words[offset+h+row],words[offset+2*h+row]
                        shifted += [(l<<16)>>shift,(d<<16)>>shift,(~(((~m&65535)<<16)>>shift))&0xffffffff]
        for order,suffix in (('<',''),('>','.be')):
            if zoom:
                payload=struct.pack(order+f'{len(words)}H',*words)
                payload+=bytes(base_size-len(payload))+struct.pack(order+f'{len(shifted)}I',*shifted)
            else:payload=struct.pack(order+f'{len(uniform)}I',*uniform)
            # Link's knockback poses follow, in the roll/effect layout.
            payload+=bytes(-len(payload)&3);hurt_base=len(payload)
            payload+=struct.pack(order+f'{len(hurt_words)}I',*hurt_words)
            # The native projectile's sixteen pre-shifts, kept out of the program.
            rock_base=len(payload);payload+=struct.pack(order+f'{len(rock)}I',*rock)
            assert len(payload)<65518
            (GAME/f'{bank}{suffix}.bin').write_bytes(payload)
        header+=('#ifdef MINISH_ZOOM\n' if zoom else '#ifndef MINISH_ZOOM\n')
        header+=f'#define FIGHT_SIZE {len(payload)}\n#define HURT_BASE {hurt_base}\n#define ROCK_BASE {rock_base}\n'+array('enemy_art','s16',meta)
        header+=array('hurt_art','s16',hurt_meta)
        if zoom:header+=array('enemy_shift','u16',shift_offsets)
        header+='#endif\n'
    (GAME/'combat_generated.h').write_text(header)

def entities(g):
    return [POOL+i*STRIDE for i in range(72) if g.read(POOL+i*STRIDE,4)]

def xy(g,e):
    return [g.read(e+o,4,True)-origin*65536 for o,origin in ((0x2c,ORIGIN_X),(0x30,ORIGIN_Y))]

def place(g,e,x,y):
    for o,v,origin in ((0x2c,x,ORIGIN_X),(0x30,y,ORIGIN_Y)):
        g.write(e+o,(v+origin)<<16,4)

def sample(g,e):
    h=g.read(e+0x48,4)
    return {'address':hex(e),'kind':g.read(e+8),'id':g.read(e+9),'xy':xy(g,e),
            **{n:g.read(e+o,s,sign) for n,o,s,sign in (
                ('action',12,1,False),('timer',14,1,False),('face',20,1,False),
                ('direction',21,1,False),('speed',36,2,False),('hp',69,1,False),
                ('iframes',61,1,True),('recoil',66,1,False),('recoil_direction',62,1,False),
                ('recoil_speed',70,2,False),('animation',88,1,False),('frame',30,1,False),
                ('duration',89,1,False),('flags',90,1,False),('tile',96,2,False),
                ('palette',26,1,False),('z',52,4,True),('damage',68,1,False))},
            'hitbox':list(ROM_DATA[h-0x08000000:h-0x08000000+8] if h>=0x08000000 else raw(g,h,8)) if h else []}

def door():
    initial=None
    for repeat in range(2):
        g=GBA(ROM)
        try:
            writes=boot(g,[(SAVE+0x25e,32,1),(SAVE+0xb4,1,1),(SAVE+0xf2,4,1)])
            current=digest(g)
            if not repeat:
                initial=current;g.save(DOOR)
                (OUTPUT/'combat_door.json').write_text(json.dumps({
                    'writes':writes,'flag':'TABIDACHI 0x15; original room enemy-list gate',
                    'state_sha256':sha(DOOR.read_bytes()),'digests':current},indent=2)+'\n')
            else:assert initial==current,'cold combat door differs'
        finally:g.close()

def probe():
    g=GBA(ROM)
    try:
        for name,x,y,keys in [('idle',320,184,[]),('contact',302,152,[]),('sword',282,152,['A']),('shots',302,210,[])]:
            g.load(DOOR);place(g,PLAYER,x,y)
            rows=[]
            for i in range(240):
                g.step(keys if i==0 else [])
                es=[e for e in entities(g) if g.read(e+8) in (3,4,8)]
                row={'i':i,'player':sample(g,PLAYER),'health':g.read(SAVE+0xaa),
                     'entities':[sample(g,e) for e in es]}
                rows.append(row)
                if i in (0,1,2,3,4,5,10,20,30,60,90,120,180,239):
                    first=[e for e in row['entities'] if e['address'] in ('0x30018d0','0x3001958') or e['kind']!=3]
                    print(name,i,'P',row['health'],row['player']['xy'],row['player']['iframes'],row['player']['recoil'],
                          [(e['address'],e['kind'],e['xy'],e['hp'],e['action'],e['timer'],e['face'],e['frame']) for e in first])
            (GAME/'fixtures'/('combat_probe_'+name+'.json')).write_text(json.dumps(rows))
            g.image().save(GAME/'captures'/('combat_source_'+name+'.png'))
    finally:g.close()

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--door',action='store_true');p.add_argument('--measure',action='store_true');a=p.parse_args()
    if a.door:door()
    measure() if a.measure else probe()
