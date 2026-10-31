#!/usr/bin/env python3
"""Controller-only PS1 job-change observation from an existing world fixture."""
from pathlib import Path
import argparse,hashlib,json
from PIL import Image
import oracle
from review_files import save_image,write_changed

HERE=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('disc');ap.add_argument('state',type=Path)
    ap.add_argument('--out',type=Path,default=HERE/'campaign/x/qa-followup/source-job')
    args=ap.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    p=oracle.PSX(args.disc);p.load(str(args.state.resolve()))
    p.pressed=0
    for _ in range(60):p.run() # release controller state carried by the fixture
    actions=[('START',12),('DOWN',8),('CIRCLE',8),('TRIANGLE',8),('DOWN',8),('DOWN',8),('CIRCLE',8),('RIGHT',8)]
    for key,hold in actions:
        p.pressed=oracle.key(key)
        for _ in range(hold):p.run()
        p.pressed=0
        for _ in range(60):p.run()
    frames=[];samples={}
    for frame in range(241):
        p.pressed=oracle.key('CIRCLE') if frame<8 else 0;p.run()
        if frame%4==0:frames.append(p.image().copy())
        if frame in (0,16,32,64,96,128,160,200,240):samples[frame]=p.image().copy()
    # One exported sample spans four60Hz source frames; GIF ticks are10ms.
    durations=[10*(round((i+1)*100/15)-round(i*100/15)) for i in range(len(frames))]
    durations[-1]+=800
    save_image(frames[0],args.out/'original.gif',save_all=True,append_images=frames[1:],duration=durations,loop=0,disposal=2)
    sheet=Image.new('RGB',(5*256,2*256),'white')
    for i,(frame,picture)in enumerate(samples.items()):sheet.paste(picture,(256*(i%5),256*(i//5)))
    save_image(sheet,args.out/'samples.png')
    report=dict(scope='Controlled original PS1 world fixture; controller inputs only in this observer, no RAM pokes. Not an unmodified full campaign playthrough.',
                state_sha256=hashlib.sha256(args.state.read_bytes()).hexdigest(),preparation=actions,
                initial_release_frames=60,idle_frames_after_each=60,confirm_hold=8,captured_source_frames=241,sample_stride=4,
                observed='Vertical purple/translucent light column envelops the stationary unit; visible in samples16..200 and gone by240. No actor jump.',
                native='Four-grey light-column adaptation, no effect-image bank. Exact original poses/pixels and duration equivalence are not claimed.')
    write_changed(args.out/'observation.json',(json.dumps(report,indent=2)+'\n').encode())
    print(args.out.resolve())

if __name__=='__main__':main()
