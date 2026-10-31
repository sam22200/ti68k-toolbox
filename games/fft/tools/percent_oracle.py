#!/usr/bin/env python3
"""Bounded original percentage HP/MP formulas, reciprocal drain packets and RNG ordering."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

SPELLS=(42,43,47,48,73)


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-percent/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(percent='battle_formula_magic_hp_percent_damage',mp='battle_formula_absorb_mp_y_percent',
        hp='battle_formula_magic_hp_absorb',finish='battle_action_finalize_target_current_action',
        sort='battle_target_sort_list',remove='battle_status_remove',random='battle_formula_get_random_0_7fff')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    rng=1;draws=0
    def rand():
        nonlocal rng,draws
        rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1
        return rng>>17
    def external(uc,address,size,data):
        value=0
        if address==s.FUNCTIONS['random'][0]:value=rand()
        elif address==s.FUNCTIONS['roll'][0]:
            bound=uc.reg_read(o.reg.UC_MIPS_REG_A0);chance=uc.reg_read(o.reg.UC_MIPS_REG_A1)
            value=int((rand()*bound>>15)>=chance)
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in('roll','random','sort'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    # ab,MA,actorFaith,targetFaith,MAUp,MDUp,Shell,signA,signT,female,
    # undead,maxHP,maxMP,currentHP,currentMP,accessory,element_traits
    profiles=[]
    for ab in SPELLS:
        profiles += [(ab,*p)for p in (
            (5,70,70,0,0,0,0,0,0,0,999,999,500,500,0,0),
            (0,100,100,0,0,0,0,0,0,0,1,1,1,0,0,0),
            (99,100,100,1,1,1,0,3,0,0,999,999,1,0,0,0),
            (5,10,10,0,0,0,0,0,0,0,52,7,1,0,0,0),
            (5,0,70,0,0,0,0,0,0,0,999,999,500,500,0,0),
            (5,70,70,0,0,0,0,0,0,1,999,999,500,500,0,0),
            (5,70,70,1,1,1,0,3,0,0,52,7,1,0,0,0),
            (5,100,100,0,0,0,0,0,0,0,999,999,500,500,1,0),
            (5,70,70,0,0,0,0,6,1,0,52,52,50,1,0,0),
            (5,70,70,0,0,0,12,0,0,0,3,3,1,1,0,0))]
    profiles += [(73,5,70,70,0,0,0,0,0,0,0,999,999,500,500,0,traits)
                 for traits in (1,2,4,8,12,16)]
    profiles += [(47,5,100,100,0,0,0,0,0,0,0,999,0,500,0,0,0)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,ma,af,tf,up,down,shell,az,tz,female,undead,mhp,mmp,hp,mp,gear,traits=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        outputs=[]
        for execute in(0,1):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d8c,actor+0x18c);o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            for ptr,faith,sign,sex in((actor,af,az,0),(target,tf,tz,female)):
                o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((64 if sex else 128,)))
                o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((faith,)))
            o.uc.mem_write((actor&0x1fffff)+0x37,bytes((ma,)));o.uc.mem_write((actor&0x1fffff)+0x90,bytes((4 if up else 0,)))
            o.uc.mem_write((target&0x1fffff)+0x90,bytes((2 if down else 0,)));o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<4H',hp,mhp,mp,mmp))
            o.uc.mem_write((target&0x1fffff)+0x1a,bytes((255,)*7))
            if gear:o.uc.mem_write((target&0x1fffff)+0x1c,b'\xe8')
            current=bytearray(5)
            if shell:current[3]|=16
            if undead:current[0]|=16
            o.uc.mem_write((target&0x1fffff)+0x58,bytes(current));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(current))
            o.uc.mem_write((actor&0x1fffff)+0x71,bytes((r[7]if traits&16 else 0,)))
            o.uc.mem_write((target&0x1fffff)+0x6d,bytes(r[7]if traits&(1<<i)else 0 for i in range(4)))
            status=d.at(s.battle.STATUS_SET+6*r[11],6)
            o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x46,status)
            o.uc.mem_write((ability&0x1fffff)+0x12,bytes((tf,af)));o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
            o.write(0x8018f5fc,0 if execute else 2);rng=1;draws=0
            o.call('remove');o.call('percent'if r[8]==9 else'mp'if r[8]==15 else'hp');o.call('finish')
            v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44))
            av=bytes(o.uc.mem_read((actor&0x1fffff)+0x18c,44))
            outputs += [v[0],struct.unpack_from('<H',v,42)[0],*struct.unpack_from('<4H',v,4),
                        av[0],*struct.unpack_from('<4H',av,4),draws]
        rows.append([*p,*outputs])
    lines=['/* Generated unchanged original percent/drain formulas; controlled RNG. */',
           'static const u16 source_percent[][41]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        scope='Original formulas09/0f/10, finalizer and paired caster result packets. Controlled native-equivalent RNG getter and percentage roll; sorting isolated. No original full cast/render/commit/reaction/AI trace.'),indent=2)+'\n')
    print('Original percent/drain:',len(rows),'profiles')


if __name__=='__main__':main()
