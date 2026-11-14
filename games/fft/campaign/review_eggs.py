#!/usr/bin/env python3
"""Current production egg/date pictures; approved prototype artifacts stay intact."""
import argparse
import tempfile
from pathlib import Path
from PIL import Image,ImageDraw
from check_flow import HERE,ROOT,data_links,fixture,run
from review_files import publish_images,save_image
import breeding_rules
import egg_art


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--sanitize',action='store_true');args=ap.parse_args()
    out=HERE/'x/eggs/current-review';out.mkdir(parents=True,exist_ok=True)
    flags=['-fsanitize=address,undefined','-fno-pie','-no-pie']if args.sanitize else []
    cue=ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    art,_=egg_art.build(cue);rules,_=breeding_rules.build(cue)
    with tempfile.TemporaryDirectory(prefix='fft-current-eggs-')as name:
        data=Path(name);data_links(data,HERE/'x/eggs/shipping')
        for bank,raw in [('ffeg',egg_art.pool(art)),('ffwb',rules)]:
            p=data/(bank+'.bin');p.unlink(missing_ok=True);p.write_bytes(raw)
        initial=bytearray(fixture());initial[4]|=2;initial[37]=7;(data/'ffsave.sav').write_bytes(initial)
        for mode,defines,cases in [('manage',[],[('egg',0),('menu',1),('confirm',2)]),('map',['-DEGG_MAP'],[('map',0)])]:
            binary=out/mode
            (out/(mode+'-build.log')).write_text(run(['cc','-std=gnu99','-O1','-g','-w',*flags,*defines,'-o',binary,
                HERE/'check_egg_render.c',ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']))
            for label,scenario in cases:
                print(label+': '+run(['env','ASAN_OPTIONS=detect_leaks=0','UBSAN_OPTIONS=halt_on_error=1',
                    binary,scenario,data/(label+'.png')],data).strip())
        publish_images(data,out)
    canvas=Image.new('RGB',(992,656),'white');draw=ImageDraw.Draw(canvas)
    for i,(name,label)in enumerate([('egg','Integrated eggs'),('menu','Compact egg menu'),('confirm','Default No'),('map','World calendar')]):
        x,y=(i%2)*496,(i//2)*328;draw.text((x+12,y+7),label,fill='black')
        canvas.paste(Image.open(out/(name+'.png')).resize((480,300),Image.Resampling.NEAREST),(x+8,y+24))
    save_image(canvas,out/'review.png');print(out/'review.png')


if __name__=='__main__':main()
