#!/usr/bin/env python3
"""Actual source Cure/Raise arithmetic, status eligibility and finalizer, in batches."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-healing/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(cure='battle_formula_magical_heal',raise_='battle_formula_heal_y_percent_faith',
        finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
        remove='battle_status_remove')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rolls=[]
    def external(uc,address,size,data):
        value=0
        if address==s.FUNCTIONS['roll'][0]:
            pair=[uc.reg_read(o.reg.UC_MIPS_REG_A0),uc.reg_read(o.reg.UC_MIPS_REG_A1)]
            rolls.append(pair);value=int(pair[1]==0)
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in('roll','sort'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    spells=(1,2,3,4,5,6,60,72);profiles=[]
    # ab,MA,actorFaith,targetFaith,MAUp,MDUp,Shell,undead,dead,innateDead,
    # petrify,wall,actorFaithMode,targetFaithMode,actorSign,targetSign,female,execute
    for ab in spells:
        for ma,af,tf in((0,100,100),(5,70,70),(99,100,100)):
            profiles.append((ab,ma,af,tf,0,0,0,0,int(ab in(5,6)),0,0,0,0,0,0,0,0,0))
        for undead,dead in((0,0),(0,1),(1,0),(1,1)):
            profiles.append((ab,5,70,70,1,1,1,undead,dead,0,0,0,0,0,0,0,0,1))
    for ab in(1,5):
        for mode in(1,2,3):
            profiles.append((ab,5,40,60,0,0,0,0,int(ab==5),0,0,0,mode,mode,0,0,0,1))
        for az,tz,female in((0,3,0),(0,6,0),(0,6,1),(12,0,0),(0,12,0)):
            profiles.append((ab,5,70,70,1,0,0,0,int(ab==5),0,0,0,0,0,az,tz,female,1))
    profiles += [(5,5,100,100,0,0,0,0,1,inn,pet,wall,0,0,0,0,0,0)
                 for inn,pet,wall in((1,0,0),(0,1,0),(0,0,1))]
    rows=[]
    for p in profiles:
        ab,ma,af,tf,up,down,shell,undead,dead,inn,pet,wall,am,tm,az,tz,female,execute=p
        for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
        o.uc.mem_write(ability&0x1fffff,bytes(0x70));o.uc.mem_write(0x193860,bytes(5))
        o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
        for ptr,faith,sign,sex,mode in((actor,af,az,0,am),(target,tf,tz,female,tm)):
            o.uc.mem_write((ptr&0x1fffff)+1,b'\x00');o.uc.mem_write((ptr&0x1fffff)+6,bytes((64 if sex else 128,)))
            o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((faith,)))
            o.uc.mem_write((ptr&0x1fffff)+0x5c,bytes(((128 if mode&1 else 0)|(64 if mode&2 else 0),)))
        bits=bytearray(mask(*([2]if dead else[]),*([3]if undead else[]),*([8]if pet else[]),*([31]if wall else[]),*([27]if shell else[])))
        bits[4]|=(128 if tm&1 else 0)|(64 if tm&2 else 0)
        o.uc.mem_write((target&0x1fffff)+0x58,bytes(bits));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(bits))
        if inn:o.uc.mem_write((target&0x1fffff)+0x4e,b'\x20')
        o.uc.mem_write((actor&0x1fffff)+0x37,bytes((ma,)));o.uc.mem_write((actor&0x1fffff)+0x90,bytes((4 if up else 0,)))
        o.uc.mem_write((target&0x1fffff)+0x90,bytes((2 if down else 0,)))
        o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HH',0 if dead else 500,999))
        r=d.at(s.battle.ABILITY_RANGE+14*ab,14);o.uc.mem_write((ability&0x1fffff)+0x30,r)
        o.uc.mem_write((ability&0x1fffff)+0x12,bytes((tf,af)));o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
        o.uc.mem_write((ability&0x1fffff)+0x46,d.at(s.battle.STATUS_SET+6*r[11],6))
        o.uc.mem_write((target&0x1fffff)+0x18c,b'\x01');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
        o.write(0x8018f5fc,0 if execute else 2);rolls.clear();o.call('remove');o.call('raise_'if r[8]==13 else'cure')
        raw=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));chance=struct.unpack_from('<h',raw,42)[0]
        o.call('finish');v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44))
        damage,heal=struct.unpack_from('<hh',v,4);kind=0 if v[37]&0x80 else 4 if r[8]==13 else 3
        amount=damage if kind==0 else heal
        rows.append([*p,amount&65535,kind,v[0],chance,len(rolls)])
    lines=['/* Generated original formula0c/0d + finalizer, local only. */',
           'static const u16 source_healing[][23]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(cases=rows,functions=o.function_hashes,battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        scope='Unchanged original Cure/Raise formulas, bitmap filter and finalizer; supplied ordinary human state. Roll observed/stubbed to pass unless chance0, target-list sorting isolated. No original full cast/renderer/HP commit trace.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Original healing:',len(rows),'profiles for all8 producers')


if __name__=='__main__':main()
