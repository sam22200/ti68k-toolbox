#!/usr/bin/env python3
"""No-write directional gesture sweeps and complete first-flight ROM fixtures."""
import json
import os
from pathlib import Path
import sys
GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path[:0] = [str(ROOT / 'tools/neogeo')]
from check_gameplay import mask, values
from check_reference import frame_fingerprint
from neogeorun import NeoGeo, sha
from measure_timing import TIMING_FIELDS
from measure_actions import PROGRAM_SHA
DIRECTIONS = ('UP', 'UP RIGHT', 'RIGHT', 'DOWN RIGHT', 'DOWN', 'DOWN LEFT', 'LEFT', 'UP LEFT')

def export(capture):
    starts, refs, inputs, reports = [], [], [], []
    for ident,t in enumerate(capture['trials']):
        initial, rows, owner = t['initial'], t['rows'], t['owner']
        launch = next(i for i,r in enumerate(rows) if r['disc_state'] in (4,6,8,22))
        stop = next(i for i in range(launch+1,len(rows)) if rows[i]['disc_state'] in (2,18,10,24,28))
        if rows[stop]['disc_state']==2:
            # Stationary front capture followed by a manual return, when present.
            if t['return_frame'] is not None:
                release = next(i for i in range(stop+1,len(rows)) if rows[i]['disc_state']==4)
                stop = next(i for i in range(release+1,len(rows)) if rows[i]['disc_state']!=4)-1
            else:
                stop = min(stop+22,len(rows)-1)
        else: stop -= 1
        if '_lob_' in t['name']: stop = launch - 1
        # Native goal-entry geometry is adapted (x=16/304); compare the
        # complete flight until its first exit from the playable court.
        outside = next((i for i in range(launch+1,stop+1) if not 16 <= rows[i]['disc_x'] >> 16 < 304), None)
        if outside is not None: stop = outside - 1
        actor=[initial[f'p{port}_{field}'] for port in (1,2) for field in ('x','y','hold_age','power','bonus')]
        starts.append((owner,*actor,initial['disc_x'],initial['disc_y'],len(inputs),stop+1,*initial['history']))
        for f,r in enumerate(rows[:stop+1]):
            inputs.append(t['inputs'][f])
            row=[ident,f]
            for port in (1,2): row += [r[f'p{port}_{field}'] for field in ('x','y','vx','vy','action','hold_age','power','bonus')]
            row += [r[k] for k in ('disc_x','disc_y','disc_state','disc_vx','disc_vy','z','vz','angle32','speed','turn')]
            refs.append(row)
        reports.append(dict(name=t['name'],steps=stop+1,launch=launch,kind=rows[launch]['disc_state'],terminal=rows[stop+1]['disc_state'] if stop+1<len(rows) else None))
    lines=['/* Local original curve fixtures; PC only. */']
    for title,data in (('CurveStart curve_start',starts),('CurveRef curve_ref',refs),('AdvancedInput curve_input',inputs)):
        lines += ['static const '+title+'[] = {']+[' {'+','.join(map(str,r))+'},' for r in data]+['};']
    (GAME/'generated/curves.h').write_text('\n'.join(lines)+'\n')
    (GAME/'generated/curves.json').write_text(json.dumps(dict(metadata=capture['metadata'],program_sha256=PROGRAM_SHA,script_sha256=sha(Path(__file__).read_bytes()),reference_frames=sum(len(t['rows']) for t in capture['trials']),native_steps=len(refs),cases=reports,excluded='goal celebration; rear deflections; curve-specific throw art'),indent=2)+'\n')
    print(f'Curves: {len(starts)} repeated no-write trials, {len(refs)} native steps',flush=True)

def main():
    out=GAME/'generated'
    (GAME/'x').mkdir(exist_ok=True)
    prepare=GAME/'x'/f'curves_prepare_{os.getpid()}.state'
    if '--fixtures-only' in sys.argv: export(json.loads((out/'curves_capture.json').read_text())); return
    n=NeoGeo();trials=[]
    fields=dict(TIMING_FIELDS,angle32=(0x100a30,4,False),speed=(0x100a34,2,False),turn=(0x100a36,4,True))
    def sample():
        return {**values(n),**{k:n.read(*v) for k,v in fields.items()},'history':[n.read(0x100900+p*64+i,1) for p in (0,1) for i in range(9)]}
    try:
        metadata=n.metadata();assert sha((ROOT/'sources/windjammers_neogeo/program.be.bin').read_bytes())==PROGRAM_SHA
        if '--returns-only' in sys.argv:
            trials = [t for t in json.loads((out/'curves_capture.json').read_text())['trials'] if not t['name'].startswith('return_')]
        for owner in (() if '--returns-only' in sys.argv else (0,1)):
            n.load(out/('actions_serve.state' if owner else 'actions_mita.state'));n.pressed[:]=[0,0];n.step();n.step();n.save(prepare);surface=n.frame
            cases=[]
            for end in range(8):
                for sign in (-1,1):
                    cases.append((f'arc_{end}_{sign}',[(end-2*sign)&7,(end-sign)&7,end],[1,1,1],0,'A',None))
            for sign in (-1,1):
                end=6 if owner else 2
                seq=[(end-2*sign)&7,(end-sign)&7,end]
                for middle in range(1,6):
                    for final in range(1,6):cases.append((f'window_{sign}_{middle}_{final}',seq,[1,middle,final],0,'A',None))
                cases += [(f'short_{sign}',seq[1:],[1,1],0,'A',None),(f'skip_{sign}',seq[::2],[1,1],0,'A',None)]
                for delay in range(1,6):cases.append((f'neutral_{sign}_{delay}',seq,[1,1,1],delay,'A',None))
                cases.append((f'lob_{sign}',seq,[1,1,1],0,'B',None))
            # A late possession raises Yoo's normalized power above 16.
            cases += [('power_short',[5,6],[1,1],0,'A',24),('power_arc',[4,5,6],[1,1,1],0,'A',24)]
            if '--quick' in sys.argv: cases = cases[:8]
            for name,seq,durations,delay,button,late in cases:
                script=[0]*(2 if late is None else late)
                for a,d in zip(seq,durations):script += [mask(DIRECTIONS[a])]*d
                if delay: script += [0]*delay
                script[-1] |= mask(button)
                def run():
                    n.load(prepare);n.frame=surface;n.pressed[:]=[0,0]
                    initial=sample();rows=[];inputs=[];hashes=[]
                    for f in range(240):
                        n.pressed[:]=[0,0]
                        if f<len(script):n.pressed[owner]=script[f]
                        n.step();inputs.append(tuple(n.pressed));rows.append(sample());hashes.append(frame_fingerprint(n))
                    return dict(owner=owner,name=f'{owner}_{name}',return_frame=None,initial=initial,rows=rows,inputs=inputs,hashes=hashes)
                t=run();assert t==run(),name;trials.append(t)
                print('replayed '+t['name'],flush=True)
        # Replay the complete reception and immediate/settled return from
        # ordinary input doors; select the first front catch for each arc/side.
        for owner in (0,1):
            for kind in (6,8):
                base = next(t for t in trials if t['owner']==owner and t['return_frame'] is None and
                            next(r['disc_state'] for r in t['rows'] if r['disc_state']!=2)==kind and
                            any(r['disc_state']==2 and r[f'p{2-owner}_action']==0x1000 for r in t['rows'][30:]))
                contact = next(f for f in range(30,240) if base['rows'][f]['disc_state']==2 and base['rows'][f][f'p{2-owner}_action']==0x1000)
                n.load(out/('actions_serve.state' if owner else 'actions_mita.state'));n.pressed[:]=[0,0];n.step();n.step();n.save(prepare);surface=n.frame
                for offset in (1,2,22):
                    counter = contact + offset
                    def returned():
                        n.load(prepare);n.frame=surface;n.pressed[:]=[0,0]
                        initial=sample();rows=[];inputs=[];hashes=[]
                        for f in range(240):
                            n.pressed[:]=base['inputs'][f]
                            if f==counter:n.pressed[owner^1] |= mask('A')
                            n.step();inputs.append(tuple(n.pressed));rows.append(sample());hashes.append(frame_fingerprint(n))
                        return dict(owner=owner,name=f'return_{owner}_{kind}_{offset}',return_frame=counter,initial=initial,rows=rows,inputs=inputs,hashes=hashes)
                    t=returned();assert t==returned(),t['name'];trials.append(t);print('replayed '+t['name'],flush=True)
        capture=dict(metadata=metadata,fields=fields,program_sha256=PROGRAM_SHA,script_sha256=sha(Path(__file__).read_bytes()),trials=trials)
        (out/'curves_capture.json').write_text(json.dumps(capture)+'\n');export(capture)
    finally:
        n.close()
        prepare.unlink(missing_ok=True)
if __name__=='__main__': main()
