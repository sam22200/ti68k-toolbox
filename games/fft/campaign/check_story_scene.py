#!/usr/bin/env python3
"""Real modal choice/skip/atomic-file/Continue flow, PC first; opt-in native."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from PIL import Image
from check_flow import HERE, ROOT, data_links, run
from check_story_choices import specs
import story_choice
import scenes
import chapter_packages as packages


def approved_hash(path):
    im=Image.open(path).convert('L');levels={216:0,152:1,88:2,24:3};h=17
    for plane in range(2):
        for y in range(100):
            for x in range(0,160,8):
                b=sum(((levels[im.getpixel((x+k,y))]>>plane)&1)<<(7-k) for k in range(8))
                h=((h<<1|h>>31)^b)&0xffffffff
    return h


def fixture(source):
    picture=packages.resources.scene_parts(source,'ffs1',True)[1][-1][2][0]
    question=scenes.beat_block([bytes((7,0,0,0))+b'Ramza\0Should Delita join the party?\0'])
    record=story_choice.build(specs()['recruit_choice'])
    card=lambda text:bytes((1,10,45,0))+text+b'\0' # portable C_WHITE on the black card
    steps=[(7,1,[3,7,1]),(2,0,[picture,20,question]),(6,7,[record,3,5]),
           (3,0,[card(b'Not recruited.')]),(7,1,[41,7,7]),
           (3,0,[card(b'Delita joined.')]),(7,1,[42,7,7])]
    raw=scenes.scene_set('',steps);bad=bytearray(raw);struct.pack_into('>H',bad,16,0)
    other=list(steps);other[3]=(3,0,[card(b'Another refusal.')])
    template=source['ffs1'];prepared=dict(source)
    for name,content in [('ffs3',raw),('ffs5',scenes.scene_set('',other))]:
        prepared[name]=packages.Variable(name,0x1c,3,content+packages.resources.TRAILER,template.folder)
    pools=packages.resources.pool_scenes(prepared,['ffs3','ffs5'])
    assert pools and prepared['ffs3'].raw[:4]==b'FSC2'
    result={name:prepared[name].raw for name in {'ffs3','ffs5'}|pools};result['ffs4']=bytes(bad)
    return result


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',action='store_true')
    ap.add_argument('--data-dir',type=Path,default=HERE/'x/guests/shipping')
    args=ap.parse_args();source_dir=args.data_dir.resolve();out=HERE/'x/story-choices/scene';out.mkdir(exist_ok=True)
    source={}
    for group in ('fft-core.89g','chapter-01.89g'):source.update(packages.read_group(HERE/'x/guests/release'/group))
    banks=fixture(source)
    hashes=[approved_hash(HERE/'x/story-choices/review'/f'{name}.png')for name in ('choice','full')]
    (HERE/'x/story-choices/scene_fixture.h').write_text('#define APPROVED_CHOICE_PLANES 0x%08XUL\n#define APPROVED_FULL_PLANES 0x%08XUL\n'%tuple(hashes))
    common=[ROOT/'runtime/core/rt_core.c'];logs=[];snapshots=[];portable=None
    for mode,flags in [('pc',[]),('san',['-fsanitize=address,undefined','-fno-sanitize=shift','-fno-pie','-no-pie'])]:
        exe=out/mode
        (out/f'{mode}-build.log').write_text(run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',exe,HERE/'check_story_scene.c',*common,ROOT/'runtime/platform-sw/rt_sw.c']))
        with tempfile.TemporaryDirectory(prefix='fft-story-scene-')as name:
            data=Path(name);data_links(data,source_dir)
            for var,content in banks.items():
                path=data/(var+'.bin');path.unlink(missing_ok=True);path.write_bytes(content)
            log=run(['env','ASAN_OPTIONS=detect_leaks=0','UBSAN_OPTIONS=halt_on_error=1',exe],data)
            (out/f'{mode}.log').write_text(log);logs.append(log)
            initial=(data/'ffsave.sav').read_bytes()
            # Reuse the source-unchanged real title child from the guest delivery.
            title=HERE/f'x/guests/chains/title-{mode}'
            for action,expected in [('save',3),('continue',2)]:
                if action=='continue':(data/'ffsave.sav').write_bytes(bytes(len(initial)))
                p=subprocess.run(['env','ASAN_OPTIONS=detect_leaks=0',str(title),action],cwd=data,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
                (out/f'title-{action}-{mode}.log').write_text(p.stdout);assert p.returncode==expected,p.stdout
                assert(data/'ffsave.sav').read_bytes()==initial
            resumed=run(['env','ASAN_OPTIONS=detect_leaks=0','UBSAN_OPTIONS=halt_on_error=1',exe,'resume'],data)
            (out/f'resume-{mode}.log').write_text(resumed);assert(data/'ffsave.sav').read_bytes()==initial
            snapshots.append(hashlib.sha256(initial).hexdigest())
            if mode=='pc':portable=run([exe,'portable'],data);(out/'portable.log').write_text(portable)
    assert logs[0]==logs[1]and snapshots[0]==snapshots[1]
    report=dict(pc_sanitizers_equal=True,session_sha256=snapshots[0],result=logs[0].strip(),
                approved_plane_hashes=[f'{h:08X}'for h in hashes],native=False)
    if args.native:
        (out/'native-build.log').write_text(run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'scnchck',HERE/'check_story_scene.c',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a']))
        with tempfile.TemporaryDirectory(prefix='fft-story-scene-native-')as name:
            data=Path(name)
            for var,content in banks.items():
                (data/(var+'.bin')).write_bytes(content)
                run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',var+'.bin',var],data)
            cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',3,'--max',20000]
            for path in source_dir.glob('*.89y'):
                if path.stem not in banks:cmd+=['--file',path]
            for var in banks:cmd+=['--file',data/(var+'.89y')]
            native=run([*cmd,out/'scnchck.89z'])
            (out/'native.log').write_text(native)
        expected=re.search(r'story-scene (\w+) failures 0 checks (\d+)',portable);assert expected,portable
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(expected[1],16),0,int(expected[2])],values[-12:]
        maximum=re.search(r'^max 31\s+\S+\s+(\d+)\s+at call\s+(\d+)',native,re.M);assert maximum,native[-2500:]
        assert int(maximum[1])<360000,maximum[0]
        report.update(native=True,portable=portable.strip(),peak_frame_cycles=int(maximum[1]))
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))


if __name__=='__main__':main()
