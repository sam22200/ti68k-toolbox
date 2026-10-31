#!/usr/bin/env python3
"""Same C scenes, scale and animation steps: strips versus shared glyphs."""
from pathlib import Path
import json,tempfile
from PIL import Image,ImageDraw,ImageFont
from check_flow import HERE,ROOT,run,data_links,fixture
from review_files import save_image,write_changed
from shared_font_assets import build

OUT=HERE/'x/shared-font'
ENTRIES=(
    ('intro','frame-*.png','title-intro','Title B'),
    ('scenes','orbonne-*.png','orbonne-title','Orbonne Monastery'),
    ('scenes','scene-0-*.png','chapter-title','Chapter I'),
    ('scenes','scene-1-*.png','academy-title','Academy'),
    ('scenes','scene-2-*.png','gariland-title','Gariland'),
    ('results','victory-*.png','victory-titles','Victory / three pages'),
)

def main():
    OUT.mkdir(parents=True,exist_ok=True);assets=build(OUT/'data')
    font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',18)
    small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',14)
    report=[];stills=[]
    with tempfile.TemporaryDirectory(prefix='fft-shared-font-')as name:
        temp=Path(name)
        for mode in ('before','after'):
            data=temp/mode;data.mkdir();data_links(data)
            (data/'ffsave.sav').write_bytes(fixture())
            (data/'ffflow.bin').unlink()
            (data/'ffflow.bin').symlink_to(OUT/'data/ffflow.bin' if mode=='after' else OUT/'data/legacy/ffflow.bin')
            for group,source in (('scenes','preview_scene_animation.c'),('results','preview_flow_animation.c'),
                                 ('intro','preview_title_ab.c')):
                shots=data/group;shots.mkdir();program=temp/(mode+'-'+group)
                flags=['-DFFT_TITLE_VARIANT=2'] if group=='intro' else []
                extra=[]
                if mode=='before':flags+=['-DFFT_LEGACY_SERIF']
                if group=='intro':
                    path=OUT/'data/fftitle.bin' if mode=='after' else OUT/'data/legacy/fftitle.bin'
                    (data/'fftitle.bin').unlink();(data/'fftitle.bin').symlink_to(path)
                    one=bytearray(fixture());one[4]=3;one[37]=7;one[298]=8
                    (data/'ffsave1.sav').write_bytes(one)
                run(['cc','-std=gnu99','-O1','-w',*flags,'-o',program,HERE/source,
                     ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c',*extra])
                run([program,shots],data)
        for group,pattern,key,label in ENTRIES:
            pairs=list(zip(sorted((temp/'before'/group).glob(pattern)),sorted((temp/'after'/group).glob(pattern))))
            assert pairs
            frames=[]
            for before,after in pairs:
                assert before.name==after.name
                frame=Image.new('RGB',(1320,478),'white');draw=ImageDraw.Draw(frame)
                draw.text((12,8),label+' / BEFORE: whole-line images',font=font,fill='black')
                draw.text((672,8),'AFTER: shared glyph font',font=font,fill='black')
                for x,path in ((12,before),(672,after)):
                    shot=Image.open(path).convert('RGB');assert shot.size==(160,100)
                    frame.paste(shot.resize((640,400),Image.Resampling.NEAREST),(x,38))
                draw.text((12,446),'Actual C renderers / identical logical steps / illustrative GIF cadence, not measured TI timing',font=small,fill='black')
                frames.append(frame)
            durations=[133 if group=='results' else 67]*len(frames);durations[-1]+=800
            gif=OUT/(key+'.gif')
            save_image(frames[0],gif,save_all=True,append_images=frames[1:],duration=durations,loop=0,disposal=2)
            # Results: include every page in the static sheet, not just the final one.
            indices=[35,77,119] if group=='results' else [-1]
            for index in indices:stills.append(frames[index])
            report.append(dict(file=gif.name,rendered_steps=len(frames),bytes=gif.stat().st_size))
            print(gif.resolve(),flush=True)
        sheet=Image.new('RGB',(1320,478*len(stills)),'white')
        for i,frame in enumerate(stills):sheet.paste(frame,(0,478*i))
        save_image(sheet,OUT/'comparison.png')
    write_changed(OUT/'review.json',(json.dumps(dict(
        scope='Accepted glyph renderer versus explicit legacy control; active source now uses glyphs. Private data/saves; live TiEmu untouched.',
        font_bytes=assets['font_bytes'],data_delta=assets['combined_data_delta'],files=report),indent=2)+'\n').encode())

if __name__=='__main__':main()
