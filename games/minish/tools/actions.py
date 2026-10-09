#!/usr/bin/env python3
"""Measure ordinary sword actions and bush mutations; prepare local art banks.

Only equipment and initial position/facing are injected. Each input trial is
replayed twice with complete emulator digests. No story flags or action-state
writes are used. Commercial pixels and numerical fixtures stay ignored.
"""
import json
import struct
import sys
import numpy as np
from PIL import Image
from reference import (GBA, ROM, OUTPUT, PLAYER, SAVE, MAP_BOTTOM, ORIGIN_X,
                       ORIGIN_Y, raw, position, digest, extract, rom_bytes, sha)
from traversal import GAME, place, BUTTONS, terrain, point
from art import (entity, objects, composite, palette, tiles4, subtile,
                 grayscale, sprite_words)
from ground import ground_gray

PSTATE = 0x03003f80
TIMING = [0,1,2,3,4,4,5,5,6,6,7,8,8,9,9]


def setup(gba, x, y, face):
    place(gba,320,184)
    gba.write(SAVE+0xb4,1)  # Smith sword in A, owned in two-bit inventory.
    gba.write(SAVE+0xf2,4)
    for _ in range(80): gba.step([])
    gba.write(PLAYER+0x2c,(ORIGIN_X+x)<<16,4)
    gba.write(PLAYER+0x30,(ORIGIN_Y+y)<<16,4)
    gba.write(PLAYER+0x14,face*2)


def sword_pixels(snapshot, previous):
    canvas=np.zeros((56,64,4),np.uint8)
    ax,ay=previous['screen']
    parts=[o for o in objects(snapshot) if
           (previous['tile']<=o['tile']<previous['tile']+32 and o['palette']==6)
           or (368<=o['tile']<400 and o['palette']==1)]
    assert parts and all(o['priority']==2 for o in parts)
    for o in reversed(parts):
        yy,xx=np.nonzero(o['rgba'][:,:,3])
        x=xx+o['x']-ax+32; y=yy+o['y']-ay+36
        assert ((x>0)&(x<63)&(y>0)&(y<55)).all()
        canvas[y,x]=o['rgba'][yy,xx]
    return canvas


def packed_actor(rgba, zoom):
    anchor=(32,36)
    if zoom:
        rgba=np.asarray(Image.fromarray(rgba).resize((45,39),Image.Resampling.NEAREST))
        # Resize the anchor with the same nearest-neighbour canvas convention.
        anchor=(23,25)
    mask=rgba[:,:,3]!=0
    padded=np.pad(mask,1)
    halo=padded.copy()
    for dy in (-1,0,1):
        for dx in (-1,0,1):
            halo[1:-1,1:-1]|=padded[1+dy:padded.shape[0]-1+dy,1+dx:padded.shape[1]-1+dx]
    level=np.pad(grayscale(rgba[:,:,:3],True),1);level[~padded]=0
    yy,xx=np.nonzero(halo); x0,x1=int(xx.min()),int(xx.max())+1
    y0,y1=int(yy.min()),int(yy.max())+1
    width=32 if x1-x0<=32 else 64
    h=y1-y0; levels=np.zeros((h,width),np.uint8); masks=np.zeros((h,width),bool)
    levels[:,:x1-x0]=level[y0:y1,x0:x1];masks[:,:x1-x0]=halo[y0:y1,x0:x1]
    words=[]
    for x in range(0,width,32): words+=sprite_words(levels[:,x:x+32],masks[:,x:x+32],32)
    return (x0-anchor[0]-1,y0-anchor[1]-1,h,width//32,x1-x0),words


def main(pack_only=False):
    assets=extract();rom=rom_bytes()
    ids=np.frombuffer(assets['map_bottom'],dtype='<u2').reshape(63,63)
    types=np.frombuffer(assets['types_bottom'],dtype='<u2')
    cut_cells=[(x,y,int(ids[y,x]),48 if types[ids[y,x]]==63 else 18)
               for y in range(20) for x in range(45) if types[ids[y,x]] in (63,78)]
    assert len(cut_cells)==53
    grid=np.zeros(2048,np.uint8)
    for i,(x,y,*_) in enumerate(cut_cells):grid[y*64+x]=i+1
    if pack_only:
        saved=np.load(GAME/'fixtures/source_actions.npz')
        pack(cut_cells,grid,saved['actors'],dict(zip((48,18),saved['patches'])),rom)
        return
    poses={};trials=[];replayed=rgb_pixels=0;visual_checks=[]
    # Edge/restart, hold, recovery motion and facing during a locked swing.
    for face in range(4):
        for seq in ([16]+[0]*23,[16]*30,[16 if n%2==0 else 0 for n in range(30)],
                    [16]+[[8,4,2,1][face]]*24):
            trials.append((320,184,face,seq))
    trials += [(320,184,face,[16,8,24]+[0]*21) for face in range(4)]
    # Study every bush with a clear neighbouring standing anchor. Interior
    # cells in dense beds are reached after cutting outer cells in gameplay.
    collision,rows,_=terrain()
    bush_trials=0
    for x,y,*_ in cut_cells:
        candidates=((x*16+8,y*16+30,0),(x*16-7,y*16+11,1),
                    (x*16+8,y*16-5,2),(x*16+23,y*16+11,3))
        for px,py,face in candidates:
            if all(not point(collision,rows,rom,px+dx,py+dy) for dx,dy in
                   ((5,0),(5,-6),(-5,0),(-5,-6),(3,2),(-3,2),(3,-8),(-3,-8))):
                trials.append((px,py,face,[16]+[0]*23));bush_trials+=1;break
    bush_trial_end=len(trials);visual_trial=len(trials)
    trials += [(424,160,0,[16]+[0]*16+[1]*13+[16]+[0]*16+[1]*13+[16]+[0]*16),
               (632,128,0,[16]+[0]*23)]
    for x,y,face,travel,swings in ((56,78,0,1,2),(232,94,0,1,3),
                                (473,59,1,8,2),(456,155,2,4,2),(632,128,0,1,3)):
        trials.append((x,y,face,([16]+[0]*16+[travel]*13)*(swings-1)+[16]+[0]*23))
    gba=GBA(ROM)
    try:
        gba.load(OUTPUT/'woods.state');gba.step([]);snapshot=gba.snapshot()
        desc=np.frombuffer(assets['metatiles_bottom'],dtype='<u2').reshape(-1,4)
        tiles=tiles4(snapshot['vram'][:0x8000]);pal=palette(snapshot['palette'])
        patches={}
        yy,xx=np.indices((16,16))
        for replacement in (48,18):
            color,_=subtile(tiles,desc[replacement,((yy&8)>>2)+((xx&8)>>3)],xx&7,yy&7)
            patches[replacement]=pal[color]
        recorded=[]
        for ti,(x,y,face,keys) in enumerate(trials):
            first=None
            for repeat in range(2):
                setup(gba,x,y,face);base=raw(gba,MAP_BOTTOM+4,8192)
                stream=[];hashes=[]
                for n,k in enumerate(keys):
                    previous=entity(gba);was_attack=bool(gba.read(PSTATE+4))
                    gba.step([name for bit,name in {**BUTTONS,16:'A'}.items() if k&bit])
                    if not repeat and ti<16:
                        snap=gba.snapshot();rgb,_=composite(snap)
                        assert np.array_equal(rgb,np.asarray(gba.image()))
                        rgb_pixels+=240*160
                        if was_attack:
                            f=previous['facing']//2
                            phase=previous['pose'][1]-(130 if f==0 else 106 if f==2 else 118)
                            assert 0<=phase<10
                            rgba=sword_pixels(snap,previous);key=f*10+phase
                            if key in poses:assert np.array_equal(poses[key],rgba),key
                            else:poses[key]=rgba
                    if not repeat and ti==visual_trial and n in (4,5):
                        # Live collision changes immediately; BG upload/video
                        # displays the new metatile one source update later.
                        snap=gba.snapshot();rgb,owner=composite(snap)
                        assert np.array_equal(rgb,np.asarray(gba.image()))
                        cx=x-previous['screen'][0];cy=y-previous['screen'][1]
                        sx,sy=416-cx,128-cy
                        region=rgb[sy:sy+16,sx:sx+16];exposed=owner[sy:sy+16,sx:sx+16]!=4
                        matches=np.all(region==patches[48],axis=2)&exposed
                        assert bool(matches.sum()==exposed.sum())==(n==5)
                        visual_checks.append([n,int(exposed.sum()),int(matches.sum())]);rgb_pixels+=240*160
                    active=bool(gba.read(PSTATE+4));now=entity(gba)
                    pose=44+(now['facing']//2)*10+now['pose'][1]-(130 if now['facing']==0 else 106 if now['facing']==4 else 118) if active else -1
                    current=raw(gba,MAP_BOTTOM+4,8192)
                    cuts=[i for i,(cx,cy,old,new) in enumerate(cut_cells)
                          if struct.unpack_from('<H',current,(cy*64+cx)*2)[0]==new]
                    # Every change is a measured bush replacement, including
                    # the live collision ID and terrain action after cutting.
                    changed=[i for i in range(4096) if current[i*2:i*2+2]!=base[i*2:i*2+2]]
                    assert set(changed)=={cut_cells[i][1]*64+cut_cells[i][0] for i in cuts}
                    for i in cuts:
                        cx,cy,_,_=cut_cells[i];cell=cy*64+cx
                        assert gba.read(MAP_BOTTOM+0x2004+cell)==0
                        assert gba.read(MAP_BOTTOM+0xb004+cell)==10
                    px,py=position(gba)
                    stream.append([k,px>>8,py>>8,int(active),pose,cuts])
                    hashes.append(digest(gba))
                if first is None:first=(stream,hashes)
                else:assert first==(stream,hashes),(ti,'replay');replayed+=len(keys)
            recorded.append({'spawn':[x,y],'face':face,'steps':first[0],
                             'replay_sha256':sha(json.dumps(first[1],sort_keys=True).encode())})
        assert len(poses)==40,sorted(poses)
        assert all(t['steps'][-1][-1] for t in recorded[20:bush_trial_end])
        assert {i for t in recorded for s in t['steps'] for i in s[-1]}==set(range(53))
        # Ground decoding is independently checked against a settled cut in
        # both bush styles, excluding the player's shadow and transient FX.
        patch_checks=0
        for x,y,target in ((424,160,(26,8)),(616,128,(38,6))):
            setup(gba,x,y,0)
            for n in range(40):gba.step(['A'] if n==0 else [])
            snap=gba.snapshot();rgb,owner=composite(snap)
            assert np.array_equal(rgb,np.asarray(gba.image()))
            cx=gba.read(0x03000bf0+0xa,2)-ORIGIN_X;cy=gba.read(0x03000bf0+0xc,2)-ORIGIN_Y
            tx,ty=target;replacement=48 if x==424 else 18
            sx,sy=tx*16-cx,ty*16-cy
            exposed=owner[sy:sy+16,sx:sx+16]!=4
            assert np.array_equal(rgb[sy:sy+16,sx:sx+16][exposed],patches[replacement][exposed])
            patch_checks+=int(exposed.sum())
    finally:gba.close()
    fixtures=GAME/'fixtures'
    (fixtures/'actions.json').write_text(json.dumps({'trials':recorded,'replayed_frames':replayed,
        'rgb_pixels':rgb_pixels,'ground_pixels':patch_checks,'bush_video_lag':visual_checks,
        'rom_sha256':sha(ROM.read_bytes()),'state_sha256':sha((OUTPUT/'woods.state').read_bytes()),
        'equipment_writes':[[hex(SAVE+0xb4),1,1],[hex(SAVE+0xf2),4,1]],
        'settle_spawn':[320,184],'neutral_settle_frames':80},indent=2)+'\n')
    with (fixtures/'actions.txt').open('w') as f:
        f.write(f'{len(recorded)}\n')
        for t in recorded:
            f.write(f'{t["spawn"][0]} {t["spawn"][1]} {t["face"]} {len(t["steps"])}\n')
            for k,x,y,active,pose,cuts in t['steps']:
                flags=sum(1<<i for i in cuts)
                f.write(f'{k} {x} {y} {active} {pose} '+ ' '.join(str((flags>>(8*i))&255) for i in range(7))+'\n')
    raw_poses=np.asarray([poses[i] for i in range(40)])
    np.savez_compressed(fixtures/'source_actions.npz',actors=raw_poses,
                        patches=np.asarray([patches[48],patches[18]]))
    pack(cut_cells,grid,raw_poses,patches,rom)
    print(f'Sword: 40 poses, {replayed} exact replay frames, {rgb_pixels} RGB pixels; {len(cut_cells)} bush cells')


def pack(cut_cells,grid,raw_poses,patches,rom):
    fixtures=GAME/'fixtures'
    header=['/* Measured ordinary sword and local bush art. */','#define CUT_COUNT 53',
            '#define ACTION_POSES 40']
    def table(name,ctype,values):
        header.append(f'static const {ctype} {name}[{len(values)}]={{'+','.join(map(str,values))+'};')
    table('cut_x','u16',[x*16 for x,y,*_ in cut_cells]);table('cut_y','u16',[y*16 for x,y,*_ in cut_cells])
    table('cut_fx_kind','u8',[int(new==18) for x,y,old,new in cut_cells])
    table('sword_phase','u8',TIMING)
    # Offsets at source updates 1,4,10. They are point samples, not a rectangle.
    points=struct.unpack_from('<28b',rom,0x129072)
    samples=[(13,3,14),(13,2,12),(11,1,12),(13,2,12)]
    table('sword_dx','s8',[points[(n-1)*2]*(-1 if face==3 else 1)
                          for face,ns in enumerate(samples) for n in ns])
    table('sword_dy','s8',[points[(n-1)*2+1] for ns in samples for n in ns])
    source_art=np.load(fixtures/'source_art.npz')
    for zoom in (False,True):
        bank=bytearray(grid);patch_words=[];patch_offsets=[];unique_patches={}
        patch_x=[];patch_y=[];patch_h=[]
        ix=(np.arange(504)*2+1)*10//14;iy=(np.arange(224)*2+1)*10//14
        for x,y,old,new in cut_cells:
            rgb=patches[new].copy()
            fg=source_art['foreground'][y*16:y*16+16,x*16:x*16+16]
            ground=ground_gray(rgb)
            ground[fg]=grayscale(source_art['scene'][y*16:y*16+16,x*16:x*16+16])[fg]
            if zoom:
                sx,ex=np.searchsorted(ix,[x*16,x*16+16]);sy,ey=np.searchsorted(iy,[y*16,y*16+16])
                ground=ground[(iy[sy:ey]-y*16)[:,None],ix[sx:ex]-x*16]
            else:sx,sy=x*16,y*16
            h,w=ground.shape;level=np.zeros((h,16),np.uint8);mask=np.zeros((h,16),bool)
            level[:,:w]=ground;mask[:,:w]=True
            original=(source_art['scene'][iy[sy:ey][:,None],ix[sx:ex]] if zoom else
                      source_art['scene'][y*16:y*16+16,x*16:x*16+16])
            # XOR changes only differing bits of the immutable base image.
            # ExtGraph's two-plane XOR blitter avoids the mask read/merge.
            level[:,:w]^=grayscale(original)
            nonzero=np.nonzero(level)[0];first,last=int(nonzero.min()),int(nonzero.max())+1
            level=level[first:last];mask=mask[first:last];sy+=first;h=last-first
            key=(h,level.tobytes())
            if key not in unique_patches:
                unique_patches[key]=len(patch_words)
                words=sprite_words(level,mask)
                # Sixteen shifts are shared by identical patches in both
                # builds. Dense combat needs the same cheap normal-scale path.
                for shift in range(16):patch_words += [(v<<16)>>shift for v in words[:h*2]]
            patch_offsets.append(unique_patches[key])
            patch_x.append(int(sx));patch_y.append(int(sy));patch_h.append(h)
        # Grid, normal u16 / zoom pre-shifted u32 patches, u32 actor rows.
        po=2048;ao=po+len(patch_words)*4
        if ao&3:ao+=2
        actor_words=[];actor_offsets=[];actor_meta=[]
        for rgba in raw_poses:
            meta,words=packed_actor(rgba,zoom)
            actor_offsets.append(len(actor_words));actor_words+=words;actor_meta.append(meta)
        for order,suffix in (('<',''),('>','.be')):
            data=bytes(bank)+struct.pack(order+f'{len(patch_words)}I',*patch_words)
            data+=b'\0'*(ao-len(data));data+=struct.pack(order+f'{len(actor_words)}I',*actor_words)
            assert len(data)<65518
            (GAME/f'{"mizact" if zoom else "miact"}{suffix}.bin').write_bytes(data)
        header+=['#ifdef MINISH_ZOOM' if zoom else '#ifndef MINISH_ZOOM',
                 f'#define ACTION_SIZE {len(data)}',f'#define ACTION_PATCH_OFFSET {po}',f'#define ACTION_ACTOR_OFFSET {ao}']
        table('cut_patch_offset','u16',patch_offsets);table('cut_draw_x','u16',patch_x);table('cut_draw_y','u16',patch_y)
        table('cut_patch_h','u8',patch_h);table('sword_offset','u16',actor_offsets)
        for i,name in enumerate(('sword_x','sword_y','sword_h','sword_parts','sword_width')):
            table(name,'s8' if i<2 else 'u8',[v[i] for v in actor_meta])
        header.append('#endif')
    (GAME/'actions_generated.h').write_text('\n'.join(header)+'\n')
    print('Packed normal/70% sword and bush banks')


if __name__=='__main__':main('--pack-only' in sys.argv)
