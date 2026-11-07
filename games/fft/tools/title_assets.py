#!/usr/bin/env python3
"""Observe the local PS1 title and reuse its existing art for the TI front end."""
from pathlib import Path
import argparse,hashlib,json,struct
import numpy as np
from PIL import Image
import oracle,extract
from review_files import write_changed,save_image

HERE=Path(__file__).resolve().parents[1]
REFERENCE=HERE.parents[1]/'sources/fft_ps1/title'

def observe(disc):
    REFERENCE.mkdir(parents=True,exist_ok=True)
    p=oracle.PSX(str(disc))
    # Cold boot: one Start skips the logo/intro; a second skips the opening movie.
    for frame in range(1501):
        p.pressed=oracle.key('START') if 360<=frame<364 else 0;p.run()
    for frame in range(301):
        p.pressed=oracle.key('START') if frame<4 else 0;p.run()
    p.save(str(REFERENCE/'title.state'));save_image(p.image(),REFERENCE/'title-original.png')
    # Hide only the foreground menu/copyright records to recover the original art.
    # This is an art capture, separate from the unmodified title observation.
    for frame in range(4):
        for index in range(16):
            record=0x80085d04+36*index
            if p.read(record+4,4) in (3,4,5,6,0x11,0x13):
                p.write(record,p.read(record,4)|0x10,4)
        p.pressed=0;p.run()
    save_image(p.image(),REFERENCE/'title-clean.png')
    p.load(str(REFERENCE/'title.state'))
    for frame in range(501):
        p.pressed=oracle.key('DOWN') if frame<4 else oracle.key('CIRCLE') if 40<=frame<44 else 0
        p.run()
        if frame==25:save_image(p.image(),REFERENCE/'title-continue.png')
    save_image(p.image(),REFERENCE/'load-background.png')
    p.save(str(REFERENCE/'load.state'))
    for frame in range(1501):
        p.pressed=oracle.key('CIRCLE') if 300<=frame<304 else 0;p.run()
    save_image(p.image(),REFERENCE/'load-empty.png')
    write_changed(REFERENCE/'observation.json',(json.dumps(dict(
        source='US disc, pcsx_rearmed HLE, cold boot, controller inputs',
        first_start=[360,363],second_start=[1501,1504],title_frame=1801,
        art_only_hide='OPEN36 anim3..6/17/19, flags|16; title observation unmodified',
        menu=['New Game','Continue','Tutorial','Sound'],
        continue_screen='memory-card screen; blank card reports No saved data.'),indent=2)+'\n').encode())

def picture(path,title=False,faded=False):
    im=Image.open(path).convert('RGB')
    im=(im.crop((0,0,320,200)).resize((160,100),Image.Resampling.BOX) if title
        else im.resize((160,100),Image.Resampling.BOX))
    lum=np.array(im.convert('L'))
    grey=np.where(lum<48,3,np.where(lum<96,2,np.where(lum<170,1,0))).astype(np.uint8)
    if faded:
        # Same source pixels/logo; a soft offline paper wash, no extra sprite.
        y,x=np.indices(grey.shape)
        strength=np.maximum(0,48-(x-80)**2//56-(y-45)**2//32)
        strength=np.where((y>=35)&(y<=64)&(x>=9)&(x<=149),0,strength)
        light=np.minimum(255,lum.astype(np.uint16)+strength)
        grey=np.where(light<48,3,np.where(light<96,2,np.where(light<170,1,0))).astype(np.uint8)
    return b''.join(np.packbits((grey&bit)!=0,axis=1).tobytes() for bit in (1,2))

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('disc',type=Path)
    ap.add_argument('--observe',action='store_true')
    ap.add_argument('--variant',choices=['current','a','b'],default='current')
    ap.add_argument('--legacy-serif',action='store_true',help='Historical title-B strips')
    ap.add_argument('--out',type=Path);args=ap.parse_args()
    if args.observe or not (REFERENCE/'load-background.png').exists():observe(args.disc.resolve())
    if args.variant=='b':
        from flow_assets import title_strip
        blobs=[extract.zx0pack.pack(picture(REFERENCE/'load-background.png')),
               extract.zx0pack.pack(title_strip('FINAL FANTASY TACTICS',11)+title_strip('68k',15)) if args.legacy_serif else b'\0\0']
    else:
        blobs=[extract.zx0pack.pack(picture(REFERENCE/'title-clean.png',True,args.variant=='a')),
               extract.zx0pack.pack(picture(REFERENCE/'load-background.png'))]
    bank=bytearray(((b'FTB1' if args.legacy_serif else b'FTB2') if args.variant=='b' else b'FTL1')+bytes(4))
    for i,blob in enumerate(blobs):
        if len(bank)&1:bank.append(0)
        struct.pack_into('>H',bank,4+2*i,len(bank));bank+=blob
    out=args.out or HERE/'campaign/data/fftitle.bin'
    out.parent.mkdir(parents=True,exist_ok=True);write_changed(out,bank)
    report=dict(variant=args.variant,bytes=len(bank),decoded_background=4000,
                original_sha256=hashlib.sha256((REFERENCE/'title-original.png').read_bytes()).hexdigest())
    write_changed(out.with_suffix('.json') if args.out else REFERENCE/'assets.json',
                  (json.dumps(report,indent=2)+'\n').encode());print(report)

if __name__=='__main__':main()
