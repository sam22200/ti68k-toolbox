#!/usr/bin/env python3
"""Natural contact coupled with weapons and recovery input on the USA ROM."""
from reference import *
from measure import row, suppress
from combat_reference import actors
from rom import DATA, pc

def sample(s):
    value = row(s)
    value['shots'] = actors(s, 0x1228, 0x1428)
    value['enemies'] = actors(s, 0xe68, 0x1228)
    value['hero_raw'] = bytes(s.ram[0xba8:0xc08]).hex()
    pointer = s.read(0x7e0bc8, 2)
    value['box_pointer'] = pointer
    value['box'] = list(DATA[pc(0x860000 | pointer):pc(0x860000 | pointer)+4]) if pointer else None
    return value

def main():
    directory = OUT/'damage'; directory.mkdir(exist_ok=True)
    s = SNES(ROM); cases = []; poses = []; reuse = []
    definitions = [
        ('hurt_hold_shoot', 'hit.state', {0:['Y'], 145:[]}, 180),
        ('hurt_tap_shoot', 'hit.state', {0:['Y'], 1:[], 12:['Y'], 13:[], 30:['Y'], 31:[], 45:['Y'], 46:[]}, 90),
        ('hurt_release_early', 'hit.state', {0:['Y'], 20:[]}, 90),
        ('hurt_charge_medium', 'hit.state', {0:['Y'], 70:[]}, 110),
        ('hurt_release_on_recovery', 'hit.state', {0:['Y'], 31:[]}, 85),
        ('hurt_press_after_recovery', 'hit.state', {0:[], 31:['Y'], 32:[]}, 85),
        ('hurt_run', 'hit.state', {0:['RIGHT']}, 90),
        ('hurt_jump_held', 'hit.state', {0:['RIGHT','B'], 60:['RIGHT']}, 90),
        ('hurt_jump_repress', 'hit.state', {0:['B'], 20:[], 35:['B'], 65:[]}, 100),
        ('contact_hold_charge', 'combat/natural-260.state', {0:['Y'], 145:[]}, 180),
        ('contact_release_charge', 'combat/natural-260.state', {0:['Y'], 55:[]}, 110),
        ('contact_jump_shoot', 'combat/natural-260.state', {0:[], 40:['B','Y'], 60:[]}, 110),
    ]
    try:
        for name, state, keys, count in definitions:
            s.load(OUT/state); initial = sample(s); held = []; samples = []
            for frame in range(count):
                if frame in keys: held = keys[frame]
                run(s, held); samples.append(sample(s))
            cases.append(dict(name=name, state=state, initial=initial, keys=keys, samples=samples))
            transitions = []; previous = None
            for frame, r in enumerate(samples):
                # Show state, charge and emitted shot kinds independently of
                # raw charge counters, whose units are not assumed here.
                value = (r['state'],r['hp'],r['charge_state'],r['charge_tier'],tuple(p['kind'] for p in r['shots']))
                if value != previous: transitions.append((frame,value,r['charge']))
                previous = value
            print(name, transitions, flush=True)
        for name, state, keys in [
            ('run_jump', 'start.state', {0:['RIGHT'],20:['RIGHT','B'],65:['RIGHT']}),
            ('wall_kick', 'wall.state', {0:['RIGHT'],12:['RIGHT','B'],45:['RIGHT']}),
            ('air_fire', 'start.state', {0:['B','Y'],55:[]}),
        ]:
            s.load(OUT/state); held=[]; samples=[]
            for frame in range(100):
                if frame in keys: held=keys[frame]
                suppress(s); run(s,held); samples.append(sample(s))
            poses.append(dict(name=name,keys=keys,samples=samples))
            print('player boxes',name,sorted({(hex(r['box_pointer']),tuple(r['box'] or [])) for r in samples}),flush=True)
        for vx in (-1536,0,123,1536):
            for xs in (0,64,192):
                s.load(OUT/'enemy/charged-260.state')
                assert not s.read(0x7e1228)
                # Component isolation: only an inactive projectile slot's
                # stored VX/XS vary; the hero, charge and input stay natural.
                s.write(0x7e1242,vx&65535,2);s.write(0x7e122c,xs)
                samples=[]
                for frame in range(8):
                    run(s,[]);samples.append(actors(s,0x1228,0x1268)[0])
                assert all(p['vx']==vx and p['xs']==xs for p in samples[:6])
                assert samples[6]['vx']==2048 and samples[7]['vx']==2048
                reuse.append(dict(initial_vx=vx,initial_xs=xs,samples=samples))
        print('large charge slot reuse:',len(reuse),'VX/XS probes pass',flush=True)
        (directory/'reference.json').write_text(json.dumps(dict(
            isolation='Damage cases use natural existing source states and input only; no RAM writes. Player-pose cases clear interfering actor/projectile pools before each step. Reuse probes vary only inactive shot VX/XS.',
            cases=cases,poses=poses,reuse=reuse),indent=2)+'\n')
    finally: s.close()

if __name__=='__main__': main()
