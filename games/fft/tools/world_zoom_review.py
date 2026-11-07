#!/usr/bin/env python3
"""Offline zoom candidates from the existing reviewed FWM3 bank, not new sprite rips.

Candidate banks stay under x/qa/review-4. The active map, progression and save are
untouched. A separate context-only bank shows the Mandalia-Igros road so its top
clearance can be reviewed before that location's campaign data is implemented.
"""
from pathlib import Path
import json
import struct
import subprocess
import tempfile

import numpy as np
from PIL import Image, ImageDraw

import interface
import world_assets
from review_files import publish_images, save_image, write_changed

HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[1]
OUT=HERE/'campaign/x/qa/review-4'
SHOTS=OUT/'new'
SCALES=(('100',1,1),('75',3,4),('67',2,3))


def run(args,cwd=ROOT):
    subprocess.run(list(map(str,args)),cwd=cwd,check=True)


def unpack(raw):
    with tempfile.TemporaryDirectory(prefix='fft-zoom-zx0-') as name:
        p=Path(name);(p/'in').write_bytes(raw)
        result=subprocess.run([ROOT/'tools/bin/dzx0','-f',p/'in',p/'out'],capture_output=True)
        if result.returncode and b'too long' in result.stderr and raw[-1:]==b'\0':
            # FWM3 aligns the next section to a word; the CLI rejects that pad.
            (p/'in').write_bytes(raw[:-1])
            result=subprocess.run([ROOT/'tools/bin/dzx0','-f',p/'in',p/'out'],capture_output=True)
        result.check_returncode()
        return (p/'out').read_bytes()


def decode(bank):
    assert bank[:4]==b'FWM3'
    offsets=struct.unpack_from('>5H',bank,4)
    a,b=offsets[:2];stride,count=struct.unpack_from('>HH',bank,a)
    starts=struct.unpack_from('>'+str(count)+'H',bank,a+4)
    light=[];dark=[]
    for i,start in enumerate(starts):
        end=a+starts[i+1] if i+1<count else b
        raw=unpack(bank[a+start:end]);assert len(raw)==stride*16
        light.append(np.unpackbits(np.frombuffer(raw[:stride*8],np.uint8)).reshape(8,-1))
        dark.append(np.unpackbits(np.frombuffer(raw[stride*8:],np.uint8)).reshape(8,-1))
    g=np.concatenate(light)|np.concatenate(dark)*2
    raw=unpack(bank[b+8:offsets[2]]);assert len(raw)==5*156
    frames=[]
    for i in range(5):
        bits=np.unpackbits(np.frombuffer(raw[156*i:156*(i+1)],np.uint8)).reshape(3,26,16).astype(np.int8)
        frames.append(np.where(bits[2],-1,bits[0]+2*bits[1]).astype(np.int8))
    return g,frames


def scaled_sprite(frame,n,d):
    if n==d:return frame.copy()
    # Reduce the already reviewed pose around its existing feet anchor(8,22).
    # Nearest sampling preserves the four greys; restore a one-pixel white rim.
    w,h=(16*n+d//2)//d,(26*n+d//2)//d
    small=np.asarray(Image.fromarray((frame+1).astype(np.uint8)).resize((w,h),Image.Resampling.NEAREST)).astype(np.int8)-1
    out=np.full((26,16),-1,np.int8)
    x,y=8-(8*n+d//2)//d,22-(22*n+d//2)//d
    out[y:y+h,x:x+w]=small
    mask=out>=0;p=np.pad(mask,1)
    ring=(p[:-2,1:-1]|p[2:,1:-1]|p[1:-1,:-2]|p[1:-1,2:])&~mask
    out[ring]=0
    assert (out[0]<0).all() and (out[:,0]<0).all() and (out[:,-1]<0).all()
    return out


def scaled_bank(bank,g,frames,n,d):
    offsets=struct.unpack_from('>5H',bank,4)
    if n!=d:
        h,w=g.shape
        g=np.asarray(Image.fromarray(g).resize(((w*n+d//2)//d,(h*n+d//2)//d),Image.Resampling.NEAREST))
        g=np.pad(g,((0,(-g.shape[0])%16),(0,(-g.shape[1])%16)),mode='edge')
    texture=world_assets.texture_block(g)
    animation=bank[offsets[1]:offsets[1]+8]+world_assets.zx0pack.pack(
        b''.join(interface.planes(scaled_sprite(f,n,d),16) for f in frames))
    body=bytearray(b'FWM3'+bytes(10)+bytes((g.shape[1]//16,g.shape[0]//16)))
    new=[]
    for data in (texture,animation):
        if len(body)&1:body.append(0)
        new.append(len(body));body+=data
    if len(body)&1:body.append(0)
    delta=len(body)-offsets[2]
    new.extend(o+delta for o in offsets[2:])
    body+=bank[offsets[2]:]
    struct.pack_into('>5H',body,4,*new)
    scale=lambda v:(v*n+d//2)//d
    for i in range(43):
        a=new[2]+8*i;x,y=struct.unpack_from('>hh',body,a)
        struct.pack_into('>hh',body,a,scale(x),scale(y))
        struct.pack_into('>H',body,a+6,struct.unpack_from('>H',body,a+6)[0]+delta)
    for i in range(48):
        a=new[3]+6*i;p=struct.unpack_from('>H',body,a+4)[0]+delta
        struct.pack_into('>H',body,a+4,p)
        for k in range(body[a+3]):
            x,y=struct.unpack_from('>hh',body,p+4*k)
            struct.pack_into('>hh',body,p+4*k,scale(x),scale(y))
    assert len(body)<65000
    return bytes(body)


def context_bank(bank):
    # Review only: show Igros using the same already-won Gariland flag as Mandalia.
    at=struct.unpack_from('>H',bank,12)[0];count=bank[at]
    story=bytes((count+1,))+bank[at+1:at+1+6*count]+struct.pack('>BBHBx',2,2,0,0)
    body=bytearray(bank)
    if len(body)&1:body.append(0)
    struct.pack_into('>H',body,12,len(body));body+=story
    return bytes(body)


def sheet(rows,name):
    out=Image.new('RGB',(3*492,len(rows)*330),'white');draw=ImageDraw.Draw(out)
    for row,entries in enumerate(rows):
        for col,(path,title) in enumerate(entries):
            x,y=col*492,row*330
            draw.text((x+5,y+5),title,fill='black')
            out.paste(Image.open(path).convert('RGB').resize((480,300),Image.Resampling.NEAREST),(x,y+24))
    save_image(out,SHOTS/name)


def main():
    SHOTS.mkdir(parents=True,exist_ok=True)
    frozen=OUT/'source-100.bin'
    if not frozen.exists():write_changed(frozen,(HERE/'campaign/data/ffwm.bin').read_bytes())
    bank=frozen.read_bytes();g,frames=decode(bank)
    before=OUT/'jobs-before.png'
    if not before.exists():write_changed(before,(HERE/'campaign/x/qa/review-3/new/jobs-after-8.png').read_bytes())
    records=[]
    with tempfile.TemporaryDirectory(prefix='fft-world-zoom-') as name:
        tmp=Path(name);data=tmp/'data';data.mkdir();shots=tmp/'shots';shots.mkdir()
        for src,dst in (('ffmg','ffmg'),('ffcj','ffcj'),('ffmg','ffneutral')):
            (data/(dst+'.bin')).symlink_to(HERE/'campaign/data'/(src+'.bin'))
        common=[ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
        run(['cc','-std=gnu99','-O1','-o',tmp/'map',HERE/'campaign/preview_world_zoom.c',*common])
        run(['cc','-std=gnu99','-O1','-o',tmp/'management',HERE/'campaign/preview_management.c',*common])
        save=HERE/'campaign/x/check/ffsave_won.sav'
        run([tmp/'management',shots,save],data);publish_images(shots,SHOTS/'management')
        for tag,n,d in SCALES:
            candidate=bank if n==d else scaled_bank(bank,g,frames,n,d)
            write_changed(OUT/tag/'ffwm.bin',candidate)
            for mode in ('current','north-context'):
                write_changed(data/'ffwm.bin',candidate if mode=='current' else context_bank(candidate))
                for p in shots.glob('*.png'):p.unlink()
                run([tmp/'map',shots,save],data);publish_images(shots,SHOTS/tag/mode)
            loc=struct.unpack_from('>H',candidate,8)[0]
            gx,gy=struct.unpack_from('>hh',candidate,loc+8*6)
            ix,iy=struct.unpack_from('>hh',candidate,loc+8*2)
            top=iy-(gy-50)
            if n!=d:assert top>=8 and abs(ix-gx)<70
            records.append(dict(scale=f'{n}/{d}',bank_bytes=len(candidate),igros_center_y=top,
                                source_sprite_canvas=[16,26],scaled_sprite=[round(16*n/d),round(26*n/d)]))
    sheet([[(SHOTS/tag/'current/map-1.png',f'{label}:current campaign locations') for tag,label in (('100','100% current'),('75','A:75%'),('67','B:67%'))],
           [(SHOTS/tag/'north-context/map-1.png',f'{label}:Mandalia-Igros road context ONLY') for tag,label in (('100','100% current'),('75','A:75%'),('67','B:67%'))]],'map-zoom-review.png')
    # Native-pixel strip as well as enlarged comparisons: label gutter stays outside LCD.
    native=Image.new('RGB',(3*168,124),'white');draw=ImageDraw.Draw(native)
    for col,(tag,label) in enumerate((('100','100%'),('75','A:75%'),('67','B:67%'))):
        draw.text((col*168+4,4),label,fill='black')
        native.paste(Image.open(SHOTS/tag/'north-context/map-1.png').convert('RGB'),(col*168,24))
    save_image(native,SHOTS/'map-zoom-native.png')
    carousel=Image.new('RGB',(984,328),'white');draw=ImageDraw.Draw(carousel)
    for col,(path,title) in enumerate(((before,'Previous:gray floor bars'),(SHOTS/'management/jobs-after-8.png','Requested:no gray bars / no diamond'))):
        draw.text((col*492+5,5),title,fill='black')
        carousel.paste(Image.open(path).convert('RGB').resize((480,300),Image.Resampling.NEAREST),(col*492,24))
    save_image(carousel,SHOTS/'carousel-clean-review.png')
    images=[Image.open(SHOTS/'management'/f'jobs-after-{i}.png').resize((640,400),Image.Resampling.NEAREST) for i in range(9)]
    save_image(images[0],SHOTS/'carousel-clean.gif',save_all=True,append_images=images[1:],duration=140,loop=0,disposal=2)
    images=[]
    for i in range(8):
        frame=Image.new('RGB',(984,328),'white');draw=ImageDraw.Draw(frame)
        for col,(tag,label) in enumerate((('75','A:75% / same reduced idle animation'),('67','B:67% / same reduced idle animation'))):
            draw.text((col*492+5,5),label,fill='black')
            frame.paste(Image.open(SHOTS/tag/'north-context'/f'map-{i}.png').convert('RGB').resize((480,300),Image.Resampling.NEAREST),(col*492,24))
        images.append(frame)
    save_image(images[0],SHOTS/'ramza-zoom-review.gif',save_all=True,append_images=images[1:],duration=[125,125,156,125,94,125,156,94],loop=0,disposal=2)
    write_changed(OUT/'review.json',(json.dumps(dict(source='Existing FWM3 source texture and five unique idle poses; no new rip',
        candidates=records,north_context='Igros is shown only in review banks, not unlocked in the campaign',
        active_bank_unchanged=(bank==(HERE/'campaign/data/ffwm.bin').read_bytes()),pending='User zoom/readability verdict before integration or benchmarks'),indent=2)+'\n').encode())
    print('Review only; active ffwm and save untouched:',SHOTS)


if __name__=='__main__':main()
