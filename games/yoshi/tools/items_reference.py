#!/usr/bin/env python3
"""Controlled PAL coin, idle and egg observations; all captures stay local."""
import json
from collections import Counter
from reference import OUT, ROM
from actors_reference import prepare
from terrain import load_map, TOP
from art import SNES, PAD, snapshot, foreground, Image

def main():
    directory=OUT/'items'; directory.mkdir(exist_ok=True)
    rom,numbers=load_map()
    counts=Counter(n for row in numbers for n in row)
    # A labelled Map16 sheet establishes which workspace cells are coins.
    pp=snapshot(0)
    sheet=Image.new('RGBA',(384,((len(counts)+11)//12)*40),(255,255,255,255))
    from PIL import ImageDraw
    draw=ImageDraw.Draw(sheet)
    for i,n in enumerate(sorted(counts)):
        rgba=foreground(rom,[[n]],pp)[:16,:16]
        sheet.paste(Image.fromarray(rgba),((i%12)*32,(i//12)*40))
        draw.text(((i%12)*32,(i//12)*40+17),f'{n:04x}',fill='black')
    sheet.save(directory/'map16.png')
    snes=SNES(ROM); traces={'source':json.loads((OUT/'start.json').read_text())}
    try:
        prepare(snes)
        idle=[]
        for f in range(460):
            snes.pressed=0; snes.lib.retro_run()
            idle.append([f,snes.read(0x7000f6,2),snes.read(0x7e0b7d,2)])
        traces['idle']=idle
        prepare(snes,True)
        eggs=[]
        for f in range(165):
            buttons=['Y'] if f==0 else ['DOWN'] if 20<=f<60 else ['A'] if f in (65,95) else []
            snes.pressed=sum(1<<PAD[b] for b in buttons); snes.lib.retro_run()
            egg=[]
            for off in range(0,96,4):
                if snes.read(0x700f00+off,2) and snes.read(0x701360+off,2) in (0x22,0x23,0x24,0x25):
                    egg.append([off,snes.read(0x701360+off,2),snes.read(0x7010e2+off,2),snes.read(0x701182+off,2),snes.read(0x701220+off,2),snes.read(0x701222+off,2)])
            eggs.append({'frame':f,'state':snes.read(0x7000de,2),'angle':snes.read(0x7000ee,2),
                         'cursor':[snes.read(0x7000e4,2),snes.read(0x7000e6,2)],'count':snes.read(0x701df6,2)>>1,'eggs':egg})
        traces['egg']=eggs
        coins=[(x*16,TOP+y*16) for y,row in enumerate(numbers) for x,n in enumerate(row) if n==0x6000]
        traces['coins']=coins
        prepare(snes)
        x,y=coins[0]
        snes.write(0x70008c,x,2); snes.write(0x700090,y-16,2)
        snes.pressed=0; snes.lib.retro_run()
        traces['coin_contact']={'x':x,'y':y,'count':snes.read(0x7e037b,1)}
    finally: snes.close()
    (directory/'reference.json').write_text(json.dumps(traces,indent=2)+'\n')
    print('Map16 counts', {hex(k):v for k,v in sorted(counts.items()) if k<0x500})
    print('Idle states',sorted({r[1] for r in idle}))
    print('Coins',coins,traces['coin_contact'])
    print('Egg aiming samples',eggs[65:70],eggs[95:100])

if __name__=='__main__': main()
