#!/usr/bin/env python3
"""Headless original-68000 execution: pixels, tactical state, CPU and heap sizes."""
from pathlib import Path
import json,re,subprocess,statistics
HERE=Path(__file__).resolve().parent; ROOT=HERE.parents[2]; OUT=HERE/'x/check'; DATA=HERE/'data'
def run(args,cwd=DATA):
    p=subprocess.run(list(map(str,args)),cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if p.returncode:print(p.stdout);p.check_returncode()
    return p.stdout
def main():
    OUT.mkdir(parents=True,exist_ok=True)
    run([ROOT/'tools/pyenv/bin/python',HERE/'../tools/contour_assets.py','--attach',HERE/'data/ffov0.bin'])
    common=[ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-o',OUT/'oracle',HERE/'check_native.c',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run([ROOT/'tools/bin/ti-cc','-o',OUT/'openchk','-DRT_CYCLES','-DFFT_MEASURE',HERE/'check_native.c',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    intro=OUT/'intro.keys';intro.write_text(''.join('%d ENTER\n%d\n'%(f,f+1) for f in range(80,2001,15)))
    fast=OUT/'fast.keys';fast.write_text('0 A\n')
    palette=OUT/'palette.keys';palette.write_text('190 CATALOG\n191\n')
    rotations=OUT/'rotations.keys';rotations.write_text('12 F5\n13\n50 F1\n51\n')
    empty=OUT/'none.keys';empty.write_text('0\n')
    launch=OUT/'launch.keys';launch.write_text('0 ENTER\n1\n')
    reach=OUT/'reach.keys';reach.write_text('20 ENTER\n21\n45 RIGHT\n46\n60 LEFT\n61\n80 MODE\n81\n110 B\n111\n130 ENTER\n131\n155 F5\n156\n190 F1\n191\n220 B\n221\n')
    settings=OUT/'settings.keys';settings.write_text('20 ENTER\n21\n40 CATALOG\n41\n43 ENTER\n44\n46 ESC\n47\n80 F5\n81\n115 CATALOG\n116\n118 ENTER\n119\n122 CATALOG\n123\n145 APPS\n146\n155 APPS\n156\n180 B\n181\n')
    base=[ROOT/'tools/bin/ti-cycles','--max',10000]
    for name in ('ffov0','ffov1','ffov2','ffov3','ffoa','ffoi','ffost','ffcu','ffcw','ffca','ffcb'):base+=['--file',DATA/(name+'.89y')]
    cases=[('title',0,30,empty),('launch-title',0,30,launch),('title-complete',0,120,empty),('prayer',0,240,empty),('prayer-white',0,240,palette),('chapel-long',0,420,intro),('story-to-fight',0,1200,intro),('player',1,60,empty),('rotations',1,100,rotations),('reach',1,260,reach),('reach-settings',1,200,settings),('reach-contour',1,70,settings),('reach-menu',1,44,settings),('fight-first',2,1200,fast),('fight-complete',2,9500,fast),('ending',3,950,intro)]
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
                native_checked_binary_bytes=(OUT/'openchk.89z').stat().st_size,archive_bytes=sum((DATA/(n+'.bin')).stat().st_size for n in ('ffov0','ffov1','ffov2','ffov3','ffoa','ffoi','ffost','ffcu','ffcw','ffca','ffcb')))
    (OUT/'checked.json').write_text(json.dumps(report,indent=2)+'\n');print('Opening: %d complete PC/TI pixels + tactical-state checkpoints passed'%len(checks))
if __name__=='__main__':main()
