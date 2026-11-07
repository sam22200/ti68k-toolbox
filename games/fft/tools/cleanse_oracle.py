#!/usr/bin/env python3
"""Original cancellation formulas and frozen removal subsets, one compatible family."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

SPELLS=(14,56)
ITEMS=(374,375,376,377,378,379,380)


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-cleanse/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(hostile='battle_formula_hit_faith_ma_x_percent',friendly='battle_formula_friendly_magic_accuracy',
        finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
        remove='battle_status_remove',random='battle_formula_get_random_0_7fff',cancel='battle_formula_apply_status_to_action')
    for k,n in names.items():bind_function(k,n,'battle',d)
    handler=struct.unpack_from('<I',d.bat,0x8018f610-s.battle.BAT+4*0x38)[0]
    assert handler==s.FUNCTIONS['cancel'][0] # Retail item0x38 dispatcher, not a guessed helper.
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
    # ab, state profile, actor Faith, target Faith. Reuse unchanged accuracy
    # arithmetic evidence; concentrate here on cancellation/finalization/RNG.
    profiles=[(ab,mode,70,70)for ab in SPELLS+ITEMS for mode in range(8)]
    profiles += [(ab,1,af,tf)for ab in SPELLS for af,tf in ((0,70),(10,10),(100,100))]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,mode,af,tf=p;ma=5;up=down=shell=az=tz=female=gear=0
        if ab<368:r=d.at(s.battle.ABILITY_RANGE+14*ab,14);sid=r[11]
        else:
            item=d.at(s.battle.ITEM_ABILITY+3*(ab-368),3);sid=item[2]
            r=bytes(8)+bytes((56,0,0,sid,0,0))
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
            current=bytearray(5);innate=bytearray(5);immune=bytearray(5)
            if mode in (1,2,3,7):current=bytearray(status[1:])
            if mode==2:innate=bytearray(current)
            if mode==3:
                innate=bytearray(v&0xaa for v in current);immune=bytearray(status[1:])
            if mode==4:current[0]|=32
            if mode==5:current[1]|=128
            if mode==6:current[3]|=1
            if mode==7:current[3]|=8 # An unrelated Haste must survive Esuna/Remedy.
            o.uc.mem_write((target&0x1fffff)+0x58,bytes(current))
            o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(current))
            o.uc.mem_write((target&0x1fffff)+0x4e,bytes(innate))
            o.uc.mem_write((target&0x1fffff)+0x53,bytes(immune))
            o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x46,status)
            o.uc.mem_write((ability&0x1fffff)+0x12,bytes((tf,af)));o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
            o.write(0x8018f5fc,0 if execute else 2);rng=1;draws=0
            o.call('remove');o.call('cancel'if ab>=368 else'friendly'if r[8]==11 else'hostile');o.call('finish')
            v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));packed=0;bit=1
            for byte in range(5):
                for k in range(8):
                    mask=128>>k
                    if status[1+byte]&mask:
                        if v[32+byte]&mask:packed|=bit
                        bit<<=1
            outputs += [v[0],struct.unpack_from('<H',v,42)[0],packed,draws]
        rows.append([*p,*outputs])
    lines=['/* Original cancellation formula/finalizer, controlled RNG, local only. */',
           'static const u16 source_cleanse[][12]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        scope='Unchanged original status0a/0b and actual item0x38 handler, removal/filter/finalizer. Supplied human states; controlled native-equivalent RNG and isolated target sorting. No live item inventory/reward/render or original future-turn AI trace.'),indent=2)+'\n')
    print('Original cleansing:',len(rows),'profiles for nine source producers')


if __name__=='__main__':main()
