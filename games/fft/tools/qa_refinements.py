#!/usr/bin/env python3
"""Historical100% map refinement review; requires explicit --historical to republish."""
from pathlib import Path
import json
import re
import subprocess
import tempfile
import argparse

from PIL import Image,ImageDraw
from review_files import save_image,publish_images,write_changed

HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[1]
OUT=HERE/'campaign/x/qa/review-3'
NEW=OUT/'new'


def run(args,cwd=ROOT):
    subprocess.run(list(map(str,args)),cwd=cwd,check=True)


def sheet(entries,name,columns=2):
    im=Image.new('RGB',(columns*492,((len(entries)+columns-1)//columns)*328),'white')
    draw=ImageDraw.Draw(im)
    for i,(path,title) in enumerate(entries):
        x,y=(i%columns)*492,(i//columns)*328
        draw.text((x+5,y+5),title,fill='black')
        im.paste(Image.open(path).convert('RGB').resize((480,300),Image.Resampling.NEAREST),(x,y+24))
    save_image(im,NEW/name)


def main():
    NEW.mkdir(parents=True,exist_ok=True)
    for variant in ('source','strong','light'):
        bank=OUT/variant/'ffwm.bin'
        if not bank.exists() or bank.read_bytes()[:4]!=b'FWM3':
            bank.parent.mkdir(parents=True,exist_ok=True)
            run([ROOT/'tools/pyenv/bin/python',HERE/'tools/world_assets.py',ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue',bank,'--contrast',variant,'--scale','100'])
    with tempfile.TemporaryDirectory(prefix='fft-refinements-') as name:
        tmp=Path(name);shots=tmp/'shots';shots.mkdir();data=tmp/'data';data.mkdir()
        for bank in ('ffmg','ffcj'):
            (data/(bank+'.bin')).symlink_to(HERE/'campaign/data'/(bank+'.bin'))
        (data/'ffneutral.bin').symlink_to(HERE/'campaign/data/ffmg.bin')
        run(['cc','-std=gnu99','-O1','-o',tmp/'management',HERE/'campaign/preview_management.c',
             ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
        run([tmp/'management',shots,HERE/'campaign/x/check/ffsave_won.sav'],data)
        publish_images(shots,NEW)
        run(['cc','-std=gnu99','-O1','-o',tmp/'worldmap',HERE/'campaign/preview_worldmap.c',
             ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
        # Same renderer/camera/pose, with only the former thinner contour restored.
        source=HERE/'campaign/worldmap.c';control=source.read_text()
        control=control.replace('draw_rect(x0-2,y0-2,5,5,C_WHITE);',
            'pset(x0-1,y0,C_WHITE);pset(x0+1,y0,C_WHITE);pset(x0,y0-1,C_WHITE);pset(x0,y0+1,C_WHITE);')
        control=control.replace('for(;row<=end;row++)strip_span(dst,row,x0-2,x0+2,C_WHITE);',
            'strip_span(dst,y0-top,x0-1,x0+1,C_WHITE);strip_span(dst,y0-top-1,x0,x0,C_WHITE);strip_span(dst,y0-top+1,x0,x0,C_WHITE);')
        control=re.sub(r'#include "([^"]+)"',lambda m:'#include "'+str((source.parent/m[1]).resolve())+'"',control)
        (tmp/'control.c').write_text(control)
        run(['cc','-std=gnu99','-O1','-DWM_SOURCE="'+str(tmp/'control.c')+'"','-o',tmp/'road-control',
             HERE/'campaign/preview_worldmap.c',ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
        for variant in ('source','strong','light'):
            bank=OUT/variant/'ffwm.bin'
            link=data/'ffwm.bin';link.unlink(missing_ok=True);link.symlink_to(bank)
            for p in shots.glob('*.png'):p.unlink()
            run([tmp/'worldmap',shots,HERE/'campaign/x/check/ffsave_won.sav'],data)
            for p in shots.glob('*.png'):
                write_changed(NEW/(variant+'-'+p.name),p.read_bytes())
            if variant=='source':
                run([tmp/'road-control',shots,HERE/'campaign/x/check/ffsave_won.sav'],data)
                write_changed(NEW/'road-control.png',(shots/'map-roads-clear.png').read_bytes())
    sheet([(OUT/'formation-after.png','Previous:thin floor diamond and gray floor lines'),
           (NEW/'formation-after.png','Requested:thicker diamond / no gray floor lines'),
           (OUT/'jobs-after-8.png','Previous:floor diamond on the job carousel'),
           (NEW/'jobs-after-8.png','Requested:no diamond;selected job faces the centre')],'management-refinements.png')
    sheet([(OUT/'map-actual-after.png','Previous texture and road contours'),
           (NEW/'source-map-actual-after.png','Same texture / stronger road contours'),
           (NEW/'strong-map-actual-after.png','A:strong contrast;source lettering retained'),
           (NEW/'light-map-actual-after.png','B:lighter paper;source lettering retained')],'map-contrast-review.png')
    sheet([(OUT/'map-actual-menu.png','Previous menu:glove overlaps the first letters'),
           (NEW/'source-map-actual-menu.png','Requested:battle-style command panel / glove outside')],'map-menu-review.png')
    sheet([(NEW/'road-control.png','Same view:former one-pixel road outline'),
           (NEW/'source-map-roads-clear.png','Requested:two-pixel white clearance around road')],'roads-review.png')
    frames=[Image.open(NEW/f'jobs-after-{i}.png').resize((640,400),Image.Resampling.NEAREST) for i in range(9)]
    save_image(frames[0],NEW/'carousel-no-diamond.gif',save_all=True,append_images=frames[1:],duration=140,loop=0,disposal=2)
    print('Only current refinements published to',NEW)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--historical',action='store_true')
    if not parser.parse_args().historical:
        parser.error('The selected map is now67%; pass --historical only to regenerate the former100% comparison.')
    main()
