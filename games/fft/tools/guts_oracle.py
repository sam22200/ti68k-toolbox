#!/usr/bin/env python3
"""Original Guts formulas 0x39/0x3a/0x3b (Yell, Cheer Up, Scream): stat changes, target validity and
the finalizer's clamps (CORE_GUTS)."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function

GUTS=(150,151,153)
# target modes: 0 plain, 1 Dead, 2 Petrify, 3 Undead, 4 Chicken, 5 Undead+Dead
MODES={0:(0,0,0,0,0),1:(0x20,0,0,0,0),2:(0,0x80,0,0,0),3:(0x10,0,0,0,0),4:(0,0,4,0,0),5:(0x30,0,0,0,0)}


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-guts/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for k,n in dict(speed='battle_formula_add_speed_y',brave='battle_formula_add_brave_y',scream='battle_formula_add_brave_x_stats_y',
            finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',remove='battle_status_remove',
            stat='battle_unit_apply_stat_increment_decrement').items():
        bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    draws=0
    def external(uc,address,size,data):
        nonlocal draws
        draws+=1   # any draw is recorded: Guts has none
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    at=s.FUNCTIONS['sort'][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    formula={150:'speed',151:'brave',153:'scream'}
    profiles=[]
    for ab in GUTS:
        for base in((1,1,1),(50,50,25),(98,98,49),(99,99,50)):
            for brave in(0,5,9,10,95,97,100):
                profiles.append((ab,*base,brave,0,0,0))
        for mode in range(1,6):profiles.append((ab,50,50,25,50,mode,0,0))
        for brave in(5,10,50):profiles.append((ab,50,50,25,brave,0,1,0))   # rider: Brave floor 10
        for bonus in(3,):
            for base in((50,50,25),(97,97,48),(99,99,50)):profiles.append((ab,*base,50,0,0,bonus))
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,pa,ma,sp,brave,mode,rider,bonus=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
        o.uc.mem_write(ability&0x1fffff,bytes(80))
        o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
        t=target&0x1fffff
        o.uc.mem_write(t+0x24,bytes((brave,)));o.uc.mem_write(t+0x28,struct.pack('<HH',300,300))
        o.uc.mem_write(t+0x30,bytes((pa,ma,sp,bonus,bonus,bonus,min(99,pa+bonus),min(99,ma+bonus),min(50,sp+bonus))))
        o.uc.mem_write(t+0x58,bytes(MODES[mode]));o.uc.mem_write(t+0x182,bytes((0x80 if rider else 0,)))
        o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
        # battle_action_clear_current_data: hit 1, accuracy 100, everything else zero.
        action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
        o.write(0x8018f5fc,0);draws=0
        o.call('remove');o.call(formula[ab]);o.call('finish')
        v=bytes(o.uc.mem_read(t+0x18c,44))
        if v[0]:
            # battle_action_finalize_attack_and_flag_reactions' order: Speed, PA, MA, Brave.
            o.call('stat',v[0x12],target+0x32,50,1);o.call('stat',v[0x14],target+0x30,99,1)
            o.call('stat',v[0x15],target+0x31,99,1);o.call('stat',v[0x16],target+0x24,100,10 if rider else 0)
        after=o.uc.mem_read(t+0x24,1)[0],*o.uc.mem_read(t+0x30,9)
        rows.append([*p,v[0],v[2],v[0x12],v[0x14],v[0x15],v[0x16],v[0x25],*after,draws])
    native=[row for row in rows if row[7]==0]   # natively one value per stat: no equipment bonus rows
    lines=['/* Original Guts 0x39/0x3a/0x3b profiles; local source bytes only. */',
           'static const u8 source_guts[][%d]={'%len(native[0])]
    assert all(0<=v<256 for row in native for v in row)
    lines+=[' {'+','.join(map(str,row))+'},'for row in native];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='ability,pa,ma,sp,brave,target mode,rider,equipment bonus; then hit,miss type,sp/pa/ma/brave change,'
                'attack type; then brave, base pa/ma/sp, bonus pa/ma/sp, effective pa/ma/sp after; draws',
        scope='Original Guts formulas, finalizer validity and stat increment clamps in the source apply order.'),indent=2)+'\n')
    print('Original guts profiles:',len(rows),'native',len(native))

if __name__=='__main__':main()
