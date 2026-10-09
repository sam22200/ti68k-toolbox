#!/usr/bin/env python3
"""Precompose the LCD label, outlined hearts and readable projectile glyph."""
import re
from pathlib import Path
GAME=Path(__file__).resolve().parents[1]
font=(GAME.parents[1]/'runtime/platform-sw/amsfont.h').read_text().split('ams_f4x6[1536] = {')[1].split('}')[0]
data=list(map(int,re.findall(r'\d+',font)));rows=[[0]*8 for _ in range(2)];x=2
for char in 'MINISH WOODS':
    advance,*glyph=data[ord(char)*6:ord(char)*6+6]
    for y,row in enumerate(glyph,1):
        for b in range(8):
            if row&(128>>b):
                px=x+b;assert px<64;rows[px//32][y]|=1<<(31-(px&31))
    x+=advance
header='/* Local AMS font; identical static label, precomposed offline. */\nstatic const u32 label_rows[2][8]={'+','.join('{'+','.join(str(v)+'UL' for v in r)+'}' for r in rows)+'};\n'
def array(name, rows):
    return 'static const u32 '+name+f'[{len(rows)}][{len(rows[0])}]={{'+','.join('{'+','.join(str(v)+'UL' for v in row)+'}' for row in rows)+'};\n'

shape=[0x66,0xff,0xff,0x7e,0x3c,0x18]
silhouette={(heart*10+x+2,y+1) for heart in range(3) for y,row in enumerate(shape) for x in range(8) if row&(128>>x)}
# Clear only the heart silhouette and its single-pixel white halo. Gaps and
# the rest of the HUD keep the scenery, including between the heart lobes.
halo={(x+dx,y+dy) for x,y in silhouette for dx in (-1,0,1) for dy in (-1,0,1)}
mask_rows=[0xffffffff]*8
for x,y in halo:
    assert 0<=x<32 and 0<=y<8
    mask_rows[y]&=~(1<<(31-x))
header+='static const u32 heart_mask[8]={'+','.join(str(v)+'UL' for v in mask_rows)+'};\n'
lights=[];darks=[]
for health in range(13):
    light_rows=[0]*8;dark_rows=[0]*8
    for heart in range(3):
        fill=max(0,min(4,health-heart*4))
        for y,row in enumerate(shape):
            for x in range(8):
                if not row&(128>>x):continue
                edge=(x==0 or x==7 or not row&(128>>(x-1)) or not row&(128>>(x+1)) or
                      y==0 or y==5 or not shape[y-1]&(128>>x) or not shape[y+1]&(128>>x))
                quarter=(0 if x<4 else 1)+(0 if y<3 else 2)
                if edge or quarter<fill:
                    bit=1<<(31-(heart*10+x+2));light_rows[y+1]|=bit
                    if edge or not (y==1 and x in (2,3)):dark_rows[y+1]|=bit
    lights.append(light_rows);darks.append(dark_rows)
header+=array('heart_light',lights)+array('heart_dark',darks)

# Six-pixel round body plus a one-pixel outline: stays readable at 70% too.
light=[0,0x3c,0x7e,0x7e,0x7e,0x7e,0x3c,0]
dark= [0,0x3c,0x4e,0x4e,0x7e,0x7e,0x3c,0]
body={(x,y) for y,row in enumerate(light) for x in range(8) if row&(128>>x)}
halo={(x+dx,y+dy) for x,y in body for dx in (-1,0,1) for dy in (-1,0,1)}
mask=[0xff]*8
for x,y in halo:mask[y]&=~(128>>x)
for name,values in (('rock_light',light),('rock_dark',dark),('rock_mask',mask)):
    header+='static const u8 '+name+'[8]={'+','.join(str(v) for v in values)+'};\n'
rock=[]
for shift in range(16):
    r=[]
    for l,d,m in zip(light,dark,mask):r.extend([((l<<24)>>shift),((d<<24)>>shift),(~(((~m&255)<<24)>>shift))&0xffffffff])
    rock.append(r)
header+=array('rock_shift',rock)
(GAME/'hud_generated.h').write_text(header)
