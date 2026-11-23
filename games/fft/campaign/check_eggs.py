#!/usr/bin/env python3
"""PC/sanitizer calendar, actual child Save/Continue and reviewed egg integration."""
import argparse
import hashlib
import json
import re
import subprocess
import tempfile
from pathlib import Path
from check_flow import HERE,ROOT,data_links,fixture,run
from review_files import publish_images
import breeding_rules
import egg_art
import save_dump


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native-save',action='store_true')
    args=ap.parse_args();out=HERE/'x/eggs';out.mkdir(parents=True,exist_ok=True)
    cue=ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    rules,_=breeding_rules.build(cue);art,_=egg_art.build(cue);art=egg_art.pool(art)
    (out/'ffwb.bin').write_bytes(rules);(out/'ffeg.bin').write_bytes(art)
    initial=bytearray(fixture()[:3918])   # the shared prefix; this check writes its own world tail
    initial[4]|=2;initial[36]=8;initial[37]=7
    parent=bytearray(initial[470:646]);parent[0:5]=bytes((95,255,3,250,42));initial[294+7*176:294+8*176]=parent
    # FSV7 tail is explicit fixture input; real FSV2..6 migration is checked
    # separately. One deterministic world seed forces a real first-route birth.
    tail=bytes((1,1,0,0))+bytes(65)+bytes((0,0,0,1,0))
    initial=b'FSV8'+bytes(initial[4:])+tail+bytes(32);assert len(initial)==4024
    summaries=[];snapshots=[]
    for mode,flags in [('pc',[]),('san',['-fsanitize=address,undefined','-fno-sanitize=shift','-fno-pie','-no-pie'])]:
        executables={}
        for name,source,needs_runtime in [('save','test_world_save.c',False),('map','check_egg_map.c',True),
                                          ('manage','check_egg_manage.c',True),('title','check_chain_title.c',True)]:
            binary=out/f'{name}-{mode}';executables[name]=binary
            cmd=['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',binary,HERE/source]
            if needs_runtime:cmd += [ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
            (out/f'{name}-{mode}-build.log').write_text(run(cmd))
        lines=[run([executables['save']])]
        with tempfile.TemporaryDirectory(prefix='fft-eggs-') as folder:
            data=Path(folder);data_links(data,HERE/'x/guests/shipping')
            for name,raw in [('ffwb',rules),('ffeg',art)]:
                path=data/(name+'.bin');path.unlink(missing_ok=True);path.write_bytes(raw)
            (data/'ffsave.sav').write_bytes(initial)
            env=['env','ASAN_OPTIONS=detect_leaks=0','UBSAN_OPTIONS=halt_on_error=1']
            for binary,argument in [('map','first'),('manage','resume'),('map','next')]:
                lines.append(run([*env,executables[binary],argument],data))
            saved=(data/'ffsave.sav').read_bytes();decoded=save_dump.decode(saved)
            assert decoded['calendar']==dict(month=1,day=3,era=0) and decoded['party'][8]['hatch_remaining']>=0
            for action,expected in [('save',3),('continue',2)]:
                if action=='continue':(data/'ffsave.sav').write_bytes(bytes(len(saved)))
                result=subprocess.run([*map(str,env),str(executables['title']),action],cwd=data,text=True,
                                      stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
                assert result.returncode==expected,result.stdout
                assert (data/'ffsave.sav').read_bytes()==saved
            lines.append(run([*env,executables['map'],'hatch'],data))
            assert not save_dump.decode((data/'ffsave.sav').read_bytes())['party'][8]['egg']
            snapshots.append(hashlib.sha256(saved).hexdigest())
            for case in ('full','multi'):
                (data/'ffsave.sav').write_bytes(initial)
                lines.append(run([*env,executables['map'],case],data))
            # Restore the original human fixture for exact approved pictures.
            (data/'ffsave.sav').write_bytes(initial)
            lines.append(run([*env,executables['manage'],'review'],data));publish_images(data,out/'review')
            (data/'ffsave.sav').write_bytes(initial)
            lines.append(run([*env,executables['map'],'review'],data));publish_images(data,out/'review')
        summaries.append(''.join(lines));(out/f'{mode}.log').write_text(summaries[-1])
    assert summaries[0]==summaries[1] and snapshots[0]==snapshots[1]
    proof=dict(pc_sanitizers_equal=True,result=summaries[0],session_sha256=snapshots[0],
               source_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in
                [HERE/n for n in ('save_types.h','world_types.h','save_world.h','save.h','breeding.h','worldmap.c','manage.c','egg_draw.inc')]})
    if args.native_save:
        (out/'native-save-build.log').write_text(run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'worldsv',HERE/'test_world_save.c']))
        log=run([ROOT/'tools/bin/ti-cycles','--frames',0,'--max',2000,out/'worldsv.89z']);(out/'native-save.log').write_text(log)
        match=re.search(r'world-save (\w+) failures 0 checks (\d+)',summaries[0]);assert match
        values=[int(x,16) for x in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
        assert values[-3:]==[int(match[1],16),0,int(match[2])],values
        proof['native_save']=True
    (out/'results.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof,indent=2))


if __name__=='__main__':main()
