#!/usr/bin/env python3
"""Real input/save replays of deployment and the source-ordered scene player."""
from pathlib import Path
import tempfile,sys,json,argparse
from check_flow import HERE,ROOT,OUT,run,data_links,fixture
import check
sys.path.insert(0,str(HERE.parent/'tools'))
import save_dump

def main():
    global OUT
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,default=OUT)
    ap.add_argument('--deployment-only',action='store_true');args=ap.parse_args();OUT=args.out.resolve()
    OUT.mkdir(parents=True,exist_ok=True)
    target=OUT/'menus';target.mkdir(exist_ok=True);check.OUT=target
    inputs=target/'input';inputs.mkdir(exist_ok=True)
    empty=target/'none.keys';empty.write_text('0\n')
    events=[];frame=2
    for key in ('F5','F5','LEFT','ENTER','F5','UP','RIGHT','RIGHT','RIGHT','RIGHT','ENTER',
                'F5','LEFT','ENTER','F5','F5','LEFT','ENTER'):
        events.append((frame,key));frame+=2
    def keys(name,tail):return check.keys(target/name,events+tail)
    picked=keys('picked.keys',[]);confirm=keys('confirm.keys',[(frame,'C')])
    commit=keys('commit.keys',[(frame,'C'),(frame+2,'ENTER')])
    return_selected=keys('return-selected.keys',[(frame,'C'),(frame+2,'DOWN')])
    return_back=keys('return.keys',[(frame,'C'),(frame+2,'DOWN'),(frame+4,'ENTER')])
    escape=keys('cancel.keys',[(frame,'C'),(frame+2,'ESC')])
    invalid=check.keys(target/'invalid.keys',[(2,'RIGHT'),(4,'ENTER')])
    initial=bytearray(fixture());initial[3114:3138]=bytes(24);initial[3114]=1
    prep=inputs/'before.sav';prep.write_bytes(initial)
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','before.sav','ffsave'],inputs)
    ti=inputs/'ffsave.89y';results=[]
    with tempfile.TemporaryDirectory(prefix='fft-flow-menu-data-') as name:
        # Campaign banks only: the opening's historical view controls (ffov*) are
        # not in this package, and letting the PC fall back to them diverges from
        # the 68000 run, which is given this chapter's variables alone.
        data=Path(name);data_links(data,HERE/'data')
        def committed(s):
            assert len(s['party'])==7 and s['money']==2000 and s['flow']==2 and len(s['deployment'])==5,s
            assert [p['slot'] for p in s['deployment']]==[0,2,3,4,6]
        cases=[('initial',200,4,empty),('invalid',200,8,invalid),('picked',200,frame,picked),
             ('confirm',200,frame+2,confirm),('return-selected',200,frame+4,return_selected),
             ('return',200,frame+6,return_back),('cancel',200,frame+4,escape)]
        cases += [(f'glove-pose-{i}',200,frame+2+4*i,confirm) for i in range(1,8)]
        cases += [('commit',200,frame+6,commit)]
        banks=['ffmg','ffcp','ffcj','ffflow','ffeg','ffma']
        results+=check.program('flowdep','check_deploy.c',banks,0x44504c31,
                               cases,prep,ti,committed,workdir=data)
        # The same roster and the same keys, with slot 6 an adult Chocobo: the
        # real deployment program must list it, place it and commit it like any
        # other fighter. Only the roster byte differs from the run above.
        monster=bytearray(initial);at=294+176*6
        monster[at+0]=94;monster[at+1]=255;monster[at+2]=3    # Chocobo, no human sheet, source monster
        monster[at+9]=0;monster[at+25:at+30]=bytes(5)         # no secondary skill set, nothing equipped
        monster[at+30:at+36]=bytes(6);monster[at+116:at+176]=bytes(60)
        mon_inputs=target/'monster';mon_inputs.mkdir(exist_ok=True)
        mon_prep=mon_inputs/'ffsave.sav';mon_prep.write_bytes(monster)
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','ffsave.sav','ffsave'],mon_inputs)
        def committed_monster(s):
            committed(s)
            unit=s['party'][6]
            assert unit['job']==94 and unit['type']==3 and unit['gfx']==255,unit
        results+=check.program('flowmon','check_deploy.c',banks,0x44504c31,
                               [('monster-roster',200,4,empty),('monster-picked',200,frame,picked),
                                ('monster-commit',200,frame+6,commit)],
                               mon_prep,mon_inputs/'ffsave.89y',committed_monster,workdir=data)
        if args.deployment_only:
            (target/'results.json').write_text(json.dumps(results,indent=2)+'\n')
            print('Deployment UI:',len(results),'PC/68000 input, pixel, state and save replays passed')
            return
        initial[3114]=0;prep.write_bytes(initial)
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','before.sav','ffsave'],inputs)
        advance=check.keys(target/'advance.keys',[(f,'ENTER') for f in range(30,1601,15)])
        def academy(s):assert s['flow']==1 and len(s['party'])==7 and s['money']==2000,s
        # The scene player wants its contour art in ffcu, as a package stores it.
        scene_banks=check.shared_contour_banks(target/'banks')
        banks=['ffs1','ffs2','ffflow','ffcp','ffcu','ffca','ffcb']
        results+=check.program('flowcin','check_flow_scene.c',banks,0xacad0001,
            [('narration',1,30,empty),('chapter',1,330,advance),('complete',1,1600,advance)],prep,ti,academy,
            workdir=scene_banks,bank_dir=scene_banks)
        prep.write_bytes(fixture());run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','before.sav','ffsave'],inputs)
        def title(s):assert s['flow']==3 and len(s['deployment'])==5,s
        results+=check.program('flowtit','check_flow_scene.c',banks,0xacad0001,
            [('title',2,40,empty),('complete',2,180,empty)],prep,ti,title,workdir=scene_banks,bank_dir=scene_banks)
    (target/'results.json').write_text(json.dumps(results,indent=2)+'\n')
    print('Flow UI:',len(results),'PC/68000 input, pixel, state and save replays passed')

if __name__=='__main__':main()
