#!/usr/bin/env python3
"""Source Chapter I rumor transitions and actual cached reader, primarily on PC."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import tempfile

from PIL import Image
import review_town_services as review
from check_flow import data_links
import bar_assets
import chapter_packages
import rumor_rules

HERE,ROOT=review.HERE,review.ROOT


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',action='store_true')
    ap.add_argument('--out',type=Path,help='keep a new follow-up proof separate from the previous one');args=ap.parse_args()
    out=(args.out or HERE/'x/bar-rumor-rules').resolve();out.mkdir(parents=True,exist_ok=True)
    protected={p for p in (HERE/'data').iterdir() if p.is_file() and p.suffix in ('.bin','.89y','.sav')}
    protected|={HERE/'x/check/ffsave_won.sav'}
    for folder in ('town-services-review','town-portrait-contrast','town-services-accepted','bar-reader-a','bar-service'):
        protected|={p for p in (HERE/'x'/folder).rglob('*') if p.is_file() and not p.is_symlink()}
    before={str(p.relative_to(ROOT)):review.fingerprint(p) for p in sorted(protected)}
    cue=ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    art,pilot,pilot_info=bar_assets.build(str(cue))
    assert art==(HERE/'data/ffbr.bin').read_bytes()
    assert pilot==(HERE/'data/ffru.bin').read_bytes() # no new content silently enabled in the pilot
    rows=[dict(event=e,flag=f) for e,f in zip(rumor_rules.EVENTS,(2,33,34,35))]
    rumor_rules.bindings(rows,complete=True)
    same,full,info=bar_assets.build(str(cue),rows);assert art==same
    (out/'source.json').write_text(json.dumps(info,indent=2)+'\n')
    shipping=out/'shipping';shipping.mkdir(exist_ok=True)
    (shipping/'ffru.bin').write_bytes(full)
    review.run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffru.bin','ffru'],shipping)
    file=shipping/'ffru.89y';b=bytearray(file.read_bytes());b[0x49]=3;file.write_bytes(b)
    # Shared shipping validation rejects a full bank with pilot-only declarations.
    bar_assets.validate(full);rumor_rules.validate_bank(full,rows)
    try:rumor_rules.validate_bank(full,pilot_info['event_bindings'])
    except ValueError:pass
    else:raise AssertionError('undeclared future gates accepted')
    common=[ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
    all_hashes=None;logs={}
    for sanitize in (False,True):
        label='sanitizers' if sanitize else 'pc';images=out/label;images.mkdir(exist_ok=True)
        flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'] if sanitize else []
        with tempfile.TemporaryDirectory(prefix='fft-rumor-rules-') as temp:
            data=Path(temp);data_links(data);(data/'ffru.bin').unlink();(data/'ffru.bin').symlink_to(shipping/'ffru.bin')
            initial=(HERE/'x/check/ffsave_won.sav').read_bytes();(data/'ffsave.sav').write_bytes(initial)
            exe=out/('rules-'+label)
            review.run(['cc','-std=gnu99','-O1','-g',*flags,'-o',exe,HERE/'test_rumor_rules.c',*common])
            logs[label]=review.run([exe,images],data,os.environ.copy())
            # Same runtime with the unchanged shipping pilot; compare its accepted pictures.
            (data/'ffru.bin').unlink();(data/'ffru.bin').symlink_to(HERE/'data/ffru.bin')
            (data/'ffsave.sav').write_bytes(initial);exe=out/('pilot-'+label)
            review.run(['cc','-std=gnu99','-O1','-g',*flags,'-o',exe,HERE/'test_bar_service.c',*common])
            pilot_images=out/('pilot-images-'+label);pilot_images.mkdir(exist_ok=True)
            logs['pilot-'+label]=review.run([exe,pilot_images],data,os.environ.copy())
            for p in pilot_images.glob('*.png'):
                assert p.read_bytes()==(HERE/'x/bar-service/pc'/p.name).read_bytes(),p.name
        hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in images.glob('*.png') if p.name!='rumor-transitions.png'}
        if sanitize:assert hashes==all_hashes
        else:all_hashes=hashes
    for name,log in logs.items():(out/(name+'.log')).write_text(log)
    # Two distinct consumers can borrow this one physical chapter bank.
    V=chapter_packages.Variable;trailer=chapter_packages.resources.TRAILER
    source={'ffru':V('ffru',0x1c,3,full+trailer),
            'ffrt':V('ffrt',0x1c,3,b'FRF1'+b'ffru\0\0\0\0'+struct.pack('>2H',0,len(full))+trailer)}
    assert chapter_packages.resources.resolve(source,'ffrt')==full
    audit=chapter_packages.resources.audit(list(source.values()),source)
    (out/'resources.json').write_text(json.dumps(audit,indent=2)+'\n')
    runtime_hashes={name:hashlib.sha256((HERE/name).read_bytes()).hexdigest() for name in
                    ('worldmap.c','bar_bank.h','save_types.h','../bar_reader.inc','../dialogue_box.inc',
                     '../choice_menu.inc','check_bar_render.c')}
    native=None;native_path=out/'native-results.json';bank_hash=hashlib.sha256(full).hexdigest()
    if not args.native and native_path.exists():
        previous=json.loads(native_path.read_text())
        if previous['runtime_sha256']==runtime_hashes and previous['bank_sha256']==bank_hash:
            native=previous['case']
    if args.native:
        exe=out/'rumorc'
        log=review.run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',exe,HERE/'check_bar_render.c',
                        ROOT/'runtime/core/rt_core.c',ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        (out/'native-build.log').write_text(log)
        pc=out/'native-pc';review.run(['cc','-std=gnu99','-O1','-o',pc,HERE/'check_bar_render.c',*common])
        with tempfile.TemporaryDirectory(prefix='fft-rumor-native-') as temp:
            data=Path(temp);data_links(data);(data/'ffru.bin').unlink();(data/'ffru.bin').symlink_to(shipping/'ffru.bin')
            (data/'ffsave.sav').write_bytes((HERE/'x/check/ffsave_won.sav').read_bytes())
            review.run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','ffsave.sav','ffsave'],data)
            normal=review.run([pc,4,out/'native-pc.png',2],data)
            command=[ROOT/'tools/bin/ti-cycles','--frames',2,'--arg',4,'--max',100]
            for name in ('ffwm','ffmg','ffwb','ffsh','ffsk','ffid','ffcp','ffbr'):
                command+=['--file',HERE/'data'/(name+'.89y')]
            log=review.run([*command,'--file',shipping/'ffru.89y','--file',data/'ffsave.89y',
                            '--png',out/'native-ti.png',exe.with_suffix('.89z')])
            m=re.search(r'bar-render (\w+) state(\w+)',normal)
            assert m and re.search(r'checksum (\w+)',log)[1].lower()==m[1].lower(),log
            values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
            assert values[-2:]==[int(m[2],16),4024]
            a=Image.open(out/'native-pc.png').convert('L').point(lambda v:(216-v)//64)
            b=Image.open(out/'native-ti.png').convert('L').crop((0,0,160,100)).point(lambda v:(255-v)//85)
            assert a.tobytes()==b.tobytes()
            (out/'native.log').write_text(log);native=dict(checksum=m[1],state=m[2],doors=1,frames=2)
        native_path.write_text(json.dumps(dict(case=native,runtime_sha256=runtime_hashes,
                                              bank_sha256=bank_hash),indent=2)+'\n')
    review.sheet(out/'pc','rumor-transitions.png',2,[
        [('early-list','Progress1 - original first three'),('middle-list','Progress8 - King report replaced')],
        [('late-top','Progress10 - new Death Corps reports'),('late-bottom','Same3-row menu - scroll to the fourth')],
        [('dycedarg','Full original33-character title'),('boundary-empty','Progress15 - Chapter I entries retired')]])
    assert before=={str(p.relative_to(ROOT)):review.fingerprint(p) for p in sorted(protected)}
    result=dict(source=info,shipping_pilot_unchanged=True,art_unchanged=True,save_bytes=4024,
                pc_sanitizers_equal=True,screens=all_hashes,native=native,native_run=args.native,protected_files=len(before),
                scope='Chapter I rumor source intervals and native input; later source events use explicit fixture bindings, no new campaign scenes.')
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    (out/'preserved.json').write_text(json.dumps(before,indent=2)+'\n')
    print(logs['pc'].strip());print('Pilot pictures exact; %d PC/sanitizer screens, %d protected files; native doors%d.'%
                                    (len(all_hashes),len(before),1 if native else 0))


if __name__=='__main__':main()
