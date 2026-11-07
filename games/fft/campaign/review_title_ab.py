#!/usr/bin/env python3
"""Source-art fade A / shared-serif animated black B, using the actual C renderer."""
from pathlib import Path
import tempfile,json
from PIL import Image,ImageDraw
from check_flow import HERE,ROOT,run,data_links,fixture
from review_files import save_image,publish_images,write_changed

OUT=HERE/'x/title-ab'
DISC=ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'

def main():
    OUT.mkdir(exist_ok=True)
    for variant in ('a','b'):
        run([ROOT/'tools/pyenv/bin/python',HERE.parent/'tools/title_assets.py',DISC,
             '--variant',variant,'--out',OUT/variant/'fftitle.bin'])
    with tempfile.TemporaryDirectory(prefix='fft-title-ab-')as tmp:
        temp=Path(tmp);data=temp/'data';data.mkdir();data_links(data)
        one=bytearray(fixture());one[4]=3;one[37]=7;one[298]=8
        (data/'ffsave1.sav').write_bytes(one)
        for number,variant in enumerate(('current','a','b')):
            bank=HERE/'data/fftitle.bin' if not number else OUT/variant/'fftitle.bin'
            (data/'fftitle.bin').unlink();(data/'fftitle.bin').symlink_to(bank)
            shots=temp/variant;shots.mkdir()
            program=temp/('preview-'+variant)
            run(['cc','-std=gnu99','-O1','-w','-DFFT_TITLE_VARIANT='+str(number),'-o',program,
                 HERE/'preview_title_ab.c',ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
            run([program,shots],data);publish_images(shots,OUT/variant/'shots')
    sheet=Image.new('RGB',(1476,656),'white');d=ImageDraw.Draw(sheet)
    for col,(variant,label)in enumerate((('current','Current: source logo/map'),('a','A: same art / faded map centre'),('b','B: white serif / black / skippable intro'))):
        for row,name in enumerate(('new-game','files')):
            x,y=col*492,row*328;d.text((x+5,y+5),label if not row else 'Same shared choices / private illustrative save',fill='black')
            sheet.paste(Image.open(OUT/variant/'shots'/f'{name}.png').resize((480,300),Image.Resampling.NEAREST),(x,y+24))
    save_image(sheet,OUT/'title-ab-review.png')
    frames=[]
    for f in range(0,226,2):
        im=Image.new('RGB',(984,324),'white');d=ImageDraw.Draw(im)
        for col,variant in enumerate(('a','b')):
            d.text((col*492+5,5),'A: faded map' if not col else 'B: title, rule, 68k, then choices',fill='black')
            im.paste(Image.open(OUT/variant/'shots'/f'frame-{f:03}.png').resize((480,300),Image.Resampling.NEAREST),(col*492,24))
        frames.append(im)
    save_image(frames[0],OUT/'title-ab-animation.gif',save_all=True,append_images=frames[1:],duration=67,loop=0,disposal=2)
    write_changed(OUT/'README.txt',b'Actual C renderer; illustrative PC cadence, not measured TI timing. Private Data1 fixture only. Active title/bank unchanged.\n')
    print(OUT/'title-ab-review.png');print(OUT/'title-ab-animation.gif')

if __name__=='__main__':main()
