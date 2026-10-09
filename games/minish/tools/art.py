#!/usr/bin/env python3
"""Decode original Woods text backgrounds and non-affine OBJ pixels offline.

Only the measured mode-0, 4bpp, unwindowed study frames are supported.
All extracted and converted commercial data stays in ignored local banks.
"""
import argparse
import json
from pathlib import Path
import numpy as np
from PIL import Image
from reference import (GBA, ROM, OUTPUT, PLAYER, ROOM, MAP_BOTTOM, MAP_TOP,
                       ORIGIN_X, ORIGIN_Y, extract, raw, position)
from traversal import GAME, place, BUTTONS
from reference import digest

SIZES = (((8,8),(16,16),(32,32),(64,64)),
         ((16,8),(32,8),(32,16),(64,32)),
         ((8,16),(8,32),(16,32),(32,64)))


def words(data):
    return np.frombuffer(data, dtype='<u2')


def palette(data):
    p = words(data)
    # The pinned libretro build uses RGB565; green gets its low bit zero.
    # Match the runner's Pillow BGR;16 conversion, including integer rounding.
    native = ((p&31)<<11)|((p&0x3e0)<<1)|((p>>10)&31)
    return np.asarray(Image.frombytes('RGB',(len(p),1),native.astype('<u2').tobytes(),
                                     'raw','BGR;16'))[0]


def tiles4(data):
    b = np.frombuffer(data,dtype=np.uint8).reshape(-1,8,4)
    a = np.empty((len(b),8,8),dtype=np.uint8)
    a[:,:,::2] = b & 15; a[:,:,1::2] = b >> 4
    return a


def subtile(tiles, descriptors, px, py):
    """Text BG descriptors, with palette and both flips preserved."""
    tx = np.where(descriptors & 0x400,7-px,px)
    ty = np.where(descriptors & 0x800,7-py,py)
    value = tiles[descriptors & 1023,ty,tx]
    return value + ((descriptors >> 12) << 4), value != 0


def bg_layer(snapshot, n):
    io = words(snapshot['io']); ctrl = int(io[4+n])
    assert not ctrl & 128, '8bpp BG not part of this study'
    base, screen = ((ctrl>>2)&3)*0x4000, ((ctrl>>8)&31)*0x800
    tile = tiles4(snapshot['vram'][base:base+0x8000])
    height, width = (512 if ctrl & 0x8000 else 256), (512 if ctrl & 0x4000 else 256)
    y,x = np.indices((160,240))
    x = (x+int(io[8+n*2])) % width; y = (y+int(io[9+n*2])) % height
    block = (y>>8)*(width>>8)+(x>>8)
    index = screen//2 + block*1024 + ((y>>3)&31)*32 + ((x>>3)&31)
    descriptors = words(snapshot['vram'])[index]
    color, opaque = subtile(tile,descriptors,x&7,y&7)
    return color,opaque,ctrl&3


def objects(snapshot):
    io = words(snapshot['io']); vram = tiles4(snapshot['vram'][0x10000:])
    result = []
    for i,(a,b,c,_) in enumerate(words(snapshot['oam']).reshape(128,4)):
        a,b,c = int(a),int(b),int(c)
        if a&0x300 == 0x200: continue
        assert not a & 0x100, 'affine OBJ requires a separate decoder'
        assert not a & 0x2000 and (a>>14)<3
        w,h = SIZES[a>>14][b>>14]
        y,x = np.indices((h,w))
        sx,sy = (w-1-x if b&0x1000 else x),(h-1-y if b&0x2000 else y)
        stride = w>>3 if int(io[0])&64 else 32
        ids = (c&1023)+(sy>>3)*stride+(sx>>3)
        value = vram[ids,sy&7,sx&7]
        index = 256+(c>>12)*16+value.astype(np.uint16)
        rgba = np.concatenate((palette(snapshot['palette'])[index],
                               (value!=0).astype(np.uint8)[...,None]*255),axis=-1)
        ox,oy = b&511,a&255
        if ox>=256: ox-=512
        if oy>=160: oy-=256
        result.append({'i':i,'x':ox,'y':oy,'tile':c&1023,'palette':c>>12,
                       'priority':(c>>10)&3,'mode':(a>>10)&3,'rgba':rgba})
    return result


def composite(snapshot):
    io = words(snapshot['io']); dc = int(io[0])
    assert dc & 7 == 0 and not dc & 0xe000, 'mode/window unsupported'
    pal = palette(snapshot['palette'])
    rgb = np.tile(pal[0],(160,240,1))
    owner = np.full((160,240),5,dtype=np.uint8)
    layers = []
    for n in range(4):
        if dc & (0x100<<n):
            colors,mask,priority = bg_layer(snapshot,n)
            layers.append(((priority,1,n),n,pal[colors],mask,False))
    for obj in objects(snapshot):
        full = np.zeros((160,240,4),np.uint8)
        x,y = obj['x'],obj['y']; h,w = obj['rgba'].shape[:2]
        x0,y0,x1,y1 = max(0,x),max(0,y),min(240,x+w),min(160,y+h)
        if x0>=x1 or y0>=y1:continue
        full[y0:y1,x0:x1] = obj['rgba'][y0-y:y1-y,x0-x:x1-x]
        assert obj['mode'] in (0,1), 'OBJ window unsupported'
        layers.append(((obj['priority'],0,obj['i']),4,full[:,:,:3],full[:,:,3]!=0,obj['mode']==1))
    for _,n,colors,mask,semi in sorted(layers,reverse=True,key=lambda x:x[0]):
        blend = int(io[0x50//2]); mode = (blend>>6)&3
        first = semi or (mode==1 and bool(blend&(1<<n)))
        if first:
            targets = ((np.uint16(blend)>>(8+owner))&1).astype(bool)
            alpha = int(io[0x52//2]); a,b = min(alpha&31,16),min((alpha>>8)&31,16)
            assert (a,b) == (0,16), 'other blend weights need native five-bit arithmetic'
            mixed = rgb
            rgb[mask & targets] = mixed.astype(np.uint8)[mask & targets]
            rgb[mask & ~targets] = colors[mask & ~targets]
        else:
            assert mode in (0,1), 'brightness effects unsupported'
            rgb[mask] = colors[mask]
        # Zero-weight BG3 is visually transparent; keep the provenance below.
        owner[mask & ~targets if first and a == 0 else mask] = n
    return rgb,owner


def world_layers(snapshot, assets):
    """World map composed from ROM metatiles and the measured live tile banks."""
    result = []
    for name,base in (('bottom',0),('top',0x4000)):
        ids = words(assets['map_'+name]).reshape(63,63)
        descriptors = words(assets['metatiles_'+name]).reshape(-1,4)
        y,x = np.indices((320,720))
        entries = descriptors[ids[y>>4,x>>4],((y&8)>>2)+((x&8)>>3)]
        tile = tiles4(snapshot['vram'][base:base+0x8000])
        color,mask = subtile(tile,entries,x&7,y&7)
        result.append((color,mask))
    return result


def inspect():
    out = GAME/'captures'; out.mkdir(exist_ok=True)
    gba = GBA(ROM)
    try:
        gba.load(OUTPUT/'woods.state'); gba.step([])
        s = gba.snapshot(); rgb,owner = composite(s)
        original = np.asarray(gba.image())
        diff = np.any(rgb!=original,axis=-1)
        print('composite differences',int(diff.sum()), 'by owner',[(i,int((diff&(owner==i)).sum())) for i in range(6)])
        Image.fromarray(rgb).save(out/'decoded_rgb.png')
        Image.fromarray(diff.astype(np.uint8)*255).save(out/'decode_diff.png')
        assets = extract(); (bot,bm),(top,tm) = world_layers(s,assets)
        world = palette(s['palette'])[np.where(tm,top,np.where(bm,bot,0))]
        Image.fromarray(world).save(out/'world_rgb.png')
        gx = int(gba.read(ROOM+0xa,2))-ORIGIN_X
        gy = int(gba.read(ROOM+0xc,2))-ORIGIN_Y
        crop = world[gy:gy+160,gx:gx+240]
        mask = (owner==1)|(owner==2)|(owner==5)
        print('world differences on background',int((np.any(crop!=original,axis=-1)&mask).sum()),int(mask.sum()))
    finally: gba.close()


def grayscale(rgb,actor=False):
    wide = rgb.astype(np.uint16)
    lum = (wide[...,0]*30+wide[...,1]*59+wide[...,2]*11)//100
    return (3-np.digitize(lum,[70,170,215] if actor else [64,112,160])).astype(np.uint8)


def sprite_words(level,mask,width=16):
    output = []
    for bit in (0,1):
        output += [sum(int((v>>bit)&1)<<(width-1-x) for x,v in enumerate(row)) for row in level]
    output += [sum(int(not v)<<(width-1-x) for x,v in enumerate(row)) for row in mask]
    return output


def hero_pixels(snapshot,previous):
    """Attribute parts by the previous entity's VRAM allocation and palette.

    The completed video/OAM depicts the previous logical pose and coordinates,
    not the post-frame entity fields. Canonical anchor is (16,32) in 32x40.
    """
    anchor_x,anchor_y = previous['screen']
    canvas = np.zeros((40,32,4),np.uint8)
    parts = [o for o in objects(snapshot) if previous['tile']<=o['tile']<previous['tile']+32
             and o['palette']==previous['palette']]
    assert parts, ('missing actor',previous)
    for o in reversed(parts):
        rgba = o['rgba']; yy,xx = np.nonzero(rgba[:,:,3])
        x = xx+o['x']-anchor_x+16; y = yy+o['y']-anchor_y+32
        assert np.all((x>=1)&(x<31)&(y>=1)&(y<39)), ('actor/outline exceeds canvas',previous,x.min(),x.max(),y.min(),y.max())
        canvas[y,x] = rgba[yy,xx]
    return canvas,parts


def entity(gba):
    x,y = position(gba)
    cx = gba.read(ROOM+0xa,2)-ORIGIN_X; cy = gba.read(ROOM+0xc,2)-ORIGIN_Y
    return {
        'screen':[(x>>16)-cx,(y>>16)-cy],
        'tile':gba.read(PLAYER+0x60,2),'palette':gba.read(PLAYER+0x1a)&15,
        'pose':[gba.read(PLAYER+0x58),gba.read(PLAYER+0x1e),bool(gba.read(PLAYER+0x18)&64)],
        'facing':gba.read(PLAYER+0x14),'duration':gba.read(PLAYER+0x59),
        'priority':gba.read(PLAYER+0x19)>>6}


def extract_art():
    out = GAME/'captures'; out.mkdir(exist_ok=True)
    fixtures = GAME/'fixtures'; fixtures.mkdir(exist_ok=True)
    gba = GBA(ROM); poses = {}; timelines = {}; checked = {'rgb_pixels':0,'background_pixels':0,'actor_pixels':0,'frames':0}
    def validate(s):
        rgb,owner = composite(s)
        video = np.asarray(gba.image())
        assert np.array_equal(rgb,video), ('source RGB mismatch',int(np.any(rgb!=video,axis=-1).sum()))
        checked['rgb_pixels'] += 240*160; checked['frames'] += 1
        return video,owner
    def capture(previous):
        s = gba.snapshot(); video,owner = validate(s)
        rgba,parts = hero_pixels(s,previous)
        key = tuple(previous['pose'])
        if key in poses: assert np.array_equal(poses[key],rgba), ('unstable pose anchor',key)
        else: poses[key] = rgba
        # Verify exposed owned pixels against the video, excluding opaque BG
        # occlusion, which the complete compositor verifies independently.
        yy,xx = np.nonzero(rgba[:,:,3]); sx=xx+previous['screen'][0]-16; sy=yy+previous['screen'][1]-32
        inside = (sx>=0)&(sx<240)&(sy>=0)&(sy<160)
        xx,yy,sx,sy=xx[inside],yy[inside],sx[inside],sy[inside]
        visible = owner[sy,sx]==4
        assert np.array_equal(rgba[yy[visible],xx[visible],:3],video[sy[visible],sx[visible]])
        checked['actor_pixels'] += int(visible.sum())
        return key
    try:
        gba.load(OUTPUT/'woods.state'); gba.step([]); s = gba.snapshot(); validate(s)
        assets = extract(); (bottom,bm),(top,tm) = world_layers(s,assets)
        scene_rgb = palette(s['palette'])[np.where(tm,top,np.where(bm,bottom,0))]
        # Prove ROM map/metatile composition against source video at route
        # checkpoints, using each frame's live graphics (animated environment).
        trial = json.loads((fixtures/'traversal.json').read_text())[-1]
        place(gba,*trial['spawn'])
        for frame,k in enumerate(trial['keys']):
            cx=gba.read(ROOM+0xa,2)-ORIGIN_X
            cy=gba.read(ROOM+0xc,2)-ORIGIN_Y
            gba.step([n for b,n in BUTTONS.items() if b&k])
            if frame % 35 != 0 and frame != len(trial['keys'])-1: continue
            s2 = gba.snapshot(); video,owner = validate(s2)
            (bc,bmask),(tc,tmask) = world_layers(s2,assets)
            rgb = palette(s2['palette'])[np.where(tmask,tc,np.where(bmask,bc,0))]
            # Completed video uses the previous room camera. Hardware BG
            # offsets include an extra eight-pixel vertical cache margin.
            valid_h=min(160,320-cy); valid_w=min(240,720-cx)
            mask = (owner[:valid_h,:valid_w]!=0)&(owner[:valid_h,:valid_w]!=4)
            assert np.array_equal(rgb[cy:cy+valid_h,cx:cx+valid_w][mask],video[:valid_h,:valid_w][mask]), ('world map pixels',frame,cx,cy)
            checked['background_pixels'] += int(mask.sum())
        # Stabilize the camera once for each facing. Each controlled pose trial
        # writes only X/Y back to its recorded anchor to avoid walls/enemies;
        # animation, OAM, tile DMA and palettes remain the original's work.
        replay_frames=0
        for face,key in enumerate((1,8,4,2)):
            recorded=None
            for repeat in range(2):
                place(gba,320,184)
                for _ in range(80):gba.step([])
                stream=[]; hashes=[]
                for step in range(100):
                    prev=entity(gba)
                    gba.write(PLAYER+0x2c,(ORIGIN_X+320)<<16,4)
                    gba.write(PLAYER+0x30,(ORIGIN_Y+184)<<16,4)
                    gba.step([BUTTONS[key]])
                    pose=capture(prev) if not repeat else tuple(prev['pose'])
                    stream.append({'pose':pose,'facing':prev['facing'],'duration':prev['duration']})
                    hashes.append(digest(gba))
                # Release: capture settled idle after the source chooses this face.
                for _ in range(5):
                    prev=entity(gba);gba.step([])
                    idle=capture(prev) if not repeat else tuple(prev['pose'])
                    hashes.append(digest(gba))
                if not repeat:
                    recorded=(stream,idle,hashes)
                    timelines[str(face)] = stream
                    timelines['idle'+str(face)] = idle
                else:
                    assert recorded == (stream,idle,hashes), ('actor trial replay',face)
                    replay_frames+=len(hashes)
        # An ordinary walkable foot position hides part of the head behind BG1;
        # the 70% compositor oracle covers the native side of this occlusion.
        place(gba,246,160)
        for _ in range(80):gba.step([])
        previous=entity(gba);gba.step([])
        snapshot=gba.snapshot();video,owner=validate(snapshot)
        rgba,parts=hero_pixels(snapshot,previous)
        assert all(o['priority']==2 for o in parts)
        yy,xx=np.nonzero(rgba[:,:,3])
        sx=xx+previous['screen'][0]-16;sy=yy+previous['screen'][1]-32
        hidden=owner[sy,sx]==1
        assert int(hidden.sum())>=50, 'canopy study must hide a meaningful actor area'
        Image.fromarray(video).save(out/'source_canopy.png')
        # Slowing floor classes do not all share OBJ priority. Study the four
        # placements containing action 38 or 52, preserving source rendering.
        depth_trials=[]; depth_replay_frames=0
        for anchor in ((424,200),(424,216),(104,248),(120,248)):
            for key in (1,4):
                recorded=None
                for repeat in range(2):
                    place(gba,*anchor)
                    for _ in range(80):gba.step([])
                    stream=[];hashes=[]
                    for _ in range(12):
                        previous=entity(gba)
                        gba.write(PLAYER+0x2c,(ORIGIN_X+anchor[0])<<16,4)
                        gba.write(PLAYER+0x30,(ORIGIN_Y+anchor[1])<<16,4)
                        gba.step([BUTTONS[key]])
                        parts=[o for o in objects(gba.snapshot())
                               if previous['tile']<=o['tile']<previous['tile']+32
                               and o['palette']==previous['palette']]
                        assert parts and all(o['priority']==previous['priority'] for o in parts)
                        assert previous['priority'] in (1,2)
                        stream.append(int(previous['priority']==2));hashes.append(digest(gba))
                    if not repeat:
                        recorded=(stream,hashes);depth_trials.append((anchor,key,stream))
                    else:
                        assert recorded==(stream,hashes), ('depth replay',anchor,key)
                        depth_replay_frames+=len(stream)
        with (fixtures/'depth.txt').open('w') as f:
            f.write(str(len(depth_trials))+'\n')
            for (ax,ay),key,stream in depth_trials:
                f.write(f'{ax} {ay} {key} {len(stream)}\n')
                f.write(' '.join(map(str,stream))+'\n')
        level = grayscale(scene_rgb)
        keys=sorted(poses); pose_ids={k:i for i,k in enumerate(keys)}
        # Unoutlined source pixels support other offline scales without trying
        # to recover the original silhouette from an already dilated mask.
        np.savez_compressed(fixtures/'source_art.npz',scene=scene_rgb,foreground=tm,
                            actors=np.stack([poses[k] for k in keys]),
                            rom_sha256=gba.identity['rom_sha256'])
        # Walking: capture one settled cycle after startup; every pose lasts 3
        # source updates. Identify the period rather than assuming frame count.
        walks=[]; periods=[]
        for face in range(4):
            seq=[pose_ids[tuple(f['pose'])] for f in timelines[str(face)][10:]]
            period=next(p for p in range(1,49) if seq[:-p]==seq[p:])
            assert period%3==0
            poses_cycle=[pose_ids[tuple(f['pose'])] for f in timelines[str(face)][1:1+period:3]]
            walks.append(poses_cycle);periods.append(period)
        assert len(set(periods))==1
        idle=[pose_ids[tuple(timelines['idle'+str(i)])] for i in range(4)]
        with (fixtures/'animation.txt').open('w') as f:
            f.write('4\n')
            for face,key in enumerate((1,8,4,2)):
                f.write(f'{key} {len(timelines[str(face)])}\n')
                for sample in timelines[str(face)]:
                    f.write(str(pose_ids[tuple(sample['pose'])])+'\n')
        generated = f'''/* Generated from the exact local USA ROM and checked reference frames. */
#define CHAR_POSES {len(keys)}
#define WALK_POSES {periods[0]//3}
static const u8 idle_pose[4] = {{{','.join(map(str,idle))}}};
static const u8 walk_pose[4][WALK_POSES] = {{
{','.join('{'+','.join(map(str,row))+'}' for row in walks)}
}};
'''
        (GAME/'generated.h').write_text(generated)
        Image.fromarray((255-level*85).astype(np.uint8)).save(out/'world_grey.png')
        metadata={'rom_sha256':gba.identity['rom_sha256'],'core':gba.identity,'checks':checked,
                  'actor_poses':len(keys),
                  'walk_period_source_updates':periods,'pose_keys':keys,'idle':idle,'walk':walks,
                  'actor_replay_frames':replay_frames,
                  'depth_replay_frames':depth_replay_frames,
                  'occlusion':{'position':[246,160],'source_hidden_pixels':int(hidden.sum())},
                  'source_anchor':'previous entity and room camera; completed OAM/video lag one update',
                  'controlled_pose_trial':{'anchor':[320,184],'settle_frames':80,'walk_frames':100,
                      'pokes':'X/Y reset before each walking frame only'},
                  'environment':'static palette and animated tile phase from the first neutral study frame'}
        (fixtures/'art.json').write_text(json.dumps(metadata,indent=2)+'\n')
        print('Art checks:',checked,'poses',len(keys),'periods',periods)
    finally:gba.close()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--inspect',action='store_true')
    args=parser.parse_args()
    inspect() if args.inspect else extract_art()
