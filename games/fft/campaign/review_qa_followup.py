#!/usr/bin/env python3
"""Hardware QA fixes, actual C images/GIFs and private fixture data only."""
from pathlib import Path
import json,tempfile
from PIL import Image,ImageDraw,ImageFont
from check_flow import HERE,ROOT,run,data_links,fixture
from review_files import save_image,write_changed

OUT=HERE/'x/qa-followup/review'

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',18)
    small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',14)
    report=[]
    with tempfile.TemporaryDirectory(prefix='fft-qa-followup-')as name:
        temp=Path(name);data=temp/'data';data.mkdir();data_links(data)
        save=bytearray(fixture());save[4]=3;save[37]=7
        (data/'ffsave.sav').write_bytes(save);(data/'ffsave1.sav').write_bytes(save)
        (data/'ffslot.sav').write_bytes(b'FSL1'+bytes((1,0)))
        for group,flag in (('deploy','REVIEW_DEPLOY'),('map','REVIEW_MAP'),('files','REVIEW_FILES'),('manage','REVIEW_MANAGE')):
            program=temp/group;shots=temp/(group+'-shots');shots.mkdir()
            run(['cc','-std=gnu99','-O1','-w','-D'+flag,'-o',program,HERE/'preview_qa_followup.c',
                 ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
            run([program,shots],data)
            if group=='map':
                (data/'ffwm.bin').unlink();(data/'ffwm.bin').symlink_to(HERE/'x/qa-followup/before/ffwm.bin')
                old=temp/'old-map';old.mkdir();run([program,old],data)
                pairs=[('Map seam / before',old/'seam-000.png'),('Map seam / corrected source',shots/'seam-000.png')]
                sheet=Image.new('RGB',(1320,462),'white');draw=ImageDraw.Draw(sheet)
                for i,(label,path)in enumerate(pairs):
                    draw.text((12+660*i,8),label,font=font,fill='black')
                    sheet.paste(Image.open(path).convert('RGB').resize((640,400),Image.Resampling.NEAREST),(12+660*i,38))
                save_image(sheet,OUT/'map-seam-before-after.png')
                (data/'ffwm.bin').unlink();(data/'ffwm.bin').symlink_to(HERE/'data/ffwm.bin')
            for prefix,label in (('deployment','Deployment / legal floor cursor,2nd placement and actor blink'),
                                 ('formation','Formation / floor diamond movement'),('job-confirm','Job confirmation / common animated glove'),
                                 ('job-change','Job change / source-observed light column, adapted to four greys'),('save-files','Map Data / exact title file-selector module')):
                paths=sorted(shots.glob(prefix+'-*.png'))
                if not paths:continue
                frames=[]
                for path in paths:
                    paired=prefix=='job-change';image=Image.new('RGB',(1320 if paired else 664,466),'white');draw=ImageDraw.Draw(image)
                    draw.text((12,8),'Job change / instant control' if paired else label,font=font,fill='black')
                    if paired:
                        before=shots/path.name.replace('job-change','job-static')
                        image.paste(Image.open(before).convert('RGB').resize((640,400),Image.Resampling.NEAREST),(12,38))
                        draw.text((672,8),'After / source-observed light column',font=font,fill='black')
                    image.paste(Image.open(path).convert('RGB').resize((640,400),Image.Resampling.NEAREST),(672 if paired else 12,38))
                    draw.text((12,444),'Actual C renderer / illustrative GIF cadence, not measured TI timing',font=small,fill='black')
                    frames.append(image)
                durations=[33 if prefix=='job-change' else 67]*len(frames);durations[-1]+=800
                dest=OUT/(prefix+'.gif');save_image(frames[0],dest,save_all=True,append_images=frames[1:],duration=durations,loop=0,disposal=2)
                report.append(dict(file=dest.name,logical_frames=len(frames)));print(dest.resolve(),flush=True)
            for prefix in ('confirm','white-jobs','learn-jp','magnet'):
                path=shots/(prefix+'-000.png')
                if path.exists():
                    im=Image.open(path).convert('RGB').resize((640,400),Image.Resampling.NEAREST)
                    save_image(im,OUT/(prefix+'.png'))
        write_changed(OUT/'review.json',(json.dumps(dict(files=report,
            scope='Actual C renderer, private saves. Job change follows the observed PS1 light-column effect; four-grey appearance and30Hz duration are native adaptations.'),indent=2)+'\n').encode())

if __name__=='__main__':main()
