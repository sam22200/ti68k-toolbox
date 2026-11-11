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


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-holy-sword/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    bind_function('f2d','battle_formula_damage_pa_times_wp_plus_y_status','battle',d)
    for k,n in dict(finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
            remove='battle_status_remove',random='battle_formula_get_random_0_7fff').items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['lib_rand']=bind_function('lib_rand','rand','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    handler=struct.unpack_from('<I',d.bat,0x8018f610-s.battle.BAT+4*0x2D)[0]
    assert handler==s.FUNCTIONS['f2d'][0]      # the retail 0x2D dispatcher entry, not a guessed helper
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
    # Native header (14 bytes a row, the 68000 check's size limit): inputs, then execute
    # draws, damage, healing (hit is always 1 with zero evasion; the critical is redrawn).
    lines=['/* Original Holy Sword 0x2D damage/critical/element profiles; local source bytes only. */',
           'typedef struct{u8 in[9],draws;u16 damage,healing;}SwordProfile;',
           'static const SwordProfile source_sword[]={']
    lines+=[' {{%s},%d,%d,%d},'%(','.join(map(str,row[:9])),row[18],row[16],row[17])for row in rows]
    lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='ability,sword item,pa,actor sign,target sign,Attack UP,target status mode,element mode,seed index (seed=(k*0x9E3779B1)|1); '
                'then preview and execute: hit,critical,hp damage,hp healing,draws',
        scope='Original 0x2D formula and finalizer with zero evasion and target_count 0 (no knockback '
              'query); controlled RNG; the equipped sword copied into weapon_data as the retail '
              'pre-formula setup does. Physical evasion is the verified Attack path.'),indent=2)+'\n')
    print('holy sword oracle: %d profiles, draws %s, healed %d'%(len(rows),
        sorted({r[18] for r in rows}),sum(1 for r in rows if r[17])))


if __name__=='__main__':main()
