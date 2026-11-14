#!/usr/bin/env python3
"""Actual approved formation pages and input flows; two native cases only on request."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tempfile

from PIL import Image,ImageDraw
from check_flow import HERE,ROOT,data_links,fixture,run
import monster_formation_bank as bank
from review_files import publish_images,save_image,write_changed


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',action='store_true');args=ap.parse_args()
    out=HERE/'x/monster-formation';out.mkdir(parents=True,exist_ok=True)
    reference=HERE/'x/monster-family-review';approved=json.loads((reference/'results.json').read_text())
    review=(reference/'ffmr.bin').read_bytes();assert hashlib.sha256(review).hexdigest()==approved['bank_sha256']
    raw=bank.from_review(review);write_changed(out/'ffma.bin',raw)
    # Prove that all retained body/portrait bytes are lossless, not a new art pass.
    for i in range(16):assert raw[bank.FRAME_BASE+660*i:bank.FRAME_BASE+660*(i+1)]==review[4136+660*i:4136+660*(i+1)]
    reports={};common=[HERE/'check_monster_formation.c',ROOT/'runtime/core/rt_core.c']
    cases=[('manage',[],[('before',0),('formation',1),('status',12),('status-chocobo',10),('status-hydra',13),
                         ('selected-chocobo',20),('selected-black-chocobo',21),('selected-morbol',24)]),
           ('deploy',['-DMONSTER_DEPLOY_REVIEW'],[('deploy-before',0),('deploy',1)])]
    for mode,flags in [('pc',[]),('san',['-fsanitize=address,undefined','-fno-sanitize=shift','-fno-pie','-no-pie'])]:
        rows={}
        with tempfile.TemporaryDirectory(prefix='fft-monster-formation-')as folder:
            data=Path(folder);data_links(data,HERE/'x/eggs/shipping');(data/'ffma.bin').symlink_to(out/'ffma.bin')
            initial=fixture();(data/'ffsave.sav').write_bytes(initial)
            for kind,defines,shots in cases:
                exe=out/(kind+'-'+mode)
                (out/(exe.name+'-build.log')).write_text(run(['cc','-std=gnu99','-O1','-g','-w',*flags,*defines,'-o',exe,*common,ROOT/'runtime/platform-sw/rt_sw.c']))
                for label,scenario in [*shots,(kind+'-flow','flow')]:
                    # Actual input and reserve guards run with the installed art.
                    log=run(['env','ASAN_OPTIONS=detect_leaks=0','UBSAN_OPTIONS=halt_on_error=1',exe,scenario,data/(label+'.png')],data)
                    match=re.search(r'monster-formation (\w+) checks(\d+) failures0',log);assert match,log
                    if scenario!='flow':
                        old=re.search(r'review (\w+)',approved['snapshots'][label])[1];assert match[1]==old,(label,log,old)
                    rows[label]=dict(checksum=match[1],checks=int(match[2]));print(mode,label,rows[label],flush=True)
                assert (data/'ffsave.sav').read_bytes()==initial
            if mode=='pc':publish_images(data,out)
        reports[mode]=rows
    assert reports['pc']==reports['san']
    image=Image.new('RGB',(992,656),'white');draw=ImageDraw.Draw(image)
    for i,(name,label)in enumerate([('formation','Integrated Formation'),('status','Integrated Status'),('selected-black-chocobo','Same art / real Black Chocobo job'),('deploy','Placement picture / battle gate still closed')]):
        x,y=(i%2)*496,(i//2)*328;draw.text((x+12,y+7),label,fill='black')
        image.paste(Image.open(out/(name+'.png')).resize((480,300),Image.Resampling.NEAREST),(x+8,y+24))
    save_image(image,out/'review.png')
    native=[]
    if args.native:
        (out/'native-build.log').write_text(run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'mform',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a']))
        with tempfile.TemporaryDirectory(prefix='fft-monster-native-')as folder:
            data=Path(folder);(data/'ffma.bin').write_bytes(raw);(data/'ffsave.sav').write_bytes(fixture())
            for name,kind in [('ffma','dat'),('ffsave','sav')]:run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89',kind,name+('.bin'if kind=='dat'else'.sav'),name],data)
            for label,scenario in [('formation',1),('status',12)]:
                cmd=[ROOT/'tools/bin/ti-cycles','--arg',scenario,'--frames',1,'--max',5000]
                for name in ('ffmg','ffcj','ffcp','ffeg'):cmd+=['--file',HERE/'x/eggs/shipping'/(name+'.89y')]
                log=run([*cmd,'--file',data/'ffma.89y','--file',data/'ffsave.89y',out/'mform.89z'])
                (out/(label+'-native.log')).write_text(log)
                checksum=re.search(r'checksum (\w+)',log)[1].upper();assert checksum==reports['pc'][label]['checksum'],log
                values=[int(x,16)for x in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)];assert values[-2:]==[55,0],values
                native.append(dict(case=label,checksum=checksum,checks=values[-2]));print('native',native[-1],flush=True)
    reports.update(native=native,art_bytes=len(raw),physical_pictures=16,logical_jobs=48,battle_deployment_enabled=False)
    (out/'results.json').write_text(json.dumps(reports,indent=2)+'\n');print(out/'review.png')


if __name__=='__main__':main()
