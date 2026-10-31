#!/usr/bin/env python3
"""Current actual-renderer flow review only; older screenshots are never regenerated."""
from pathlib import Path
import tempfile,argparse
from PIL import Image,ImageDraw
from check_flow import HERE,ROOT,OUT,run,data_links,fixture
from review_files import publish_images,save_image

def sheet(entries,name,columns=3):
    im=Image.new('RGB',(columns*492,((len(entries)+columns-1)//columns)*328),'white');d=ImageDraw.Draw(im)
    for i,(path,title) in enumerate(entries):
        x=i%columns*492;y=i//columns*328;d.text((x+5,y+5),title,fill='black')
        im.paste(Image.open(path).convert('RGB').resize((480,300),Image.Resampling.NEAREST),(x,y+24))
    save_image(im,OUT/'new'/name)

def main():
    global OUT
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,default=OUT)
    ap.add_argument('--before',type=Path);ap.add_argument('--deployment-only',action='store_true')
    ap.add_argument('--before-label',default='light-gray inactive slabs')
    ap.add_argument('--after-label',default='legal slabs only / thicker sides')
    args=ap.parse_args();OUT=args.out.resolve()
    if args.deployment_only and not args.before:ap.error('--deployment-only requires --before')
    out=OUT/'new';out.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='fft-flow-review-') as name:
        temp=Path(name);data_links(temp);shots=temp/'shots';shots.mkdir();(temp/'ffsave.sav').write_bytes(fixture())
        for source,program in (('preview_flow_scene.c','scene-preview'),('preview_deploy.c','deploy-preview')):
            if args.deployment_only and source!='preview_deploy.c':continue
            run(['cc','-std=gnu99','-O1','-w','-o',OUT/program,HERE/source,
                 ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
            run([OUT/program,shots],temp)
        publish_images(shots,out)
    if args.deployment_only:
        before=args.before.resolve()
        entries=[]
        for name,label in (('deploy-initial.png','Ramza'),('deploy-full.png','five units'),('deploy-confirm.png','confirmation')):
            entries.extend(((before/name,'Previous:'+label+' / '+args.before_label),
                            (out/name,'Requested:'+label+' / '+args.after_label)))
        sheet(entries,'deployment-slabs-review.png',2)
        frames=[]
        for i in range(8):
            im=Image.new('RGB',(984,328),'white');d=ImageDraw.Draw(im)
            for x,path,title in ((0,before/'deploy-confirm.png','Previous:confirmation'),
                                 (492,out/f'deploy-glove-{i}.png','Requested:closer animated hand / compact box')):
                d.text((x+5,5),title,fill='black')
                im.paste(Image.open(path).convert('RGB').resize((480,300),Image.Resampling.NEAREST),(x,24))
            frames.append(im)
        save_image(frames[0],out/'deployment-glove-review.gif',save_all=True,append_images=frames[1:],
                   duration=125,loop=0,disposal=2)
        print('Scoped deployment comparison:',out);return
    sheet([(out/'academy-narration-0.png','1.Dark Academy narration / original text'),
           (out/'academy-chapter.png','2.Chapter title / same dark scenery'),
           (out/'academy-location.png','3.Academy location / same dark scenery'),
           (out/'academy-dialogue.png','4.Normal Academy / Ramza portrait'),
           (out/'gariland-location.png','6.Gariland title / after deployment'),
           (out/'gariland-before-dialogue.png','7.Source pre-battle dialogue / CT paused')],'flow-scene-order.png')
    sheet([(out/'deploy-initial.png','5.Deployment:Ramza auto-placed / 7-member roster'),
           (out/'deploy-full.png','5.Chosen five / two reserves kept'),
           (out/'deploy-confirm.png','5.Explicit confirmation / legal world tiles')],'flow-deployment-review.png')
    sheet([(out/'victory-1.png','After dialogue:actual runtime congratulations'),
           (out/'victory-2.png','Actual calculated battle bonus:500gil'),
           (out/'victory-3.png','Actual live trophies:2000gil + source items')],'flow-victory-runtime.png')
    sheet([(out/'orbonne-after-ko.png','Orbonne:enemies hidden / KO ally stays KO'),
           (out/'gariland-after-dialogue.png','Gariland:survivors face Ramza / KO stays KO')],'flow-aftermath.png',2)
    if args.before:
        before=args.before.resolve()
        sheet([(before/'gariland-location.png','Previous:world texture behind Gariland title'),
               (out/'gariland-location.png','Requested:white title on black background')],'gariland-title-review.png',2)
        sheet([(before/'deploy-initial.png','Previous:selected Ramza / flat merged tiles'),
               (out/'deploy-initial.png','Requested:Formation details / raised stone slabs'),
               (before/'deploy-full.png','Previous:five selected units'),
               (out/'deploy-full.png','Requested:five units / white rims / depth order'),
               (before/'deploy-confirm.png','Previous:confirmation'),
               (out/'deploy-confirm.png','Requested:confirmation / reserves kept')],'deployment-refined-review.png',2)
        sheet([(before/'victory-3.png','Previous:left-aligned trophy rows'),
               (out/'victory-3.png','Requested:each icon and name centred together')],'victory-trophies-centered-review.png',2)
        paths=[out/f'deploy-unit-{i}.png' for i in range(7)]+[out/f'deploy-prev-{i}.png' for i in range(5,-1,-1)]
        frames=[Image.open(p).resize((640,400),Image.Resampling.NEAREST) for p in paths]
        save_image(frames[0],out/'deployment-f1-f5.gif',save_all=True,append_images=frames[1:],duration=550,loop=0,disposal=2)
    print('Versioned actual-renderer comparisons:',out)

if __name__=='__main__':main()
