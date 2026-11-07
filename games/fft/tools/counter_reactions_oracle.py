#!/usr/bin/env python3
"""Retail Counter Tackle / Counter Flood / Absorb Used MP boundaries."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask

def main():
    import unicorn
    out=s.HERE/'campaign/x/core-counter-reactions/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(finalize='battle_action_finalize_attack_and_flag_reactions',
        perform='battle_action_perform_reaction_ability',targets='battle_action_build_reaction_targets',
        handlers='battle_status_enable_special_flags',
        prepare='battle_reaction_prepare_next',counter='battle_action_store_counter_ability',
        consume='battle_action_check_and_consume_mp',strike='battle_action_init_current_ability_strike_data')
    isolated=['battle_action_check_mp_switch_distribute_and_damage_split_usability',
        'battle_action_remove_broken_or_stolen_equipment','battle_action_add_war_funds',
        'battle_action_handle_steal_exp','battle_unit_apply_level_up_down_ability',
        'battle_action_add_poached_item_to_fur_shop_inventory','battle_status_remove_control']
    for key,name in names.items():bind_function(key,name,'battle',d)
    for i,name in enumerate(isolated):bind_function('external'+str(i),name,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=300000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    draws=0
    def external(uc,address,size,data):
        nonlocal draws
        if address==0x8002230c:draws+=1
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in ['handlers']+['external'+str(i)for i in range(len(isolated))]:
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=0x8002230c,end=0x8002230c)
    unit,actor,ability=0x801908cc,0x801d4000,0x801938c0;action=unit+0x18c
    def load(sid=255,secondary=0):
        for ptr in(unit,actor):o.uc.mem_write(ptr&0x1fffff,bytes(448))
        bits=mask(sid)if sid!=255 else bytes(5);at=unit&0x1fffff
        o.uc.mem_write(at+0x58,bits);o.uc.mem_write(at+0x1bb,bits)
        o.uc.mem_write(at+0x24,b'\x64');o.uc.mem_write(at+0x28,struct.pack('<4H',100,100,4,20))
        o.uc.mem_write(at+0x30,bytes([6]*9));o.uc.mem_write(at+0x22,b'\x01')
        o.uc.mem_write((actor&0x1fffff)+0x22,b'\x01');o.uc.mem_write(ability&0x1fffff,bytes(80))
        o.write(0x80192d94,actor);o.write(0x80192d98,unit);o.write(0x80192d90,action)
        o.write(0x8018f5f0,secondary);o.write(0x8018f5fc,0);o.uc.mem_write(s.TILES&0x1fffff,bytes(8))
    # Hit/category/amount/status/secondary/raw flags/skillset/formula/Brave/cost.
    profiles=[(1,128,7,255,0,144,11,1,100,12),(0,128,7,255,0,144,11,1,100,12),
        (1,128,0,255,0,144,11,1,100,12),(1,64,0,255,0,144,11,1,100,12),
        (1,0,0,255,0,144,11,1,100,12),(1,128,100,255,0,144,11,1,100,12),
        (1,128,7,255,0,0,11,1,100,12),(1,128,7,255,0,16,11,1,100,12),
        (1,128,7,255,0,128,11,1,100,12),(1,128,7,255,0,144,21,1,100,0),
        (1,128,7,255,0,144,11,7,100,12),(1,128,7,255,1,144,11,1,100,12),
        (1,128,7,255,0,144,11,1,0,12),(1,128,7,35,0,144,11,1,100,12),
        (1,128,7,30,0,144,11,1,100,12),(1,128,7,37,0,144,11,1,100,12),
        (1,128,7,22,0,144,11,1,100,12),(1,128,7,34,0,144,11,1,100,12),
        (1,128,7,255,0,144,11,1,100,0),(1,128,7,255,0,144,11,1,100,1)]
    stages=[]
    for reaction in(436,437,438):
        for p in profiles:
            hit,kind,amount,sid,secondary,flags,skillset,formula,brave,cost=p
            load(sid,secondary);draws=0;at=unit&0x1fffff
            o.uc.mem_write(at+0x24,bytes((brave,)))
            o.uc.mem_write(at+(0x8d if reaction==438 else 0x8c),bytes((128 if reaction==438 else 2 if reaction==436 else 1,)))
            o.uc.mem_write(action&0x1fffff,bytes((hit,)));o.uc.mem_write((action&0x1fffff)+4,struct.pack('<H',amount))
            o.uc.mem_write((action&0x1fffff)+0x25,bytes((kind,)))
            for off,value in((0x14,skillset),(0x25,formula),(0x36,flags),(0x2b,cost)):
                o.uc.mem_write((ability&0x1fffff)+off,bytes((value,)))
            o.call('finalize',0);queued=struct.unpack('<H',o.uc.mem_read((action&0x1fffff)+14,2))[0]
            last=struct.unpack('<H',o.uc.mem_read((action&0x1fffff)+0x26,2))[0]
            hp=struct.unpack('<H',o.uc.mem_read(at+0x28,2))[0];stages.append([reaction,*p,queued,last,hp,draws])
    effects=[]
    for mp,maximum,cost,sid,late in((0,20,6,255,65535),(19,20,6,255,65535),(20,20,6,255,65535),
        (4,20,99,255,65535),(4,20,255,255,65535),(4,20,6,31,65535),
        (4,20,6,3,65535),(4,20,6,255,19),(4,20,6,255,0)):
        load(sid,1);at=unit&0x1fffff;draws=0
        o.uc.mem_write(at+0x2c,struct.pack('<2H',mp,maximum));o.write(0x80192d9c,438)
        o.uc.mem_write((action&0x1fffff)+0x26,struct.pack('<H',cost));o.call('perform')
        packet=bytes(o.uc.mem_read(action&0x1fffff,44))
        if late!=65535:o.uc.mem_write(at+0x2c,struct.pack('<H',late))
        o.call('finalize',0);after=struct.unpack('<H',o.uc.mem_read(at+0x2c,2))[0]
        effects.append([mp,maximum,cost,sid,late,packet[0],struct.unpack_from('<H',packet,10)[0],after,draws])
    calls=[]
    def prepare_hook(uc,address,size,data):
        if address==s.FUNCTIONS['counter'][0]:
            calls.append([uc.reg_read(o.reg.UC_MIPS_REG_A1)&255,uc.reg_read(o.reg.UC_MIPS_REG_A2)&65535,uc.reg_read(o.reg.UC_MIPS_REG_A3)])
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in('counter','consume','strike'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,prepare_hook,begin=at,end=at)
    constructors=[]
    for reaction,surface in((436,0),(437,0),(437,2),(437,63)):
        base=0x801908cc;o.uc.mem_write(base&0x1fffff,bytes(448*21));u=base+10*448
        o.uc.mem_write((u&0x1fffff)+0x24,b'\x64');o.uc.mem_write((u&0x1fffff)+0x28,struct.pack('<4H',100,100,4,20))
        o.uc.mem_write((u&0x1fffff)+0x19a,struct.pack('<H',reaction));o.write(0x8018f8c8,0)
        o.uc.mem_write(s.TILES&0x1fffff,bytes((surface,))+bytes(7));o.write(0x8018f5f0,0);o.write(0x8018f5fc,0)
        calls.clear();result=o.call('prepare',0x801d8000)
        constructors.append([reaction,surface,result,*calls[0]])
    targets=[]
    for reaction in(436,437,438):
        load(secondary=1);o.write(0x80192d9c,reaction);o.write(0x801d8100,0xffffffff)
        result=o.call('targets',10,0x801d8000,0x801d8100,0x801d8200)
        count=struct.unpack('<I',o.uc.mem_read(0x1d8100,4))[0]
        targets.append([reaction,result,count&65535,o.uc.mem_read(0x1d8200,1)[0]])
    lines=['/* Generated from unmodified retail MIPS; local-only. */']
    for name,rows in(('source_counter_stages',stages),('source_absorb_effects',effects),('source_counter_constructors',constructors)):
        lines+=['static const u16 %s[][%d]={'%(name,len(rows[0]))]
        lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(stages=stages,effects=effects,constructors=constructors,targets=targets,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),isolated=isolated,
        scope='Unmodified dispatch/finalizer and Absorb effect; unmodified reaction constructor with counter-preview, MP-consumption and strike-init hooks recording its arguments. Existing Dash/Geomancy formula oracles reused; not a full original UI/AI/animation replay.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Counter reaction oracle:',len(stages),'stages,',len(effects),'effects,',constructors,'constructors')

if __name__=='__main__':main()
