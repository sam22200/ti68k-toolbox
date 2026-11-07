#!/usr/bin/env python3
"""Actual battle renderer:1px versus2px geometric scenery edges, unchanged actors/depth."""
from pathlib import Path
import hashlib,json,tempfile
import numpy as np
from PIL import Image,ImageDraw
from check_flow import HERE,ROOT,run,data_links
from review_files import save_image,publish_images,write_changed
import isometric_style as style
import campaign_scenery,contour_assets

OUT=HERE/'x/qa/followup/edges'
def main():
    OUT.mkdir(parents=True,exist_ok=True);banks=OUT/'two-pixel';banks.mkdir(exist_ok=True);report=[]
    for rot in range(4):
        source=HERE.parent/f'x/isometric-style/variant-{rot}.raw';raw=source.read_bytes();plane=len(raw)//6
        cache=source.parent;tones=np.array(Image.open(cache/f'contrast-{rot}.png').convert('L'))
        grey=((255-tones.astype(np.uint16))//85).astype(np.uint8)
        mask=np.array(Image.open(cache/f'mask-{rot}.png').convert('L'))!=0
        assert style.pack(style.edge_colours(grey,mask))==raw[:2*plane]
        thick=mask.copy();thick[1:]|=mask[:-1];thick[:,1:]|=mask[:,:-1];thick[1:,1:]|=mask[:-1,:-1]
        # Same adaptive white/black contrast; only the geometric stroke grows.
        candidate=style.pack(style.edge_colours(grey,thick))+raw[2*plane:]
        assert candidate[2*plane:]==raw[2*plane:]
        write_changed(banks/f'ffgv{rot}.bin',campaign_scenery.chapter_view(candidate,rot))
        report.append(dict(view=rot,one_pixel=int(mask.sum()),two_pixel=int(thick.sum()),
                           depth_sha256=hashlib.sha256(raw[2*plane:]).hexdigest()))
    contour_assets.attach(banks/'ffgv0.bin')
    with tempfile.TemporaryDirectory(prefix='fft-edge-width-')as name:
        temp=Path(name);data=temp/'data';data.mkdir();data_links(data);shots=temp/'shots';shots.mkdir()
        run(['cc','-std=gnu99','-O1','-w','-o',temp/'preview',HERE/'preview_scenery.c',
             ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
        run([temp/'preview',shots,'one'],data)
        for rot in range(4):
            path=data/f'ffgv{rot}.bin';path.unlink();path.symlink_to(banks/f'ffgv{rot}.bin')
        run([temp/'preview',shots,'two'],data);publish_images(shots,OUT/'shots')
    sheet=Image.new('RGB',(984,1312),'white');draw=ImageDraw.Draw(sheet)
    for rot in range(4):
        for col,variant in enumerate(('one','two')):
            x,y=492*col,328*rot;draw.text((x+5,y+5),f'View {rot} / '+('A:1px scenery edges'if not col else'B:2px scenery edges / same sprites'),fill='black')
            sheet.paste(Image.open(OUT/'shots'/f'scenery-{variant}-{rot}.png').resize((480,300),Image.Resampling.NEAREST),(x,y+24))
    save_image(sheet,OUT/'edge-width-ab.png');write_changed(OUT/'report.json',(json.dumps(report,indent=2)+'\n').encode())
    print(OUT/'edge-width-ab.png')

if __name__=='__main__':main()
