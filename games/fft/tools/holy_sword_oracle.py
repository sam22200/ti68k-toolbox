#!/usr/bin/env python3
"""Original Holy Sword formula 0x2D (battle_formula_damage_pa_times_wp_plus_y_status): PA x
(WP + Y) through the physical stages, the weapon's element, critical and draws (campaign/HOLY_SWORD.md)."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function

SKILLS=(0x9B,0x9C,0x9D,0x9E)
SWORDS=(19,24,29,33,37)        # Broad Sword, Ice/Thunder elements (24 lightning, 29 ice), knight swords
# target status modes: 0 none, 1 Protect, 2 Sleep, 3 Charging, 4 Chicken, 5 Frog
STATUS={0:(0,0,0,0,0),1:(0,0,0,0x20,0),2:(0,0,0,0,0x10),3:(8,0,0,0,0),4:(0,0,4,0,0),5:(0,0,2,0,0)}
# element modes: 0 none, 1 target half, 2 target weak, 3 target absorb, 4 target null, 5 actor strengthen
ELEMENT_MODES=6
# Mighty Sword target equipment (head, body, accessory, right weapon, right shield, left weapon,
# left shield): 0 two weapons, 1 weapon + shield, 2 none, 3 monster (full), 4 Maintenance (full),
# 5 a left-hand weapon only.
MIGHTY_EQ=((144,172,208,19,255,24,255),(144,172,208,33,255,255,128),(255,)*7,
           (144,172,208,19,255,24,255),(144,172,208,19,255,24,255),(144,172,208,255,255,24,255))


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-holy-sword/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    bind_function('f2d','battle_formula_damage_pa_times_wp_plus_y_status','battle',d)
    bind_function('f2f','battle_formula_absorb_mp_pa_times_wp','battle',d)
    bind_function('f2e','battle_formula_break_equipped_damage_pa_times_wp','battle',d)
    for k,n in dict(finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
            remove='battle_status_remove',random='battle_formula_get_random_0_7fff').items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['lib_rand']=bind_function('lib_rand','rand','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    handler=struct.unpack_from('<I',d.bat,0x8018f610-s.battle.BAT+4*0x2D)[0]
    assert handler==s.FUNCTIONS['f2d'][0]      # the retail 0x2D dispatcher entry, not a guessed helper
    assert struct.unpack_from('<I',d.bat,0x8018f610-s.battle.BAT+4*0x2F)[0]==s.FUNCTIONS['f2f'][0]
    assert struct.unpack_from('<I',d.bat,0x8018f610-s.battle.BAT+4*0x2E)[0]==s.FUNCTIONS['f2e'][0]
    rng=1;draws=0
    seed_of=lambda k:(k*0x9E3779B1&0xffffffff)|1   # spread: small xorshift seeds draw tiny first values
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
    # ability, sword item, pa, actor sign, target sign, Attack UP, target status, element mode, seed
    profiles=[]
    for ab in SKILLS:
        for sword in SWORDS:
            profiles+=[(ab,sword,pa,az,tz,up,mode,0,1)for pa,az,tz,up in((5,0,0,0),(14,0,6,1),(20,4,4,0),(31,1,7,1))for mode in range(6)]
            profiles+=[(ab,sword,18,0,0,0,0,em,1)for em in range(ELEMENT_MODES)]
        profiles+=[(ab,24,18,0,0,0,0,0,seed)for seed in range(2,80)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,sword,pa,az,tz,up,mode,em,seed=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        w=d.at(s.battle.WEAPONS+8*sword,8);element=w[6]
        outputs=[]
        for execute in(0,1):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            for ptr,sign in((actor,az),(target,tz)):
                o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((128,)))
                o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((70,)))
            o.uc.mem_write((actor&0x1fffff)+0x36,bytes((pa,)))
            o.uc.mem_write((actor&0x1fffff)+0x90,bytes((0x10 if up else 0,0)))
            o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HH',999,999))
            o.uc.mem_write((target&0x1fffff)+0x1a,bytes((255,)*7))
            o.uc.mem_write((target&0x1fffff)+0x58,bytes(STATUS[mode]));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(STATUS[mode]))
            if element and em:
                if em==5:o.uc.mem_write((actor&0x1fffff)+0x71,bytes((element,)))
                else:o.uc.mem_write((target&0x1fffff)+0x6d+{1:2,2:3,3:0,4:1}[em],bytes((element,)))
            o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.uc.mem_write((ability&0x1fffff)+0x3e,w)            # the equipped sword's weapon_data
            o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed_of(seed);draws=0
            o.call('remove');o.call('f2d');o.call('finish')
            v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,0x28))
            outputs+=[v[0],v[1],struct.unpack_from('<h',v,4)[0],struct.unpack_from('<h',v,6)[0],draws]
        rows.append([*p,*outputs])
    # Dark Sword 0x2F: sword, pa, actor sign, target sign, Attack UP, target status, target MP, seed.
    dark=[]
    for sword in SWORDS:
        dark+=[(sword,pa,az,tz,up,mode,mp,1)for pa,az,tz,up in((5,0,0,0),(14,0,6,1),(20,4,4,0),(31,1,7,1))
               for mode in range(6)for mp in(999,7)]
    dark+=[(33,18,0,0,0,0,999,seed)for seed in range(2,80)]
    dark_rows=[];r=d.at(s.battle.ABILITY_RANGE+14*0xA4,14)
    for p in dark:
        sword,pa,az,tz,up,mode,mp,seed=p;w=d.at(s.battle.WEAPONS+8*sword,8)
        for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
        o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
        o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c);o.write(0x80192d8c,actor+0x18c)
        for ptr,sign in((actor,az),(target,tz)):
            o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((128,)))
            o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((70,)))
        o.uc.mem_write((actor&0x1fffff)+0x36,bytes((pa,)))
        o.uc.mem_write((actor&0x1fffff)+0x90,bytes((0x10 if up else 0,0)))
        o.uc.mem_write((actor&0x1fffff)+0x28,struct.pack('<HHHH',500,500,10,999))
        o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HHHH',999,999,mp,999))
        o.uc.mem_write((target&0x1fffff)+0x1a,bytes((255,)*7))
        o.uc.mem_write((target&0x1fffff)+0x58,bytes(STATUS[mode]));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(STATUS[mode]))
        o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',0xA4))
        o.uc.mem_write((ability&0x1fffff)+0x3e,w)
        o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
        o.write(0x8018f5fc,0);rng=seed_of(seed);draws=0
        o.call('remove');o.call('f2f')
        v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,0x10));u=bytes(o.uc.mem_read((actor&0x1fffff)+0x18c,0x10))
        dark_rows.append([*p,v[0],v[1],*struct.unpack_from('<hhhh',v,4),u[0],*struct.unpack_from('<hhhh',u,4),draws])
    # Mighty Sword 0x2E: ability, sword, pa, actor sign, target sign, Attack UP, target
    # equipment mode (MIGHTY_EQ), element mode, seed.
    mighty=[]
    for ab in range(0xA0,0xA4):
        for sword in(24,33):
            mighty+=[(ab,sword,pa,az,tz,up,eq,0,1)for pa,az,tz,up in((5,0,0,0),(14,0,6,1),(20,4,4,0),(31,1,7,1))for eq in range(6)]
            mighty+=[(ab,sword,18,0,0,0,0,em,1)for em in range(1,ELEMENT_MODES)]
        mighty+=[(ab,24,18,0,0,0,0,0,seed)for seed in range(2,12)]
    mighty_rows=[];r_m={ab:d.at(s.battle.ABILITY_RANGE+14*ab,14)for ab in range(0xA0,0xA4)}
    for p in mighty:
        ab,sword,pa,az,tz,up,eq,em,seed=p;w=d.at(s.battle.WEAPONS+8*sword,8);element=w[6]
        for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
        o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
        o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c);o.write(0x80192d8c,actor+0x18c)
        for ptr,sign in((actor,az),(target,tz)):
            o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((128,)))
            o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((70,)))
        if eq==3:o.uc.mem_write((target&0x1fffff)+6,bytes((128|0x20,)))
        if eq==4:o.uc.mem_write((target&0x1fffff)+0x91,bytes((0x04,)))
        o.uc.mem_write((actor&0x1fffff)+0x36,bytes((pa,)))
        o.uc.mem_write((actor&0x1fffff)+0x90,bytes((0x10 if up else 0,0)))
        o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HH',999,999))
        o.uc.mem_write((target&0x1fffff)+0x1a,bytes(MIGHTY_EQ[eq]))
        if element and em:
            if em==5:o.uc.mem_write((actor&0x1fffff)+0x71,bytes((element,)))
            else:o.uc.mem_write((target&0x1fffff)+0x6d+{1:2,2:3,3:0,4:1}[em],bytes((element,)))
        o.uc.mem_write((ability&0x1fffff)+0x30,r_m[ab]);o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
        o.uc.mem_write((ability&0x1fffff)+0x3e,w)
        o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
        o.write(0x8018f5fc,0);rng=seed_of(seed);draws=0
        o.call('remove');o.call('f2e');o.call('finish')
        v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,0x20))
        mighty_rows.append([*p,v[0],v[0x19],v[3],*struct.unpack_from('<hh',v,4),draws])
    # Never evaded: flags_4 lacks 0x02 (evadeable), so the evade helper's final stage is skipped.
    # A target with an evasive accessory and shield and class evade 99, facing the attacker,
    # is always hit; the same fixture with the bit forced on does evade (the control).
    def gear_hits(ab,fn,evadeable):
        nonlocal rng,draws
        hits=0;r=bytearray(d.at(s.battle.ABILITY_RANGE+14*ab,14));w=d.at(s.battle.WEAPONS+8*33,8)
        if evadeable:r[6]|=2
        for seed in range(1,41):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c);o.write(0x80192d8c,actor+0x18c)
            for ptr in(actor,target):
                o.uc.mem_write((ptr&0x1fffff)+6,bytes((128,)));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((70,)))
            o.uc.mem_write((actor&0x1fffff)+0x36,bytes((12,)))
            t=target&0x1fffff
            o.uc.mem_write(t+0x28,struct.pack('<HHHH',999,999,999,999))
            o.uc.mem_write(t+0x1a,bytes((144,172,232,255,128,255,255)))   # helmet, armour, evasive accessory, shield
            o.uc.mem_write(t+0x43,b'\x63');o.uc.mem_write(t+0x47,b'\x01');o.uc.mem_write(t+0x48,struct.pack('<H',0x0100))
            o.uc.mem_write((ability&0x1fffff)+0x30,bytes(r));o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.uc.mem_write((ability&0x1fffff)+0x1c,bytes((100,)));o.uc.mem_write((ability&0x1fffff)+0x3e,w)
            o.uc.mem_write(t+0x18c,b'\1');o.uc.mem_write(t+0x1b6,struct.pack('<H',100))
            o.write(0x8018f5fc,0);rng=seed_of(seed);draws=0
            o.call('remove');o.call(fn)
            hits+=bytes(o.uc.mem_read(t+0x18c,1))[0]
        return hits
    evasion={hex(ab):(gear_hits(ab,fn,0),gear_hits(ab,fn,1))for ab,fn in((0x9C,'f2d'),(0x9F,'f2d'),(0xA0,'f2e'),(0xA4,'f2f'))}
    assert all(plain==40 and forced<40 for plain,forced in evasion.values()),evasion
    print('never evaded (hits of 40 with evasive gear: retail record, evadeable bit forced):',evasion)
    # Native header (14 bytes a row, the 68000 check's size limit): inputs, then execute
    # draws, damage, healing (hit is always 1 with zero evasion; the critical is redrawn).
    lines=['/* Original Holy Sword 0x2D damage/critical/element profiles; local source bytes only. */',
           'typedef struct{u8 in[9],draws;u16 damage,healing;}SwordProfile;',
           'static const SwordProfile source_sword[]={']
    lines+=[' {{%s},%d,%d,%d},'%(','.join(map(str,row[:9])),row[18],row[16],row[17])for row in rows]
    lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    # Dark Sword (its own header: the 68000 check programs' size limit): inputs, then
    # target hit, MP damage, caster MP healing, draws.
    lines=['/* Original Dark Sword 0x2F MP drain profiles; local source bytes only. */']
    # in: sword, pa, actor sign, target sign, Attack UP, target status, target MP 7 (else 999), seed
    lines+=['typedef struct{u8 in[8],hit,draws;u16 mp_damage,mp_healing;}DarkProfile;',
            'static const DarkProfile source_dark[]={']
    lines+=[' {{%s},%d,%d,%d,%d},'%(','.join(map(str,(*row[:6],int(row[6]==7),row[7]))),row[8],row[19],row[12],row[18])for row in dark_rows]
    lines+=['};']
    # Mighty Sword: inputs (ability - 0xA0), then hit, slot mask, item lost, damage, healing, draws.
    lines+=['typedef struct{u8 in[9],hit,draws,slot,lost;u16 damage,healing;}MightyProfile;',
            'static const u8 mighty_eq[6][7]={'+','.join('{'+','.join(map(str,e))+'}'for e in MIGHTY_EQ)+'};',
            'static const MightyProfile source_mighty[]={']
    lines+=[' {{%s},%d,%d,%d,%d,%d,%d},'%(','.join(map(str,(row[0]-0xA0,*row[1:9]))),row[9],row[14],row[10],
            row[11],row[12],row[13])for row in mighty_rows]
    lines+=['};']
    (out/'dark.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,dark_cases=dark_rows,mighty_cases=mighty_rows,evasion_hits_of_40=evasion,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='ability,sword item,pa,actor sign,target sign,Attack UP,target status mode,element mode,seed index (seed=(k*0x9E3779B1)|1); '
                'then preview and execute: hit,critical,hp damage,hp healing,draws',
        scope='Original 0x2D formula and finalizer with zero evasion and target_count 0 (no knockback '
              'query); controlled RNG; the equipped sword copied into weapon_data as the retail '
              'pre-formula setup does. Physical evasion is the verified Attack path.'),indent=2)+'\n')
    print('holy sword oracle: %d profiles, draws %s, healed %d'%(len(rows),
        sorted({r[18] for r in rows}),sum(1 for r in rows if r[17])))
    print('mighty sword oracle: %d profiles, hits %d, slots %s, draws %s'%(len(mighty_rows),
        sum(1 for r in mighty_rows if r[9]),sorted({(r[0],r[10],r[11]) for r in mighty_rows if r[9]}),
        sorted({r[14] for r in mighty_rows})))
    print('dark sword oracle: %d profiles, hits %d, crit %d, hp fields %s, mp damage>mp %d, healing!=damage %d'%(len(dark_rows),
        sum(r[8] for r in dark_rows),sum(r[9] for r in dark_rows),sorted({(r[10],r[11]) for r in dark_rows})[:3],
        sum(1 for r in dark_rows if r[12]>r[6]),sum(1 for r in dark_rows if r[8] and r[18]!=r[12])))


if __name__=='__main__':main()
