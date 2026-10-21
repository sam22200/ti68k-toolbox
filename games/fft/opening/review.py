#!/usr/bin/env python3
"""Portable presentation GIFs; playback is not a calculator FPS measurement."""
from pathlib import Path
import subprocess, json, argparse
from PIL import Image
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];OUT=HERE/'x/review'
def main():
    parser=argparse.ArgumentParser(__doc__);parser.add_argument('--title-only',action='store_true');args=parser.parse_args()
    OUT.mkdir(parents=True,exist_ok=True)
    subprocess.run(['cc','-std=gnu99','-O1','-g','-o',str(OUT/'replay'),str(HERE/'review.c'),str(ROOT/'runtime/core/rt_core.c'),str(ROOT/'runtime/platform-sw/rt_sw.c')],check=True)
    cases=((0,'orbonne-title'),) if args.title_only else ((0,'orbonne-opening'),(2,'orbonne-battle'),(3,'orbonne-ending'),(2,'orbonne-animation'))
    for scenario,name in cases:
        frames=OUT/name;frames.mkdir(exist_ok=True)
        for p in frames.glob('*.png'):p.unlink()
        subprocess.run([str(OUT/'replay'),str(scenario),str(frames)]+(['title'] if args.title_only else ['normal'] if name=='orbonne-animation' else []),check=True,cwd=HERE/'data')
        source=sorted(frames.glob('*.png'));images=[]
        # Battle playback: show its first 40 seconds, the complete replay is checked
        # separately. Source frames are every third 30Hz engine update (100 ms).
        for p in source[:400 if scenario==2 else len(source)]:
            with Image.open(p) as im:images.append(im.resize((640,400),Image.Resampling.NEAREST).convert('P'))
        images[0].save(OUT/(name+'.gif'),save_all=True,append_images=images[1:],duration=100,loop=0,optimize=True)
    (OUT/'README.txt').write_text('PC framebuffer review at nominal 30 engine updates/s. orbonne-animation uses no fast-forward input. GIF speed is illustrative, NOT a TI benchmark. Native timings: ../check/checked.json.\n')
    print(OUT)
if __name__=='__main__':main()
