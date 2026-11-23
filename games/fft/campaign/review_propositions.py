#!/usr/bin/env python3
"""Two PC-only worker-picker proposals using the real shared management/town UI."""
import hashlib
import json
import os
from pathlib import Path
import struct
import tempfile

from PIL import Image, ImageChops
import review_town_services as review
from check_flow import data_links
import proposition_source

HERE,ROOT=review.HERE,review.ROOT


def main():
    out=HERE/'x/propositions-review';out.mkdir(parents=True,exist_ok=True)
    protected={p for p in (HERE/'data').iterdir() if p.is_file() and p.suffix in ('.bin','.89y','.sav')}
    protected|={HERE/'x/check/ffsave_won.sav'}
    for folder in ('town-services-review','town-portrait-contrast','town-services-accepted',
                   'bar-reader-a','bar-service','bar-rumor-rules'):
        protected|={p for p in (HERE/'x'/folder).rglob('*') if p.is_file() and not p.is_symlink()}
    before={str(p.relative_to(ROOT)):review.fingerprint(p) for p in sorted(protected)}
    cue=ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    source=proposition_source.sample(str(cue))
    assert source['title']=='Testimony of Ex-miner' and source['location']==6 and source['required_stock']==5
    (out/'source.json').write_text(json.dumps(source,indent=2)+'\n')
    (out/'sample.bin').write_bytes(bytes((source['min_days'],source['max_days']))+struct.pack('>H',source['fee'])+
                                 source['title'].encode()+b'\0'+source['description'].encode()+b'\0')
    normal=None;common=[ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
    for sanitize in (False,True):
        flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'] if sanitize else []
        exe=out/('preview-san' if sanitize else 'preview')
        review.run(['cc','-std=gnu99','-O1','-g',*flags,'-o',exe,HERE/'preview_propositions.c',*common])
        with tempfile.TemporaryDirectory(prefix='fft-proposition-review-') as temp:
            data=Path(temp);data_links(data)
            initial=(HERE/'x/check/ffsave_won.sav').read_bytes();(data/'ffsave.sav').write_bytes(initial)
            log=review.run([exe,out],data,os.environ.copy());assert 'Save unchanged' in log
            assert (data/'ffsave.sav').read_bytes()==initial and not (data/'ffslot.sav').exists()
        hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob('*.png')
                if p.name in ('detail.png','confirm.png','a-workers.png','b-workers.png') or '-hand-' in p.name}
        if sanitize:assert hashes==normal
        else:normal=hashes
        (out/('sanitizers.log' if sanitize else 'pc.log')).write_text(log)
    for letter in ('a','b'):
        first=Image.open(out/(letter+'-hand-0.png')).convert('RGB')
        middle=Image.open(out/(letter+'-hand-3.png')).convert('RGB')
        bounds=ImageChops.difference(first,middle).getbbox();assert bounds and bounds[1]>=43 and bounds[3]<=80
        assert (out/(letter+'-hand-0.png')).read_bytes()==(out/(letter+'-hand-8.png')).read_bytes()
    review.sheet(out,'propositions-options.png',2,[
        [('detail','COMMON - original Gariland proposition /600gil /8-11days'),('confirm','COMMON - final confirmation /default No')],
        [('a-workers','A - Bar room /unit sheet /separate Team list'),('b-workers','B - Formation background /jobs and selection marks')]])
    frames=[Image.open(out/('a-hand-%u.png'%i)).resize((640,400),Image.Resampling.NEAREST)for i in range(8)]
    frames[0].save(out/'propositions-hand.gif',save_all=True,append_images=frames[1:],duration=133,loop=0)
    assert before=={str(p.relative_to(ROOT)):review.fingerprint(p) for p in sorted(protected)}
    result=dict(approval='pending',source=source,pc_sanitizers_equal=True,pc_screens=len(normal),
                save_unchanged=True,shipping_changed=False,native_run=False,
                source_room_and_portrait='approved FBR1 room0/owner117D, read in place',
                unit_panel='actual manage.c plate_unit, original snapshots/portraits/stats; no second renderer',
                protected_files=len(before),screen_sha256=normal,
                scope='PC worker-picker design proposals only; no Propositions dispatch/save/reward implementation yet')
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    (out/'preserved.json').write_text(json.dumps(before,indent=2)+'\n')
    print(log.strip());print('%d PC/sanitizer screens agree;%d prior files untouched.'%(len(normal),len(before)))


if __name__=='__main__':main()
