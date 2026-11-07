#!/usr/bin/env python3
"""Individual white-letter animation reviews from current C renderers; private saves."""
from pathlib import Path
import json,tempfile
from PIL import Image,ImageDraw,ImageFont
from check_flow import HERE,ROOT,run,data_links,fixture
from review_files import save_image,write_changed
from shared_font_assets import build

OUT=HERE/'x/white-titles'
ENTRIES=(
    ('intro','frame-*.png','title-intro.gif','Title B preview / title, rule,68k, then choices',67),
    ('scenes','orbonne-*.png','orbonne-title.gif','Orbonne Monastery / source white lettering',67),
    ('scenes','scene-0-*.png','chapter-title.gif','Chapter I / left-to-right white reveal',67),
    ('scenes','scene-1-*.png','academy-title.gif','Academy location / left-to-right white reveal',67),
    ('scenes','scene-2-*.png','gariland-title.gif','Gariland location / black background',67),
    ('results','victory-*.png','victory-titles.gif','Victory / three pages, scripted2nd-ENTER confirmations',133),
)

def main():
    OUT.mkdir(parents=True,exist_ok=True);report=[];build(HERE/'x/shared-font/data')
    font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',18)
    small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',14)
    with tempfile.TemporaryDirectory(prefix='fft-white-titles-')as folder:
        temp=Path(folder);data=temp/'data';data.mkdir();data_links(data)
        (data/'ffsave.sav').write_bytes(fixture())
        for name,source,flags in (
            ('scenes','preview_scene_animation.c',[]),
            ('results','preview_flow_animation.c',[]),
            ('intro','preview_title_ab.c',['-DFFT_TITLE_VARIANT=2']),
        ):
            shots=temp/name;shots.mkdir();program=temp/(name+'-preview')
            if name=='intro':
                (data/'fftitle.bin').unlink();(data/'fftitle.bin').symlink_to(HERE/'x/shared-font/data/fftitle.bin')
                one=bytearray(fixture());one[4]=3;one[37]=7;one[298]=8
                (data/'ffsave1.sav').write_bytes(one)
            run(['cc','-std=gnu99','-O1','-w',*flags,'-o',program,HERE/source,
                 ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
            run([program,shots],data)
        for group,pattern,name,label,ms in ENTRIES:
            files=sorted((temp/group).glob(pattern));assert files,(group,pattern)
            frames=[]
            for source in files:
                frame=Image.new('RGB',(664,466),'white');draw=ImageDraw.Draw(frame)
                draw.text((12,8),label,font=font,fill='black')
                shot=Image.open(source).convert('RGB');assert shot.size==(160,100)
                frame.paste(shot.resize((640,400),Image.Resampling.NEAREST),(12,38))
                draw.text((12,445),'Actual C renderer / illustrative playback, not measured TI timing',font=small,fill='black')
                frames.append(frame)
            durations=[ms]*len(frames);durations[-1]+=800
            path=OUT/name
            save_image(frames[0],path,save_all=True,append_images=frames[1:],duration=durations,loop=0,disposal=2)
            with Image.open(path)as gif:
                assert gif.n_frames>1
                actual=sum(gif.seek(i)or gif.info['duration'] for i in range(gif.n_frames))
                report.append(dict(file=name,rendered_frames=len(frames),encoded_frames=gif.n_frames,
                                   duration_ms=actual,bytes=path.stat().st_size))
            print(path.resolve(),flush=True)
    write_changed(OUT/'review.json',(json.dumps(dict(
        scope='Accepted shared glyph font, current C renderers. Title B remains isolated. Private saves; no emulator change.',
        files=report),indent=2)+'\n').encode())

if __name__=='__main__':main()
