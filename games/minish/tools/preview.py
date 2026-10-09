#!/usr/bin/env python3
"""Capture the native opening as a GIF without an SDL window or TI emulator."""
import argparse
import ctypes as C
import os
from pathlib import Path
import subprocess
import tempfile
from PIL import Image, ImageDraw, ImageFont

GAME=Path(__file__).resolve().parents[1]
ROOT=GAME.parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sword',action='store_true')
    parser.add_argument('--combat',action='store_true')
    parser.add_argument('--shots',action='store_true')
    parser.add_argument('--effects',action='store_true')
    parser.add_argument('--showcase',action='store_true',help='walk, bushes and rolls, combat, rolls in one GIF')
    args=parser.parse_args()
    if args.showcase:return showcase()
    out=GAME/'captures'
    out.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='minish-preview-') as tmp:
        tmp=Path(tmp);so=tmp/'preview.so'
        sources=[GAME/'minish.c',GAME/'actions.c',GAME/'combat.c',GAME/'effects.c',GAME/'render_zoom.c',ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
        subprocess.run(['cc','-std=gnu99','-O1','-shared','-fPIC','-DRT_FRAME_TICKS2=17',
                        '-o',str(so),*map(str,sources)],check=True)
        os.chdir(GAME)
        lib=C.CDLL(str(so));lib.sw_init.argtypes=[C.c_uint16]
        lib.sw_step.argtypes=[C.c_uint32]
        lib.sw_parse_keys.argtypes=[C.c_char_p];lib.sw_parse_keys.restype=C.c_uint32
        lib.sw_write_png.argtypes=[C.c_char_p,C.c_int]
        prefix='effects' if args.effects else 'shots' if args.shots else 'combat' if args.combat else 'sword' if args.sword else 'opening'
        scenario=140 if args.effects else 420 if args.shots else 256 if args.combat else 140 if args.sword else 0
        lib.sw_init(scenario);lib.sw_step(0)
        if args.shots:
            for _ in range(13):lib.sw_step(0)
        if args.effects:
            lib.sw_step(16)
            for _ in range(8):lib.sw_step(0)
        start=prefix+'_start.png' if args.effects or args.shots or args.combat or args.sword else 'entrance.png'
        assert not lib.sw_write_png(str(out/start).encode(),3)
        palette=Image.new('P',(1,1));palette.putpalette([255,255,255,170,170,170,85,85,85,0,0,0]+[0]*756)
        script='keys/idle.txt' if args.shots else 'keys/combat.txt' if args.combat else 'keys/sword.txt' if args.sword else 'keys/opening.txt'
        cases=[(140,'keys/sword.txt',120),(432,'keys/kill.txt',90),(430,'keys/roll.txt',120)] if args.effects else [(scenario,script,120 if args.shots else 300 if args.combat else 120 if args.sword else 400)]
        frames=[]
        for scenario,script,count in cases:
            events={}
            for line in (GAME/script).read_text().splitlines():
                fields=line.split('#')[0].split()
                if fields:events[int(fields[0])]=lib.sw_parse_keys(' '.join(fields[1:]).encode())
            lib.sw_init(scenario);held=0
            for frame in range(count):
                held=events.get(frame,held);assert lib.sw_step(held)
                if args.effects and ((scenario==432 and frame==22) or (scenario==430 and frame==4)):
                    assert not lib.sw_write_png(str(out/('death.png' if scenario==432 else 'roll.png')).encode(),3)
                if frame&1:continue
                png=tmp/'frame.png';assert not lib.sw_write_png(str(png).encode(),3)
                with Image.open(png) as im:
                    frames.append(im.convert('RGB').quantize(palette=palette,dither=Image.Dither.NONE))
        frames[0].save(out/(prefix+'.gif'),save_all=True,append_images=frames[1:],duration=66,loop=0,optimize=False)
    print('Native preview:',out/(prefix+'.gif'))


SHOWCASE=[('Minish Woods',0,'keys/opening.txt',280),
          ('Sword, bushes and roll',140,'keys/showcase_bushes.txt',190),
          ('Octoroks',256,'keys/combat.txt',300),
          ('Roll',430,'keys/roll.txt',120)]


def showcase():
    """Chain continuous headless runs, a caption band under the 3x LCD."""
    out=GAME/'captures'
    out.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='minish-showcase-') as tmp:
        tmp=Path(tmp);so=tmp/'preview.so'
        sources=[GAME/'minish.c',GAME/'actions.c',GAME/'combat.c',GAME/'effects.c',GAME/'render_zoom.c',ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
        subprocess.run(['cc','-std=gnu99','-O1','-shared','-fPIC','-DRT_FRAME_TICKS2=17',
                        '-o',str(so),*map(str,sources)],check=True)
        os.chdir(GAME)
        lib=C.CDLL(str(so));lib.sw_init.argtypes=[C.c_uint16]
        lib.sw_step.argtypes=[C.c_uint32]
        lib.sw_parse_keys.argtypes=[C.c_char_p];lib.sw_parse_keys.restype=C.c_uint32
        lib.sw_write_png.argtypes=[C.c_char_p,C.c_int]
        palette=Image.new('P',(1,1));palette.putpalette([255,255,255,170,170,170,85,85,85,0,0,0]+[0]*756)
        font=ImageFont.load_default(size=16);frames=[]
        for title,scenario,script,count in SHOWCASE:
            events={}
            for line in (GAME/script).read_text().splitlines():
                fields=line.split('#')[0].split()
                if fields:events[int(fields[0])]=lib.sw_parse_keys(' '.join(fields[1:]).encode())
            lib.sw_init(scenario);held=0
            for frame in range(count):
                held=events.get(frame,held);assert lib.sw_step(held)
                if frame&1:continue
                png=tmp/'frame.png';assert not lib.sw_write_png(str(png).encode(),3)
                canvas=Image.new('RGB',(480,324),'white')
                with Image.open(png) as im:canvas.paste(im.convert('RGB'),(0,0))
                ImageDraw.Draw(canvas).text((6,303),title,font=font,fill='black')
                frames.append(canvas.quantize(palette=palette,dither=Image.Dither.NONE))
        frames[0].save(out/'showcase.gif',save_all=True,append_images=frames[1:],duration=66,loop=0,optimize=False)
    print('Native showcase:',out/'showcase.gif')


if __name__=='__main__':main()
