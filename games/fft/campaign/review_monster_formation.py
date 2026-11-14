#!/usr/bin/env python3
"""Render private proposed monster art through actual PC management/deployment.

Generate explicit review-only copies with anchored hooks; never edit production
pages or activate deployment rules before the user's visual verdict. No native
build, optimization, benchmark or shipping package belongs to this review.
"""
import argparse
import hashlib
import json
import re
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw

from check_flow import HERE, ROOT, data_links, fixture, run
import core_audit
import monster_formation_review as art
from review_files import publish_images, save_image, write_changed


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise ValueError('review hook no longer matches production: '+old)
    return text.replace(old, new)


def prepare(out):
    """Only generated source copies gain hooks; relative imports retain owners."""
    hashes = {}
    def read(name):
        path=HERE/name;raw=path.read_bytes();hashes[str(path.relative_to(ROOT))]=hashlib.sha256(raw).hexdigest()
        return raw.decode()
    def includes(text, origin, overrides=None):
        overrides=overrides or {}
        return re.sub(r'^#include "([^"]+)"',lambda m:'#include "'+str(overrides.get(m[1],(origin/m[1]).resolve()))+'"',text,flags=re.M)
    egg=read('egg_draw.inc')
    egg=replace_once(egg,'else actor(x,y,picture(unit(slot),career(unit(slot))));',
        'else if(review_enabled&&unit(slot)->type==3)review_actor(x,y,unit(slot));\n    else actor(x,y,picture(unit(slot),career(unit(slot))));')
    write_changed(out/'egg-preview.inc',includes(egg,HERE).encode())
    manage=read('manage.c')
    manage=replace_once(manage,'static u8 dirty;','static u8 dirty;\n#include "review_monster_art.inc"')
    manage=replace_once(manage,'d->portrait=u->type==3?RT_NULL:',
        'd->portrait=u->type==3?(review_enabled?review_record(u->job)+312:RT_NULL):')
    manage=replace_once(manage,'actor(140,15,picture(u,career(u)));',
        'if(review_enabled&&u->type==3)review_actor(132,15,u);else actor(140,15,picture(u,career(u)));')
    manage=replace_once(manage,'if(u->type==3)return (const char *)(cj_section(common,CJ_JOB_NAMES)+14U*u->job);',
        'if(u->type==3){if(review_enabled)return review_name(u->name);return (const char *)(cj_section(common,CJ_JOB_NAMES)+14U*u->job);}')
    manage=replace_once(manage,'return u->type==3?unit_name(u):str',
        'return u->type==3?(const char *)(cj_section(common,CJ_JOB_NAMES)+14U*u->job):str')
    manage=replace_once(manage,'for (i = 0; i < 8; i++) {',
        'for (i = 0; i < (review_enabled&&u->type==3?7:8); i++) {')
    manage=replace_once(manage,'black(85, 80, "Total JP"); right(151, 80, tjp(u, c));',
        'if(!review_enabled||u->type!=3){black(85, 80, "Total JP"); right(151, 80, tjp(u, c));}')
    write_changed(out/'manage-preview.c',includes(manage,HERE,{'egg_draw.inc':out/'egg-preview.inc'}).encode())
    deploy=read('deploy.c')
    deploy=replace_once(deploy,'void game_scenario(u16 n)','void source_scenario(u16 n)')
    deploy=replace_once(deploy,'const SaveUnit *u=unit(dp.placed[cell]);actor(92+((s16)r-c)*9,63-diagonal*4,picture(u,career(u)));',
        'const SaveUnit *u=unit(dp.placed[cell]);\n            if(review_enabled&&u->type==3)review_actor(92+((s16)r-c)*9,63-diagonal*4,u);\n            else actor(92+((s16)r-c)*9,63-diagonal*4,picture(u,career(u)));')
    write_changed(out/'deploy-preview.c',includes(deploy,HERE,{'manage.c':out/'manage-preview.c'}).encode())
    return hashes


def collage(out):
    rows=[('before','formation','Formation: current / proposed'),
          ('status-before','status','Behemoth Status: current / proposed'),
          ('deploy-before','deploy','Placement preview: current / proposed (rules still closed)')]
    image=Image.new('RGB',(992,328*len(rows)),'white');draw=ImageDraw.Draw(image)
    for i,(before,after,label) in enumerate(rows):
        y=i*328;draw.text((12,y+7),label,fill='black')
        for col,name in enumerate((before,after)):
            image.paste(Image.open(out/(name+'.png')).resize((480,300),Image.Resampling.NEAREST),(8+col*496,y+24))
    save_image(image,out/'review.png')


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--sanitize',action='store_true');args=ap.parse_args()
    out=HERE/'x/monster-formation-review';out.mkdir(parents=True,exist_ok=True)
    info=art.build(core_audit.DEFAULT_DISC,out);info['production_source_sha256']=prepare(out)
    flags=['-fsanitize=address,undefined','-fno-sanitize=shift','-fno-pie','-no-pie']if args.sanitize else []
    snapshots={}
    with tempfile.TemporaryDirectory(prefix='fft-monster-picture-') as name:
        data=Path(name);data_links(data,HERE/'x/eggs/shipping');(data/'ffmr.bin').symlink_to(out/'ffmr.bin')
        (data/'ffsave.sav').write_bytes(fixture());initial=(data/'ffsave.sav').read_bytes()
        for mode,defines,cases in [
            ('manage',[],[('before',0),('formation',1),('status',12),('status-chocobo',10),('status-hydra',13),
                          ('selected-chocobo',20),('selected-black-chocobo',21),('selected-morbol',24)]),
            ('deploy',['-DMONSTER_DEPLOY_REVIEW'],[('deploy-before',0),('deploy',1)])]:
            binary=out/(mode+('-san'if args.sanitize else '-pc'))
            (out/(binary.name+'-build.log')).write_text(run(['cc','-std=gnu99','-O1','-g','-w',*flags,*defines,
                '-DREVIEW_SOURCE="'+str(out/(mode+'-preview.c'))+'"','-o',binary,HERE/'review_monster_formation.c',
                ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']))
            for label,scenario in cases:
                log=run(['env','ASAN_OPTIONS=detect_leaks=0','UBSAN_OPTIONS=halt_on_error=1',binary,scenario,data/(label+'.png')],data)
                snapshots[label]=log.strip();print(label+': '+log.strip())
            if mode=='manage':
                # Same selected unit/page, with private art hooks disabled.
                log=run(['env','ASAN_OPTIONS=detect_leaks=0','UBSAN_OPTIONS=halt_on_error=1',binary,112,data/'status-before.png'],data)
                snapshots['status-before']=log.strip()
        assert (data/'ffsave.sav').read_bytes()==initial
        publish_images(data,out)
    collage(out)
    info.update(snapshots=snapshots,sanitizers=args.sanitize,no_ti_run=True,save_preserved=True)
    (out/('san-results.json'if args.sanitize else 'results.json')).write_text(json.dumps(info,indent=2)+'\n')
    print(out/'review.png')


if __name__=='__main__':main()
