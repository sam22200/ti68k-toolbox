#!/usr/bin/env python3
"""Original formula50 physical-status accuracy and Secret Fist finalization."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

AB=104
# No attacker Frog/Berserk cases: neither can select this immediate command.
STATUS=((),(26,),(35,),(4,),(21,),(22,),(27,),(32,),(33,),(39,),(),(),(3,),
        (26,35,21),(26,4,22),(35,4),(24,))


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-secret-fist/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(formula='battle_formula_hit_ma_x_percent_physical_evade',
        finish='battle_action_finalize_target_current_action',remove='battle_status_remove',
        sort='battle_target_sort_list',random='battle_formula_get_random_0_7fff')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    for k,n in dict(roll='main_util_roll_pass_fail',lib_rand='rand',init='main_status_init_check_data').items():
        o.function_hashes[k]=bind_function(k,n,'main',d)
    o.call('init');rng=1;draws=[]
    def rand():
        nonlocal rng
        rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff
        return rng>>17
    def external(uc,address,size,data):
        value=0
        if address in(s.FUNCTIONS['random'][0],s.FUNCTIONS['lib_rand'][0]):
            value=rand();draws.append(['random',value])
        elif address==s.FUNCTIONS['roll'][0]:
            bound=uc.reg_read(o.reg.UC_MIPS_REG_A0);chance=uc.reg_read(o.reg.UC_MIPS_REG_A1)
            r=rand();value=int((r*bound>>15)>=chance);draws.append([bound,chance,r])
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in('roll','random','sort','lib_rand'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    # MA, zodiac pair, target sex, actor support (0 none/1 AttackUP/2 MartialArts/
    # 3 MagicAttackUP), target support (1 DefenseUP/2 MagicDefenseUP), status mode,
    # Faith pair, equipped weapon, target evasive equipment, seed.
    profiles=[]
    for ma,az,tz,sex,up,defense in((1,0,0,0,0,0),(7,0,3,0,1,1),(14,0,6,1,2,0),
        (14,0,6,0,2,1),(31,4,4,0,3,2),(50,0,0,0,0,0),(99,0,6,1,2,1),
        (1,12,4,0,0,0),(14,4,12,0,1,1)):
        for mode in range(len(STATUS)):profiles.append((ma,az,tz,sex,up,defense,mode,0,100,1,0,1))
    # Pure support/faith/weapon controls, including boundary100 where no RNG runs.
    for ma in(1,14,49,50,99):
        for up in range(4):profiles.append((ma,0,0,0,up,0,0,100,0,up&1,0,1))
    for seed in(2,73,255,10000,32767,65535):
        for gear in(0,1):profiles.append((14,0,0,0,0,0,0,70,70,1,gear,seed))
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    raw=d.at(s.battle.ABILITY_RANGE+14*AB,14);request=d.at(s.battle.STATUS_SET+6*raw[11],6)
    for p in profiles:
        ma,az,tz,sex,up,defense,mode,af,tf,armed,gear,seed=p;outputs=[]
        for execute in(0,1):
            for ptr,sign,female,faith in((actor,az,0,af),(target,tz,sex,tf)):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448))
                o.uc.mem_write(at+6,bytes((64 if female else 128,)))
                o.uc.mem_write(at+8,struct.pack('<H',sign<<12));o.uc.mem_write(at+0x26,bytes((faith,)))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999))
                o.uc.mem_write(at+0x1a,bytes((255,)*7))
            a=actor&0x1fffff;t=target&0x1fffff;v=ability&0x1fffff
            o.uc.mem_write(a+0x37,bytes((ma,)))
            o.uc.mem_write(a+0x90,bytes((16 if up==1 else 4 if up==3 else 0,32 if up==2 else 0)))
            o.uc.mem_write(t+0x90,bytes((8 if defense==1 else 2 if defense==2 else 0,)))
            if armed:o.uc.mem_write(a+0x1d,b'\x13')
            if gear:
                o.uc.mem_write(t+0x1c,bytes((232,255,128,255,255)))
                o.uc.mem_write(t+0x43,b'\x63')  # equipment_stats[7]: class physical evade
                o.uc.mem_write(t+0x47,b'\x01')  # attacker x0 -> target x1, facing west (front)
                o.uc.mem_write(t+0x48,struct.pack('<H',0x0100))
            current=mask(*STATUS[mode]);innate=mask(39)if mode==11 else bytes(5)
            o.uc.mem_write(t+0x4e,innate+(mask(39)if mode==10 else bytes(5))+current)
            o.uc.mem_write(t+0x1bb,current)
            o.uc.mem_write(v,bytes(80));o.uc.mem_write(v+0x30,raw);o.uc.mem_write(v+0x46,request)
            o.uc.mem_write(v+0x12,bytes((tf,af)));o.uc.mem_write(v+0x16,struct.pack('<H',AB))
            # Non-weapon ability: weapon_id0 despite the attacker's real equipment.
            o.uc.mem_write(v+0x18,b'\0');o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws.clear()
            o.call('remove');o.call('formula');o.call('finish')
            action=bytes(o.uc.mem_read(t+0x18c,44))
            outputs.append(dict(hit=action[0],chance=struct.unpack_from('<H',action,42)[0],
                add=list(action[27:32]),remove=list(action[32:37]),type=action[37],draws=list(draws),
                damage=struct.unpack_from('<h',action,4)[0]))
        rows.append(dict(input=p,preview=outputs[0],execute=outputs[1]))
    lines=['/* Retail Secret Fist formula50/finalizer. No source code modification. */',
        'static const u8 source_secret_status[][5]={']
    lines+=[' {'+','.join(map(str,mask(*ids)))+'},'for ids in STATUS]
    lines+=['};','static const struct {u16 seed;u8 ma,az,tz,sex,up,defense,mode,af,tf,armed,gear,forecast,hit,draws,add;} source_secret_fist[]={']
    for row in rows:
        p=row['input'];pre=row['preview'];exe=row['execute']
        values=[p[-1],*p[:-1],pre['chance']if pre['hit']else 0,exe['hit'],len(exe['draws']),exe['add'][4]]
        assert all(0<=v<=255 for v in values[1:]),values
        lines.append(' {'+','.join(map(str,values))+'},')
    lines+=['};'];(out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),raw=raw.hex(),
        scope='Original unmodified formula50, physical evade helper, support/status/zodiac stages and target finalizer. '
              'Retail Secret Fist flags4=0 bypasses evasion draws despite the helper call. Controlled external RNG '
              'and graphical sorting hook; human targets. No source menu/area/reward/reaction/campaign simulation.'),indent=2)+'\n')
    print('Original Secret Fist profiles:',len(rows),flush=True)


if __name__=='__main__':main()
