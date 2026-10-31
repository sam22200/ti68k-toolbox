#!/usr/bin/env python3
"""Original Punch Art strikes 0x31 (Wave Fist) and 0x32 (Repeating Fist): damage, critical and draws (CORE_PUNCH_STRIKES)."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function

ARTS=(102,101,100)
# target status modes: 0 none, 1 Protect, 2 Sleep, 3 Charging, 4 Chicken, 5 Frog, 6 Protect+Sleep+Chicken
STATUS={0:(0,0,0,0,0),1:(0,0,0,0x20,0),2:(0,0,0,0,0x10),3:(8,0,0,0,0),4:(0,0,4,0,0),5:(0,0,2,0,0),6:(0,0,4,0x20,0x10)}


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-punch-strikes/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names={0x31:'battle_formula_damage_pa_plus_y_half_times_pa',0x32:'battle_formula_damage_random_x_times_pa_plus_y_half_times_pa'}
    for k,n in names.items():bind_function('f%02x'%k,n,'battle',d)
    for k,n in dict(finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
            remove='battle_status_remove',random='battle_formula_get_random_0_7fff').items():bind_function(k,n,'battle',d)
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
    # ability, pa, actor sign, target sign, target female, Attack UP, Martial Arts, target status mode, seed
    profiles=[]
    for ab in ARTS:
        profiles += [(ab,pa,az,tz,0,up,ma,mode,1)for pa,az,tz,up,ma in
            ((5,0,0,0,0),(14,0,6,0,0),(14,0,3,1,0),(20,4,4,0,1),(99,0,0,1,0),(99,0,6,0,1),(31,1,7,0,0))for mode in range(7)]
        profiles += [(ab,18,0,0,0,0,1,0,seed)for seed in range(2,90)]
        profiles += [(ab,40,2,8,1,0,0,1,1)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,pa,az,tz,female,up,ma,mode,seed=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        outputs=[]
        for execute in(0,1):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            for ptr,sign,sex in((actor,az,0),(target,tz,female)):
                o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((64 if sex else 128,)))
                o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((70,)))
            o.uc.mem_write((actor&0x1fffff)+0x36,bytes((pa,)))
            o.uc.mem_write((actor&0x1fffff)+0x90,bytes((0x10 if up else 0,0x20 if ma else 0)))
            o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HH',999,999))
            o.uc.mem_write((target&0x1fffff)+0x1a,bytes((255,)*7))
            o.uc.mem_write((target&0x1fffff)+0x58,bytes(STATUS[mode]));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(STATUS[mode]))
            o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws=0
            o.call('remove');o.call('f%02x'%r[8]);o.call('finish')
            v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));mult=o.uc.mem_read((ability&0x1fffff)+0x2a,1)[0]
            outputs += [v[0],v[1],struct.unpack_from('<h',v,4)[0],mult,draws]
        rows.append([*p,*outputs])
    # Native header (68000 check program limit): inputs and execute crit/damage/multiplier/draws;
    # the seed sweep keeps its criticals and a few ordinary draws.
    native=[row[:9]+row[15:19]for row in rows if row[8]<12 or row[15]]
    lines=['/* Original Punch Art 0x31/0x32 damage/critical profiles; local source bytes only. */',
           'static const s16 source_strike[][%d]={'%len(native[0])]
    lines+=[' {'+','.join(map(str,row))+'},'for row in native];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='ability,pa,actor sign,target sign,target female,Attack UP,Martial Arts,target status mode,seed; '
                'then preview and execute: hit,critical,hp damage,random multiplier,draws',
        scope='Original Wave Fist/Repeating Fist formulas and finalizer with zero evasion and target_count 0 '
              '(no knockback query); controlled RNG. Physical evasion is the verified Attack path.'),indent=2)+'\n')
    print('Original strike profiles:',len(rows),'criticals',sum(1 for r in rows if r[-4]))

if __name__=='__main__':main()
