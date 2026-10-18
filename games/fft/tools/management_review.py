#!/usr/bin/env python3
"""Actual 160x100 runtime menu GIFs, and optional native 68000 cross-checks."""
from pathlib import Path
import argparse
import csv
import hashlib
import json
import re
import subprocess

from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
CODE = HERE/'management'
OUT = HERE/'x/management'
LABELS = ['Formation', 'Unit menu', 'Item', 'Equipment', 'Ability', 'Learn',
          'Change Job', 'Status', 'Dialogue', 'Set Ability']
VARIANTS = ['A - Classic grid', 'B - Portrait and roster', 'C - Unit card']


def run(args,cwd=CODE):
    return subprocess.check_output(list(map(str,args)),cwd=cwd,text=True)


def decorate(screens, titles, columns, scale=4):
    w,h=160*scale,100*scale+34
    sheet=Image.new('RGB',(columns*w,((len(screens)+columns-1)//columns)*h),'#eee9df')
    d=ImageDraw.Draw(sheet)
    font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',18)
    for i,(screen,title) in enumerate(zip(screens,titles)):
        x,y=i%columns*w,i//columns*h
        d.text((x+8,y+6),title,fill='black',font=font)
        sheet.paste(screen.convert('RGB').resize((w,100*scale),Image.Resampling.NEAREST),(x,y+34))
    return sheet


def visuals():
    out=OUT/'shots';out.mkdir(parents=True,exist_ok=True)
    pages=[]
    for scenario in [0,10,20,1,2,3,4,5,9,6,7,8]:
        path=out/f'{scenario:02}.png'
        run([CODE/'fftmenu_pc','--headless','--scenario',scenario,'--frames',16,'--shot',path])
        pages.append(Image.open(path).copy())
    decorate(pages[:3],VARIANTS,3).save(OUT/'layouts.png')
    decorate(pages[3:],[LABELS[i] for i in [1,2,3,4,5,9,6,7,8]],3).save(OUT/'menus.png')
    # Both bitmap sizes are shadow-free. Dialogue uses the smaller one to fit original pages.
    key=OUT/'font-toggle.txt';key.write_text('0 MODE\n1\n')
    samples=[]
    for scenario in (3,8):
        for comfort in (False,True):
            path=out/f'font-{scenario}-{int(comfort)}.png'
            run([CODE/'fftmenu_pc','--headless','--scenario',scenario,'--frames',16,
                 *(['--keys',key] if comfort else []),'--shot',path])
            samples.append(Image.open(path).copy())
    decorate(samples,['Small menu font / original icons','Comfort menu font / original icons',
                      'Small dialogue / fitted window','Small dialogue stays independent'],2).save(OUT/'font-comparison.png')
    light=out/'dialogue-light.png'
    run([CODE/'fftmenu_pc','--headless','--scenario',38,'--frames',16,'--shot',light])
    decorate([samples[2],Image.open(light).copy()],['Gradient black box / white ink (default)',
                  'Inverted white box / black ink'],2).save(OUT/'dialogue-styles.png')
    previous=OUT/'previous/shots'
    if (previous/'03.png').exists():
        decorate([Image.open(previous/'03.png'),samples[0],Image.open(previous/'08.png'),samples[2]],
                 ['Previous / equipment','Small font / all five slots',
                  'Previous / dialogue','Larger font / gradient / no shadows'],2).save(OUT/'small-font-before-after.png')
    run(['cc','-std=gnu99','-O1','-o',OUT/'capture','menu.c','capture.c',
         '../../../runtime/core/rt_core.c','../../../runtime/platform-sw/rt_sw.c'])
    tracks=[]
    for i in range(3):
        folder=OUT/f'frames-{i}';folder.mkdir(exist_ok=True)
        log=run([OUT/'capture',10*i,CODE/'keys/demo.txt',448,folder])
        (OUT/f'frames-{i}.csv').write_text('frame,screen,unit,row,checksum\n'+log)
        frames=[Image.open(p).convert('RGB').resize((640,400),Image.Resampling.NEAREST)
                for p in sorted(folder.glob('*.png'))]
        frames[0].save(OUT/f'prototype-{chr(97+i)}.gif',save_all=True,append_images=frames[1:],
                       duration=125,loop=0,disposal=2)
        tracks.append(frames)
    comparison=[decorate([tracks[j][i].resize((160,100),Image.Resampling.NEAREST) for j in range(3)],
                         VARIANTS,3,scale=3) for i in range(len(tracks[0]))]
    comparison[0].save(OUT/'comparison.gif',save_all=True,append_images=comparison[1:],duration=125,loop=0,disposal=2)
    folder=OUT/'frames-dialogue';folder.mkdir(exist_ok=True)
    dialogue_keys=OUT/'dialogue.txt';dialogue_keys.write_text('0 CATALOG\n1\n2 DOWN\n3\n4 ENTER\n5\n96 ENTER\n97\n192 ENTER\n193\n')
    log=run([OUT/'capture',0,dialogue_keys,256,folder]);(OUT/'dialogue.csv').write_text(log)
    frames=[Image.open(p).resize((640,400),Image.Resampling.NEAREST) for p in sorted(folder.glob('*.png'))]
    frames[0].save(OUT/'dialogue.gif',save_all=True,append_images=frames[1:],duration=125,loop=0,disposal=2)
    decorate([Image.open(folder/f'{n:04}.png').copy() for n in (4,20,40,128)],
             ['Future text in dark grey','Light-grey frontier',
              'Revealed text becomes white','Complete first page'],2).save(OUT/'dialogue-progression.png')

    new=[]
    for n in range(40,48):
        path=out/f'{n:02}.png'
        run([CODE/'fftmenu_pc','--headless','--scenario',n,'--frames',64,'--shot',path])
        new.append(Image.open(path).copy())
    decorate([pages[0],new[2],new[0]],['Normal actors','Small actors / four columns, three rows','Configurable from CATALOG'],3).save(OUT/'small-characters.png')
    decorate([pages[-1],new[3],new[7]],['Small font / fitted box','Longest referenced page / 103 characters','Short dialogue / fitted width and height'],3).save(OUT/'dialogue-capacity.png')
    decorate([pages[9],new[4],new[5],new[6]],['Squire current','Chemist available','Confirm local preview','Knight requirement'],2).save(OUT/'job-change.png')
    dense=[]
    for n in (48,49):
        path=out/f'{n:02}.png'
        run([CODE/'fftmenu_pc','--headless','--scenario',n,'--frames',64,'--shot',path])
        dense.append(Image.open(path).copy())
    decorate(dense,['4 x 3 / 12 slots (repeated test fixtures)','5 x 3 / 15 slots (repeated test fixtures)'],2).save(OUT/'dense-grids.png')
    scripts={
        'location':(41,400,'264 F5\n265\n'),
        'jobs':(6,220,'24 RIGHT\n25\n50 ENTER\n51\n82 ENTER\n83\n120 LEFT\n121\n148 ENTER\n149\n175 ENTER\n176\n'),
        'small-grid':(0,180,'16 CATALOG\n17\n36 ENTER\n37\n56 B\n57\n88 RIGHT\n89\n112 DOWN\n113\n136 LEFT\n137\n'),
        'dense-four':(48,160,'16 DOWN\n17\n40 DOWN\n41\n64 RIGHT\n65\n88 ENTER\n89\n116 B\n117\n'),
        'dense-five':(49,160,'16 DOWN\n17\n40 DOWN\n41\n64 RIGHT\n65\n88 ENTER\n89\n116 B\n117\n'),
        'themes-dark':(8,144,'0 F5\n1\n'),
        'themes-light':(38,144,'0 F5\n1\n'),
        'theme-config':(0,240,'0 CATALOG\n1\n8 UP\n9\n16 ENTER\n17\n24 B\n25\n32 CATALOG\n33\n40 DOWN\n41\n48 ENTER\n49\n176 ENTER\n177\n192 CATALOG\n193\n200 UP\n201\n208 ENTER\n209\n216 B\n217\n'),
        'capacity':(43,220,'0 F5\n1\n32 F1\n33\n152 F5\n153\n')}
    for name,(scenario,count,keys) in scripts.items():
        folder=OUT/('frames-'+name);folder.mkdir(exist_ok=True)
        keyfile=OUT/(name+'.txt');keyfile.write_text(keys)
        log=run([OUT/'capture',scenario,keyfile,count,folder]);(OUT/(name+'.csv')).write_text(log)
        frames=[Image.open(folder/f'{n:04}.png').resize((640,400),Image.Resampling.NEAREST) for n in range(0,count,4)]
        frames[0].save(OUT/(name+'.gif'),save_all=True,append_images=frames[1:],duration=125,loop=0,disposal=2)
    tracks=[]
    for variant in ('dense-four','dense-five'):
        tracks.append([Image.open(OUT/f'frames-{variant}'/f'{n:04}.png').copy() for n in range(0,160,4)])
    frames=[decorate([tracks[0][i],tracks[1][i]],['4 x 3 / repeated test roster','5 x 3 / repeated test roster'],2,scale=3) for i in range(40)]
    frames[0].save(OUT/'dense-grids.gif',save_all=True,append_images=frames[1:],duration=125,loop=0,disposal=2)
    tracks=[]
    for variant in ('themes-dark','themes-light'):
        tracks.append([Image.open(OUT/f'frames-{variant}'/f'{n:04}.png').copy() for n in range(0,144,4)])
    frames=[decorate([tracks[0][i],tracks[1][i]],['Black dialogue / white text','Inverted white dialogue / black text'],2,scale=3) for i in range(36)]
    frames[0].save(OUT/'dialogue-themes.gif',save_all=True,append_images=frames[1:],duration=125,loop=0,disposal=2)


def check():
    # Reuse the two existing artwork variables; only font is a new bank.
    banks={}
    for name,source in [('fftu',HERE/'fftu.bin'),('ffti',HERE/'ffti.bin'),('fftmfont',OUT/'data/fftmfont.bin'),
                        ('fftmicon',OUT/'data/fftmicon.bin'),('fftmvis',OUT/'data/fftmvis.bin')]:
        target=OUT/'data'/f'{name}.bin'
        if target!=source:target.write_bytes(source.read_bytes())
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',target,name],OUT/'data')
        variable=OUT/'data'/f'{name}.89y'
        raw=bytearray(variable.read_bytes());raw[0x49]=3;variable.write_bytes(raw)
        banks[name]=len(target.read_bytes())
    args=[ROOT/'tools/bin/ti-cycles','--max',1000]
    for name in banks:args+=['--file',OUT/'data'/f'{name}.89y']
    cases=[]
    # 30 independent screen/layout cases, both fonts, and an actual complete input tour.
    chemist=OUT/'chemist.txt';chemist.write_text('0 F5\n1\n2 F5\n3\n4 UP\n5\n')
    confirm=OUT/'confirm.txt';confirm.write_text('0 ENTER\n1\n')
    order=OUT/'order-preview.txt';order.write_text('0 UP\n1\n2 ENTER\n3\n')
    restart=OUT/'dialogue-restart.txt'
    restart.write_text('0 CATALOG\n1\n2 DOWN\n3\n4 ENTER\n5\n20 ENTER\n21\n24 F5\n25\n28 F1\n29\n')
    for name,frames,key,scenarios in [('settled',16,None,range(30)),
                                      ('comfort',16,OUT/'font-toggle.txt',range(10)),
                                      ('chemist',16,chemist,[3,5,9]),
                                      ('confirm',16,confirm,[3,5,9]),
                                      ('order-preview',16,order,[1]),
                                      ('light-dialogue',16,None,[38]),
                                      ('dialogue-early',12,OUT/'dialogue.txt',[0]),
                                      ('dialogue-mid',40,OUT/'dialogue.txt',[0]),
                                      ('dialogue-full',140,OUT/'dialogue.txt',[0]),
                                      ('dialogue-last-page',240,OUT/'dialogue.txt',[0]),
                                      ('dialogue-restart',32,restart,[0]),
                                      ('new-pages',16,None,range(40,50)),
                                      ('dense-four',160,OUT/'dense-four.txt',[48]),
                                      ('dense-five',160,OUT/'dense-five.txt',[49]),
                                      ('location-replay',400,OUT/'location.txt',[41]),
                                      ('job-change',220,OUT/'jobs.txt',[6]),
                                      ('small-config',180,OUT/'small-grid.txt',[0]),
                                      ('capacity-flow',220,OUT/'capacity.txt',[43]),
                                      ('capacity-mid',80,OUT/'capacity.txt',[43]),
                                      ('themes-dark',144,OUT/'themes-dark.txt',[8]),
                                      ('themes-light',144,OUT/'themes-light.txt',[38]),
                                      ('theme-config-early',72,OUT/'theme-config.txt',[0]),
                                      ('theme-config-full',160,OUT/'theme-config.txt',[0]),
                                      ('theme-config-return',240,OUT/'theme-config.txt',[0]),
                                      ('tour',448,CODE/'keys/demo.txt',[0,10,20])]:
        for scenario in scenarios:
            case=f'{name}-{scenario}'
            pc=run([CODE/'fftmenu_pc','--headless','--scenario',scenario,'--frames',frames,
                    *(['--keys',key] if key else [])])
            ti=run([*args,'--arg',scenario,'--frames',frames,*(['--keys',key] if key else []),CODE/'fftmenuc.89z'])
            (OUT/f'{case}.log').write_text(ti)
            assert '_main returned' in ti,case
            a=re.search(r'checksum (\w+)',pc)[1];b=re.search(r'checksum (\w+)',ti)[1]
            assert a.lower()==b.lower(),(case,a,b)
            zones={}
            values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
            assert values[0]==0x4D454E55 and values[-1]==frames
            assert len(values)==3+2*frames
            cold=[c for tag,c in zip(values[2:-1:2],values[3:-1:2]) if tag>>24]
            warm=[c for tag,c in zip(values[2:-1:2],values[3:-1:2]) if not tag>>24]
            cases.append(dict(case=case,frames=frames,checksum=a,
                              warm_max_cycles=max(warm,default=0),rebuild_max_cycles=max(cold,default=0),
                              state_bytes=values[1]))
    report=dict(scope='Native MC68000 datasheet cycles, no TiEmu/hardware claim',
                matched_cases=len(cases),cases=cases,new_font_bytes=banks['fftmfont'],
                new_equipment_icon_bytes=banks['fftmicon'],new_visual_bytes=banks['fftmvis'],
                reused_banks={k:v for k,v in banks.items() if k not in ('fftmfont','fftmicon','fftmvis')},
                scene_cache_heap_bytes=7680,menu_state_bytes=cases[0]['state_bytes'],
                executable_container_bytes=(CODE/'fftmenu.89z').stat().st_size,
                source_sha256={n:hashlib.sha256((CODE/n).read_bytes()).hexdigest() for n in ('menu.c','menu.h')})
    (OUT/'checked.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Matched %d native/PC menu checkpoints, including three 448-update tours' % len(cases))


def main():
    ap=argparse.ArgumentParser(__doc__);ap.add_argument('--check',action='store_true');args=ap.parse_args()
    OUT.mkdir(parents=True,exist_ok=True)
    visuals()
    if args.check:check()
    print(OUT/'layouts.png');print(OUT/'menus.png');print(OUT/'comparison.gif')
    print('GIFs are PC visual previews at 32 updates/s; export cadence is not measured TI FPS.')


if __name__=='__main__':main()
