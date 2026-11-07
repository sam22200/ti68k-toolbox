#!/usr/bin/env python3
"""Reproducible accepted glyph banks and explicit historical strip controls."""
from pathlib import Path
import argparse,json,struct,sys
from PIL import ImageFont
from flow_assets import TITLES,legacy_titles,title_strip
import extract
from review_files import write_changed

HERE=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(HERE.parents[1]/'lib'))
import bitmap_font

TEXTS=TITLES+[('FINAL FANTASY TACTICS',11),('68k',15),('Orbonne Monastery',11)]
SIZES=(8,9,10,11,12,15)

def font_parts():
    strings=[];chars={size:set() for size in SIZES}
    for text,size in TEXTS:
        while ImageFont.truetype(str(bitmap_font.DEFAULT_FACE),size).getbbox(text)[2]>154:size-=1
        assert size in SIZES
        chars[size].update(text);strings.append((SIZES.index(size),text))
    chars[10].update('0123456789 GIL')
    font,faces=bitmap_font.build((size,chars[size]) for size in SIZES)
    titles=bytes((len(strings),))+b''.join(bytes((face,))+text.encode().ljust(24,b'\0') for face,text in strings)
    return titles,font,faces,strings

def build(out):
    titles,font,faces,strings=font_parts()
    active=(HERE/'campaign/data/ffflow.bin').read_bytes();assert active[:4]in(b'FQF1',b'FQF2')
    count=7 if active[:4]==b'FQF1' else 8
    offsets=list(struct.unpack_from('>'+str(count)+'H',active,4))+[len(active)]
    parts=[active[a:b] for a,b in zip(offsets,offsets[1:])][:7]
    parts[3]=legacy_titles()
    source=bytearray(b'FQF1'+bytes(14))
    for i,part in enumerate(parts):
        if len(source)&1:source.append(0)
        struct.pack_into('>H',source,4+2*i,len(source));source+=part
    old_title_bytes=len(parts[3])
    parts[3]=titles
    parts.append(font)
    bank=bytearray(b'FQF2'+bytes(16))
    for i,part in enumerate(parts):
        if len(bank)&1:bank.append(0)
        struct.pack_into('>H',bank,4+2*i,len(bank));bank+=part
    out.mkdir(parents=True,exist_ok=True);write_changed(out/'ffflow.bin',bank)
    write_changed(out/'ffserif.bin',font)
    current=(HERE/'campaign/data/fftitle.bin').read_bytes();a,b=struct.unpack_from('>2H',current,4)
    background=current[b:] if current[:4]==b'FTL1' else current[a:b]
    if len(background)&1:background+=b'\0'
    title=bytearray(b'FTB1'+struct.pack('>2H',8,8+len(background))+background)
    title+=extract.zx0pack.pack(title_strip('FINAL FANTASY TACTICS',11)+title_strip('68k',15))
    compact=bytearray(b'FTB2'+struct.pack('>2H',8,8+len(background))+background+b'\0\0')
    write_changed(out/'fftitle.bin',compact)
    legacy=out/'legacy';legacy.mkdir(exist_ok=True)
    write_changed(legacy/'ffflow.bin',source);write_changed(legacy/'fftitle.bin',title)
    report=dict(scope='Shared glyph renderer accepted2026-10-24. Explicit legacy controls generated separately.',
                font_bytes=len(font),faces=faces,strings=[dict(face=f,text=t) for f,t in strings],
                flow_before=len(source),flow_after=len(bank),old_title_bytes=old_title_bytes,
                title_b_before=len(title),title_b_after=len(compact),
                combined_data_delta=len(bank)+len(compact)-len(source)-len(title),
                unused_legacy_orbonne_strip='Still present in ffs0; no saving claimed for it.')
    write_changed(out/'assets.json',(json.dumps(report,indent=2)+'\n').encode())
    return report

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'campaign/x/shared-font/data')
    args=ap.parse_args();print(json.dumps(build(args.out),indent=2))
