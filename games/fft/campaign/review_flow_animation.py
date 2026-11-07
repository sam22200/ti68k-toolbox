#!/usr/bin/env python3
"""Source-order titles/results: real C renderers, isolated save, no shipping-art changes."""
from pathlib import Path
import tempfile
from PIL import Image,ImageDraw
from check_flow import HERE,ROOT,run,data_links,fixture
from review_files import save_image,publish_images,write_changed

OUT=HERE/'x/qa/followup/animation'
def main():
    OUT.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='fft-flow-animation-')as name:
        temp=Path(name);data=temp/'data';data.mkdir();data_links(data)
        (data/'ffsave.sav').write_bytes(fixture());shots=temp/'shots';shots.mkdir()
        for kind in ('flow','scene'):
            program=temp/kind
            run(['cc','-std=gnu99','-O1','-w','-o',program,HERE/f'preview_{kind}_animation.c',
                 ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
            run([program,shots],data)
        publish_images(shots,OUT/'frames')
    for pattern,target in [('scene-*.png','scene-title-reveal.gif'),('victory-*.png','victory-reveal.gif')]:
        files=sorted((OUT/'frames').glob(pattern));frames=[]
        for p in files:
            im=Image.new('RGB',(640,420),'white');draw=ImageDraw.Draw(im)
            draw.text((6,4),'Actual scene renderer' if target.startswith('scene') else 'Actual results / 2nd or ENTER:next page',fill='black')
            im.paste(Image.open(p).resize((640,400),Image.Resampling.NEAREST),(0,20));frames.append(im)
        save_image(frames[0],OUT/target,save_all=True,append_images=frames[1:],duration=67 if target.startswith('scene') else 133,loop=0,disposal=2)
    sheet=Image.new('RGB',(984,328),'white');draw=ImageDraw.Draw(sheet)
    for col,(name,label)in enumerate([('kill-first-frame','Last enemy KO:no premature results'),('after-dialogue','Post-battle dialogue before congratulations')]):
        draw.text((492*col+5,5),label,fill='black');sheet.paste(Image.open(OUT/'frames'/f'{name}.png').resize((480,300),Image.Resampling.NEAREST),(492*col,24))
    save_image(sheet,OUT/'victory-order.png')
    write_changed(OUT/'README.txt',b'Actual C scene/results renderers; private starting-party fixture. Illustrative PC cadence, not measured TI FPS.\n')
    print(OUT)

if __name__=='__main__':main()
