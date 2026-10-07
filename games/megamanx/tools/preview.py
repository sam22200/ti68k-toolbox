#!/usr/bin/env python3
"""Consecutive native LCD frames, calculator-speed GIF, no artificial holds."""
import json,subprocess
from PIL import Image,ImageDraw
from reference import GAME

def main():
    directory=GAME/'x/preview';directory.mkdir(parents=True,exist_ok=True)
    clips=[(101,'charge-preview',95,'Charge: converging sparks and pulsing armor'),
           (0,'traverse',330,'Run, charge shot, enemy defeat and gap jump'),
           (2,'wall',90,'Wall slide, short wall jump and held wall jump'),
           (16,'roller-double',95,'Two small hits: the roller brakes, then explodes'),
           (18,'idle',55,'Medium charge passes through the roller'),
           (16,'damage-charge',90,'Charge survives contact; recoil blocks firing'),
           (101,'run-charge',75,'Charge formation follows the running cannon'),
           (101,'jump-fire',35,'Shoot while jumping'),
           (120,'wall-slide-charge',30,'Charged shot points away from the wall'),
           (120,'wall-launch-charge',40,'Wall kick moves left while X fires right'),
           (101,'left-charge',75,'Charged shot to the left, including screen-edge travel'),
           (101,'stop-fire',45,'Stopping retains the running muzzle for one update'),
           (16,'roller-behind',85,'Jump past the roller, turn and fire from behind'),
           (101,'stop-jump',60,'Jump as you release the run direction'),
           (101,'run-jump-large',100,'Charge, run and release while jumping')]
    pictures=[];durations=[];labels=[]
    palette=Image.new('P',(1,1))
    palette.putpalette([v for shade in (24,88,152,216,0,255) for v in (shade,)*3]+[0]*750)
    for clip,(n,keys,count,label) in enumerate(clips):
        for f in range(1,count+1):
            path=directory/f'{clip}-{f:04d}.png'
            subprocess.run([str(GAME/'mmx_pc'),'--headless','--scenario',str(n),'--keys',f'keys/{keys}.txt',
                '--frames',str(f),'--shot',str(path)],cwd=GAME,check=True,capture_output=True)
            picture=Image.new('RGB',(480,324),'white')
            with Image.open(path) as src:picture.paste(src.resize((480,300),Image.Resampling.NEAREST),(0,24))
            ImageDraw.Draw(picture).text((8,6),label,fill='black')
            pictures.append(picture.quantize(palette=palette,dither=Image.Dither.NONE))
            # Calculator: 512/17 rendered frames/sec. GIF needs centiseconds.
            i=len(pictures);durations.append((round(i*1700/512)-round((i-1)*1700/512))*10)
        labels.append(dict(scenario=n,keys=keys,frames=count,label=label))
        print('Captured',label,count,'consecutive frames',flush=True)
    output=GAME/'x/mmx-highway-smooth.gif'
    pictures[0].save(output,save_all=True,append_images=pictures[1:],duration=durations,loop=0,optimize=False,disposal=1)
    with Image.open(output) as gif:
        encoded=gif.n_frames;actual=0
        for f in range(encoded):gif.seek(f);actual+=gif.info['duration']
    assert actual==sum(durations)
    (GAME/'x/preview.json').write_text(json.dumps(dict(clips=labels,consecutive_frames=len(pictures),
        encoded_frames=encoded,duration_ms=actual,render_hz=512/17,no_artificial_holds=True),indent=2)+'\n')
    print('Saved',output,'duration',actual,'ms;',len(pictures),'frames at512/17Hz')
if __name__=='__main__':main()
