#!/usr/bin/env python3
"""Isolated white-priority ISO edge proposal; never rewrites active scenery banks."""
from pathlib import Path
import hashlib,json,tempfile
import numpy as np
from PIL import Image,ImageDraw
from check_flow import HERE,ROOT,run,data_links
from review_files import save_image,publish_images
import isometric_style as style
import campaign_scenery
import contour_assets

OUT=HERE/'x/qa/flow/review-2'


def main():
    new=OUT/'new';new.mkdir(parents=True,exist_ok=True)
    banks=OUT/'white-edges';banks.mkdir(exist_ok=True);report=[]
    for rot in range(4):
        cache=HERE.parent/'x/isometric-style'
        source=(cache/f'variant-{rot}.raw').read_bytes();plane=len(source)//6
        tones=np.array(Image.open(cache/f'contrast-{rot}.png').convert('L'))
        grey=((255-tones.astype(np.uint16))//85).astype(np.uint8)
        mask=np.array(Image.open(cache/f'mask-{rot}.png').convert('L'))!=0
        assert style.pack(style.edge_colours(grey,mask))==source[:2*plane],('stale source edge cache',rot)
        changed=style.edge_colours(grey,mask,prefer_white=True)
        assert np.array_equal(changed[~mask],grey[~mask])
        raw=style.pack(changed)+source[2*plane:]
        assert raw[2*plane:]==source[2*plane:]
        (banks/f'ffgv{rot}.bin').write_bytes(campaign_scenery.chapter_view(raw,rot))
        report.append(dict(view=rot,edge_pixels=int(mask.sum()),
                           former_white=int((style.edge_colours(grey,mask)[mask]==0).sum()),
                           proposed_white=int((changed[mask]==0).sum()),
                           remaining_black=int((changed[mask]==3).sum()),
                           source_depth_sha256=hashlib.sha256(raw[2*plane:]).hexdigest()))
    contour_assets.attach(banks/'ffgv0.bin')
    with tempfile.TemporaryDirectory(prefix='fft-white-edges-') as name:
        temp=Path(name);data=temp/'data';data.mkdir();data_links(data);shots=temp/'shots';shots.mkdir()
        program=temp/'preview'
        run(['cc','-std=gnu99','-O1','-w','-o',program,HERE/'preview_scenery.c',
             ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
        run([program,shots,'before'],data)
        for rot in range(4):
            path=data/f'ffgv{rot}.bin';path.unlink();path.symlink_to(banks/f'ffgv{rot}.bin')
        run([program,shots,'white'],data)
        publish_images(shots,new)
    sheet=Image.new('RGB',(984,4*328),'white');draw=ImageDraw.Draw(sheet)
    for rot in range(4):
        for col,label in enumerate(('before','white')):
            x,y=col*492,rot*328
            draw.text((x+5,y+5),f'View {rot}: '+('previous adaptive edges' if not col else 'white first; black only on white faces'),fill='black')
            sheet.paste(Image.open(new/f'scenery-{label}-{rot}.png').resize((480,300),Image.Resampling.NEAREST),(x,y+24))
    save_image(sheet,new/'white-edges-review.png')
    (OUT/'white-edges.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Four actual-renderer before/after views; original depth/masks unchanged; active banks untouched')


if __name__=='__main__':main()
