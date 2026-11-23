#!/usr/bin/env python3
"""Bounded Bar/map/save PC acceptance, plus four selected native pointer/render doors."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import tempfile

from PIL import Image, ImageChops
import review_town_services as review
import bar_assets
import chapter_packages
from check_flow import data_links

HERE,ROOT=review.HERE,review.ROOT


def save_regressions(out):
    common=[ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'];results={}
    for sanitize in (False,True):
        flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'] if sanitize else []
        label='sanitizers' if sanitize else 'pc'
        with tempfile.TemporaryDirectory(prefix='fft-bar-save-') as temp:
            folder=Path(temp);data=folder/'data';data.mkdir();data_links(data)
            fixture=folder/'x/check';fixture.mkdir(parents=True)
            (fixture/'ffsave_won.sav').write_bytes((HERE/'x/check/ffsave_won.sav').read_bytes())
            for name in ('title','manage','deploy','save'):
                exe=out/('regression-'+name+'-'+label)
                review.run(['cc','-std=gnu99','-O1','-g',*flags,'-o',exe,HERE/('test_'+name+'.c'),*common])
                log=review.run([exe],folder if name=='title' else data,os.environ.copy())
                (out/('regression-'+name+'-'+label+'.log')).write_text(log)
                results[name+'-'+label]=log;print(label,name,'passed',flush=True)
    (out/'save-regressions.json').write_text(json.dumps(results,indent=2)+'\n')


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',action='store_true')
    ap.add_argument('--save-regressions',action='store_true',help='only PC/sanitizer title/management/deployment/battle-save closure')
    args=ap.parse_args()
    out=HERE/'x/bar-service';out.mkdir(parents=True,exist_ok=True)
    if args.save_regressions:save_regressions(out);return
    protected=set((HERE/'data').glob('*.bin'))|set((HERE/'data').glob('*.89y'))|set((HERE/'data').glob('*.sav'))
    protected|={HERE/'x/check/ffsave_won.sav'}
    for folder in ('town-services-review','town-portrait-contrast','town-services-accepted','bar-reader-a'):
        protected|={p for p in (HERE/'x'/folder).rglob('*') if p.is_file() and not p.is_symlink()}
    before={str(p.relative_to(ROOT)):review.fingerprint(p) for p in sorted(protected)}
    cue=ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    art,rumors,source=bar_assets.build(str(cue));shipping=out/'shipping';shipping.mkdir(exist_ok=True)
    for name,raw in (('ffbr',art),('ffru',rumors)):
        (shipping/(name+'.bin')).write_bytes(raw)
        review.run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',name+'.bin',name],shipping)
        variable=shipping/(name+'.89y');data=bytearray(variable.read_bytes());data[0x49]=3;variable.write_bytes(data)
    accepted=HERE/'x/town-services-accepted'
    assert art[12:360]==(accepted/'bar-face.bin').read_bytes()
    assert art[360:]==(accepted/'bar-planes.bin').read_bytes()
    # Independent historical PS1 map snapshot, progress1; packed variable words.
    ram_path=ROOT/'sources/fft_ps1/world/ram0.bin';ram=ram_path.read_bytes()
    variables=struct.unpack_from('<I',ram,0x153280)[0]&0x1fffff
    assert struct.unpack_from('<I',ram,variables+4*110)[0]==1
    enabled=[]
    for identity in range(64):
        var=0x2d1+identity;word=struct.unpack_from('<I',ram,variables+4*(0x80+((var-0x80)>>5)))[0]
        if word>>((var-0x80)&31)&1:enabled.append(identity)
    assert enabled==[0,1,2]
    title_at=struct.unpack_from('<I',ram,0x1cd8a4+25*4)[0]&0x1fffff
    assert bar_assets.md.names(ram,title_at,3)==source['titles']
    source['snapshot']=dict(path=str(ram_path.relative_to(ROOT)),sha256=hashlib.sha256(ram).hexdigest(),
                            story_progress=1,enabled_rumors=enabled,kind_to_location='source world index, unchanged0..42')
    (out/'source.json').write_text(json.dumps(source,indent=2)+'\n')
    common=[ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
    pictures=None;logs={}
    for sanitize in (False,True):
        flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'] if sanitize else []
        label='sanitizers' if sanitize else 'pc';images=out/label;images.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='fft-bar-service-') as temp:
            folder=Path(temp);data_links(folder)
            for name in ('ffbr','ffru'):
                link=folder/(name+'.bin')
                if link.exists():link.unlink()
                link.symlink_to(shipping/(name+'.bin'))
            # Historical saves are inputs; all writes happen in this disposable directory.
            initial=(HERE/'x/check/ffsave_won.sav').read_bytes();(folder/'ffsave.sav').write_bytes(initial)
            for name in ('bar_service','world_save','save_progress'):
                exe=out/(name+'-'+label)
                units=[*common] if name=='bar_service' else []
                review.run(['cc','-std=gnu99','-O1','-g',*flags,'-o',exe,HERE/('test_'+name+'.c'),*units])
                log=review.run([exe,*([images] if name=='bar_service' else [])],folder,os.environ.copy())
                logs[name+'-'+label]=log
            # Existing Shop/Office/travel regression, with its fixture at the expected private path.
            private_data=folder/'data';private_data.mkdir();data_links(private_data)
            fixture_dir=folder/'x/check';fixture_dir.mkdir(parents=True)
            (fixture_dir/'ffsave_won.sav').write_bytes(initial)
            exe=out/('map-'+label)
            review.run(['cc','-std=gnu99','-O1','-g',*flags,'-o',exe,HERE/'test_worldmap.c',*common])
            logs['map-'+label]=review.run([exe],private_data,os.environ.copy())
        hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in images.glob('*.png') if p.name!='bar-service.png'}
        if sanitize:assert hashes==pictures
        else:pictures=hashes
    assert (out/'pc/home.png').read_bytes()==(HERE/'x/bar-reader-a/selected-home.png').read_bytes()
    a=Image.open(out/'pc/blink-0.png').convert('RGB');b=Image.open(out/'pc/blink-16.png').convert('RGB')
    assert ImageChops.difference(a,b).getbbox()==(151,91,154,93)
    assert (out/'pc/blink-0.png').read_bytes()==(out/'pc/blink-32.png').read_bytes()
    for name,log in logs.items():(out/(name+'.log')).write_text(log)
    # The factored reader reproduces the previous accepted A with its historical title.
    visual=out/'reference';visual.mkdir(exist_ok=True)
    for name in ('ffsk.bin','bar-face.bin','bar-planes.bin','rumor.txt'):
        (visual/name).write_bytes((accepted/name).read_bytes())
    (visual/'long-rumor.txt').write_bytes((HERE/'x/bar-reader-a/long-rumor.txt').read_bytes())
    exe=out/'preview';review.run(['cc','-std=gnu99','-O1','-o',exe,HERE/'preview_town_services.c',*common])
    review.run([exe,visual,HERE/'x/check/ffsave_won.sav','--bar-a'],accepted/'data')
    assert (visual/'selected-short.png').read_bytes()==(HERE/'x/bar-reader-a/selected-short.png').read_bytes()
    # Reuse audit sees one core art owner and one chapter text owner.
    V=chapter_packages.Variable
    owners={n:V(n,0x1c,3,raw+b'\0dat\0\xf8') for n,raw in (('ffbr',art),('ffru',rumors))}
    alias=V('ffrt',0x1c,3,b'FRF1'+b'ffru\0\0\0\0'+struct.pack('>2H',0,len(rumors))+b'\0dat\0\xf8')
    owners[alias.name]=alias
    assert chapter_packages.resources.resolve(owners,'ffrt')==rumors
    audit=chapter_packages.resources.audit(list(owners.values()),owners)
    (out/'resources.json').write_text(json.dumps(audit,indent=2)+'\n')
    try:chapter_packages.resources.audit([*owners.values(),V('ffrx',0x1c,3,owners['ffru'].payload)],owners)
    except ValueError:pass
    else:raise AssertionError('duplicated rumor bank accepted')
    runtime_hashes={name:hashlib.sha256((HERE/name).read_bytes()).hexdigest() for name in
                    ('worldmap.c','bar_bank.h','save_types.h','save.h','save_world.h','../bar_reader.inc',
                     '../dialogue_box.inc','../choice_menu.inc')}
    native=[];native_proof=out/'native-results.json'
    if not args.native and native_proof.exists():
        previous=json.loads(native_proof.read_text())
        if previous['source']==source and previous['runtime_sha256']==runtime_hashes:native=previous['cases']
    if args.native:
        for name in ('world_save','save_progress'):
            exe=out/('native-'+name)
            review.run([ROOT/'tools/bin/ti-cc','-o',exe,HERE/('test_'+name+'.c')])
            log=review.run([ROOT/'tools/bin/ti-cycles','--max',100,exe.with_suffix('.89z')])
            values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
            pc=logs[name+'-pc'];match=re.search(r'([0-9A-F]{8}) failures (\d+) checks (\d+)',pc)
            assert match and values[-3:]==[int(match[1],16),0,int(match[3])],(name,log,pc)
            (out/('native-'+name+'.log')).write_text(log)
        exe=out/'native-bar'
        review.run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',exe,HERE/'check_bar_render.c',
                    ROOT/'runtime/core/rt_core.c',ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        pc=out/'native-bar-pc';review.run(['cc','-std=gnu99','-O1','-o',pc,HERE/'check_bar_render.c',*common])
        with tempfile.TemporaryDirectory(prefix='fft-bar-native-') as temp:
            folder=Path(temp);data_links(folder)
            for name in ('ffbr','ffru'):
                link=folder/(name+'.bin')
                if link.exists():link.unlink()
                link.symlink_to(shipping/(name+'.bin'))
            (folder/'ffsave.sav').write_bytes((HERE/'x/check/ffsave_won.sav').read_bytes())
            review.run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','ffsave.sav','ffsave'],folder)
            for door in range(4):
                normal=review.run([pc,door,out/('native-pc-%d.png'%door),2],folder)
                command=[ROOT/'tools/bin/ti-cycles','--frames',2,'--arg',door,'--max',150]
                for name in ('ffwm','ffmg','ffwb','ffsh','ffsk','ffid','ffcp'):
                    command+=['--file',HERE/'data'/(name+'.89y')]
                for name in ('ffbr','ffru'):command+=['--file',shipping/(name+'.89y')]
                log=review.run([*command,'--file',folder/'ffsave.89y','--png',out/('native-ti-%d.png'%door),exe.with_suffix('.89z')])
                match=re.search(r'bar-render (\w+) state(\w+)',normal)
                assert match and re.search(r'checksum (\w+)',log)[1].lower()==match[1].lower(),log
                values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
                assert values[-2:]==[int(match[2],16),4024],log
                a=Image.open(out/('native-pc-%d.png'%door)).convert('L').point(lambda v:(216-v)//64)
                b=Image.open(out/('native-ti-%d.png'%door)).convert('L').crop((0,0,160,100)).point(lambda v:(255-v)//85)
                assert a.tobytes()==b.tobytes()
                (out/('native-%d.log'%door)).write_text(log);native.append(dict(door=door,checksum=match[1],state=match[2]))
        native_proof.write_text(json.dumps(dict(cases=native,source=source,runtime_sha256=runtime_hashes),indent=2)+'\n')
    review.sheet(out/'pc','bar-service.png',2,[
        [('home','Playable Bar - approved room / portrait D'),('list','Original titles - Gariland / progress1')],
        [('long-first','The Fifty Year War - first page'),('long-last','Final page - same campaign cue')]])
    assert before=={str(p.relative_to(ROOT)):review.fingerprint(p) for p in sorted(protected)}
    result=dict(source=source,pc_sanitizers_equal=True,screens=pictures,protected_files=len(before),
                native=native,native_run=args.native,save_format='FSV8',save_bytes=4024,save_growth=32,old_save_migration='FSV2..7',
                art_exact=True,reader_reference_exact=True,installed_groups_changed=False,
                scope='Playable post-Gariland Bar; later rumor/quest profiles, Propositions and Fur stock remain.')
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    (out/'preserved.json').write_text(json.dumps(before,indent=2)+'\n')
    print(logs['bar_service-pc'].strip());print(logs['world_save-pc'].strip());print(logs['save_progress-pc'].strip())
    print('%d PC/sanitizer screens; %d existing files unchanged; %d selected native render doors.'%(len(pictures),len(before),len(native)))


if __name__=='__main__':main()
