#!/usr/bin/env python3
"""Original wall-fire poses, pending launches and movement on compiled TI."""
import json
from reference import GAME,OUT
from check_original_ti import execute
from wall_header import scenario

def keyfile(path,keys):
    translate={'B':'A','Y':'B'}
    path.write_text(''.join(str(f)+' '+' '.join(translate.get(k,k) for k in held)+'\n' for f,held in keys.items()))
    return path

def main():
    data=json.loads((OUT/'wall_charge/reference.json').read_text())
    directory=GAME/'x/original';directory.mkdir(exist_ok=True)
    total=shots=0;reports=[]
    for case in data['cases']:
        keys=keyfile(directory/('wall-'+case['name']+'.txt'),case['keys'])
        rows=execute(scenario(case['kind']),keys,len(case['samples']));previous={};count=0
        for f,(ti,r) in enumerate(zip(rows,case['samples'])):
            got=[ti[i] for i in (0,1,5,6,7,8,9,17)]
            expected=[r[k]&65535 for k in ('x','y','vx','vy','xs','ys','state','hp')]
            assert got==expected,(case['name'],f,'hero',got,expected)
            tier=0 if ti[19]<31 else 3 if ti[19]<101 else 2
            assert (ti[19] and tier==r['charge_tier'] if r['charge_state'] else not ti[19]),(case['name'],f,'charge',ti[19],r['charge_tier'])
            live={p['slot']:p['kind'] for p in r['shots']}
            births=[((slot-0x1228)//64,kind) for slot,kind in live.items() if previous.get(slot)!=kind]
            native=[(i,ti[32+8*i]) for i in range(3) if ti[33+8*i] and not ti[34+8*i]]
            assert native==births,(case['name'],f,'births',native,births)
            previous=live
            for p in r['shots']:
                offset=28+8*((p['slot']-0x1228)//64)
                got=[ti[offset+i] for i in (0,1,2,4)]
                expected=[p[k]&65535 for k in ('x','y','vx','kind')]
                assert ti[offset+5] and got==expected,(case['name'],f,'shot',got,expected)
                if not p['kind']:assert ti[offset+3]==p['xs'],(case['name'],f,'normal fraction')
                count+=1
        total+=len(rows);shots+=count;reports.append(dict(case=case['name'],frames=len(rows),projectile_samples=count))
        print(case['name'],len(rows),'wall states,',count,'projectile samples pass on68000',flush=True)
    fraction_adaptations=[]
    for n,probe in enumerate(data['probes']):
        frame=probe['release'];keys={}
        for f in range(frame+1):
            held=['RIGHT']+(['B'] if probe['kick'] and f>=10 else [])+(['Y'] if f in (probe['earlier'],frame) else [])
            keys[f]=held
        rows=execute(102,keyfile(directory/f'wall-probe-{n}.txt',keys),frame+1);ti=rows[-1]
        born=[(i,ti[28+8*i:36+8*i]) for i in range(3) if ti[33+8*i] and not ti[34+8*i]]
        p=probe['shot'];r=probe['player']
        assert len(born)==int(p is not None),(n,'born',born,p)
        assert [ti[i] for i in (0,1,5)]==[r[k]&65535 for k in ('x','y','vx')],(n,'hero')
        if p:
            slot,shot=born[0];source_slot=(p['slot']-0x1228)//64
            assert shot[:3]==[p[k]&65535 for k in ('x','y','vx')],(n,'muzzle',born,p)
            if shot[3]!=p['xs']:
                # The source reuses its culled first shot; target retains it
                # in the wider viewport and allocates a fresh second slot.
                assert source_slot!=slot and ti[33+8*source_slot] and ti[34+8*source_slot]>0 and rows[-2][31+8*slot]==shot[3],(n,'unexplained fraction',born,p)
                fraction_adaptations.append(dict(probe=n,source_slot=source_slot,native_slot=slot,source_xs=p['xs'],native_xs=shot[3]))
    fraction_samples=0
    for case in data['fractions']:
        xs=case['initial_xs'];n=101 if not xs else 121 if xs==64 else 122
        keys=keyfile(directory/f'wall-fraction-{xs}.txt',{0:['Y'],1:[]})
        rows=execute(n,keys,len(case['samples']))
        for ti,p in zip(rows,case['samples']):
            assert ti[33] and [ti[i] for i in (28,29,30,31,32)]==[p[k]&65535 for k in ('x','y','vx','xs','kind')]
            fraction_samples+=1
    report=dict(hero_states=total,projectile_samples=shots,muzzle_probes=len(data['probes']),cases=reports,
                normal_fraction_samples=fraction_samples,muzzle_fraction_adaptations=fraction_adaptations,
                fields='Hero X/Y/fractions/VX/VY/state/HP, charge presence/tier, projectile births and source-active X/Y/VX/kind; normal XS exact, unused charged XS canonicalized.',
                adaptation='Wider native viewport may retain projectiles after source removal; repeated-shot probes compare the new birth, independent of later slot reuse.')
    (GAME/'x/wall-ti.json').write_text(json.dumps(report,indent=2)+'\n')
    print(total,'original wall states,',shots,'projectile samples and',len(data['probes']),'muzzle probes pass')

if __name__=='__main__':main()
