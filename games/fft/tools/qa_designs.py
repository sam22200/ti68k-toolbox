#!/usr/bin/env python3
"""Local160x100 playtest design proposals; no shipping banks or saves are changed.

The map alternatives, carousel discovery placeholders and victory typography are
visual candidates, not source-faithful behavior or native performance evidence.
"""
from pathlib import Path
import math
import struct
import subprocess
import tempfile

from review_files import save_image, publish_images

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage

import art
import extract
import interface
import management_data
import units
import world_assets

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
OUT = HERE / 'campaign/x/qa'
PALETTE = np.array([255, 170, 85, 0], np.uint8)


def word(b, at):
    return int.from_bytes(b[at:at+2], 'big')


def sheet(names, path, columns=2):
    result = Image.new('RGB', (columns*492, math.ceil(len(names)/columns)*328), 'white')
    d = ImageDraw.Draw(result)
    for i, (name, title) in enumerate(names):
        x, y = i % columns * 492, i // columns * 328
        d.text((x+5, y+5), title, fill='black')
        im = Image.open(OUT / name).convert('RGB')
        result.paste(im.resize((480,300), Image.Resampling.NEAREST), (x, y+24))
    save_image(result,OUT / path)


def small_text(im, x, y, s, bank, shade=0):
    f = bank[word(bank,18):]  # management font, section7
    for c in s:
        k = max(0, min(94, ord(c)-32))
        advance = f[103+k]
        if x+advance > 159:
            break
        g = f[1718+10*k:1718+10*k+10]
        for r in range(5):
            for col in range(8):
                bit = 0x80 >> col
                if not(g[5+r] & bit) and 0 <= x+col < 160 and 0 <= y+r < 100:
                    im.putpixel((x+col,y+r),shade if g[r]&bit else 255)
        x += advance


def management(disc, files):
    folder = OUT/'design-data'
    folder.mkdir(exist_ok=True)
    bank = bytearray((HERE/'campaign/data/ffmg.bin').read_bytes())
    shp = {k: units.shp_frames(disc.read(*files[f'BATTLE/{k}.SHP'])) for k in ('TYPE1','TYPE2')}
    start = word(bank,14)
    ramza = []
    for career in range(21):
        for sex in ('MW' if career < 20 else ' '):
            name = f'BATTLE/{management_data.SHEETS[career]}_{sex}.SPR' if career<20 else 'BATTLE/RAMUZA.SPR'
            if name not in files:
                name = name.replace('_W','_M') if sex=='W' else name.replace('_M','_W')
            spr = disc.read(*files[name])
            frame = units.sprite(spr,shp['TYPE2' if career==1 else 'TYPE1'][11])
            idx = 2*career+(sex=='W') if career<20 else 40
            bank[start+156*idx:start+156*(idx+1)] = interface.planes(frame,16)
            if career==20:
                ramza = [units.sprite(spr,shp['TYPE1'][k]) for k in (11,10,9,10,11,12,13,12)]
    (folder/'ffneutral.bin').write_bytes(bank)
    for name in ('ffmg.bin','ffcj.bin'):
        p = folder/name
        if not p.exists():
            p.symlink_to(HERE/'campaign/data'/name)
    subprocess.run(['cc','-std=gnu99','-O1','-o',OUT/'management-preview',
                    HERE/'campaign/preview_management.c',ROOT/'runtime/core/rt_core.c',
                    ROOT/'runtime/platform-sw/rt_sw.c'],check=True)
    with tempfile.TemporaryDirectory(prefix='fft-management-review-') as temp:
        subprocess.run([OUT/'management-preview',temp,HERE/'campaign/x/check/ffsave_won.sav'],cwd=folder,check=True)
        publish_images(temp,OUT)
    sheet([('formation-before.png','Before:4 columns / walking extreme'),
           ('formation-after.png','Candidate:5 columns / neutral pose / selected portrait'),
           ('jobs-before.png','Before:all job names in a list'),
           ('jobs-after-8.png','Candidate:floor diamond / jobs discovered by another unit in gray'),
           ('status-before.png','Before:Status title13px'),
           ('status-after.png','Candidate:HP/MP bars / level beside name / compact title')], 'management-review.png')
    return bytes(bank), ramza


def composite_actor(im, actor, x, y):
    for r in range(actor.shape[0]):
        for c in range(actor.shape[1]):
            if actor[r,c]>=0 and 0<=x+c<160 and 0<=y+r<90:
                im.putpixel((x+c,y+r),int(PALETTE[actor[r,c]]))


def maps(disc,files,bank,ramza):
    tex = world_assets.texture(disc,files)
    land = world_assets.land_mask(tex)
    coast = land & ~ndimage.binary_erosion(land,border_value=1)
    yy,xx = np.indices(land.shape)
    a = np.where(land,0,1).astype(np.uint8);a[coast]=3
    b = np.where(land,0,1).astype(np.uint8);b[coast]=3
    b[~land & world_assets.motif(land.shape,[(x+3,y+5) for x,y in world_assets.WAVE])]=2
    relief = ndimage.gaussian_filter(tex @ np.array([.299,.587,.114]),4)
    # Muted native-pixel relief clusters from the source image, without its fine grid.
    hills = land & (relief < np.percentile(relief[land],35))
    b[hills]=1
    b[hills & ~ndimage.binary_erosion(hills) & ((xx+yy)%3==0)]=2
    # Keep source relief/lettering in the third option rather than inventing landmarks.
    lum = tex @ np.array([.299,.587,.114]);q=np.percentile(lum,[20,50,80])
    c = (3-np.digitize(lum,q)).astype(np.uint8)
    core = disc.read(*files['WORLD/WLDCORE.BIN'])
    locs=[]
    for i in range(43):
        v,y=struct.unpack_from('<II',core,world_assets.DOTS+8*i)
        locs.append((world_assets.s16(v&65535)+263,world_assets.s16(y&65535)+186))
    cx,cy=locs[6][0]-80,locs[6][1]-50
    wm=(HERE/'campaign/data/ffwm.bin').read_bytes()
    route_start=word(wm,10)
    paths=[]
    for k in range(48):
        rec=wm[route_start+6*k:route_start+6*(k+1)]
        if set(rec[:2])=={6,24}:
            off=word(rec,4);paths.append([struct.unpack_from('>hh',wm,off+4*j) for j in range(rec[3])])
    hand=np.array(art.outlined_pattern(art.HAND,32),object)
    hand=np.array([[-1 if c is None else c for c in row] for row in hand])

    def draw(g,frame=0,menu=False,free=False):
        im=Image.fromarray(PALETTE[g]).crop((cx,cy,cx+160,cy+100));d=ImageDraw.Draw(im)
        for path in paths:
            pts=[(x-cx,y-cy) for x,y in path]
            d.line(pts,fill=255,width=3);d.line(pts,fill=0,width=1)
        for k in (6,24):
            x,y=locs[k][0]-cx,locs[k][1]-cy
            d.ellipse((x-4,y-4,x+4,y+4),fill=255,outline=0)
            d.ellipse((x-2,y-2,x+2,y+2),fill=0 if k==6 else 85)
        x,y=locs[6][0]-cx,locs[6][1]-cy
        composite_actor(im,ramza[frame%8],x-8,y-21)
        hx,hy=(35,57) if free else (locs[6][0]-cx-26,locs[6][1]-cy-7)
        composite_actor(im,hand,hx,hy)
        d.rectangle((0,90,159,99),fill=255);d.line((0,90,159,90),fill=0)
        small_text(im,3,93,'Move cursor' if free else 'Magic City Gariland',bank)
        if menu:
            d.rectangle((3,8,73,81),fill=0);d.rectangle((4,9,72,80),fill=255)
            small_text(im,9,12,'Menu',bank)
            for i,s in enumerate(('Move','Formation','Brave Story','Tutorial','Data','Option')):
                small_text(im,18,24+9*i,s,bank)
            composite_actor(im,hand,2,19)
        return im
    for name,g in [('a',a),('b',b),('c',c)]:
        save_image(draw(g),OUT/f'map-{name}.png')
    save_image(draw(c,menu=True),OUT/'map-menu.png')
    save_image(draw(c,free=True),OUT/'map-free-hand.png')
    before=HERE/'campaign/x/check/worldmap-gariland-pc.png'
    save_image(Image.open(before),OUT/'map-before.png')
    sheet([('map-before.png','Before:current shipping motifs'),('map-a.png','A:light geography / clean sea'),
           ('map-b.png','B:muted source relief / sparse waves'),('map-c.png','C:source texture / relief and old lettering'),
           ('map-menu.png','Candidate menu (layout only)'),('map-free-hand.png','Free-hand position (layout only)')], 'map-designs-review.png',3)
    # Refined map review uses the real runtime renderer and live menu availability.
    subprocess.run(['cc','-std=gnu99','-O1','-o',OUT/'worldmap-preview',
                    HERE/'campaign/preview_worldmap.c',ROOT/'runtime/core/rt_core.c',
                    ROOT/'runtime/platform-sw/rt_sw.c'],check=True)
    with tempfile.TemporaryDirectory(prefix='fft-map-review-') as temp:
        subprocess.run([OUT/'worldmap-preview',temp,HERE/'campaign/x/check/ffsave_won.sav'],cwd=HERE/'campaign/data',check=True)
        publish_images(temp,OUT)
    return [draw(c,i) for i in range(8)]


def victories(bank):
    # This is an explicit scenery-backed layout sketch, not a battle result replay.
    bg=Image.open(OUT/'after/victory-background.png').convert('L')
    idx=np.abs(np.array(bg)[...,None].astype(int)-PALETTE.astype(int)).argmin(2)
    bg=Image.fromarray(PALETTE[np.minimum(idx+1,3)])
    font='/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf'
    def centered(im,s,y,size=11):
        d=ImageDraw.Draw(im);f=ImageFont.truetype(font,size)
        while d.textbbox((0,0),s,font=f)[2]>154:
            size-=1;f=ImageFont.truetype(font,size)
        w=d.textbbox((0,0),s,font=f)[2]
        x=(160-w)//2
        d.text((x+1,y+1),s,font=f,fill=0)
        d.text((x,y),s,font=f,fill=255)
    frames=[]
    for k in range(3):
        im=bg.copy()
        if k==0:
            centered(im,'CONGRATULATIONS!',25,11);centered(im,'THIS BATTLE IS',54,8);centered(im,'COMPLETE!',65,8)
        elif k==1:
            centered(im,'BONUS MONEY',20,12);centered(im,'500 GIL',53,12)
        else:
            centered(im,'WAR TROPHIES',13,12);centered(im,'2000 GIL',43,10)
            centered(im,'Mythril Knife',61,8);centered(im,'Phoenix Down',75,8)
            # Exact source item icons from the management bank, no generic symbols.
            for item,y in ((0x02,62),(0xFD,76)):
                rec=word(bank,6)+14*item
                icon=word(bank,12)+24*bank[rec+12]
                for row in range(8):
                    for col in range(8):
                        if not bank[icon+16+row]&(0x80>>col):
                            for dx,dy in ((-1,0),(1,0),(0,-1),(0,1)):
                                im.putpixel((20+col+dx,y+row+dy),255)
                for row in range(8):
                    for col in range(8):
                        bit=0x80>>col
                        if not bank[icon+16+row]&bit:
                            shade=((bank[icon+row]&bit)!=0)+2*((bank[icon+8+row]&bit)!=0)
                            im.putpixel((20+col,y+row),int(PALETTE[shade]))
        # Palette control: antialiased type is quantized to the calculator's four tones.
        arr=np.array(im);im=Image.fromarray(PALETTE[np.abs(arr[...,None].astype(int)-PALETTE.astype(int)).argmin(2)])
        save_image(im,OUT/f'victory-{k+1}.png');frames.append(im)
    sheet([(f'victory-{k+1}.png',f'{k+1}/3:layout candidate / reference example rewards') for k in range(3)],'victory-review.png',3)
    return frames


def turns():
    names=[];frames=[];durations=[]
    # Compare geometry only: both controls share all other QA changes and camera state.
    for case in (0,3):
        for f in range(5):
            im=Image.new('RGB',(960,348),'white');d=ImageDraw.Draw(im)
            for column,(folder,label) in enumerate((('half-turns','Historical80x50, doubled'),('after','Candidate:native160x100'))):
                pic=Image.open(OUT/folder/f'turn-{case}-{f}.png').convert('RGB')
                im.paste(pic.resize((480,300),Image.Resampling.NEAREST),(480*column,24))
                angle=('source','22.5 deg','45 deg','67.5 deg','destination')[f]
                d.text((480*column+5,5),label+' / '+('F5' if case==0 else 'F1')+' / '+angle,fill='black')
            d.text((5,331),'PC framebuffer preview:fixed illustrative cadence, not elapsed native timing or LCD frame-order proof.',fill='black')
            frames.append(im);durations.append(900 if f in (0,4) else 350)
    save_image(frames[2],OUT/'rotation-review.png')
    save_image(frames[0],OUT/'rotation-review.gif',save_all=True,append_images=frames[1:],duration=durations,loop=0,disposal=2)


def main():
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--all',action='store_true',help='explicitly regenerate the historical complete sheet set')
    args=parser.parse_args()
    if not args.all:
        parser.error('Use qa_refinements.py for the current review; --all deliberately rebuilds the historical set.')
    OUT.mkdir(parents=True,exist_ok=True)
    disc,files=extract.disc_files(str(ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'))
    bank,ramza=management(disc,files)
    frames=maps(disc,files,bank,ramza)
    sheet([('map-actual-before.png','Selected source map:previous paths'),('map-actual-after.png','Actual renderer:white road outlines'),('map-actual-menu.png','Actual menu:Formation and Data available'),('map-actual-hand.png','Actual free hand;confirmation required')],'map-selected-review.png')
    sheet([('before/lavian-equipment.png','Before:item name truncated'),('after/lavian-equipment.png','After:complete source item name'),
           ('before/wait-0-0.png','Before:Wait selection'),('after/wait-0-0.png','After:reflected Wait selection'),
           ('before/headroom-3.png','Before:top-edge clipping'),('after/headroom-3.png','After:virtual blank rows'),
           ('after/cursor-prior-full-fallback.png','Prior full-cursor fallback:recreated on the same tile'),
           ('after/cursor-masked.png','Candidate:cursor uses terrain dashes immediately')],'battle-review.png')
    save_image(frames[0].resize((640,400),Image.Resampling.NEAREST),OUT/'map-ramza-review.gif',save_all=True,
        append_images=[f.resize((640,400),Image.Resampling.NEAREST) for f in frames[1:]],duration=[100,130,170,130,100,130,170,130],loop=0,disposal=2)
    jobs=[Image.open(OUT/f'jobs-after-{i}.png').resize((640,400),Image.Resampling.NEAREST) for i in range(9)]
    save_image(jobs[0],OUT/'carousel-review.gif',save_all=True,append_images=jobs[1:],duration=140,loop=0,disposal=2)
    victories(bank)
    sheet([('review-1/victory-3.png','Previously reviewed trophies'),('victory-3.png','Source item icons with white outline')],'victory-icons-review.png')
    if (OUT/'after/turn-0-0.png').exists():
        turns()
    if (OUT/'scenery-after-0.png').exists():
        sheet([(f'scenery-{variant}-{rot}.png',f'View{rot}: '+('previous unstyled bank' if variant=='before' else 'restored selected contrast/height edges'))
               for rot in range(4) for variant in ('before','after')],'scenery-review.png')
    print('Wrote isolated visual candidates to',OUT)


if __name__=='__main__':
    main()
