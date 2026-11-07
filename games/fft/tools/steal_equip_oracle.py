#!/usr/bin/env python3
"""Original equipment steals, formula 0x26 (Steal Helmet/Armor/Shield/Weapon/Accessory): slot choice
(nothing to steal: a forced failure), physical evasion, (SP'+zodiac(X))% hit and Maintenance
(CORE_STEAL_EQUIP)."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function

STEALS=(0x6e,0x6f,0x70,0x71,0x72)
STATUS={0:(0,0,0,0,0),1:(0,0,0,0x20,0),2:(0,0,0,0,0x10),3:(8,0,0,0,0),4:(0,0,4,0,0),5:(0,0,2,0,0)}
N=255
# source slots: head, body, accessory, right weapon, right shield, left weapon, left shield
# (natively representable: one item per hand, a shield only in the left hand)
GEAR={0:(0x90,0xb0,0xd0,0x13,N,N,0x80),   # sword + shield
      1:(N,N,N,N,N,N,N),                  # nothing: each break becomes Attack
      2:(0x91,N,N,N,N,N,N),               # helmet only, bare hands
      3:(N,0xb1,N,0x14,N,0x01,N),         # two weapons: right level 4, left level 1
      4:(N,N,N,0x01,N,0x14,N),            # two weapons: left higher (level 4)
      5:(N,N,N,0x13,N,0x01,N),            # two weapons: equal levels (1): right
      6:(N,N,N,N,N,0x02,N),               # left weapon only (right hand empty)
      7:(N,N,0xd1,N,N,N,N)}               # accessory only


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-steal-equip/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for k,n in dict(f26='battle_formula_steal_equipment',finish='battle_action_finalize_target_current_action',
            sort='battle_target_sort_list',remove='battle_status_remove',random='battle_formula_get_random_0_7fff').items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['lib_rand']=bind_function('lib_rand','rand','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    rng=1;draws=0
    def rand():
        nonlocal rng,draws
        rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1
        return rng>>17
    def external(uc,address,size,data):
        value=0
        if address in(s.FUNCTIONS['random'][0],s.FUNCTIONS['lib_rand'][0]):value=rand()
        elif address==s.FUNCTIONS['roll'][0]:
            bound=uc.reg_read(o.reg.UC_MIPS_REG_A0);chance=uc.reg_read(o.reg.UC_MIPS_REG_A1)
            value=int((rand()*bound>>15)>=chance)
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in('roll','random','sort','lib_rand'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    # ability, sp, armed, actor sign, target sign, Attack UP, Martial Arts, target mode, gear, Maintenance, monster, seed
    profiles=[]
    for ab in STEALS:
        for pa,wp,az,tz,up,ma in((1,0,0,0,0,1),(5,1,0,6,0,0),(9,1,0,3,1,0),(14,0,4,4,0,1),(14,1,4,4,0,1),(30,1,1,7,0,0),(70,1,0,6,1,0)):
            for mode in range(6):profiles.append((ab,pa,wp,az,tz,up,ma,mode,0,0,0,1))
        for gear in GEAR:profiles.append((ab,9,6,0,0,0,0,0,gear,0,0,1))
        profiles.append((ab,9,6,0,0,0,0,0,0,1,0,1));profiles.append((ab,9,6,0,0,0,0,0,0,0,1,1))
        profiles += [(ab,12,5,0,0,0,0,0,0,0,0,seed)for seed in range(2,14)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,pa,wp,az,tz,up,ma,mode,gear,maint,monster,seed=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14);outputs=[]
        for execute in(0,1):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            for ptr,sign in((actor,az),(target,tz)):
                o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((128,)))
                o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((70,)))
            o.uc.mem_write((actor&0x1fffff)+0x38,bytes((pa,)))
            o.uc.mem_write((actor&0x1fffff)+0x90,bytes((0x10 if up else 0,0x20 if ma else 0)))
            t=target&0x1fffff
            if monster:o.uc.mem_write(t+6,bytes((128|0x20,)))
            o.uc.mem_write(t+0x28,struct.pack('<HH',300,300));o.uc.mem_write(t+0x1a,bytes(GEAR[gear]))
            o.uc.mem_write(t+0x91,bytes((0x04 if maint else 0,)))
            o.uc.mem_write(t+0x58,bytes(STATUS[mode]));o.uc.mem_write(t+0x1bb,bytes(STATUS[mode]))
            a=ability&0x1fffff
            o.uc.mem_write(a+0x30,r);o.uc.mem_write(a+0x16,struct.pack('<H',ab))
            o.uc.mem_write(a+0x18,bytes((0 if not wp else 0x13,)))
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws=0
            o.call('remove');o.call('f26');o.call('finish')
            v=bytes(o.uc.mem_read(t+0x18c,44))
            outputs += [v[0],struct.unpack_from('<h',v,42)[0]&0xffff,v[0x19],v[3]if v[0x19] else 255,draws]
        rows.append([*p,*outputs])
    # Native header (no monsters natively): inputs, preview hit and accuracy, slot mask and item, execute hit.
    native=[row[:10]+[row[12],min(row[13],255),row[14],row[15],row[17]] for row in rows if not row[10]]
    lines=['/* Original equipment steal 0x26 profiles; local source bytes only. */',
           'static const u8 source_gear[%d][7]={'%len(GEAR)]
    lines+=[' {'+','.join(map(str,GEAR[g]))+'},'for g in sorted(GEAR)];lines+=['};']
    lines+=['static const u8 source_steal_equip[][%d]={'%len(native[0])]
    assert all(0<=x<256 for row in native for x in row)
    lines+=[' {'+','.join(map(str,row))+'},'for row in native];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,gear=GEAR,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='ability,sp,armed,actor sign,target sign,Attack UP,Martial Arts,target mode,gear,Maintenance,monster,seed; '
                'then preview and execute: hit,accuracy,slot mask,item lost,draws',
        scope='Original formula 0x26 and finalizer with zero evasion and controlled RNG; the slot hardcoding '
              '(forced failure), Maintenance. Monster rows are source-only.'),indent=2)+'\n')
    print('Original equipment steal profiles:',len(rows),'native',len(native))

if __name__=='__main__':main()
