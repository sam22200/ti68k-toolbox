#!/usr/bin/env python3
"""Long source routes with explicit logic-clock alignment and enemy selection."""
import json
from reference import GAME, OUT, ROM, SNES, run
from measure import row
from combat_reference import actors

CLOCK = 0x7e0b9b
ROUTES = [
    ('gap_jump', {0:['RIGHT'], 140:['RIGHT','Y'], 242:['RIGHT'],
                  450:['RIGHT','B'], 480:['RIGHT']}),
    ('wall_recovery', {0:['RIGHT'], 140:['RIGHT','Y'], 242:['RIGHT'],
                      490:['RIGHT','B'], 535:['RIGHT']})]

def sample(s):
    r = row(s)
    r['shots'] = actors(s, 0x1228, 0x1428)
    roller = next((e for e in actors(s, 0xe68, 0x1228) if e['kind'] == 21), None)
    r['roller'] = roller
    r['roller_phase'] = s.ram[roller['slot']+1] if roller else 0
    r['roller_fuse'] = s.ram[roller['slot']+0x34] if roller else 0
    return r

def main():
    directory = OUT/'traverse'
    directory.mkdir(exist_ok=True)
    s = SNES(ROM)
    cases = []
    try:
        for name, keys in ROUTES:
            s.load(OUT/'start.state')
            held = []
            previous = sample(s)
            clock = s.read(CLOCK)
            samples, skipped, removed = [], [], []
            effective_keys = {}
            for frame in range(800):
                if frame in keys:
                    held = keys[frame]
                # Preserve the selected roller, hero, shots, camera and timers.
                # Other source enemies are outside the declared prototype.
                for a in range(0xe68, 0x1228, 64):
                    if s.ram[a] and s.ram[a+10] != 21:
                        removed.append(dict(frame=frame, slot=a, kind=s.ram[a+10]))
                        s.ram[a] = 0
                run(s, held)
                current = sample(s)
                now = s.read(CLOCK)
                delta = (now-clock) & 255
                assert delta in (0,1), (name,frame,clock,now)
                if not delta:
                    # Independent state checks keep clock filtering honest.
                    assert current == previous, (name,frame,'nonconstant lag state')
                    assert frame not in keys, (name,frame,'input edge during lag')
                    skipped.append(frame)
                else:
                    if not samples or held != samples[-1]['buttons']:
                        effective_keys[len(samples)] = list(held)
                    samples.append(dict(current, source_frame=frame, buttons=list(held)))
                clock, previous = now, current
                if name == 'wall_recovery' and frame in (466,478):
                    s.save(directory/('ledge.state' if frame==466 else 'pre-clamp.state'))
                if current['x'] >= 1008 and current['state'] in (0,2,4):
                    break
            else:
                raise AssertionError((name,'did not finish',current))
            assert current['hp'] == 16 and not current['roller']
            assert any(r['roller'] for r in samples)
            cases.append(dict(name=name, keys=keys, effective_keys=effective_keys,
                              source_frames=frame+1, skipped=skipped,
                              removed=removed, samples=samples))
            print(name,frame+1,'source frames;',len(samples),'logic states;',
                  'skipped',skipped,'finish',current['x'],current['y'],flush=True)

        ledge=[]
        edge_inputs=[('last_run_jump',{0:['RIGHT','B']}),
                     ('too_late_jump',{0:['RIGHT'],1:['RIGHT','B']}),
                     ('stop',{0:[]}),('stop_jump',{0:['B'],12:[]}),
                     ('turn',{0:['LEFT']}),('turn_jump',{0:['LEFT','B'],12:['LEFT']})]
        for name,keys in edge_inputs:
            s.load(directory/'ledge.state')
            assert (s.read(0x7e0bad,2),s.read(0x7e0bac),s.read(0x7e0bd3))==(807,160,0)
            rows=[];held=[]
            for frame in range(24):
                if frame in keys:held=keys[frame]
                run(s,held)
                rows.append(dict(row(s),wall=s.read(0x7e0bd3)&1))
            ledge.append(dict(name=name,keys=keys,samples=rows,scenario=125))
        # A supported run also permits jump on the direction-release update.
        s.load(OUT/'start.state');rows=[];held=[]
        keys={0:['RIGHT'],40:['B'],70:[]}
        for frame in range(120):
            if frame in keys:held=keys[frame]
            run(s,held);rows.append(dict(row(s),wall=s.read(0x7e0bd3)&1))
        ledge.append(dict(name='flat_stop_jump',keys=keys,samples=rows,scenario=101))
        clamp=[]
        for x in (821,822,823,824):
            for xs in (0,64,128,200):
                s.load(directory/'pre-clamp.state')
                s.write(0x7e0bad,x,2);s.write(0x7e0bac,xs)
                run(s,['RIGHT'])
                r=dict(row(s),wall=s.read(0x7e0bd3)&1)
                assert r['wall']==int(x+((xs+376)>>8)+7>=832)
                clamp.append(dict(x=x,xs=xs,sample=r))

        # Negative control: this is a selected-actor prototype, not the complete
        # original section. Record the next natural actor and contact outcome.
        s.load(OUT/'start.state')
        held = []
        control = []
        last = None
        for frame in range(425):
            if frame in ROUTES[0][1]:
                held = ROUTES[0][1][frame]
            run(s, held)
            enemies = actors(s, 0xe68, 0x1228)
            event = (s.read(0x7e0bcf), tuple(e['kind'] for e in enemies))
            if event != last:
                control.append(dict(frame=frame,x=s.read(0x7e0bad,2),
                                    hp=event[0],kinds=list(event[1])))
            last = event
        assert any(r['hp'] == 13 and 41 in r['kinds'] for r in control)
        result = dict(
            isolation='Cases clear only active non-roller enemy slots; preserve hero, '
                      'roller, player shots, camera, collision and timing. Negative '
                      'control uses natural input without writes.',
            clock=dict(address=CLOCK,unit='byte; increments per observed logic update',
                       check='Each skipped frame independently preserves all measured '
                             'hero/charge/projectile/roller fields; no input edge skipped.'),
            cases=cases, ledge=ledge, clamp=clamp, natural_control=control,
            controlled='Clamp probes change player X/XS only in a naturally '
                       'prepared pre-contact state; ledge probes use natural '
                       'input from the unmodified last unsupported RUN state.')
        (directory/'reference.json').write_text(json.dumps(result,indent=2)+'\n')
    finally:
        s.close()

if __name__ == '__main__':
    main()
