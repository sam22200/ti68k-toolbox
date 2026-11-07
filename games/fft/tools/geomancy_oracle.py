#!/usr/bin/env python3
"""Original Geomancy formula, elemental/status filtering and terrain selector."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-geomancy/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(damage='battle_formula_ma_pa_half_damage',finish='battle_action_finalize_target_current_action',
        sort='battle_target_sort_list',remove='battle_status_remove',random='battle_formula_get_random_0_7fff',
        variable='battle_script_get_variable',terrain='battle_action_get_elemental_ability_id')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    for k,n in dict(roll='main_util_roll_pass_fail',lib_rand='rand',init='main_status_init_check_data').items():
        o.function_hashes[k]=bind_function(k,n,'main',d)
    o.call('init');rng=1;draws=0;weather=0
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
        elif address==s.FUNCTIONS['variable'][0]:
            assert uc.reg_read(o.reg.UC_MIPS_REG_A0)==0x23;value=weather
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in('roll','random','sort','lib_rand','variable'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0
    # Run the actual unit-location + terrain-table selector on all64 surface IDs.
    o.uc.mem_write(actor&0x1fffff,bytes(448));terrain=[]
    for layer in range(2):
        for surface in range(64):
            o.uc.mem_write((actor&0x1fffff)+0x48,struct.pack('<H',layer<<15))
            o.uc.mem_write((s.TILES&0x1fffff)+2048*layer,bytes((surface,)))
            terrain.append(o.call('terrain',actor))
    assert terrain[:64]==terrain[64:]==list(d.at(0x8005e9d0,64)),terrain
    # ab,PA,MA,Faith pair,zodiac pair,target sex,MAUp,MDUp,status mode,affinity,weather,seed.
    profiles=[]
    for ab in range(126,138):
        for mode in range(11):profiles.append((ab,13,17,0,100,0,0,0,0,0,mode,0,0,1))
        for pa,ma,az,tz,sex,up,defense in((1,1,0,3,0,0,1),(99,99,0,6,1,1,0),
                (14,31,0,6,0,1,1),(20,20,12,4,0,0,0)):
            profiles.append((ab,pa,ma,70,30,az,tz,sex,up,defense,0,0,0,1))
        for seed in(10000,32767,65535):profiles.append((ab,13,17,100,0,0,0,0,0,0,0,0,0,seed))
        if d.at(s.battle.ABILITY_RANGE+14*ab+7,1)[0]:
            for affinity in range(1,6):profiles.append((ab,13,17,70,70,0,0,0,0,0,0,affinity,0,1))
            for w in(3,4):profiles.append((ab,13,17,70,70,0,0,0,0,0,4,0,w,1))
    rows=[]
    for p in profiles:
        ab,pa,ma,af,tf,az,tz,sex,up,defense,mode,affinity,w,seed=p
        r=d.at(s.battle.ABILITY_RANGE+14*ab,14);request=d.at(s.battle.STATUS_SET+6*r[11],6);outputs=[]
        for execute in(0,1):
            for ptr,sign,female,faith in((actor,az,0,af),(target,tz,sex,tf)):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+6,bytes((64 if female else 128,)))
                o.uc.mem_write(at+8,struct.pack('<H',sign<<12));o.uc.mem_write(at+0x26,bytes((faith,)))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999));o.uc.mem_write(at+0x1a,bytes((255,)*7))
            a=actor&0x1fffff;t=target&0x1fffff;o.uc.mem_write(a+0x36,bytes((pa,ma)))
            o.uc.mem_write(a+0x90,bytes((4 if up else 0,)));o.uc.mem_write(t+0x90,bytes((2 if defense else 0,)))
            ids={1:27,2:22,3:21,4:16,5:17,6:35,7:28}
            inflicted=mask(ids[mode])if mode in ids else request[1:]if mode in(9,10)else bytes(5)
            innate=request[1:]if mode==10 else bytes(5);immune=request[1:]if mode==8 else bytes(5)
            o.uc.mem_write(t+0x4e,innate+immune+bytes(a|b for a,b in zip(innate,inflicted)))
            o.uc.mem_write(t+0x1bb,inflicted)
            o.uc.mem_write(t+0x6d,bytes(r[7]if affinity==k else 0 for k in range(1,6)))
            if affinity==5:o.uc.mem_write(a+0x71,bytes((r[7],)))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write((ability&0x1fffff)+0x30,r)
            o.uc.mem_write((ability&0x1fffff)+0x46,request);o.uc.mem_write((ability&0x1fffff)+0x12,bytes((tf,af)))
            o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);o.write(0x800b6698,0);weather=w;rng=seed;draws=0
            o.call('remove');o.call('damage');o.call('finish');v=bytes(o.uc.mem_read(t+0x18c,44))
            damage,healing=struct.unpack_from('<hh',v,4)
            outputs.append(dict(hit=v[0],chance=struct.unpack_from('<H',v,42)[0],damage=damage,healing=healing,
                add=list(v[27:32]),remove=list(v[32:37]),draws=draws,type=v[37]))
        rows.append(dict(input=p,preview=outputs[0],execute=outputs[1]))
    lines=['/* Original retail Geomancy formula/finalizer and surface selector. */',
        'static const u8 source_terrain[64]={'+','.join(map(str,terrain[:64]))+'};',
        'static const struct {u16 seed,amount;u8 ab,pa,ma,af,tf,az,tz,sex,up,defense,mode,affinity,weather,hit,kind,draws,proc,oil;} source_geomancy[]={']
    for index,row in enumerate(rows):
        if index==0:lines.append('#if !defined(GEOMANCY_PROFILE_HALF) || GEOMANCY_PROFILE_HALF == 0')
        if index==len(rows)//2:
            lines.extend(['#endif','#if !defined(GEOMANCY_PROFILE_HALF) || GEOMANCY_PROFILE_HALF == 1'])
        p=row['input'];v=row['execute'];kind=3 if v['type']&64 else 0
        amount=v['healing']if kind==3 else v['damage']
        values=[p[13],amount&65535,*p[:13],v['hit'],kind,v['draws'],int(any(v['add'])or any(x&y for x,y in zip(v['remove'],d.at(s.battle.STATUS_SET+6*d.at(s.battle.ABILITY_RANGE+14*p[0]+11,1)[0]+1,5)))),int(bool(v['remove'][2]&128))]
        lines.append(' {'+','.join(map(str,values))+'},')
    lines+=['#endif','};'];(out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,terrain=terrain,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        scope='Unmodified original formula24 and post-formula target finalizer, including elemental/status filtering and controlled RNG; '
              'actual terrain selector on both source layers. No original area traversal, reaction dispatch or campaign simulation.'),indent=2)+'\n')
    print('Original Geomancy profiles:',len(rows),'terrain selectors:',len(terrain))


if __name__=='__main__':main()
