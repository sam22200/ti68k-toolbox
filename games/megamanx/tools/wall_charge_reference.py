#!/usr/bin/env python3
"""Charged release across wall grip, kick windup and forced-away launch."""
from reference import *
from measure import row
from combat_reference import actors

def run_isolated(s,held):
    for a in range(0xe68,0x1228,64):s.ram[a]=0
    run(s,held)

def main():
    directory=OUT/'wall_charge';directory.mkdir(exist_ok=True)
    s=SNES(ROM);cases=[];probes=[];fractions=[];clamp=[]
    definitions=[('fall',0,False),('grip',3,False),('late_slide',10,False),
                 ('kick_edge',10,True),('kick_windup',12,True),
                 ('kick_launch_transition',15,True),('kick_launch',17,True)]
    try:
        for kind,hold in [(0,0),(1,31),(3,101)]:
            s.load(OUT/'start.state')
            for f in range(480):run_isolated(s,['RIGHT','Y'] if f>=480-hold else ['RIGHT'])
            initial=row(s);initial['shots']=actors(s,0x1228,0x1428)
            initial['shot_raw']=bytes(s.ram[0x1228:0x1268]).hex()
            s.save(directory/f'ready-{kind}.state')
            print('Wall door',kind,initial['x'],initial['y'],initial['charge_state'],initial['charge_tier'],flush=True)
            for action,release,kick in definitions:
                keys={}
                for f in sorted({0,release,release+1} | ({10} if kick else set())):
                    keys[f]=['RIGHT']+(['B'] if kick and f>=10 else [])+(['Y'] if (f<release and kind) or (f==release and not kind) else [])
                s.load(directory/f'ready-{kind}.state');held=[];samples=[]
                for f in range(40):
                    if f in keys:held=keys[f]
                    run_isolated(s,held);r=row(s);r['shots']=actors(s,0x1228,0x1428)
                    r['pose']=s.read(0x7e0bbf);r['anim_group']=s.read(0x7e0bbe)
                    r['shot_raw']=bytes(s.ram[0x1228:0x1268]).hex();samples.append(r)
                name=f'{action}_{kind}';cases.append(dict(name=name,kind=kind,hold=hold,initial_player=initial,keys=keys,samples=samples))
                birth=next(((f,r) for f,r in enumerate(samples) if any(p['kind']==kind for p in r['shots'])),None)
                if birth is None:
                    print(name,'no emitted projectile',flush=True);continue
                f,r=birth;p=next(p for p in r['shots'] if p['kind']==kind)
                print(name,'birth',f,'hero',r['state'],r['substate'],r['x'],r['y'],'shot',p['x'],p['y'],p['vx'],flush=True)
        # Probe adjacent birth updates and a prior shot. All positions,
        # physics, input and timing still come from natural playback.
        for kick in (False,True):
            for release in range(0 if not kick else 10,29):
                for earlier in (None,3):
                    if earlier is not None and release<earlier+2:continue
                    s.load(directory/'ready-0.state');samples=[]
                    for f in range(release+1):
                        before=row(s)
                        held=['RIGHT']+(['B'] if kick and f>=10 else [])+(['Y'] if f in (earlier,release) else [])
                        run_isolated(s,held);r=row(s)
                        r['shots']=actors(s,0x1228,0x1428)
                        r['pose']=s.read(0x7e0bbf);r['anim_group']=s.read(0x7e0bbe)
                        samples.append(r)
                    # One-update taps are far enough apart to leave a free
                    # buster slot here; normal launch VX distinguishes birth.
                    born=[p for p in r['shots'] if abs(p['vx'])==1024]
                    assert len(born)<=1,(kick,release,earlier,r['shots'])
                    p=born[0] if born else None
                    probes.append(dict(kick=kick,release=release,earlier=earlier,
                        before=before,player=r,shot=p,offset=[p['x']-r['x'],p['y']-r['y']] if p else None))
            print('Muzzle profiles',kick,[(r['release'],r['offset'],r['player']['pose'],r['player']['facing']) for r in probes if r['kick']==kick and r['earlier'] is None],flush=True)
        for xs in (0,64,192):
            s.load(OUT/'start.state');assert not s.read(0x7e1228)
            # Only inactive slot XS is changed; the normal buster should
            # retain its prior fractional byte, including subsequent motion.
            s.write(0x7e122c,xs);samples=[]
            for frame in range(8):
                run(s,['Y'] if not frame else []);samples.append(actors(s,0x1228,0x1268)[0])
            assert samples[0]['xs']==xs
            fractions.append(dict(initial_xs=xs,samples=samples))
        print('Normal slot fraction probes',[p['initial_xs'] for p in fractions],flush=True)
        s.load(directory/'ready-0.state')
        for f in range(3):run_isolated(s,['RIGHT'])
        s.save(directory/'grip.state')
        for vx in (0,128,256,376,512):
            for xs in (0,64,192):
                s.load(directory/'grip.state');s.write(VX,vx,2);s.write(0x7e0bac,xs)
                run_isolated(s,['RIGHT','Y']);p=actors(s,0x1228,0x1268)[0]
                expected=824+((xs+vx)>>8)+18
                assert s.read(X,2)==824 and p['x']==expected
                clamp.append(dict(vx=vx,xs=xs,player=row(s),shot=p,preclamp_x=expected-18))
        print('Pre-clamp muzzle:',len(clamp),'independent VX/XS probes pass',flush=True)
        (directory/'reference.json').write_text(json.dumps(dict(
            isolation='Natural cases/probes: traverse480 original updates holding RIGHT, with0/31/101 final updates also holding Y; clear only enemy active bytes, preserve player projectiles, hero/camera/timing. Controlled fractions: vary inactive normal-slot XS only. Controlled clamp: vary grip VX and hero XS independently; retain the wall and final X clamp.',
            cases=cases,probes=probes,fractions=fractions,clamp=clamp),indent=2)+'\n')
    finally:s.close()
if __name__=='__main__':main()
