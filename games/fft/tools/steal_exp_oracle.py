#!/usr/bin/env python3
"""Original Steal Exp formula 0x28: hit (SP'+X)%, stolen amount, both units' EXP and the attacker's
level check (CORE_STEAL_EXP)."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function

AB=115
# target status modes: 0 none, 1 Protect, 2 Sleep, 3 Charging, 4 Chicken, 5 Frog
STATUS={0:(0,0,0,0,0),1:(0,0,0,0x20,0),2:(0,0,0,0,0x10),3:(8,0,0,0,0),4:(0,0,4,0,0),5:(0,0,2,0,0)}
ATTACKER_DATA=0x80192da0


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-steal-exp/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for k,n in dict(f28='battle_formula_steal_exp_hit_sp_x_percent',finish='battle_action_finalize_target_current_action',
            sort='battle_target_sort_list',remove='battle_status_remove',random='battle_formula_get_random_0_7fff',
            steal='battle_action_handle_steal_exp').items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['lib_rand']=bind_function('lib_rand','rand','main',d)
    o.function_hashes['level']=bind_function('level','main_unit_check_level_up','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    rng=1;draws=0;levels=0
    def rand():
        nonlocal rng,draws
        rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1
        return rng>>17
    def external(uc,address,size,data):
        nonlocal levels
        value=0
        if address in(s.FUNCTIONS['random'][0],s.FUNCTIONS['lib_rand'][0]):value=rand()
        elif address==s.FUNCTIONS['roll'][0]:
            bound=uc.reg_read(o.reg.UC_MIPS_REG_A0);chance=uc.reg_read(o.reg.UC_MIPS_REG_A1)
            value=int((rand()*bound>>15)>=chance)
        elif address==s.FUNCTIONS['level'][0]:
            # main_unit_check_level_up without the stat growth (native unit_stats, checked elsewhere)
            unit=uc.reg_read(o.reg.UC_MIPS_REG_A0)&0x1fffff;exp,lv=uc.mem_read(unit+0x21,2)
            if exp>=100:
                if lv>=99:uc.mem_write(unit+0x21,b'\x63')
                else:uc.mem_write(unit+0x21,bytes((0,lv+1)));value=1;levels+=1
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in('roll','random','sort','lib_rand','level'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    # sp, actor sign, target sign, Attack UP, Martial Arts, target mode, target exp, attacker exp, attacker level, seed
    profiles=[]
    for sp,az,tz,up,ma in((1,0,0,0,0),(5,0,6,0,0),(9,0,3,1,0),(14,4,4,0,1),(30,1,7,0,0),(50,0,0,1,0),(99,0,6,0,1)):
        for mode in range(6):profiles.append((sp,az,tz,up,ma,mode,60,10,20,1))
    for texp in(0,1,4,9,99):
        for aexp,alv in((0,20),(90,20),(99,20),(95,99)):profiles.append((9,0,0,0,0,0,texp,aexp,alv,1))
    profiles += [(12,0,0,0,0,0,50,40,20,seed)for seed in range(2,40)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[];r=d.at(s.battle.ABILITY_RANGE+14*AB,14)
    for p in profiles:
        sp,az,tz,up,ma,mode,texp,aexp,alv,seed=p
        outputs=[]
        for execute in(0,1):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            o.write(0x80192d8c,ATTACKER_DATA);o.uc.mem_write(ATTACKER_DATA&0x1fffff,bytes(44))
            for ptr,sign in((actor,az),(target,tz)):
                o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((128,)))
                o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((70,)))
            o.uc.mem_write((actor&0x1fffff)+0x21,bytes((aexp,alv)));o.uc.mem_write((target&0x1fffff)+0x21,bytes((texp,20)))
            o.uc.mem_write((actor&0x1fffff)+0x38,bytes((sp,)))
            o.uc.mem_write((actor&0x1fffff)+0x90,bytes((0x10 if up else 0,0x20 if ma else 0)))
            o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HH',300,300))
            o.uc.mem_write((target&0x1fffff)+0x1a,bytes((255,)*7))
            o.uc.mem_write((target&0x1fffff)+0x58,bytes(STATUS[mode]));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(STATUS[mode]))
            o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',AB))
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write((target&0x1fffff)+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws=0;levels=0
            o.call('remove');o.call('f28');o.call('finish')
            v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));a=bytes(o.uc.mem_read(ATTACKER_DATA&0x1fffff,44))
            if execute and v[0]:
                o.call('steal',target,v[0x28]);o.call('steal',actor,a[0x28])
            texp2,tlv2=o.uc.mem_read((target&0x1fffff)+0x21,2);aexp2,alv2=o.uc.mem_read((actor&0x1fffff)+0x21,2)
            outputs += [v[0],struct.unpack_from('<h',v,42)[0]&0xffff,v[0x28],a[0x28],draws]
            if execute:outputs += [texp2,aexp2,alv2,levels]
        rows.append([*p,*outputs])
    # Native header: inputs, forecast hit/accuracy/amount, execute hit/target change/draws, EXP after.
    native=[row[:10]+row[10:13]+row[15:16]+row[17:18]+row[19:24] for row in rows]
    lines=['/* Original Steal Exp 0x28 profiles; local source bytes only. */',
           'static const u8 source_steal[][%d]={'%len(native[0])]
    assert all(0<=x<256 for row in native for x in row)
    lines+=[' {'+','.join(map(str,row))+'},'for row in native];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='sp,actor sign,target sign,Attack UP,Martial Arts,target mode,target exp,attacker exp,attacker level,seed; '
                'then preview: hit,accuracy,target exp change,attacker exp change,draws; execute: the same, then '
                'target exp, attacker exp, attacker level, level-ups',
        scope='Original Steal Exp formula, finalizer and battle_action_handle_steal_exp with zero evasion; '
              'controlled RNG; the level check without stat growth (native unit_stats, checked elsewhere).'),indent=2)+'\n')
    print('Original Steal Exp profiles:',len(rows))

if __name__=='__main__':main()
