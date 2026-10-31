#!/usr/bin/env python3
"""Animated1px/2px actual-renderer comparison, four views at identical ticks."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile
from PIL import Image,ImageDraw,ImageFont

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];OUT=HERE/'x/edge-width'
sys.path.insert(0,str(HERE.parent/'tools'))
from review_files import save_image,write_changed

def run(args,cwd=HERE/'data'):
    subprocess.run(list(map(str,args)),cwd=cwd,check=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',20)
    small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',14)
    frames=[];audit=[]
    with tempfile.TemporaryDirectory(prefix='fft-edge-animation-')as folder:
        temp=Path(folder);shots=temp/'shots';shots.mkdir()
        run(['cc','-std=gnu99','-O1','-w','-o',temp/'preview',HERE/'preview_edge_animation.c',
             ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
        run([temp/'preview',shots])
        for rot in range(4):
            hashes=[[],[]]
            for t in range(48):
                frame=Image.new('RGB',(1008,400),'white');draw=ImageDraw.Draw(frame)
                for wide in range(2):
                    x=12+504*wide
                    draw.text((x,10),f'{"A" if not wide else "B"}  /  {wide+1}px scenery edges',font=font,fill='black')
                    picture=Image.open(shots/f'view-{rot}-width-{wide}-frame-{t:02d}.png').convert('RGB')
                    assert picture.size==(160,100)
                    hashes[wide].append(hashlib.sha256(picture.tobytes()).hexdigest())
                    frame.paste(picture.resize((480,300),Image.Resampling.NEAREST),(x,46))
                draw.text((12,354),f'Gariland / view {rot+1} of 4 / identical camera and character poses',font=small,fill='black')
                draw.text((12,377),'PC renderer / illustrative playback; not a TI speed measurement',font=small,fill='black')
                frames.append(frame)
            assert all(len(set(h))>1 for h in hashes),'Idle poses must actually animate.'
            audit.append(dict(view=rot,frames_per_width=48,unique_images=[len(set(h))for h in hashes]))
    path=OUT/'edge-width-decision.gif'
    save_image(frames[0],path,save_all=True,append_images=frames[1:],duration=70,loop=0,disposal=2)
    with Image.open(path)as gif:
        assert gif.n_frames==192 and gif.size==(1008,400)
        assert sum(gif.seek(i)or gif.info['duration']for i in range(gif.n_frames))==13440
    write_changed(OUT/'animation.json',(json.dumps(dict(views=audit,frames=192,duration_ms=13440,
        scope='Actual portable renderer; animated idle poses, paired widths, four direct view cuts; not captured camera rotation or measured TI FPS.'),indent=2)+'\n').encode())
    print(path.resolve())

if __name__=='__main__':main()
