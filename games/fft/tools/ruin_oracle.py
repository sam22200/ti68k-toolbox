#!/usr/bin/env python3
"""Original Ruins (Arc Knight): Speed/Power/Mind Ruin formula 0x1A
(battle_formula_magical_stat_reduction) and Magic Ruin 0x1B (battle_formula_damage_mp_percent):
magical evasion, the (MA + X or Y)% Faith chance, the stat loss or the max-MP percent
(campaign/RUINS.md)."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function

SKILLS=(0xC4,0xC5,0xC6,0xC7)
# target status modes: 0 none, 1 Shell, 2 Frog, 3 Chicken
STATUS={0:(0,0,0,0,0),1:(0,0,0,0x10,0),2:(0,0,2,0,0),3:(0,0,4,0,0)}


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-ruin/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    bind_function('f1a','battle_formula_magical_stat_reduction','battle',d)
    bind_function('f1b','battle_formula_damage_mp_percent','battle',d)
    for k,n in dict(finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
            remove='battle_status_remove',random='battle_formula_get_random_0_7fff').items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['lib_rand']=bind_function('lib_rand','rand','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    table=lambda f:struct.unpack_from('<I',d.bat,0x8018f610-s.battle.BAT+4*f)[0]
    assert table(0x1A)==s.FUNCTIONS['f1a'][0] and table(0x1B)==s.FUNCTIONS['f1b'][0]   # retail dispatch entries
    rng=1;draws=0
    seed_of=lambda k:(k*0x9E3779B1&0xffffffff)|1
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
    # ability, MA, actor Faith, target Faith, actor sign, target sign, Magic Attack UP,
    # target Magic Defense UP, target status, target max MP / 10, seed
    profiles=[]
    for ab in SKILLS:
        for ma,fa,ft,az,tz in((5,100,100,0,0),(12,70,40,0,6),(20,30,20,4,4),(31,10,5,1,7),(8,0,90,2,3),(15,50,0,5,11)):
            profiles+=[(ab,ma,fa,ft,az,tz,up,dn,mode,37,1)for up in(0,1)for dn in(0,1)for mode in range(4)]
        profiles+=[(ab,9,40,30,0,0,0,0,0,mp,seed)for mp in(0,1,99)for seed in range(2,14)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,ma,fa,ft,az,tz,up,dn,mode,mp10,seed=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
        o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
        o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c);o.write(0x80192d8c,actor+0x18c)
        for ptr,sign,faith in((actor,az,fa),(target,tz,ft)):
            o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((128,)))
            o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((faith,)))
        o.uc.mem_write((actor&0x1fffff)+0x36,bytes((10,ma,8)))
        o.uc.mem_write((target&0x1fffff)+0x36,bytes((12,11,7)))
        o.uc.mem_write((actor&0x1fffff)+0x90,bytes((0x04 if up else 0,)))
        o.uc.mem_write((target&0x1fffff)+0x90,bytes((0x02 if dn else 0,)))
        o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HHHH',999,999,mp10*10,mp10*10))
        o.uc.mem_write((target&0x1fffff)+0x1a,bytes((255,)*7))
        o.uc.mem_write((target&0x1fffff)+0x58,bytes(STATUS[mode]));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(STATUS[mode]))
        o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
        o.uc.mem_write((ability&0x1fffff)+0x1c,bytes((100,)))   # base hit, as the pre-formula setup
        o.uc.mem_write((ability&0x1fffff)+0x12,bytes((ft,fa)))   # the setup's target/attacker Faith copies
        o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
        o.write(0x8018f5fc,0);rng=seed_of(seed);draws=0
        o.call('remove');o.call('f1b' if ab==0xC4 else 'f1a');o.call('finish')
        v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,0x30))
        rows.append([*p,v[0],struct.unpack_from('<h',v,8)[0],v[0x12],v[0x14],v[0x15],
                     struct.unpack_from('<h',v,0x2a)[0],draws])
    # in: ability - 0xC4, MA, Faiths, signs, supports, status, max MP / 10, seed;
    # out: hit, MP damage, stat change (the record's X), accuracy, draws
    lines=['/* Original Ruin 0x1A/0x1B profiles; local source bytes only. */',
           'typedef struct{u8 in[11],hit,change,draws;u16 mp_damage;s16 accuracy;}RuinProfile;',
           'static const RuinProfile source_ruin[]={']
    lines+=[' {{%s},%d,%d,%d,%d,%d},'%(','.join(map(str,(r[0]-0xC4,*r[1:11]))),r[11],max(r[13],r[14],r[15]),
            r[17],r[12],r[16])for r in rows]
    lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='ability,MA,actor Faith,target Faith,actor sign,target sign,Magic Attack UP,'
                'target Magic Defense UP,target status mode,target max MP/10,seed index '
                '(seed=(k*0x9E3779B1)|1); then hit,MP damage,SP change,PA change,MA change,'
                'accuracy,draws',
        scope='Original 0x1A/0x1B formulas and finalizer, no target equipment (zero magical '
              'evasion), controlled RNG.'),indent=2)+'\n')
    print('ruin oracle: %d profiles, hits %d, draws %s, accuracy %s, changes %s, mp %s'%(len(rows),
        sum(r[11] for r in rows),sorted({r[17] for r in rows}),sorted({r[16] for r in rows})[:8],
        sorted({(r[0],r[13],r[14],r[15]) for r in rows if r[11]}),sorted({r[12] for r in rows})))


if __name__=='__main__':main()
