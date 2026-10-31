#!/usr/bin/env python3
"""Original Punch Art support formulas 0x33 (Stigma Magic), 0x34 (Chakra) and 0x35 (Revive) with the finalizer (CORE_PUNCH_ART)."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function

ARTS=(105,106,107)


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-punch-art/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names={0x33:'battle_formula_hit_pa_x_percent',0x34:'battle_formula_heal_pa_times_y_and_mp',
           0x35:'battle_formula_heal_y_percent_hit_pa_x_percent'}
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
    # pa, attacker/target signs, target sex, Attack UP, Martial Arts, attacker
    # (0 none,1 Berserk,2 Frog), target mode, max HP, max MP. Target modes:
    # 0 alive, 1 alive with the cancellable set, 2 KO, 3 KO Undead, 4 alive
    # Undead, 5 Petrify (with the set), 6 Death Sentence/Reraise only (no set bit).
    profiles=[]
    for ab in ARTS:
        modes=(0,1,4,5,6)if ab==105 else(0,2,3,4)if ab==107 else(0,2,4)
        for mode in modes:
            profiles += [(ab,pa,az,tz,0,up,ma,ctl,mode,mh,mm)for pa,az,tz,up,ma,ctl,mh,mm in
                ((5,0,0,0,0,0,57,30),(14,0,6,0,0,0,401,12),(14,0,3,1,0,0,401,99),(20,4,4,0,1,0,999,999),
                 (99,0,0,1,0,0,999,999),(99,0,6,0,1,0,999,999),(30,1,7,0,0,1,250,77),(30,1,7,0,0,2,250,77))]
        profiles += [(ab,40,2,8,1,0,0,0,1 if ab==105 else 2 if ab==107 else 0,333,61)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,pa,az,tz,female,up,ma,ctl,mode,mh,mm=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        status=d.at(s.battle.STATUS_SET+6*r[11],6)
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
            o.uc.mem_write((actor&0x1fffff)+0x58,bytes((0,0,8 if ctl==1 else 2 if ctl==2 else 0,0,0)))
            dead=mode in(2,3)
            o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HHHH',0 if dead else mh//3+1,mh,mm//4,mm))
            o.uc.mem_write((target&0x1fffff)+0x1a,bytes((255,)*7))
            current=bytearray(5)
            if mode in(1,5):current=bytearray(status[1:])
            if mode==5:current[1]|=0x80
            if dead:current[0]|=0x20
            if mode in(3,4):current[0]|=0x10
            if mode==6:current[2]|=0x20;current[4]|=0x01
            o.uc.mem_write((target&0x1fffff)+0x58,bytes(current));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(current))
            o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x46,status)
            o.uc.mem_write((ability&0x1fffff)+0x12,bytes((70,70)));o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
            o.write(0x8018f5fc,0 if execute else 2);rng=1;draws=0
            o.call('remove');o.call('f%02x'%r[8]);o.call('finish')
            v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44))
            outputs += [v[0],struct.unpack_from('<h',v,42)[0]&0xffff,*struct.unpack_from('<hhh',v,4),
                        struct.unpack_from('<h',v,10)[0],v[37],*v[32:37],draws]
        rows.append([*p,*outputs])
    # Native header: reachable rows (no Berserk/Frog actor) and the compared columns:
    # inputs, forecast hit/accuracy, execute hit/hp damage/hp healing/mp healing/type/removal[5]/draws.
    native=[row[:13]+row[24:25]+row[26:28]+row[29:]for row in rows if not row[7]]
    lines=['/* Original Punch Art 0x33/0x34/0x35 formula/finalizer profiles; local source bytes only. */',
           'static const s16 source_punch[][%d]={'%len(native[0])]
    lines+=[' {'+','.join(map(str,row))+'},'for row in native];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='ability,pa,actor sign,target sign,target female,Attack UP,Martial Arts,actor control,target mode,max HP,max MP; '
                'then forecast and execute: hit,accuracy,hp damage,hp healing,mp damage,mp healing,attack type,removal[5],draws',
        scope='Original Stigma Magic/Chakra/Revive formulas and finalizer; controlled RNG. No original area targeting/AI/render simulation.'),indent=2)+'\n')
    print('Original punch profiles:',len(rows))

if __name__=='__main__':main()
