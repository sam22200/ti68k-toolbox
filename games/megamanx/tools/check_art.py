#!/usr/bin/env python3
"""Check original hero pixels and offline banks without emulator screenshots."""
import json,struct,sys
import numpy as np
from reference import *
from art import actor,anchor
from measure import suppress
from ppu import PPU,composite

def visible_pixels(s,rgba,origin,kind):
    """Exclude measured OBJ occlusion, not unexplained color mismatches.

    Gun flashes, charge sparks and wall dust use palettes other than X's.
    Lower OAM indices cover X even when the standalone hero extraction is
    correct. Resolve ownership before comparing with the independent video.
    """
    owners=np.full((64,64),-1,dtype=np.int8)
    composed=np.zeros((64,64,4),dtype=np.uint8)
    for n,x,y,pal,img in PPU(*s.ppu()).objects():
        x-=origin[0]-32;y-=origin[1]-32
        x0,y0,x1,y1=max(0,x),max(0,y),min(64,x+img.shape[1]),min(64,y+img.shape[0])
        if x1<=x0 or y1<=y0:continue
        opaque=img[y0-y:y1-y,x0-x:x1-x,3]!=0
        owners[y0:y1,x0:x1][opaque]=pal
        composite(composed,img,x,y)
    opaque=rgba[:,:,3]!=0
    owned=(owners==1) if kind=='hero' else ((owners>=0)&(owners!=1))
    yy,xx=np.nonzero(opaque)
    sx,sy=origin[0]-32+xx,origin[1]-32+yy
    video=np.asarray(s.image())
    inside=(sx>=0)&(sy>=0)&(sx<video.shape[1])&(sy<video.shape[0])
    yy,xx,sx,sy=yy[inside],xx[inside],sx[inside],sy[inside]
    hidden=~owned[yy,xx]
    # Every excluded pixel must actually show its covering OBJ in the video.
    if hidden.any():
        error=np.abs(video[sy[hidden],sx[hidden],:3].astype(np.int16)-
                     composed[yy[hidden],xx[hidden],:3].astype(np.int16))
        assert error.max()<=4,('unexplained occlusion',origin,int(error.max()))
    return video,yy[~hidden],xx[~hidden],sx[~hidden],sy[~hidden],int(hidden.sum())

def main():
    s=SNES(ROM)
    try:
        pixels=poses=error=0;actions=[]
        for buttons,count in [([],1),(['RIGHT'],40),(['B'],22),(['RIGHT','B'],22)]:
            s.load(OUT/'start.state')
            for frame in range(count):
                suppress(s);origin=anchor(s);run(s,buttons)
                rgba=actor(s,origin=origin);video=np.asarray(s.image())
                x,y=origin[0]-32,origin[1]-32
                yy,xx=np.nonzero(rgba[:,:,3]);expected=rgba[yy,xx,:3].astype(np.int16)
                actual=video[y+yy,x+xx,:3].astype(np.int16)
                difference=np.abs(actual-expected)
                # Snes9x exports RGB565; palette expansion differs by <=2.
                assert difference.max()<=4,(buttons,frame,difference.max(),np.count_nonzero(difference.max(1)>4))
                pixels+=len(xx);poses+=1;error=max(error,int(difference.max()))
        movement=dict(frames=poses,pixels=pixels,max_rgb_error=error)
        cases=[('shoot','start.state',['Y'],24,'hero',0xba8,0),
            ('run_shoot','start.state',['RIGHT','Y'],48,'hero',0xba8,0),
            ('jump_shoot','start.state',['B','Y'],40,'hero',0xba8,0),
            ('slide','wall.state',['RIGHT'],18,'hero',0xba8,0),
            ('kick','wall.state',['RIGHT','B'],25,'hero',0xba8,0),
            ('wall_shoot','wall.state',['RIGHT','Y'],20,'hero',0xba8,0),
            ('hurt','hit.state',[],32,'hero',0xba8,0),
            ('pellet','start.state',[],24,'shot',0x1228,1),
            ('medium','start.state',[],24,'shot',0x1228,31),
            ('large','start.state',[],24,'shot',0x1228,101),
            ('roller','combat/natural-260.state',[],28,'enemy',0xe68,0),
            ('roller_broken','enemy/brake.state',[],42,'enemy',0xe68,0),
            ('explosion','combat/shoot-290.state',[],16,'enemy',0xe68,0)]
        for name,state,buttons,count,kind,slot,hold in cases:
            s.load(OUT/state)
            for frame in range(hold):run(s,['Y'])
            checked=opaque=hidden=max_error=empty=0
            for frame in range(count):
                if kind=='hero':
                    # Preserve player shots, flashes and dust in action cases.
                    for address in range(0xe68,0x1228,64):s.ram[address]=0
                origin=anchor(s,slot);run(s,buttons)
                rgba=actor(s,kind,slot,origin)
                video,yy,xx,sx,sy,occluded=visible_pixels(s,rgba,origin,kind)
                hidden+=occluded
                if not len(xx):empty+=1;continue
                difference=np.abs(video[sy,sx,:3].astype(np.int16)-rgba[yy,xx,:3].astype(np.int16))
                assert difference.max()<=4,(name,frame,int(difference.max()))
                opaque+=len(xx);checked+=1;max_error=max(max_error,int(difference.max()))
            assert checked>=count//2 and opaque>100,(name,checked,opaque)
            actions.append(dict(action=name,kind=kind,frames=checked,pixels=opaque,
                                max_rgb_error=max_error,obj_occluded_pixels=hidden,empty_frames=empty))
            if kind=='hero':pixels+=opaque;poses+=checked
            error=max(error,max_error)
        for name,press in [('kick_windup_shoot',12),('kick_launch_shoot',17)]:
            s.load(OUT/'wall_charge/ready-0.state');checked=opaque=hidden=max_error=0
            for frame in range(25):
                for address in range(0xe68,0x1228,64):s.ram[address]=0
                buttons=['RIGHT']+(['B'] if frame>=10 else [])+(['Y'] if frame==press else [])
                origin=anchor(s);run(s,buttons)
                if frame<press:continue
                rgba=actor(s,origin=origin)
                video,yy,xx,sx,sy,occluded=visible_pixels(s,rgba,origin,'hero');hidden+=occluded
                assert len(xx)>100,(name,frame,'empty hero')
                difference=np.abs(video[sy,sx,:3].astype(np.int16)-rgba[yy,xx,:3].astype(np.int16))
                assert difference.max()<=4,(name,frame,int(difference.max()))
                opaque+=len(xx);checked+=1;max_error=max(max_error,int(difference.max()))
            actions.append(dict(action=name,kind='hero',frames=checked,pixels=opaque,
                                max_rgb_error=max_error,obj_occluded_pixels=hidden,empty_frames=0))
            pixels+=opaque;poses+=checked;error=max(error,max_error)
    finally:s.close()
    little=(GAME/'mmxart.bin').read_bytes();big=(GAME/'mmxart.be.bin').read_bytes()
    count,end=struct.unpack_from('>2H',big,4)
    assert len(little)==len(big)==end<65536 and big[:16+count*8]==little[:16+count*8]
    for i in range(count):
        width,h,ox,oy,offset,length=struct.unpack_from('>BBbbHH',big,16+i*8)
        assert width in (8,16,32) and h<=32 and length==width//8*h and offset%4==0
        a=np.frombuffer(little[offset:offset+length*3],dtype=f'=u{width//8}')
        b=np.frombuffer(big[offset:offset+length*3],dtype=f'>u{width//8}')
        assert np.array_equal(a,b)
        l,d,m=np.split(b,3);assert np.all((l|d)&m==0)
    report=dict(original_hero_pixels=pixels,original_hero_frames=poses,max_rgb_error=error,
                movement=movement,actions=actions,art_frames=count,
                art_bytes=end,map_bytes=len((GAME/'mmxmap.bin').read_bytes()),native_ti_rows_equal=True)
    (GAME/'x/art-checks.json').write_text(json.dumps(report,indent=2)+'\n')
    print(report)
if __name__=='__main__':main()
