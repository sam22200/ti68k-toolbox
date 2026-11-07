#!/usr/bin/env python3
"""Source-backed campaign flow bank: observed seven-member roster, deployment and rewards.

The recruitment state is oracle.boot's unmodified Gariland deployment door.
Its generated names/stats are a fixed native starting-party fixture, not a claim
of reproducing FFT's random recruitment generator. Existing artwork is reused.
FQF1: seven u16 section offsets: roster, deployment, rewards, title strips, prices,
Gariland beats/script. Derived data remain local; no source code is translated.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct

import numpy as np
from PIL import Image, ImageDraw, ImageFont
import battle
import extract
import interface
import oracle
import scenes
import story_text

HERE=Path(__file__).resolve().parents[1]
TITLES=[('CONGRATULATIONS!',11),('THIS BATTLE IS',8),('COMPLETE!',8),
        ('BONUS MONEY',12),('WAR TROPHIES',12),('CHAPTER 1',10),
        ('THE MEAGER',12),("Military Academy's",9),('Auditorium',10),
        ('Magic City Gariland',11)]


def title_strip(text,size):
    font='/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf'
    f=ImageFont.truetype(font,size)
    while f.getbbox(text)[2]>154:
        size-=1;f=ImageFont.truetype(font,size)
    im=Image.new('L',(160,16),128);d=ImageDraw.Draw(im)
    box=f.getbbox(text);x=(160-box[2])//2;y=-box[1]+1
    d.text((x+1,y+1),text,font=f,fill=0);d.text((x,y),text,font=f,fill=255)
    a=np.array(im);g=np.where(a==128,-1,np.where(a>212,0,np.where(a>127,1,np.where(a>42,2,3))))
    # Twenty 8x16 sprites, no runtime font scaling or large framebuffer bank.
    return b''.join(interface.planes(g[:,x:x+8],8) for x in range(0,160,8))

def legacy_titles():
    titles=bytes((len(TITLES),))+b''.join(title_strip(t,s) for t,s in TITLES)
    for ch in '0123456789 GIL':
        im=Image.new('L',(8,16),128);draw=ImageDraw.Draw(im)
        f=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf',10)
        y=1-f.getbbox(ch)[1];draw.text((1,y+1),ch,font=f,fill=0);draw.text((0,y),ch,font=f,fill=255)
        a=np.array(im);g=np.where(a==128,-1,np.where(a>212,0,np.where(a>127,1,np.where(a>42,2,3))))
        titles+=interface.planes(g,8)
    return titles


def roster(p):
    out=[]
    for i in range(16):
        raw=bytes(p.read(0x80057f74+256*i+k) for k in range(256))
        if raw[1]==255:continue
        female=bool(raw[4]&64);ramza=raw[2]==1
        gfx=0 if ramza else (4 if raw[2]==75 else 2)+female
        unit=bytearray((raw[2],gfx,2 if ramza else int(female),0 if ramza else raw[0xce],
                        raw[0x16],raw[0x15],raw[0x17],raw[0x18],raw[6]>>4,raw[7]))
        unit+=b''.join(raw[0x19+3*k:0x1c+3*k][::-1] for k in range(5))
        unit+=bytes(0 if raw[0x0e+i]==255 else raw[0x0e+i] for i in (0,1,2,3,6))
        unit+=b''.join(raw[8+2*k:10+2*k][::-1] for k in range(3))
        unit+=b''.join(raw[0x6e+2*k:0x70+2*k][::-1] for k in range(20))
        unit+=b''.join(raw[0x96+2*k:0x98+2*k][::-1] for k in range(20))
        unit+=raw[0x2b:0x64]+bytes(3)  # Mime has no source learned-action row.
        assert len(unit)==176
        out.append(bytes(unit))
    assert len(out)==7 and [u[0] for u in out]==[1,74,74,75,74,74,75]
    inventory=bytes(p.read(0x800596e0+i) for i in range(256))
    money=oracle.var(p,0x2c);assert money==2000
    return bytes((len(out),))+struct.pack('>I',money)+inventory+b''.join(out)


def deployment(p):
    ptr=p.read(0x801cd270,4)
    raw=bytes(p.read(ptr+256*12+i) for i in range(12))
    bitmap=int.from_bytes(raw[:4],'little');cx,cy=raw[4:6]
    orient=raw[7]&15;world=(orient-1+(raw[7]>>4))&3
    cells=[[0]*5 for _ in range(5)];coords=[]
    order=([(y,x) for y in range(5) for x in range(5)] if orient==0 else
           [(y,x) for x in range(4,-1,-1) for y in range(5)] if orient==1 else
           [(y,x) for y in range(4,-1,-1) for x in range(4,-1,-1)] if orient==2 else
           [(y,x) for x in range(5) for y in range(4,-1,-1)])
    for bit,(y,x) in enumerate(order):cells[y][x]=int(bool(bitmap&(1<<bit)))
    assert [''.join(map(str,row)) for row in cells]==['00000','00011','11111','10000','00000']
    for r in range(5):
        for c in range(5):
            x,z=((cx-c+2,cy+r-2),(cx-r+2,cy-c+2),(cx+c-2,cy-r+2),(cx+r-2,cy+c-2))[world]
            coords.extend((cells[r][c],x,z))
    # Source facing0(-y) -> native3(-z), etc; same shared conversion as battle.py.
    face=(3,0,2,1)[world]
    initial=[p.read(oracle.DEPLOY_GRID+i) for i in range(25)]
    assert p.read(oracle.DEPLOY_COUNT)==1 and initial.count(0)==1
    cell=initial.index(0)
    return struct.pack('>HBB',200,raw[8],face)+bytes(coords)+bytes((cell,)),dict(record=raw.hex(),grid=cells,world_orientation=world,initial_ramza_cell=cell)


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('disc');ap.add_argument('state',type=Path)
    ap.add_argument('--legacy-serif',action='store_true',help='Historical whole-line strip control')
    ap.add_argument('--out',type=Path)
    args=ap.parse_args();p=oracle.PSX(args.disc);p.load(str(args.state))
    assert oracle.var(p,oracle.EVENT)==9
    disk,files=extract.disc_files(args.disc);d=battle.Disc(args.disc)
    boot=roster(p);deploy,info=deployment(p)
    # Orbonne has no result sequence. Gariland metadata follows native unit order.
    rewards=bytearray((0,4,3))+bytes(22)+bytes((1,255,255))
    for i in range(11):
        rec=d.entd(0x184,i) if i<6 else bytes(32)
        rewards+=bytes((rec[0x1e],rec[0x1f]))
    prices=b''.join(struct.pack('>H',int.from_bytes(d.at(battle.ITEMS+12*i+8,2),'little')>>2) for i in range(256))
    titles=legacy_titles() if args.legacy_serif else b''
    evt=disk.read(*files['EVENT/TEST.EVT']);beats=[]
    for mid in range(7):
        name,text=story_text.message(evt,10,mid)
        beats.append(bytes((10 if mid<6 else 12,0,0,0))+name.encode()+b'\0'+text.encode()+b'\0')
    script=struct.pack('>H',2)+struct.pack('>4B2h',3,10,2,0,0,0)+struct.pack('>4B2h',4,12,3,0,0,0)
    parts=(boot,deploy,bytes(rewards),titles,prices,scenes.beat_block(beats),script)
    if not args.legacy_serif:
        from shared_font_assets import font_parts
        title_strings,font,_,_=font_parts()
        parts=parts[:3]+(title_strings,)+parts[4:]+(font,)
    body=bytearray((b'FQF1' if args.legacy_serif else b'FQF2')+bytes(2*len(parts)))
    for i,data in enumerate(parts):
        if len(body)&1:body.append(0)
        struct.pack_into('>H',body,4+2*i,len(body));body+=data
    out=args.out or HERE/'campaign/data/ffflow.bin';out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(body)
    report=dict(source='Unmodified original recruitment/deployment state; disc ENTD/text/price tables',
                state_sha256=hashlib.sha256(args.state.read_bytes()).hexdigest(),
                starting_money=2000,party_members=7,deployment=info,bytes=len(body),
                starting_party='Fixed observed source roster; no claim of native random recruitment equality')
    report_path=HERE/'campaign/x/qa/flow-source.json';report_path.parent.mkdir(parents=True,exist_ok=True)
    report_path.write_text(json.dumps(report,indent=2)+'\n')
    print('ffflow:',len(body),'bytes; Ramza plus six recruits, legal five-unit grid, source rewards/dialogue and reviewed serif titles')


if __name__=='__main__':main()
