#!/usr/bin/env python3
"""Original resource item handlers, typed HP/MP flags, finalization and RNG."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

ITEMS=(368,369,370,371,372,373,381)

def main():
    import unicorn
    out=s.HERE/'campaign/x/core-restoration/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(hp='battle_formula_heal_z_times_ten',mp='battle_formula_heal_mp_z_times_ten',
        both='battle_formula_heal_full_hp_mp',raise_='battle_formula_heal_random_one_to_z_add_status',
        finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
        remove='battle_status_remove',random='battle_formula_get_random_0_7fff',
        commit='battle_action_finalize_attack_and_flag_reactions',
        dispatch='battle_action_dispatch_target_reaction_ability',
        switch='battle_action_check_mp_switch_distribute_and_damage_split_usability')
    for key,name in names.items():bind_function(key,name,'battle',d)
    for formula,key in [(0x48,'hp'),(0x49,'mp'),(0x4a,'both'),(0x4b,'raise_')]:
        at=struct.unpack_from('<I',d.bat,0x8018f610-s.battle.BAT+4*formula)[0]
        assert at==s.FUNCTIONS[key][0]
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    rng=1;draws=0
    def external(uc,address,size,data):
        nonlocal rng,draws
        value=0
        if address==s.FUNCTIONS['random'][0]:
            rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff
            draws+=1;value=rng>>17
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in ['random','sort','dispatch','switch']:
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    # ab,maxHP,maxMP,HP,MP, status bits: Undead/Dead/innateDead/Petrify/Wall/Sleep.
    profiles=[]
    for ab in ITEMS:
        profiles += [(ab,*v)for v in [
            (300,100,100,20,0),(300,100,300,100,0),(3,0,1,0,0),
            (300,100,100,20,1),(300,100,0,0,3),(300,100,0,0,2),
            (300,100,0,0,6),(300,100,100,20,8),(300,100,100,20,16),
            (1200,1500,1200,0,0),(300,100,100,20,32)]]
    profiles += [(381,300,100,0,0,10),(381,300,100,0,0,18),(381,300,100,0,0,14)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[];stores=[]
    observed={};recording=False
    def store(uc,access,address,size,value,data):
        if not recording:return
        for offset,key in [(0x28,'hp'),(0x2c,'mp')]:
            if address==(actor&0x1fffff)+offset or address==actor+offset:observed[key]=value&65535
        if len(observed)==2:uc.emu_stop()
    for p in profiles:
        ab,mhp,mmp,hp,mp,bits=p;item=d.at(s.battle.ITEM_ABILITY+3*(ab-368),3)
        for ptr in [actor,target]:o.uc.mem_write(ptr&0x1fffff,bytes(448))
        o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
        o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
        current=mask(*([3]if bits&1 else[]),*([2]if bits&2 else[]),
            *([8]if bits&8 else[]),*([31]if bits&16 else[]),*([35]if bits&32 else[]))
        o.uc.mem_write((target&0x1fffff)+0x58,current);o.uc.mem_write((target&0x1fffff)+0x1bb,current)
        if bits&4:o.uc.mem_write((target&0x1fffff)+0x4e,b'\x20')
        o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<4H',hp,mhp,mp,mmp))
        o.uc.mem_write((ability&0x1fffff)+0x1b,bytes((ab-368,)))
        o.uc.mem_write((ability&0x1fffff)+0x46,d.at(s.battle.STATUS_SET+6*item[2],6))
        o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
        o.write(0x8018f5fc,0);rng=1;draws=0;o.call('remove')
        try:
            o.call({0x48:'hp',0x49:'mp',0x4a:'both',0x4b:'raise_'}[item[0]]);o.call('finish')
        except Exception:
            print('Failed source profile',p,'PC',hex(o.uc.reg_read(o.reg.UC_MIPS_REG_PC)),flush=True);raise
        v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));flags=v[37]
        # Canonical native event omits inactive numerical fields and final misses.
        kind=15 if item[0]==0x4a and bits&1 else 0 if flags&128 else 14 if flags&64 and flags&16 else 12 if flags&16 else 4 if item[0]==0x4b else 3
        amount=struct.unpack_from('<H',v,4 if kind in (0,15) else 10 if kind==12 else 6)[0]
        second=struct.unpack_from('<H',v,10)[0]if kind in (14,15)else 0
        if not v[0]:kind,amount,second=8,0,0
        rows.append([*p,v[0],kind,amount,second,draws,flags])
        # Execute the original resource stores too: attack_type can omit MP
        # healing although the numerical field is still committed (Undead Elixir).
        if v[0]:
            init=s.FUNCTIONS.pop('init')
            try:z=s.Original(d)
            finally:s.FUNCTIONS['init']=init
            for key in ['dispatch','switch']:
                at=s.FUNCTIONS[key][0];z.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
            z.uc.hook_add(unicorn.UC_HOOK_MEM_WRITE,store)
            unit=bytes(o.uc.mem_read(target&0x1fffff,448));z.uc.mem_write(actor&0x1fffff,unit)
            z.uc.mem_write((actor&0x1fffff)+0x18c,v)
            observed.clear();recording=True
            z.uc.reg_write(z.reg.UC_MIPS_REG_A0,0);z.uc.reg_write(z.reg.UC_MIPS_REG_RA,0x801ff000)
            z.uc.reg_write(z.reg.UC_MIPS_REG_SP,0x801fe000)
            z.uc.emu_start(s.FUNCTIONS['commit'][0],0x801ff000,count=200000)
            recording=False;assert len(observed)==2,(p,observed)
            hd,hh,md,mh=struct.unpack_from('<4H',v,4)
            assert observed==dict(hp=max(0,min(mhp,hp-hd+hh)),mp=max(0,min(mmp,mp-md+mh)))
            stores.append(dict(profile=len(rows)-1,hp=observed['hp'],mp=observed['mp']))
    lines=['/* Original item48..4b, typed flags and finalizer; local only. */','static const u16 source_restoration[][12]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,resource_stores=stores,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        scope='Unchanged original handlers48..4b, flags, cancellation helper, finalizer and actual HP/MP store instructions. Controlled RNG; target sorting, reaction dispatch and MP Switch/Distribute/Damage Split are isolated. Store trace stops before unrelated status/reward handlers. No full original inventory/reward/renderer/AI/Auto-Potion selection replay.'),indent=2)+'\n')
    print('Original restoration:',len(rows),'profiles for all seven producers')

if __name__=='__main__':main()
