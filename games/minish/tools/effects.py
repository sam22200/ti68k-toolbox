#!/usr/bin/env python3
"""Measure native-roll parameters and capture original destruction sprites.

Study preparation writes initial equipment/position and enemy phase only.
Every subsequent step uses real inputs, with two complete memory/video replays.
Commercial snapshots, generated tables and banks remain ignored locally.
"""
import argparse
import json
import struct
import numpy as np
from PIL import Image
from reference import GBA, ROM, OUTPUT, PLAYER, ROOM, ORIGIN_X, ORIGIN_Y, position, digest, sha
from traversal import GAME, BUTTONS
from actions import setup
from art import entity, hero_pixels, objects, composite, grayscale, sprite_words
from combat import setup as enemy_setup, entities, sample, xy, ENEMY


def roll_pixels(snapshot,previous):
    canvas=np.zeros((64,64,4),np.uint8);ax,ay=previous['screen']
    parts=[o for o in objects(snapshot) if previous['tile']<=o['tile']<previous['tile']+32
           and o['palette']==previous['palette']]
    assert parts, ('missing roll parts',previous,[(o['tile'],o['palette']) for o in objects(snapshot)])
    for o in reversed(parts):
        yy,xx=np.nonzero(o['rgba'][:,:,3]);x=xx+o['x']-ax+32;y=yy+o['y']-ay+48
        assert ((x>0)&(x<63)&(y>0)&(y<63)).all()
        canvas[y,x]=o['rgba'][yy,xx]
    return canvas


def capture(g, previous, anchor=(32,48)):
    snap=g.snapshot();rgb,owner=composite(snap)
    assert np.array_equal(rgb,np.asarray(g.image())), 'source RGB decoder'
    canvas=np.zeros((64,64,4),np.uint8)
    cx,cy=previous['camera'];px,py=previous['xy']
    ax,ay=(px>>16)-cx,(py>>16)-cy
    # The visible death-poof phase uses48 static tiles; the earlier spark
    # phase uses separate fixed graphics and is deliberately not ported here.
    count=48 if previous['id']==1 else 32
    parts=[o for o in objects(snap) if previous['tile']<=o['tile']<previous['tile']+count
           and o['palette']==(previous['palette']&15)
           and abs(o['x']-ax)<48 and abs(o['y']-ay)<48]
    assert parts, ('missing effect',previous)
    for o in reversed(parts):
        yy,xx=np.nonzero(o['rgba'][:,:,3]);x=xx+o['x']-ax+anchor[0];y=yy+o['y']-ay+anchor[1]
        assert ((x>0)&(x<63)&(y>0)&(y<63)).all(), ('effect canvas',x.min(),x.max(),y.min(),y.max())
        canvas[y,x]=o['rgba'][yy,xx]
    return canvas


def fx_sample(g,e):
    s=sample(g,e);s['type']=g.read(e+10)
    s['camera']=[g.read(ROOM+10,2)-ORIGIN_X,g.read(ROOM+12,2)-ORIGIN_Y]
    return s


def measure():
    poses=[];keys={};trials=[];rolls=[];fx_timelines=[];rgb_pixels=replayed=0
    def pose(rgba,anchor):
        key=(anchor,rgba.tobytes())
        if key not in keys:keys[key]=len(poses);poses.append((rgba,anchor))
        return keys[key]
    g=GBA(ROM)
    try:
        # Flat, blocked, changed direction, held/repeated R and A during a roll.
        specs=[(320,184,f,[32|k]+[0]*35) for f,k in enumerate((1,8,4,2))]
        specs += [(320,184,1,[40]*40), (320,184,1,[40,0,40]+[0]*35),
                  (320,184,1,[40]+[2]*35),(320,184,1,[40]+[16]*35),
                  (424,160,0,[33]+[0]*35),(248,88,3,[34]+[0]*35)]
        for ti,(x,y,face,seq) in enumerate(specs):
            first=None
            for repeat in range(2):
                g.load(OUTPUT/'woods.state');setup(g,x,y,face)
                # The original R-action becomes available after walking.
                k=(1,8,4,2)[face]
                for _ in range(2):g.step([name for bit,name in BUTTONS.items() if k&bit])
                initial=[v>>8 for v in position(g)];stream=[];hashes=[];timeline=[]
                for n,k in enumerate(seq):
                    prev=entity(g);was_roll=g.read(PLAYER+12)==24
                    g.step([name for bit,name in {**BUTTONS,16:'A',32:'R'}.items() if k&bit])
                    active=g.read(PLAYER+12)==24
                    now=entity(g);current=-1
                    if active or was_roll:
                        if not repeat:
                            snap=g.snapshot();rgb,_=composite(snap)
                            assert np.array_equal(rgb,np.asarray(g.image()));rgb_pixels+=240*160
                        if active and ti<4:
                            # Post-update pose determines the next source image.
                            timeline.append(tuple(now['pose']))
                    stream.append([k,*[v>>8 for v in position(g)],int(active),g.read(PLAYER+36,2),
                                   g.read(PLAYER+0x40),now['pose']])
                    hashes.append(digest(g))
                current=(initial,stream,hashes)
                if first is None:first=current
                else:assert first==current,('roll replay',ti);replayed+=len(seq)
            trials.append({'spawn':[x,y],'face':face,'initial':initial,'steps':first[1],
                           'replay_sha256':sha(json.dumps(first[2],sort_keys=True).encode())})
            if ti<4:rolls.append(timeline)
        # Capture stable source roll art by pose key, without duplicating
        # walking/startup sprites or the rendered previous-coordinate convention.
        rolling_poses={};roll_streams=[]
        for face,k in enumerate((1,8,4,2)):
            g.load(OUTPUT/'woods.state');setup(g,320,184,face)
            for _ in range(2):g.step([name for bit,name in BUTTONS.items() if k&bit])
            g.step([name for bit,name in {**BUTTONS,32:'R'}.items() if (32|k)&bit])
            seq=[]
            while g.read(PLAYER+12)==24:
                prev=entity(g);seq.append(tuple(prev['pose']))
                g.step([]);snap=g.snapshot();rgb,_=composite(snap)
                assert np.array_equal(rgb,np.asarray(g.image()));rgb_pixels+=240*160
                large=roll_pixels(snap,prev)
                pid=pose(large,(32,48));key=tuple(prev['pose'])
                if key in rolling_poses:assert rolling_poses[key]==pid,('unstable roll pose',key)
                rolling_poses[key]=pid
            roll_streams.append([rolling_poses[k] for k in seq])
        assert len({len(s) for s in roll_streams})==1
        # One isolated ordinary bush and one grass style; natural random
        # variants are captured, not selected by animation-pointer writes.
        for kind,(x,y,face) in enumerate(((536,158,0),(632,128,0),(320,184,0))):
            first=None;sequence=[];fx_identity=None
            for repeat in range(2):
                if kind<2:
                    g.load(OUTPUT/'woods.state');setup(g,x,y,face)
                    # Settle the original camera after a distant study placement.
                    for _ in range(120):g.step([])
                else:enemy_setup(g,320,164,0,1,200,320,184);g.write(PLAYER+20,0)
                tracked=None;started=False;stream=[];hashes=[];seq=[]
                for n in range(125):
                    previous=fx_sample(g,tracked) if tracked and g.read(tracked,4) else None
                    g.step(['A'] if n==0 else [])
                    if not tracked:
                        found=[e for e in entities(g) if g.read(e+8)==6 and g.read(e+9)==(1 if kind==2 else 15)
                               and abs((xy(g,e)[0]>>16)-x)<20 and abs((xy(g,e)[1]>>16)-(164 if kind==2 else y-24))<40]
                        if found:tracked=found[0]
                    active=bool(tracked and g.read(tracked,4) and g.read(tracked+8)==6
                                and g.read(tracked+9)==(1 if kind==2 else 15))
                    now=fx_sample(g,tracked) if active else None
                    stream.append({'effect':now,'enemy':sample(g,ENEMY) if kind==2 else None})
                    hashes.append(digest(g))
                    if previous and previous['tile'] and previous['action']==1 and (kind<2 or previous['frame']>=13):
                        if not repeat:
                            rgba=capture(g,previous);pid=pose(rgba,(32,48));seq.append(pid);rgb_pixels+=240*160
                    if active:started=True
                    if started and not active:break
                assert started,('no natural effect',kind)
                current=(stream,hashes)
                if first is None:first=current;sequence=seq;fx_identity=stream[next(i for i,s in enumerate(stream) if s['effect'])]['effect']
                else:assert first==current,('effect replay',kind);replayed+=len(stream)
            assert sequence,kind
            fx_timelines.append(sequence)
            trials.append({'effect_kind':kind,'spawn':[x,y],'sequence':sequence,'initial_effect':fx_identity,
                           'steps':first[0],'replay_sha256':sha(json.dumps(first[1],sort_keys=True).encode())})
    finally:g.close()
    # Remove unrelated walking/startup images from the shared bank.
    used=sorted(set(v for seq in roll_streams+fx_timelines for v in seq));remap={v:i for i,v in enumerate(used)}
    poses=[poses[i] for i in used];roll_streams=[[remap[v] for v in s] for s in roll_streams]
    fx_timelines=[[remap[v] for v in s] for s in fx_timelines]
    flat=trials[1]['steps'];active=[s for s in flat if s[3]]
    speeds=[0]+[s[4] for s in active[1:]]
    guard=[int(s[5]==0) for s in active]
    assert all(len(s)==len(speeds) for s in roll_streams),(len(speeds),list(map(len,roll_streams)))
    data={'trials':trials,'roll':roll_streams,'roll_speed':speeds,'roll_guard':guard,'effects':fx_timelines,
          'source_replayed_frames':replayed,'source_rgb_pixels':rgb_pixels,'poses':len(poses),
          'rom_sha256':sha(ROM.read_bytes()),'state_sha256':sha((OUTPUT/'woods.state').read_bytes())}
    (GAME/'fixtures/effects.json').write_text(json.dumps(data,indent=2)+'\n')
    np.savez_compressed(GAME/'fixtures/source_effects.npz',actors=np.stack([r for r,a in poses]))
    write_fixtures(data)
    pack(data,np.stack([r for r,a in poses]))
    print('Roll/effects:',len(poses),'poses;',replayed,'replayed source frames;',rgb_pixels,'checked RGB pixels;',
          'roll',len(speeds),'updates; effects',list(map(len,fx_timelines)))


def write_fixtures(data):
    with (GAME/'fixtures/roll.txt').open('w') as f:
        # A contextual R press facing a bush lifts it in the source. The
        # native B action is dedicated to rolling; retain that source trial
        # in JSON but do not claim its unimplemented pickup action is matched.
        selected=[t for t in data['trials'][:10] if any(s[3] for s in t['steps'])]
        f.write(str(len(selected))+'\n')
        for t in selected:
            f.write(' '.join(map(str,[t['face'],*t['initial'],len(t['steps'])]))+'\n')
            for k,x,y,active,speed,hurt,p in t['steps']:f.write(f'{k} {x} {y} {active}\n')


def packed(rgba):
    rgba=np.asarray(Image.fromarray(rgba).resize((45,45),Image.Resampling.NEAREST));anchor=(23,34)
    mask=np.pad(rgba[:,:,3]!=0,1);halo=mask.copy()
    for dy in (-1,0,1):
        for dx in (-1,0,1):halo[1:-1,1:-1]|=mask[1+dy:mask.shape[0]-1+dy,1+dx:mask.shape[1]-1+dx]
    level=np.pad(grayscale(rgba[:,:,:3],True),1);level[~mask]=0
    yy,xx=np.nonzero(halo)
    if not len(xx):return (0,0,0,0,0),[]
    x0,x1=int(xx.min()),int(xx.max())+1;y0,y1=int(yy.min()),int(yy.max())+1
    width=32 if x1-x0<=32 else 64;h=y1-y0
    levels=np.zeros((h,width),np.uint8);masks=np.zeros((h,width),bool)
    levels[:,:x1-x0]=level[y0:y1,x0:x1];masks[:,:x1-x0]=halo[y0:y1,x0:x1]
    words=[]
    for x in range(0,width,32):words+=sprite_words(levels[:,x:x+32],masks[:,x:x+32],32)
    return (x0-anchor[0]-1,y0-anchor[1]-1,h,width//32,x1-x0),words


def pack(data,actors):
    def array(name,typ,rows):
        def braces(v):return '{'+','.join(braces(x) if isinstance(x,list) else str(x) for x in v)+'}'
        dims=[];v=rows
        while isinstance(v,list):dims.append(len(v));v=v[0]
        return 'static const '+typ+' '+name+''.join(f'[{n}]' for n in dims)+'='+braces(rows)+';\n'
    header='/* Source roll and destruction art, packed locally. */\n'
    header+=f'#define ROLL_LEN {len(data["roll_speed"])}\n#define FX_POSES {len(actors)}\n'
    for name,typ,key in (('roll_pose','u8','roll'),('roll_speed','u16','roll_speed'),('roll_guard','u8','roll_guard')):
        header+=array(name,typ,data[key])
    for name,seq in zip(('bush_fx','grass_fx','death_fx'),data['effects']):
        header+=f'#define {name.upper()}_LEN {len(seq)}\n'+array(name,'u8',seq)
    words=[];meta=[];shift=[];packed_poses=[]
    for rgba in actors:
        m,p=packed(rgba);meta.append([len(words),*m]);words+=p;packed_poses.append(p)
    base=len(words)*4;shift_offsets=[65535]*len(actors);spans=[1 if m[5]<=17 else 2 for m in meta]
    # Prioritize frequently displayed narrow poses. Fast aligned masked
    # rows fit entirely in32 bits; wide/clipped poses use ExtGraph.
    weights={i:sum(s.count(i) for s in data['roll']+data['effects']) for i in range(len(actors))}
    for i in sorted(range(len(actors)),key=lambda i:weights[i],reverse=True):
        _,x,y,h,parts,w=meta[i];p=packed_poses[i]
        span=spans[i]
        if not h or w>32 or base+(len(shift)+h*48*span)*4>65516:continue
        shift_offsets[i]=base+len(shift)*4
        for n in range(16):
            for row in range(h):
                shift.extend([p[row]>>n,p[row+h]>>n,(~((~p[row+h*2]&0xffffffff)>>n))&0xffffffff])
                if span==2:
                    shift.extend([(p[row]<<(32-n))&0xffffffff if n else 0,
                                  (p[row+h]<<(32-n))&0xffffffff if n else 0,
                                  (~((~p[row+h*2]<<(32-n))&0xffffffff))&0xffffffff if n else 0xffffffff])
    header+=f'#define FX_SIZE {(len(words)+len(shift))*4}\n'
    header+=array('fx_art','s16',meta)+array('fx_shift','u16',shift_offsets)+array('fx_span','u8',spans)
    bank='mizfx'
    for order,suffix in (('<',''),('>','.be')):
        payload=struct.pack(order+f'{len(words)+len(shift)}I',*(words+shift))
        assert len(payload)<65518
        (GAME/f'{bank}{suffix}.bin').write_bytes(payload)
    print('Effects bank',bank,len(payload),'bytes;',sum(s!=65535 for s in shift_offsets),'fast poses')
    (GAME/'effects_generated.h').write_text(header)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--pack-only',action='store_true');args=parser.parse_args()
    if args.pack_only:
        data=json.loads((GAME/'fixtures/effects.json').read_text());write_fixtures(data);pack(data,np.load(GAME/'fixtures/source_effects.npz')['actors'])
    else:measure()
