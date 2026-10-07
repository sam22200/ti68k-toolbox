#!/usr/bin/env python3
"""Buster muzzle/formation probes during run, jump and wall traversal."""
from reference import *
from measure import row
from combat_reference import actors
def main():
    s=SNES(ROM);cases=[]
    definitions=[('run_tap','start',0,{0:['RIGHT','Y'],1:['RIGHT'],12:['RIGHT','Y'],13:['RIGHT']},70),
        ('jump_tap','start',0,{0:['B','Y'],1:['B'],6:['B','Y'],7:['B'],30:[]},70),
        ('run_charge','start',0,{0:['RIGHT','Y'],110:['RIGHT']},150),
        ('run_medium','start',0,{0:['RIGHT','Y'],45:['RIGHT']},80),
        ('run_charge_turn','start',0,{0:['RIGHT','Y'],110:['RIGHT'],112:['LEFT']},150),
        ('jump_charge','start',101,{0:['B'],22:[]},55),
        ('jump_late_charge','start',101,{0:['B','Y'],10:['B'],22:[]},55),
        ('wall_tap','wall',0,{0:['RIGHT'],3:['RIGHT','Y'],4:['RIGHT'],16:['RIGHT','Y'],17:['RIGHT']},40),
        ('left_tap','start',0,{0:['LEFT','Y'],1:['LEFT'],12:['LEFT','Y'],13:['LEFT'],45:[]},55),
        ('left_medium','start',45,{0:['LEFT'],18:[]},35),
        ('left_charge','start',101,{0:['LEFT'],18:[]},35),
        ('run_stop_tap','start',0,{0:['RIGHT'],40:['Y'],41:[],60:['Y'],61:[]},100),
        ('run_stop_medium','start',0,{0:['RIGHT','Y'],45:[]},90),
        ('run_stop_charge','start',0,{0:['RIGHT','Y'],110:[]},150),
        ('run_jump_tap','start',0,{0:['RIGHT'],40:['RIGHT','B','Y'],41:['RIGHT','B'],70:['RIGHT']},90),
        ('run_jump_medium','start',31,{0:['RIGHT','Y'],40:['RIGHT','B'],70:['RIGHT']},100),
        ('run_jump_large','start',101,{0:['RIGHT','Y'],40:['RIGHT','B'],70:['RIGHT']},100),
        ('ledge_tap','ledge',0,{0:['RIGHT','Y'],1:['RIGHT']},40),
        ('ledge_stop_tap','ledge',0,{0:['Y'],1:[]},24),
        ('ledge_medium','medium_ledge',0,{0:['RIGHT']},40),
        ('ledge_large','large_ledge',0,{0:['RIGHT']},40)]
    try:
        for name,state,hold,keys,count in definitions:
            if state in ('medium_ledge','large_ledge'):
                s.load(OUT/'start.state');charging=False
                threshold=758 if state=='medium_ledge' else 500
                for f in range(800):
                    charging=charging or s.read(X,2)>=threshold
                    for a in range(0xe68,0x1228,64):s.ram[a]=0
                    run(s,['RIGHT']+(['Y'] if charging else []))
                    if s.read(X,2)==807 and s.read(0x7e0baa)==4 and s.read(0x7e0bd3)==0:break
                else:raise AssertionError(('no charged ledge',state))
                assert not actors(s,0x1228,0x1428)
            else:s.load(OUT/'traverse/ledge.state' if state=='ledge' else OUT/(state+'.state'))
            for f in range(hold):run(s,['Y'])
            initial_player=row(s);initial_shots=actors(s,0x1228,0x1428);held=[];samples=[]
            for f in range(count):
                if f in keys:held=keys[f]
                for a in range(0xe68,0x1228,64):s.ram[a]=0
                run(s,held);r=row(s);r['pose']=s.read(0x7e0bbf);r['anim_group']=s.read(0x7e0bbe)
                r['shots']=actors(s,0x1228,0x1428);samples.append(r)
                if f in (0,3,6,12,16,110,116,120):
                    (OUT/'combined').mkdir(exist_ok=True);save_scene(s,f'combined/{name}-{f}')
            cases.append(dict(name=name,initial_state=state,initial_player=initial_player,precharge=hold,keys=keys,samples=samples,
                              initial_shots=initial_shots,
                              boundary_bob=name.startswith('run_jump_') or name.startswith('ledge_')))
            print(name,'captured',len(samples),'frames')
        (OUT/'combined/reference.json').write_text(json.dumps(dict(isolation='Clear enemy active bytes E68..1228 only; player projectiles and player/camera/timing untouched. Charged ledge preparation holds RIGHT and begins Y at X758 (medium) or X500 (large) until unsupported RUN at X807. Normal ledge door is the earlier selected-roller traversal state; inactive slot history is retained.',cases=cases),indent=2)+'\n')
    finally:s.close()
if __name__=='__main__':main()
