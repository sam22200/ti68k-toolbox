#!/usr/bin/env python3
"""Original Talk Skill formula 0x2a: accuracy, Brave/Faith/CT changes and statuses (CORE_TALK_SKILL)."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function

TALK=(117,118,119,120,121,122,124,125)


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-talk-skill/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for k,n in dict(talk='battle_formula_talk_skill_hit_ma_x_percent',upkeep='battle_status_build_upkeep_action',finish='battle_action_finalize_target_current_action',
            sort='battle_target_sort_list',remove='battle_status_remove',random='battle_formula_get_random_0_7fff').items():
        bind_function(k,n,'battle',d)
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
    # ability, ma, actor sign, target sign, target female, Magic AttackUP, target mode, seed
    # target modes: 0 plain, 1 Sleep, 2 Berserk already, 3 Berserk immune, 4 Sleep immune
    profiles=[]
    for ab in TALK:
        profiles += [(ab,ma,az,tz,0,up,mode,1)for ma,az,tz,up in
            ((5,0,0,0),(14,0,6,0),(14,0,3,1),(31,4,4,0),(60,1,7,0),(99,0,0,1))for mode in range(5)]
        profiles += [(ab,20,0,0,0,0,0,seed)for seed in range(2,14)]
        profiles += [(ab,40,2,8,1,0,0,1)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,ma,az,tz,female,up,mode,seed=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        status=d.at(s.battle.STATUS_SET+6*r[11],6)
        outputs=[]
        for execute in(0,1):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            for ptr,sign,sex in((actor,az,0),(target,tz,female)):
                o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((64 if sex else 128,)))
                o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((70,)))
            o.uc.mem_write((actor&0x1fffff)+0x37,bytes((ma,)));o.uc.mem_write((actor&0x1fffff)+0x90,bytes((4 if up else 0,)))
            o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HH',300,300))
            o.uc.mem_write((target&0x1fffff)+0x1a,bytes((255,)*7))
            current=bytearray(5)
            if mode==1:current[4]|=0x10
            if mode==2:current[2]|=0x08
            o.uc.mem_write((target&0x1fffff)+0x58,bytes(current));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(current))
            if mode==3:o.uc.mem_write((target&0x1fffff)+0x53+2,b'\x08')
            if mode==4:o.uc.mem_write((target&0x1fffff)+0x53+4,b'\x10')
            o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x46,status)
            o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws=0
            o.call('remove');o.call('talk');o.call('finish')
            v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44))
            outputs += [v[0],struct.unpack_from('<h',v,42)[0]&0xffff,v[19],v[22],v[23],v[37],*v[27:32],draws]
        rows.append([*p,*outputs])
    # Death Sentence upkeep (timer slot 15 at +0x6c): undead, counter, Brave.
    upkeeps=[]
    for undead in(0,1):
        for counter in(1,2,3):
            for brave in(5,50):
                o.uc.mem_write(target&0x1fffff,bytes(448));o.uc.mem_write((target&0x1fffff)+0x24,bytes((brave,)))
                o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HH',300,300))
                cur=bytes((0x10 if undead else 0,0,0,0,1))
                o.uc.mem_write((target&0x1fffff)+0x58,cur);o.uc.mem_write((target&0x1fffff)+0x6c,bytes((counter,)))
                o.write(0x8018f5fc,0);rng=1;draws=0;result=o.call('upkeep',0,target)
                v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));left=o.uc.mem_read((target&0x1fffff)+0x6c,1)[0]
                upkeeps.append([undead,counter,brave,result,v[27],v[36],v[22],left,draws])
    # Native header: inputs, forecast hit/accuracy, execute hit/ct/brave/faith/type/infliction[5]/draws.
    native=[row[:10]+row[20:]for row in rows if row[7]<6]   # every field fits a byte; fewer seeds
    lines=['/* Original Talk Skill 0x2a formula/finalizer profiles; local source bytes only. */',
           'static const u8 source_talk[][%d]={'%len(native[0])]
    assert all(0<=v<256 for row in native for v in row)
    lines+=[' {'+','.join(map(str,row))+'},'for row in native];lines+=['};','static const u8 source_sentence_upkeep[][9]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in upkeeps];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,sentence_upkeep=upkeeps,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='ability,ma,actor sign,target sign,target female,Magic AttackUP,target mode,seed; '
                'then forecast and execute: hit,accuracy,ct change,brave change,faith change,attack type,infliction[5],draws',
        scope='Original Talk Skill formula and finalizer; controlled RNG; human targets without Finger Guard.'),indent=2)+'\n')
    print('Original talk profiles:',len(rows))

if __name__=='__main__':main()
