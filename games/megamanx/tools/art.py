#!/usr/bin/env python3
"""Local ROM/PPU art: bounded Highway map and original animated actors."""
import json,struct
from pathlib import Path
import numpy as np
from PIL import Image
from reference import *
from rom import level,DATA,word
from terrain import WIDTH,TOP,HEIGHT
from measure import suppress,row
from ppu import PPU,composite

ART=OUT/'art'
def grey(rgba,background=False):
    reduced=np.asarray(Image.fromarray(rgba).resize((rgba.shape[1]//2,rgba.shape[0]//2),Image.Resampling.BOX))
    rgb=reduced[:,:,:3].astype(np.uint16)
    lum=(rgb[:,:,0]*77+rgb[:,:,1]*150+rgb[:,:,2]*29)>>8
    shades=(lum<64).astype(np.uint8) if background else (3-np.digitize(lum,[64,132,210])).astype(np.uint8)
    return shades,reduced[:,:,3]>=96

def scenery():
    v=level();ppu=PPU(*[(OUT/('start.'+k)).read_bytes() for k in ('vram','cgram','oam','regs')])
    front=np.zeros((HEIGHT,WIDTH,4),dtype=np.uint8);stride=v['width']<<4
    for y in range(HEIGHT>>4):
        for x in range(WIDTH>>4):
            ident=v['cells'][(y+(TOP>>4))*stride+x]
            for q in range(4):
                tile=ppu.bg_tile(word(v['ptr']['maps']+ident*8+q*2))
                dx,dy=(q&1)*8,(q>>1)*8
                front[y*16+dy:y*16+dy+8,x*16+dx:x*16+dx+8]=tile
    # Exact map/CHR extraction check against original visible BG1 tile pixels.
    streamed=ppu.background(0)[:,:256]
    assert np.array_equal(front[:256,:256],streamed),'ROM foreground != streamed BG1'
    bg=ppu.background(1);yy,xx=np.indices((HEIGHT,WIDTH))
    backdrop=bg[(yy+TOP)%bg.shape[0],xx%bg.shape[1]].copy()
    backdrop[backdrop[:,:,3]==0]=[*ppu.colors[0],255]
    Image.fromarray(front).save(ART/'foreground.png')
    color=backdrop.copy();composite(color,front,0,0);Image.fromarray(color).save(ART/'highway-color.png')
    shades,_=grey(backdrop,True);fg,opaque=grey(front);shades[opaque]=fg[opaque]
    Image.fromarray(255-shades*85).save(ART/'highway-grey.png')
    collision=bytes(v['collision'][(y+(TOP>>4))*stride+x] for y in range(HEIGHT>>4) for x in range(WIDTH>>4))
    planes=b''.join(np.packbits((shades>>n)&1,axis=1).tobytes() for n in (0,1))
    bank=b'MXM2'+struct.pack('>6H',WIDTH,TOP,HEIGHT,64,16,16+len(collision)+len(planes))+collision+planes
    assert len(bank)==17424
    (GAME/'mmxmap.bin').write_bytes(bank);(GAME/'mmxmap.be.bin').write_bytes(bank)

def anchor(s,slot=0xba8):
    return (s.read(0x7e0000+slot+5,2)-s.read(CAMX,2),
            s.read(0x7e0000+slot+8,2)-s.read(CAMY,2))

def actor(s,kind='hero',slot=0xe68,origin=None):
    pp=PPU(*s.ppu());r=s.ram
    a=0xba8 if kind=='hero' else slot
    x,y=origin if origin is not None else anchor(s,a)
    canvas=np.zeros((64,64,4),dtype=np.uint8)
    for n,ox,oy,pal,img in pp.objects():
        if n<16:continue
        if kind=='hero' and pal!=1:continue
        if kind!='hero' and pal==1:continue
        if kind=='shot' and s.ram[slot+10] and pal!=3:continue
        if abs(ox-x)<32 and abs(oy-y)<40:composite(canvas,img,ox-x+32,oy-y+32)
    return canvas

def packed(rgba,projectile=False):
    shades,opaque=grey(rgba);shades=np.pad(shades,1);opaque=np.pad(opaque,1)
    if projectile:shades[opaque & (shades==1)]=2
    outline=opaque.copy()
    for dy in (-1,0,1):
        for dx in (-1,0,1):outline|=np.roll(np.roll(opaque,dy,0),dx,1)
    shades[~opaque]=0
    ys,xs=np.nonzero(outline)
    if len(ys)==0:return None
    x0,y0,x1,y1=xs.min(),ys.min(),xs.max()+1,ys.max()+1
    width=8 if x1-x0<=8 else 16 if x1-x0<=16 else 32
    h=int(y1-y0);assert h<=32 and x1-x0<=32
    pixels=np.zeros((h,width),dtype=np.uint8);mask=np.ones((h,width),dtype=np.uint8)
    pixels[:,:x1-x0]=shades[y0:y1,x0:x1];mask[:,:x1-x0]=~outline[y0:y1,x0:x1]
    rows=[]
    for p in ((pixels&1),(pixels>>1),mask):
        rows+= [int.from_bytes(np.packbits(line).tobytes(),'big') for line in p]
    return width,h,int(x0)-17,int(y0)-17,rows

def sprites():
    groups={};sources=[];launch={};s=SNES(ROM)
    def add(name,rgba,f):
        frame=packed(rgba,name in ('pellet','medium','large'))
        if frame is None:return
        group=groups.setdefault(name,[])
        if frame not in group:
            group.append(frame);Image.fromarray(rgba).save(ART/f'{name}-{len(group)-1}.png')
            sources.append(dict(group=name,frame=f,player=row(s)))
    try:
        for name,buttons,count,step in [('idle',[],300,10),('run',['RIGHT'],70,2),
                                      ('jump',['B'],23,3),('fall',['B'],44,3),
                                      ('shoot',['Y'],20,3),('run_shoot',['RIGHT','Y'],65,3),
                                      ('jump_shoot',['B','Y'],23,3)]:
            s.load(OUT/'start.state')
            for f in range(count):
                suppress(s)
                pressed=buttons
                if name in ('shoot','jump_shoot','run_shoot') and f%12:pressed=[b for b in buttons if b!='Y']
                origin=anchor(s);run(s,pressed)
                if f%step==0 and (name!='fall' or f>=23):add(name,actor(s,origin=origin),f)
        for name,buttons,count in [('slide',['RIGHT'],18),('kick',['RIGHT','B'],25),('wall_shoot',['RIGHT','Y'],20)]:
            s.load(OUT/'wall.state')
            for f in range(count):
                suppress(s);origin=anchor(s);run(s,buttons)
                if f%3==0:add(name,actor(s,origin=origin),f)
        # Windup and forced-away firing use distinct original gun poses;
        # do not animate the wall push after the flight has already begun.
        for action,press in [('kick_windup_shoot',12),('kick_launch_shoot',17)]:
            s.load(OUT/'wall_charge/ready-0.state')
            for f in range(25):
                for a in range(0xe68,0x1228,64):s.ram[a]=0
                buttons=['RIGHT']+(['B'] if f>=10 else [])+(['Y'] if f==press else [])
                origin=anchor(s);run(s,buttons)
                if s.read(0x7e0baa)!=16 or f<press:continue
                phase=s.read(0x7e0bab)
                if action=='kick_windup_shoot' and f==15:add('kick_push',actor(s,origin=origin),f)
                if action=='kick_windup_shoot' and phase==2:add(action,actor(s,origin=origin),f)
                if action=='kick_launch_shoot' and phase==4 and f>=16:add(action,actor(s,origin=origin),f)
        s.load(OUT/'hit.state')
        for f in range(32):
            origin=anchor(s);run(s,[])
            if f%2==0 and s.read(0x7e0baa)==14:add('hurt',actor(s,origin=origin),f)
        s.load(OUT/'combat/natural-260.state')
        for f in range(28):
            origin=anchor(s,0xe68);run(s,[])
            if f%2==0:add('roller',actor(s,'enemy',origin=origin),f)
        s.load(OUT/'enemy/brake.state')
        for f in range(42):
            origin=anchor(s,0xe68);run(s,[])
            if f%2==0:add('roller_broken',actor(s,'enemy',origin=origin),f)
        # Projectile poses captured from their real pools, not redrawn circles.
        for kind,hold in [('pellet',1),('medium',31),('large',101)]:
            s.load(OUT/'start.state')
            for f in range(hold):run(s,['Y'])
            if hold>1:run(s,[])
            for f in range(24):
                if f:
                    origin=anchor(s,0x1228);run(s,[])
                else:origin=anchor(s,0x1228)
                if kind in ('medium','large') and f==(10 if kind=='medium' else 7):
                    launch[kind]=len(groups[kind])
                add(kind,actor(s,'shot',0x1228,origin=origin),f)
        s.load(OUT/'combat/shoot-290.state')
        # Enemy-death effects are OAM composites around the saved enemy anchor.
        for f in range(16):
            run(s,[])
            if f%2==0:add('explosion',actor(s,'enemy'),f)
    finally:s.close()
    required=('idle','run','jump','fall','shoot','run_shoot','jump_shoot','slide','kick','wall_shoot','kick_push','kick_windup_shoot','kick_launch_shoot','hurt','roller','roller_broken','pellet','medium','large')
    assert all(groups.get(k) for k in required)
    for name in list(groups):
        groups[name+'_left']=[packed(np.asarray(Image.open(ART/f'{name}-{i}.png'))[:,::-1],name in ('pellet','medium','large')) for i in range(len(groups[name]))]
    frames=[];ids=['/* Generated from local original poses; ignored. */']
    for name,poses in groups.items():
        ids.extend([f'#define MXART_{name.upper()} {len(frames)}',f'#define MXART_{name.upper()}_N {len(poses)}'])
        base=name.removesuffix('_left')
        if base in launch:
            assert 0<launch[base]<len(poses),(name,'missing distinct flight poses')
            ids.extend([f'#define MXART_{name.upper()}_FLIGHT {len(frames)+launch[base]}',
                        f'#define MXART_{name.upper()}_FLIGHT_N {len(poses)-launch[base]}',
                        f'#define MXART_{name.upper()}_START_N {launch[base]}'])
        frames.extend(poses)
    ids.append(f'#define MXART_COUNT {len(frames)}')
    header=bytearray(b'MXA1'+struct.pack('>6H',len(frames),0,16,8,0,0));records=bytearray();native=bytearray();big=bytearray()
    start=16+len(frames)*8
    for width,h,ox,oy,rows in frames:
        pad=(-(start+len(big)))&3;native+=bytes(pad);big+=bytes(pad)
        offset=start+len(big);size=width//8*h
        records+=struct.pack('>BBbbHH',width,h,ox,oy,offset,size)
        for value in rows:
            native+=value.to_bytes(width//8,sys.byteorder);big+=value.to_bytes(width//8,'big')
    end=start+len(big);assert end<65536;struct.pack_into('>H',header,6,end)
    (GAME/'mmxart.bin').write_bytes(header+records+native);(GAME/'mmxart.be.bin').write_bytes(header+records+big)
    (GAME/'generated/art_ids.h').write_text('\n'.join(ids)+'\n')
    (ART/'sources.json').write_text(json.dumps(sources,indent=2)+'\n')
    print('ROM art:',{k:len(v) for k,v in groups.items() if not k.endswith('_left')},'frames',len(frames),'bank',end)

def main():
    ART.mkdir(exist_ok=True);(GAME/'generated').mkdir(exist_ok=True);scenery();sprites()
if __name__=='__main__':main()
