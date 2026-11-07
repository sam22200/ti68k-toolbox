#!/usr/bin/env python3
"""Original Gil Taking / Negotiate: two accuracy paths, one gil transfer and funds finalizer."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function

STATUS=((0,0,0,0,0),(0,0,0,0x20,0),(0,0,0,0,0x10),(8,0,0,0,0),
        (0,0,4,0,0),(0,0,2,0,0))
ATTACKER_DATA=0x80192da0


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-gil/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for k,n in dict(gil='battle_formula_steal_gil',talk='battle_formula_talk_skill_hit_ma_x_percent',
            finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
            remove='battle_status_remove',random='battle_formula_get_random_0_7fff',
            get_funds='battle_script_get_variable',set_funds='battle_script_set_variable',
            add_funds='battle_action_add_war_funds').items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    for k,n in dict(roll='main_util_roll_pass_fail',lib_rand='rand',init='main_status_init_check_data').items():
        o.function_hashes[k]=bind_function(k,n,'main',d)
    o.call('init');rng=1;draws=0;funds=0
    def rand():
        nonlocal rng,draws
        rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1
        return rng>>17
    def external(uc,address,size,data):
        nonlocal funds
        value=0
        if address in(s.FUNCTIONS['random'][0],s.FUNCTIONS['lib_rand'][0]):value=rand()
        elif address==s.FUNCTIONS['roll'][0]:
            bound=uc.reg_read(o.reg.UC_MIPS_REG_A0);chance=uc.reg_read(o.reg.UC_MIPS_REG_A1)
            value=int((rand()*bound>>15)>=chance)
        elif address==s.FUNCTIONS['get_funds'][0]:value=funds
        elif address==s.FUNCTIONS['set_funds'][0]:funds=uc.reg_read(o.reg.UC_MIPS_REG_A1)
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in('roll','random','sort','lib_rand','get_funds','set_funds'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    # ability, Speed, level, MA, actor/target signs, Attack UP, Martial Arts, target status,
    # player target, player actor, funds, seed. Bare-handed human profiles.
    profiles=[]
    for ab in(108,123):
        for sp,lv,ma,az,tz,up,martial in((1,1,1,0,0,0,0),(9,20,14,0,6,0,0),
                (14,40,31,0,3,1,0),(30,60,60,4,4,0,1),(50,99,99,1,7,1,0)):
            for mode in range(len(STATUS)):
                profiles.append((ab,sp,lv,ma,az,tz,up,martial,mode,0,1,2000,1))
        for available in(0,1,179,180,181,2000,99999998,99999999):
            for target_player,actor_player in((0,1),(1,0),(1,1),(0,0)):
                profiles.append((ab,9,20,20,0,0,0,0,0,target_player,actor_player,available,1))
        for seed in(2,3,5,17,255):profiles.append((ab,12,50,5,0,0,0,0,0,0,1,2000,seed))
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,sp,lv,ma,az,tz,up,martial,mode,tp,ap,initial,seed=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14);outputs=[]
        for execute in(0,1):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            o.write(0x80192d8c,ATTACKER_DATA);o.uc.mem_write(ATTACKER_DATA&0x1fffff,bytes(44))
            for ptr,sign,player in((actor,az,ap),(target,tz,tp)):
                at=ptr&0x1fffff
                o.uc.mem_write(at+5,bytes((0 if player else 0x10,)))
                o.uc.mem_write(at+6,b'\x80');o.uc.mem_write(at+8,struct.pack('<H',sign<<12))
                o.uc.mem_write(at+0x26,b'\x46');o.uc.mem_write(at+0x1a,bytes((255,)*7))
                o.uc.mem_write(at+0x28,struct.pack('<HH',300,300))
            a=actor&0x1fffff;t=target&0x1fffff
            o.uc.mem_write(a+0x22,bytes((lv,)));o.uc.mem_write(a+0x37,bytes((ma,sp)))
            o.uc.mem_write(a+0x90,bytes((0x10 if up else 0,0x20 if martial else 0)))
            o.uc.mem_write(t+0x58,bytes(STATUS[mode]));o.uc.mem_write(t+0x1bb,bytes(STATUS[mode]))
            o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws=0;funds=initial
            o.call('remove');o.call('gil'if ab==108 else'talk');o.call('finish')
            v=bytes(o.uc.mem_read(t+0x18c,44));av=bytes(o.uc.mem_read(ATTACKER_DATA&0x1fffff,44))
            loss=struct.unpack_from('<h',v,12)[0];gain=struct.unpack_from('<h',av,12)[0]
            if execute and v[0]:
                o.call('add_funds',target,loss&0xffffffff,255);o.call('add_funds',actor,gain&0xffffffff,255)
            outputs += [v[0],struct.unpack_from('<h',v,42)[0],loss,gain,draws,funds]
        rows.append([*p,*outputs])
    # Compact native inputs plus compared outputs. Full signed packets/provenance remain JSON.
    lines=['/* Original Gil Taking / Negotiate: local formula/finalizer bytes only. */',
           'static const struct {u32 funds,after;u16 amount;u8 ab,sp,level,ma,az,tz,up,martial,mode,tp,ap,seed,hit,chance,execute,draws;} source_gil[]={']
    for row in rows:
        p=row[:13];preview=row[13:19];executed=row[19:25]
        values=[p[11],executed[5],-preview[2],*p[:11],p[12],preview[0],min(preview[1],100),executed[0],executed[4]]
        lines.append(' {'+','.join(str(v)+('UL'if i<2 else'')for i,v in enumerate(values))+'},')
    lines+=['};'];(out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='ability,Speed,level,MA,actor/target signs,Attack UP,Martial Arts,status,player target,player actor,funds,seed; '
                'then preview and execute: hit,accuracy,target gil,caster gil,draws,funds after',
        scope='Original two formulas, target finalizer and both add_war_funds calls, zero evasion, controlled RNG; '
              'only script-variable get/set and random/sort are hooked.'),indent=2)+'\n')
    print('Original gil profiles:',len(rows))

if __name__=='__main__':main()
