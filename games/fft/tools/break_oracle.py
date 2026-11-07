#!/usr/bin/env python3
"""Original stat breaks, formula 0x2b (Speed/Power/Mind Break): physical evasion, (PA'+Y)% hit and
the X loss through the finalizer's clamp (CORE_BREAKS)."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function

BREAKS=(0x8f,0x90,0x91)
# target modes: 0 none, 1 Protect, 2 Sleep, 3 Charging, 4 Chicken, 5 Frog
STATUS={0:(0,0,0,0,0),1:(0,0,0,0x20,0),2:(0,0,0,0,0x10),3:(8,0,0,0,0),4:(0,0,4,0,0),5:(0,0,2,0,0)}


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-breaks/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for k,n in dict(f2b='battle_formula_physical_stat_reduction',finish='battle_action_finalize_target_current_action',
            sort='battle_target_sort_list',remove='battle_status_remove',random='battle_formula_get_random_0_7fff',
            stat='battle_unit_apply_stat_increment_decrement').items():bind_function(k,n,'battle',d)
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
    # ability, pa, actor sign, target sign, Attack UP, Martial Arts, armed, target mode, target stat, seed
    profiles=[]
    for ab in BREAKS:
        for pa,az,tz,up,ma,armed in((1,0,0,0,0,1),(5,0,6,0,0,1),(9,0,3,1,0,1),(14,4,4,0,1,0),(14,4,4,0,1,1),
                                    (30,1,7,0,0,1),(50,0,0,1,0,0),(70,0,6,0,1,0)):
            for mode in range(6):profiles.append((ab,pa,az,tz,up,ma,armed,mode,20,1))
        for stat in(1,2,3,4,50):profiles.append((ab,9,0,0,0,0,1,0,stat,1))
        profiles += [(ab,12,0,0,0,0,1,0,20,seed)for seed in range(2,20)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    # finalizer offsets: target data sp/pa/ma change; unit base sp/pa/ma (as Guts, no equipment bonus)
    change={0x8f:(0x12,0x32,50),0x90:(0x14,0x30,99),0x91:(0x15,0x31,99)}
    for p in profiles:
        ab,pa,az,tz,up,ma,armed,mode,stat,seed=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        off,base,cap=change[ab];outputs=[]
        for execute in(0,1):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            for ptr,sign in((actor,az),(target,tz)):
                o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((128,)))
                o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((70,)))
            o.uc.mem_write((actor&0x1fffff)+0x36,bytes((pa,)))
            o.uc.mem_write((actor&0x1fffff)+0x90,bytes((0x10 if up else 0,0x20 if ma else 0)))
            t=target&0x1fffff
            o.uc.mem_write(t+0x28,struct.pack('<HH',300,300));o.uc.mem_write(t+0x1a,bytes((255,)*7))
            o.uc.mem_write(t+0x30,bytes((stat,stat,stat,0,0,0,stat,stat,stat)))
            o.uc.mem_write(t+0x58,bytes(STATUS[mode]));o.uc.mem_write(t+0x1bb,bytes(STATUS[mode]))
            a=ability&0x1fffff
            o.uc.mem_write(a+0x30,r);o.uc.mem_write(a+0x16,struct.pack('<H',ab));o.uc.mem_write(a+0x18,bytes((1 if armed else 0,)))
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws=0
            o.call('remove');o.call('f2b');o.call('finish')
            v=bytes(o.uc.mem_read(t+0x18c,44))
            if execute and v[0]:o.call('stat',v[off],target+base,cap,1)
            outputs += [v[0],struct.unpack_from('<h',v,42)[0]&0xffff,v[off],draws]
            if execute:outputs.append(o.uc.mem_read(t+base,1)[0])
        rows.append([*p,*outputs])
    # Native header: inputs, preview accuracy and change, execute hit and stat after.
    native=[row[:10]+[min(row[11],255),row[12],row[14],row[18]] for row in rows]
    lines=['/* Original stat break 0x2b profiles; local source bytes only. */',
           'static const u8 source_breaks[][%d]={'%len(native[0])]
    assert all(0<=x<256 for row in native for x in row)
    lines+=[' {'+','.join(map(str,row))+'},'for row in native];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='ability,pa,actor sign,target sign,Attack UP,Martial Arts,armed,target mode,target stat,seed; '
                'then preview: hit,accuracy,stat change,draws; execute: hit,accuracy,stat change,draws,stat after',
        scope='Original formula 0x2b, finalizer and the stat decrement clamp with zero evasion; controlled RNG; '
              'no Two Hands weapon (not modelled natively).'),indent=2)+'\n')
    print('Original stat break profiles:',len(rows))

if __name__=='__main__':main()
