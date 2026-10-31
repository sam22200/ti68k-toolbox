#!/usr/bin/env python3
"""Bounded original status10/11 preparation, bitmap filter and Protect ordering."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

SPELLS=(30,41,51,59)


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-terminal/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(hostile='battle_formula_hit_faith_ma_x_percent',friendly='battle_formula_friendly_magic_accuracy',
        finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
        death='battle_formula_damage_hp_percent_hit_faith_ma_x_percent',quick='battle_formula_set_quick',
        upkeep='battle_status_build_upkeep_action',remove='battle_status_remove',random='battle_formula_get_random_0_7fff',protect='battle_formula_apply_target_xa_affecting_statuses')
    for k,n in names.items():bind_function(k,n,'battle',d)
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
    # Keep existing accuracy arithmetic provenance; focus new effect/validity edges.
    profiles=[(ab,5,70,70,0,0,0,0,0,0,mode,0)for ab in SPELLS for mode in range(10)]
    profiles += [(ab,ma,af,tf,up,down,shell,0,0,0,mode,gear)for ab in SPELLS
        for ma,af,tf,up,down,shell,mode,gear in
        ((99,100,100,0,0,0,0,0),(5,0,70,0,0,0,0,0),
         (5,70,70,1,1,1,0,0),(5,70,70,1,1,1,7,1))]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,ma,af,tf,up,down,shell,az,tz,female,mode,gear=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        outputs=[]
        for execute in(0,1):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            for ptr,faith,sign,sex in((actor,af,az,0),(target,tf,tz,female)):
                o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((64 if sex else 128,)))
                o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((faith,)))
            o.uc.mem_write((actor&0x1fffff)+0x37,bytes((ma,)));o.uc.mem_write((actor&0x1fffff)+0x90,bytes((4 if up else 0,)))
            o.uc.mem_write((target&0x1fffff)+0x90,bytes((2 if down else 0,)));o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HH',500,999))
            o.uc.mem_write((target&0x1fffff)+0x1a,bytes((255,)*7))
            if gear:o.uc.mem_write((target&0x1fffff)+0x1c,b'\xe8')
            status=d.at(s.battle.STATUS_SET+6*r[11],6)
            current=bytearray(5)
            if shell:current[3]|=16
            if mode in(2,3):current=bytearray(a|b for a,b in zip(current,status[1:]))
            if mode==4:current[0]|=32
            if mode==5:current[1]|=128
            if mode==6:current[3]|=1
            if mode==7:current[0]|=16
            if mode==9:current[0]|=8;current[3]|=8;current[4]|=16
            o.uc.mem_write((target&0x1fffff)+0x58,bytes(current));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(current))
            if mode==1:o.uc.mem_write((target&0x1fffff)+0x53,status[1:])
            if mode==3:o.uc.mem_write((target&0x1fffff)+0x4e,status[1:])
            if mode==8:o.uc.mem_write((target&0x1fffff)+5,b'\x04')
            o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x46,status)
            o.uc.mem_write((ability&0x1fffff)+0x12,bytes((tf,af)));o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
            o.write(0x8018f5fc,0 if execute else 2);rng=1;draws=0
            o.call('remove');o.call('death'if r[8]==14 else'quick'if r[8]==18 else'hostile');o.call('finish')
            v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));packed=0;kind=8
            if v[0]:
                if v[19]==255:kind=17;packed=255
                elif v[37]&128:kind=0;packed=struct.unpack_from('<H',v,4)[0]
                elif v[37]&64:kind=3;packed=struct.unpack_from('<H',v,6)[0]
                elif v[37]&8:
                    kind=10
                    for byte in range(5):
                        if v[27+byte]:packed=(byte<<8)|v[27+byte]
            outputs += [v[0],struct.unpack_from('<H',v,42)[0],kind,packed,draws]
        rows.append([*p,*outputs])
    upkeeps=[]
    for counter in range(4):
        for seed in(1,16,17):
            o.uc.mem_write(target&0x1fffff,bytes(448));o.uc.mem_write((target&0x1fffff)+7,bytes((counter,)))
            o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HH',0,999))
            o.uc.mem_write((target&0x1fffff)+0x58,b'\x30'+bytes(4))
            o.write(0x8018f5fc,0);rng=seed;draws=0;result=o.call('upkeep',0,target)
            v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));dc=o.uc.mem_read((target&0x1fffff)+7,1)[0]
            hp=struct.unpack_from('<H',v,6)[0]
            obj=1 if v[27]&64 else 2 if v[28]&1 else 0
            upkeeps.append([counter,seed,result,dc,hp,obj,draws])
    lines=['/* Original decisive formula/finalizer profiles; local source bytes only. */','static const u16 source_terminal[][22]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines+=['};','static const u16 source_terminal_upkeep[][7]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in upkeeps];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,upkeep=upkeeps,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        scope='Original Death/Quick/Zombie/Petrify formula and finalizer; controlled RNG, target sorting isolated. No original full inventory/reward/render/AI simulation.'),indent=2)+'\n')
    print('Original decisive profiles:',len(rows))

if __name__=='__main__':main()
