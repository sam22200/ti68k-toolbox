#!/usr/bin/env python3
"""Private13-actor board/rotation/last-actor Status planes on both CPUs."""
import json
import argparse
import re
import sys
from pathlib import Path
import tempfile
from check_flow import HERE,ROOT,data_links
from check import run
from check_core_boundaries import descriptor13


def synthetic_body_bank(out,scale=1.0):
    """The two synthetic creatures of check_monster_body.py, packed as FMB1.

    `scale` shrinks every pose (nearest neighbour, the feet with it) to measure a
    body of the size of the selected art instead of the worst-case test shapes.
    """
    import json
    from PIL import Image
    import check_monster_body as body_check
    sys.path.insert(0,str(HERE.parent/'tools'))
    import monster_body_bank as bank
    poses=out/'poses';poses.mkdir(parents=True,exist_ok=True)
    manifest=body_check.fixtures(poses)
    if scale!=1.0:
        described=json.loads(manifest.read_text())
        for family in described['families']:
            for anim in family['anims'].values():
                anim['foot']=[int(v*scale)for v in anim['foot']]
                for facing in('front','back'):
                    for name in anim[facing]:
                        image=Image.open(poses/name)
                        w,h=(max(1,int(v*scale))for v in image.size)
                        image.resize((w,h),Image.NEAREST).save(poses/name)
        manifest.write_text(json.dumps(described,indent=1))
    raw,_=bank.pack(bank.load(manifest))
    return raw


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'x/core-capacity/render')
    ap.add_argument('--module-dir',type=Path,help='private matching modules.h and four FMB3 .bin shards; active banks remain untouched')
    ap.add_argument('--arena-dir',type=Path,help='private ffg*/ffcu .bin overrides for another source map')
    ap.add_argument('--source',type=Path,default=HERE/'check_capacity_render.c',help='isolated reference carrier; default is the actual current engine')
    ap.add_argument('--measure-only',action='store_true',help='retain CPU overruns for profiling; framebuffer/state equality still required, not performance acceptance')
    ap.add_argument('--actors',type=int,choices=(11,12,13),default=13,help='keep the original eleven and add up to two identical diagnostic actors')
    ap.add_argument('--profile-zones',action='store_true',help='enable renderer component zones in the private carrier; instrumented timings are diagnostic')
    ap.add_argument('--case',action='append',choices=('board','rotations','status-last','equipment-last','monster-ko','monster-ko-morph','monster-ko-transition','monster-body','battle-menu','battle-act'),
                    help='select affected cases; default is the original four-case certification')
    ap.add_argument('--ko-bank',type=Path,help='private FKO1 bank for the monster-ko case')
    ap.add_argument('--body-bank',type=Path,help='FMB1 bank for the monster-body case; the synthetic creatures of check_monster_body.py by default (games/fft/tools/monster_body_assets.py builds a real one)')
    ap.add_argument('--body-scale',type=float,default=1.0,help='shrink the synthetic bodies (0.62 gives the 32x26 bounds of the selected art)')
    ap.add_argument('--body-chapter',action='store_true',help="use the chapter's own ffmb/ffmc as built, instead of a synthetic bank")
    ap.add_argument('--bodies',type=int,default=13,help='how many of the13 actors wear a wide body in the monster-body case')
    ap.add_argument('--body-job',type=int,default=0,metavar='J',
                    help='dress all thirteen in this FFT job (94..141), to price one family alone')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    if args.case and any(c.startswith('monster-ko')for c in args.case)and not args.ko_bank:ap.error('monster-ko needs --ko-bank')
    body=None
    if args.case and 'monster-body'in args.case:
        body=None if args.body_chapter else \
            args.body_bank.read_bytes()if args.body_bank else synthetic_body_bank(out,args.body_scale)
    common=['-DRT_DRAW_ROWS=100',args.source.resolve(),ROOT/'runtime/core/rt_core.c']
    # The whole battle plus every scenario no longer fits one 68000 variable, so each
    # group of scenarios is its own build: the certified four (CAP_STATUS), the monster
    # KO boards (CAP_KO) and the wide bodies (CAP_BODY). Only the requested one is built.
    wanted=set(args.case or ('board','rotations','status-last','equipment-last'))
    if wanted&{'battle-menu','battle-act'} and wanted-{'battle-menu','battle-act'}:
        ap.error('Static menu carriers must be selected separately from gameplay/Status cases')
    groups=[]
    if wanted&{'battle-menu','battle-act'}:groups.append('-DCAP_MENU')
    if wanted&{'status-last','equipment-last'}:groups.append('-DCAP_STATUS')
    if wanted&{'monster-ko','monster-ko-morph','monster-ko-transition'}:groups.append('-DCAP_KO')
    if 'monster-body'in wanted:groups.append('-DCAP_BODY')
    run(['cc','-std=gnu99','-O1','-w',*groups,'-o',out/'render',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    # The zone markers do not fit beside thirteen bodies and two banks; the per-frame
    # values the gate reads come from FRAME_END, not from them.
    module_flags=['-DFFT_MODULE_HEADER="'+str(args.module_dir.resolve()/'modules.h')+'"'] if args.module_dir else []
    zone_flags=[] if args.profile_zones else ['-DFFT_CHECK_NO_ZONES']
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES',*zone_flags,*module_flags,*groups,'-o',out/'renderc',*common,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    results=[]
    files={p.stem:p for folder in (HERE.parent/'opening/data',HERE/'data')for p in folder.glob('*.89y')}
    if args.module_dir:
        import chapter_packages
        for name in ('ffbm','ffbn','ffbo','ffbp'):
            var=chapter_packages.Variable(name,0x1c,3,(args.module_dir/(name+'.bin')).read_bytes()+b'\0dat\0\xf8')
            path=out/(name+'.89y');path.write_bytes(chapter_packages.group_bytes([var],single=True));files[name]=path
    with tempfile.TemporaryDirectory(prefix='fft-render13-')as name:
        data=Path(name);data_links(data)
        if args.arena_dir:
            for src in args.arena_dir.resolve().glob('*.bin'):
                target=data/src.name;target.unlink(missing_ok=True);target.write_bytes(src.read_bytes())
                run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',target.name,target.stem],data)
                files[target.stem]=target.with_suffix('.89y')
        dense=descriptor13((data/'ffgd.bin').read_bytes(),(data/'ffgi.bin').read_bytes(),args.actors)
        (data/'ffgd.bin').unlink()
        (data/'ffgd.bin').write_bytes(dense)
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffgd.bin','ffgd'],data)
        files['ffgd']=data/'ffgd.89y'
        if args.ko_bank:
            (data/'ffko.bin').write_bytes(args.ko_bank.read_bytes())
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffko.bin','ffko'],data)
            files['ffko']=data/'ffko.89y'
        if body:
            (data/'ffmb.bin').unlink(missing_ok=True)   # data_links symlinks the chapter's own
            (data/'ffmb.bin').write_bytes(body)
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffmb.bin','ffmb'],data)
            files['ffmb']=data/'ffmb.89y'
        empty=out/'none.keys';empty.write_text('0\n')
        rotations=out/'rotate.keys';rotations.write_text('12 F5\n13\n50 F1\n51\n')
        for label,scenario,frames,keys in (('board',0,90,empty),('rotations',0,100,rotations),
                                         ('status-last',1,100,empty),('equipment-last',2,100,empty),
                                         ('monster-ko',3,60,empty),('monster-ko-morph',4,60,empty),
                                         ('monster-ko-transition',5,60,empty),('monster-body',1000+args.body_job if args.body_job else 5+args.bodies,90,empty),('battle-menu',0,90,empty),('battle-act',1,90,empty)):
            if args.case and label not in args.case:continue
            if label.startswith('monster-')and not args.case:continue
            pc=run([out/'render',scenario,frames,keys,out/(label+'-pc.png')],data)
            command=[ROOT/'tools/bin/ti-cycles','--arg',scenario,'--frames',frames,'--max',3000,
                     '--keys',keys,'--png',out/(label+'-native.png')]
            for p in files.values():command+=['--file',p]
            native=run([*command,out/'renderc.89z'])
            (out/(label+'.log')).write_text(native) # retain failure evidence before any gate
            match=re.search(r'capacity checksum (\w+) identity (\w+)',pc);assert match,pc
            assert re.search(r'checksum (\w+)',native)[1].lower()==match[1].lower()
            values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
            assert values[-1]==int(match[2],16)and values[-1]>>24==args.actors,(label,pc,native[-1000:])
            assert values[-2]==frames and len(values[:-2])==2*frames,(label,len(values))
            frame_costs=values[:-2]
            ordinary=[cost for mode,cost in zip(frame_costs[::2],frame_costs[1::2])if not mode>>31]
            cold=[cost for mode,cost in zip(frame_costs[::2],frame_costs[1::2])if mode>>31]
            if label in ('status-last','equipment-last'):assert(values[-1]>>16)&255==args.actors-1
            results.append(dict(case=label,checksum=match[1],identity=match[2],actors=args.actors,
                                bodies=args.bodies if label=='monster-body' else 0,
                                ordinary_average_cycles=sum(ordinary)//len(ordinary),
                                ordinary_peak_cycles=max(ordinary),ordinary_budget_passed=max(ordinary)<360000,
                                cold_or_turn_peak_cycles=max(cold,default=0)))
            (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')  # numbers kept, gate after
            if not args.measure_only:
                assert max(ordinary)<360000,(label,max(ordinary),'over the 360k ordinary budget',
                                             frame_costs[1::2].index(max(ordinary)))
            print(label+': '+pc.strip()+'; ordinary peak '+str(max(ordinary))+
                  '/360000 '+('PASS' if max(ordinary)<360000 else 'FAIL (measurement only)'),flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')


if __name__=='__main__':main()
