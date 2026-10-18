#!/usr/bin/env python3
"""Native low-poly F1/F5: eight directions, PC/TI pictures and datasheet costs."""
import csv
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[1]
OUT=HERE/'x/turn-audit'
FLAGS=['-DFFT_EXPLORATION','-DFFT_FIXED','-DFFT_CINEMA','-DFFT_CAMERA_CUT_DEMO','-DFFT_CAMERA_AUDIT']


def run(args,cwd=HERE):
    return subprocess.check_output([str(a) for a in args],cwd=cwd,text=True)


def read_tiemu():
    """Read the printed 256 Hz timer table against the local AMS digit bitmaps."""
    from PIL import Image
    text=(ROOT/'runtime/platform-sw/amsfont.h').read_text()
    font=list(map(int,re.findall(r'\d+',re.search(r'ams_f4x6\[1536\] = \{(.*?)\};',text,re.S).group(1))))
    def trim(rows):
        cols=[x for x in range(len(rows[0])) if any(r[x] for r in rows)]
        return tuple(tuple(r[cols[0]:cols[-1]+1]) for r in rows)
    digits={str(n):trim([[(font[(48+n)*6+1+y]>>(7-x))&1 for x in range(8)] for y in range(5)]) for n in range(10)}
    shot=Image.open(OUT/'timer-results.png').convert('L'); assert shot.size==(220,492)
    lcd=shot.crop((30,46,192,148)); rows=[]
    for c in range(8):
        bitmap=[[int(lcd.getpixel((x,15+c*8+y))<128) for x in range(lcd.width)] for y in range(5)]
        x=0; last=-10; groups=[]
        while x<lcd.width:
            if not any(r[x] for r in bitmap): x+=1; continue
            left=x
            while x<lcd.width and any(r[x] for r in bitmap): x+=1
            piece=tuple(tuple(r[left:x]) for r in bitmap)
            found=[n for n,glyph in digits.items() if glyph==piece]; assert len(found)==1,(c,left,found)
            if left-last>4: groups.append('')
            groups[-1]+=found[0]; last=left
        case,total,frames,worst,ok=map(int,groups)
        assert case==c and frames==4 and ok==1 and total>=worst
        rows.append(dict(case=c,source_orientation=c>>1,key='F1' if c&1 else 'F5',
                         total_ms=total,frames=frames,worst_frame_ms=worst,ok=True))
    headless=json.loads((OUT/'headless.json').read_text())
    report=dict(platform='Docker TiEmu 3.04, Titanium HW3 AMS 3.10, configured 12 MHz',
        timer='256 Hz; grayscale, GrayDBuf swaps and ordinary eight-tick/32 FPS cap active.',
        scope=headless['scope']+' Includes destination decode/render; setup/source/destination holds are outside timer.',
        results=rows,binary_sha256=headless['binary_sha256'],
        screenshot_sha256=hashlib.sha256((OUT/'timer-results.png').read_bytes()).hexdigest(),
        limits='Not physical hardware. Captured GIF playback rate is separate from measured interaction time.')
    (OUT/'tiemu.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


def main():
    data=OUT/'data'; data.mkdir(parents=True,exist_ok=True)
    frames=OUT/'frames'; frames.mkdir(exist_ok=True)
    source=HERE/'x/tiemu-camera/data'
    assert (source/'fftlook.bin').exists(), 'Prepare make tiemu-camera-demo first.'
    for p in source.iterdir():
        if p.suffix in ('.bin','.89y'): shutil.copy2(p,data/p.name)
    run(['cc','-std=gnu99','-O1','-g',*FLAGS,'-o',OUT/'oracle',
         'tools/turn_audit_cycles.c','../../runtime/core/rt_core.c','../../runtime/platform-sw/rt_sw.c'])
    pc=run([OUT/'oracle',frames],data); (OUT/'pc.csv').write_text(pc)
    for name,source,extra in [('fftturn','turn_audit_tiemu.c',[]),('fftturnc','turn_audit_cycles.c',['-DRT_CYCLES'])]:
        run([ROOT/'tools/bin/ti-cc','-o',OUT/name,*FLAGS,*extra,'tools/'+source,
             '../../runtime/core/rt_core.c','../../lib/unpack68k.s','../../tools/extgraph/lib/tilemap.a'])
    args=[ROOT/'tools/bin/ti-cycles','--max',100000]
    for p in sorted(data.glob('*.89y')): args+=['--file',p]
    log=run([*args,OUT/'fftturnc.89z'],data); (OUT/'cycles.log').write_text(log)
    actual=re.findall(r'shot \d+ checksum (\w+)',log)
    expected=list(csv.reader(pc.splitlines()))
    assert [v.lower() for v in actual]==[r[2].lower() for r in expected] and len(actual)==32
    values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
    samples=[[] for _ in range(8)]; c=None; ok=0
    for i,v in enumerate(values):
        if v>>24==0x71: c=v&255
        elif v>>24==0x72: samples[c].append(values[i+1])
        elif v>>24==0x73: assert v&255; ok+=1
    assert ok==8
    cases=[dict(case=c,source_orientation=c>>1,key='F1' if c&1 else 'F5',
                frames=len(s),intermediate_frames=3,total_cycles=sum(s),worst_cycles=max(s),
                cpu_only_ms_at_12mhz=sum(s)/12000,frame_cycles=s) for c,s in enumerate(samples)]
    textured='-DFFT_TURN_TEXTURE' in FLAGS
    flat='-DFFT_TURN_TEXTURE_FLAT' in FLAGS
    resolution=('40x100' if '-DFFT_TURN_TEXTURE_QUARTER_X' in FLAGS else
                '80x25' if '-DFFT_TURN_TEXTURE_HALF_Y' in FLAGS else '80x50')
    geometry=('compatible merged surfaces, conservative angle lists' if '-DFFT_TURN_TEXTURE_MERGE' in FLAGS else
              'original 866-triangle mesh, conservative angle lists' if '-DFFT_TURN_TEXTURE_SURFACES' in FLAGS else
              'original 866-triangle mesh')
    scope=(f'Native {resolution} affine texture mapping, {geometry}, depth-aware TI actors.' if textured and not flat else
           'Native 80x50 flat control, same 866-triangle mesh/depth/actors, no UV sampling.' if textured else
           'Native 160x100 low-poly terrain rotation, outlined TI actors, textured endpoints.' if '-DFFT_TURN_LOWPOLY_NATIVE' in FLAGS else
           'Native 80x50 low-poly terrain rotation, outlined TI actors, textured endpoints.')
    report=dict(scope=scope,
                matched_pc_ti_frames=32,checks='Distinct intermediate pictures; correct final view; unchanged units/RNG; no perspective travelling or attack shot.',
                work_bytes=values[0],cases=cases,build_flags=FLAGS,
                binary_bytes=(OUT/'fftturn.89z').stat().st_size,
                binary_sha256=hashlib.sha256((OUT/'fftturn.89z').read_bytes()).hexdigest(),
                timing='Datasheet CPU only, excludes grayscale/interrupts/swaps/waits. Not measured hardware FPS.')
    (OUT/'headless.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    if '--read-tiemu' in sys.argv: read_tiemu()
    else: main()
