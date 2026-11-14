#!/usr/bin/env python3
"""Reproduce the review-only source egg/roster/menu picture on PC. No TI runs."""
from pathlib import Path
import argparse
import tempfile
from PIL import Image, ImageDraw
from check_flow import HERE, ROOT, data_links, run, fixture
import egg_art
from review_files import save_image,publish_images


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--sanitize',action='store_true',help='check preview pointer/plane bounds on PC')
    args=ap.parse_args()
    out=HERE/'x/egg-review';out.mkdir(parents=True,exist_ok=True)
    cue=ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    bank,proof=egg_art.build(cue)
    import json
    (out/'source.json').write_text(json.dumps(proof,indent=2)+'\n')
    flags=['-fsanitize=address,undefined','-fno-pie','-no-pie'] if args.sanitize else []
    suffix='-san' if args.sanitize else ''
    (out/('build'+suffix+'.log')).write_text(run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',out/('preview'+suffix),
        HERE/'review_eggs.c',ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']))
    (out/('map-build'+suffix+'.log')).write_text(run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',out/('map-preview'+suffix),
        HERE/'review_world_calendar.c',ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']))
    with tempfile.TemporaryDirectory(prefix='fft-eggs-review-') as folder:
        data=Path(folder);data_links(data,HERE/'x/guests/shipping')
        (data/'ffeg.bin').write_bytes(bank)
        shots=data/'shots';shots.mkdir()
        roster=out/'roster.sav';roster.write_bytes(fixture())
        logs=[]
        for executable,label in [('preview','review'),('map-preview','map-review')]:
            log=run(['env','ASAN_OPTIONS=detect_leaks=1','UBSAN_OPTIONS=halt_on_error=1',
                     out/(executable+suffix),shots,roster],data)
            if args.sanitize:assert log==(out/(label+'.log')).read_text()
            (out/(label+suffix+'.log')).write_text(log);logs.append(log)
        print(''.join(logs).strip());publish_images(shots,out)
    canvas=Image.new('RGB',(992,656),'white');draw=ImageDraw.Draw(canvas)
    for i,(name,label) in enumerate([('before','Before: monster slot blank'),('egg','Proposal: egg + incubation'),
                                    ('menu','Egg menu'),('confirm','Dismiss: No selected by default')]):
        x,y=(i%2)*496,(i//2)*328
        draw.text((x+12,y+7),label,fill='black')
        canvas.paste(Image.open(out/(name+'.png')).resize((480,300),Image.Resampling.NEAREST),(x+8,y+24))
    save_image(canvas,out/'review.png');print(out/'review.png')
    canvas=Image.new('RGB',(992,328),'white');draw=ImageDraw.Draw(canvas)
    for i,(name,label) in enumerate([('map-before','Existing map'),('map-date','Calendar proposal (example date)')]):
        draw.text((12+496*i,7),label,fill='black')
        canvas.paste(Image.open(out/(name+'.png')).resize((480,300),Image.Resampling.NEAREST),(8+496*i,24))
    save_image(canvas,out/'calendar.png');print(out/'calendar.png')


if __name__=='__main__':main()
