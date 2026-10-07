#!/usr/bin/env python3
"""Create a deterministic LCD-only art preview without touching TiEmu."""
import subprocess
import argparse
from pathlib import Path
from PIL import Image, ImageDraw

GAME = Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--damage',action='store_true')
    parser.add_argument('--items',action='store_true')
    args=parser.parse_args()
    if args.items:
        directory=GAME/'x/items-preview'; directory.mkdir(parents=True,exist_ok=True)
        (GAME/'x/idle.txt').write_text('0\n')
        clips=[(60,'x/idle.txt',40,'Animated coins and collection'),
               (61,'x/idle.txt',420,'Idle animation and egg train'),
               (67,'keys/tongue-visible.txt',70,'Tongue, capture and egg conversion'),
               (62,'keys/items-spit.txt',80,'Hold and spit the Shy Guy'),
               (68,'keys/jump-stomp.txt',70,'Ground jump and Shy Guy stomp'),
               (61,'keys/aim-lock.txt',65,'Alpha: lock, unlock and relock aim'),
               (69,'keys/egg-hit.txt',85,'Locked aim, throw and Shy Guy hit'),
               (70,'keys/egg-bounce.txt',80,'Egg bouncing off the ground')]
        # GIF delays are centiseconds. Use20ms (~50fps versus51.2 updates/s):
        # browsers often clamp10ms delays. Every game update gets an image;
        # there are no artificial chapter holds or slow-motion intervals.
        palette=Image.new('P',(1,1))
        palette.putpalette([v for shade in (24,88,152,216,0,255) for v in (shade,)*3]+[0]*750)
        pictures=[]
        for clip,(scenario,keys,count,label) in enumerate(clips):
            for f in range(1,count+1):
                path=directory/f'{clip}-{f:04d}.png'
                subprocess.run([str(GAME/'yjprobe_pc'),'--headless','--scenario',str(scenario),
                    '--keys',keys,'--frames',str(f),'--shot',str(path)],cwd=GAME,check=True,capture_output=True)
                picture=Image.new('RGB',(480,324),'white')
                with Image.open(path) as source: picture.paste(source.resize((480,300),Image.Resampling.NEAREST),(0,24))
                ImageDraw.Draw(picture).text((8,6),label,fill='black')
                pictures.append(picture.quantize(palette=palette,dither=Image.Dither.NONE))
        output=GAME/'x/items-preview.gif'
        pictures[0].save(output,save_all=True,append_images=pictures[1:],duration=20,loop=0,optimize=False,disposal=1)
        # A distinct link avoids a viewer caching the preceding slow preview.
        (GAME/'x/items-preview-smooth.gif').write_bytes(output.read_bytes())
        print('Saved x/items-preview-smooth.gif:',len(pictures),'consecutive LCD updates at50fps,',len(pictures)*20,'ms; no chapter pauses.')
        return
    name='damage-preview' if args.damage else 'art-preview'
    directory = GAME / ('x/'+name)
    directory.mkdir(parents=True,exist_ok=True)
    endpoint=540 if args.damage else 1193
    samples = [*range(1,endpoint,6),endpoint]
    frames = []
    for count in samples:
        path = directory / f'{count:04d}.png'
        subprocess.run([str(GAME/'yjprobe_pc'),'--headless','--scenario','55' if args.damage else '0',
                        '--keys','x/idle.txt' if args.damage else 'keys/actors.txt','--frames',str(count),'--shot',str(path)],
                       cwd=GAME,check=True,capture_output=True)
        with Image.open(path) as source:
            assert source.size == (640,400)
            frames.append(source.resize((480,300),Image.Resampling.NEAREST))
    durations = [round((b-a)*1000/51.2) for a,b in zip(samples,samples[1:])] + [1500]
    frames[0].save(GAME/('x/'+name+'.gif'),save_all=True,append_images=frames[1:],
                   duration=durations,loop=0,optimize=False)
    print('Saved x/'+name+'.gif:',len(frames),'LCD frames; provisional 51.2 Hz playback.')


if __name__ == '__main__': main()
