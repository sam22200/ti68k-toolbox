#!/usr/bin/env python3
"""Headless original-68000 execution: pixels, tactical state, CPU and heap sizes."""
from pathlib import Path
import json,re,subprocess,statistics,tempfile,sys
HERE=Path(__file__).resolve().parent; ROOT=HERE.parents[2]; OUT=HERE/'x/check'; DATA=HERE/'data'
sys.path.insert(0,str(HERE.parent/'tools'))
from review_files import write_changed
def run(args,cwd=DATA):
    # Repeating a regression must not refresh old, unchanged gallery pictures.
    with tempfile.TemporaryDirectory(prefix='fft-opening-images-') as folder:
        argv=list(map(str,args));images=[]
        for i,arg in enumerate(argv):
            if arg.endswith('.png'):
                staged=Path(folder)/(str(i)+'.png');images.append((staged,Path(arg)));argv[i]=str(staged)
        p=subprocess.run(argv,cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        if p.returncode:print(p.stdout);p.check_returncode()
        for staged,dest in images:
            if staged.exists():write_changed(dest,staged.read_bytes())
        return p.stdout
def main():
    OUT.mkdir(parents=True,exist_ok=True)
    if '--scenes-only' in sys.argv:
        check_scenes();return
    run([ROOT/'tools/pyenv/bin/python',HERE/'../tools/contour_assets.py','--attach',HERE/'data/ffov0.bin'])
    common=[ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-o',OUT/'oracle',HERE/'check_native.c',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run([ROOT/'tools/bin/ti-cc','-o',OUT/'openchk','-DRT_CYCLES','-DFFT_MEASURE',HERE/'check_native.c',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    intro=OUT/'intro.keys';intro.write_text(''.join('%d ENTER\n%d\n'%(f,f+1) for f in range(80,2001,15)))
    fast=OUT/'fast.keys';fast.write_text('0 A\n')
    palette=OUT/'palette.keys';palette.write_text('190 CATALOG\n191\n')
    rotations=OUT/'rotations.keys';rotations.write_text('12 F5\n13\n50 F1\n51\n')
    empty=OUT/'none.keys';empty.write_text('0\n')
    status=OUT/'status.keys';status.write_text('30 UP\n31\n40 UP\n41\n50 ENTER\n51\n')
    launch=OUT/'launch.keys';launch.write_text('0 ENTER\n1\n')
    reach=OUT/'reach.keys';reach.write_text('20 ENTER\n21\n45 RIGHT\n46\n60 LEFT\n61\n80 MODE\n81\n110 B\n111\n130 ENTER\n131\n155 F5\n156\n190 F1\n191\n220 B\n221\n')
    settings=OUT/'settings.keys';settings.write_text('20 ENTER\n21\n40 CATALOG\n41\n43 ENTER\n44\n46 ESC\n47\n80 F5\n81\n115 CATALOG\n116\n118 ENTER\n119\n122 CATALOG\n123\n145 APPS\n146\n155 APPS\n156\n180 B\n181\n')
    base=[ROOT/'tools/bin/ti-cycles','--max',10000]
    base+=['--file',HERE.parent/'campaign/data/ffbm.89y','--file',HERE.parent/'campaign/data/ffbn.89y','--file',HERE.parent/'campaign/data/ffbo.89y']   # the battle program's modules
    for name in ('ffov0','ffov1','ffov2','ffov3','ffoa','ffoi','ffost','ffod','ffcp','ffcu','ffcw','ffca','ffcb','ffcj'):base+=['--file',DATA/(name+'.89y')]
    cases=[('gate',0,60,empty),('story-to-fight',0,700,intro),('player',1,60,empty),('rotations',1,100,rotations),('status',1,90,status),('reach',1,260,reach),('reach-settings',1,200,settings),('reach-contour',1,70,settings),('reach-menu',1,44,settings),('fight-first',2,1200,fast),('fight-complete',2,9500,fast),('ending',3,400,intro)]
    checks=[]
    for name,scenario,frames,keys in cases:
        pc=run([OUT/'oracle',scenario,frames,keys,OUT/(name+'-pc.png')])
        ti=run([*base,'--arg',scenario,'--frames',frames,'--keys',keys,'--png',OUT/(name+'-ti.png'),OUT/'openchk.89z'])
        (OUT/(name+'.log')).write_text(ti)
        assert '_main returned' in ti,name
        checksum=re.search(r'checksum (\w+)',pc)[1];assert re.search(r'checksum\s+(?:\S+\s+)?(\w+)',ti) or 'checksum' in ti
        # Runner says "screen checksum: XXXX"; PC oracle says "checksum XXXX".
        native=re.search(r'checksum\s+(\w+)',ti)[1];assert native.lower()==checksum.lower(),(name,checksum,native)
        values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
        marker=values.index(0x0b0e0001);state,work=values[marker+1:marker+3]
        expected=[int(s,16) for s in re.findall(r'STATE,(\w+)',pc)];assert values[marker+3:]==expected,(name,values[marker+3:],expected)
        executed=int(re.search(r'executed (\d+)',pc)[1]);assert values[marker-1]==executed
        costs=values[:marker-1];assert len(costs) in (executed*2,(executed-1)*2),(name,len(costs),executed)
        ordinary=[c for m,c in zip(costs[::2],costs[1::2]) if not m>>31];battle_costs=[c for m,c in zip(costs[::2],costs[1::2]) if not m>>31 and ((m>>24)&127)<32];story_costs=[c for m,c in zip(costs[::2],costs[1::2]) if not m>>31 and ((m>>24)&127)>=32];cold=[c for m,c in zip(costs[::2],costs[1::2]) if m>>31]
        check=dict(case=name,scenario=scenario,frames=executed,requested_frames=frames,checksum=checksum,native_state_bytes=state,native_work_bytes=work,
                   ordinary_average_cycles=round(statistics.mean(ordinary)),ordinary_peak_cycles=max(ordinary),battle_peak_cycles=max(battle_costs,default=0),story_peak_cycles=max(story_costs,default=0),load_or_turn_peak_cycles=max(cold,default=0),state_summary=expected)
        checks.append(check);print(json.dumps(check),flush=True)
    report=dict(measurement='Actual TI binary, MC68000 datasheet cycles, grayscale/interrupt/frame-wait overhead excluded; no PC FPS inference.',checks=checks,
                native_game_heap_bytes=8*40*192+checks[0]['native_work_bytes'],native_grayscale_heap_bytes=3840+7688,
                native_checked_binary_bytes=(OUT/'openchk.89z').stat().st_size,archive_bytes=sum((DATA/(n+'.bin')).stat().st_size for n in ('ffov0','ffov1','ffov2','ffov3','ffoa','ffoi','ffost','ffod','ffcp','ffcu','ffcw','ffca','ffcb')))
    (OUT/'checked.json').write_text(json.dumps(report,indent=2)+'\n');print('Opening: %d complete PC/TI pixels + tactical-state checkpoints passed'%len(checks))
    check_scenes()

def check_scenes():
    # Shared lettering needs ffflow, including in this independent checker.
    # Retain the original-strip control at CURRENT approved title timings.
    common=[ROOT/'runtime/core/rt_core.c']
    empty=OUT/'none.keys';empty.write_text('0\n')
    launch=OUT/'launch.keys';launch.write_text('0 ENTER\n1\n')
    palette=OUT/'palette.keys';palette.write_text('190 CATALOG\n191\n')
    intro=OUT/'intro.keys';intro.write_text(''.join('%d ENTER\n%d\n'%(f,f+1) for f in range(80,2001,15)))
    run(['cc','-std=gnu99','-O1','-DCHECK_SCENE','-o',OUT/'scene',HERE/'../campaign/check_native.c',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run([ROOT/'tools/bin/ti-cc','-o',OUT/'scenechk','-DRT_CYCLES','-DCHECK_SCENE',HERE/'../campaign/check_native.c',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    sbase=[ROOT/'tools/bin/ti-cycles','--max',10000]
    for name in ('ffs0','ffoa','ffca','ffcb','ffcu','ffcp'):sbase+=['--file',DATA/(name+'.89y')]
    sbase+=['--file',HERE.parent/'campaign/data/ffflow.89y']
    title_golden={'title':'AFE4','launch-title':'AFE4'} # historical strip at the same30-frame reveal
    cases=(('title',0,30,empty),('launch-title',0,30,launch),('title-complete',0,120,empty),('prayer',0,240,empty),('prayer-white',0,240,palette),('chapel-long',0,420,intro))
    with tempfile.TemporaryDirectory(prefix='fft-opening-scenes-')as name:
        folder=Path(name)
        active={}
        for origin in (DATA,HERE.parent/'campaign/data'):
            for p in origin.glob('*.bin'):
                q=folder/p.name;q.unlink(missing_ok=True);q.symlink_to(p)
        for label,scenario,frames,keys in cases:
            pc=run([OUT/'scene',scenario,frames,keys,OUT/(label+'-scene-pc.png')],cwd=folder)
            ti=run([*sbase,'--arg',scenario,'--frames',frames,'--keys',keys,'--png',OUT/(label+'-scene-ti.png'),OUT/'scenechk.89z'])
            a=re.search(r'checksum (\w+)',pc)[1];b=re.search(r'checksum\s+(\w+)',ti)[1]
            assert a.lower()==b.lower()and a!='0000',(label,'shared-font scene',a,b)
            active[label]=(a,re.search(r'state (\w+)',pc)[1])
        run(['cc','-std=gnu99','-O1','-DFFT_LEGACY_SERIF','-DCHECK_SCENE','-o',OUT/'scene-legacy',HERE/'../campaign/check_native.c',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
        run([ROOT/'tools/bin/ti-cc','-o',OUT/'scnold','-DRT_CYCLES','-DFFT_LEGACY_SERIF','-DCHECK_SCENE',HERE/'../campaign/check_native.c',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        for label,scenario,frames,keys in cases:
            pc=run([OUT/'scene-legacy',scenario,frames,keys,OUT/(label+'-legacy-control.png')],cwd=folder)
            ti=run([*sbase,'--arg',scenario,'--frames',frames,'--keys',keys,OUT/'scnold.89z'])
            a=re.search(r'checksum (\w+)',pc)[1];b=re.search(r'checksum\s+(\w+)',ti)[1]
            assert a.lower()==b.lower()and a!='0000',(label,'legacy control',a,b)
            assert re.search(r'state (\w+)',pc)[1]==active[label][1],(label,'font must not alter scene timing/state')
            if label in title_golden:assert a==title_golden[label],(label,a,title_golden[label])
            if frames>=150:assert a==active[label][0],(label,'chapel pixels changed')
    print('Scene player:6 shared-font PC/68000 replays,6 strip controls, equal scene state/timing and post-title pixels passed')
if __name__=='__main__':main()
