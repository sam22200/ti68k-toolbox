#!/usr/bin/env python3
"""Actual title/file/overwrite renderings with isolated, labelled review saves."""
from pathlib import Path
import tempfile
from PIL import Image,ImageDraw
from check_flow import HERE,ROOT,run,data_links,fixture
from review_files import save_image,publish_images,write_changed

OUT=HERE/'x/qa/title/review-1'
def main():
    OUT.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='fft-title-review-') as folder:
        data=Path(folder);data_links(data);shots=data/'shots';shots.mkdir()
        one=bytearray(fixture());one[4]=3;one[37]=7;one[298]=8
        three=bytearray(one);three[37]=25;three[298]=17
        (data/'ffsave1.sav').write_bytes(one);(data/'ffsave3.sav').write_bytes(three)
        run(['cc','-std=gnu99','-O1','-w','-o',OUT/'preview',HERE/'preview_title.c',
             ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
        run([OUT/'preview',shots],data);publish_images(shots,OUT)
    entries=[('title-new.png','New Game / source logo and map'),('title-continue.png','Continue / animated shared glove'),
             ('files.png','Three data files / illustrative saves'),('overwrite.png','Overwrite confirmation / Return by default')]
    sheet=Image.new('RGB',(984,656),'white');d=ImageDraw.Draw(sheet)
    for i,(name,label) in enumerate(entries):
        x=i%2*492;y=i//2*328;d.text((x+5,y+5),label,fill='black')
        sheet.paste(Image.open(OUT/name).resize((480,300),Image.Resampling.NEAREST),(x,y+24))
    save_image(sheet,OUT/'title-review.png')
    frames=[Image.open(OUT/f'hand-{i}.png').resize((640,400),Image.Resampling.NEAREST) for i in range(8)]
    save_image(frames[0],OUT/'title-glove.gif',save_all=True,append_images=frames[1:],duration=125,loop=0,disposal=2)
    write_changed(OUT/'README.txt',b'Actual portable C renderer; slots1/3 are private review fixtures. No real save changed.\n')
    print(OUT/'title-review.png')

if __name__=='__main__':main()
