#!/usr/bin/env python3
"""Retail Brave Up / Faith Up / Sunken State reaction rules."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask

def main():
    import unicorn
    out=s.HERE/'campaign/x/core-brave-faith-reactions/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(finalize='battle_action_finalize_attack_and_flag_reactions',
        perform='battle_action_perform_reaction_ability',targets='battle_action_build_reaction_targets',
        handlers='battle_status_enable_special_flags',
        evade='battle_formula_calculate_final_hit_percent',
        transparent='battle_formula_calculate_transparent',remove='battle_status_remove_transparent')
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
    # hit, attack type, amount, status, secondary, eligibility flag, skillset, formula, Brave.
    profiles=[(1,128,7,255,0,16,11,1,100),(0,128,7,255,0,16,11,1,100),
        (1,128,0,255,0,16,11,1,100),(1,64,0,255,0,16,11,1,100),
        (1,0,0,255,0,16,11,1,100),(1,128,100,255,0,16,11,1,100),
        (1,128,7,255,0,0,11,1,100),(1,128,7,255,0,16,21,1,100),
        (1,128,7,255,0,16,11,7,100),(1,128,7,255,1,16,11,1,100),
        (1,128,7,255,0,16,11,1,0),(1,128,7,35,0,16,11,1,100),
        (1,128,7,30,0,16,11,1,100),(1,128,7,37,0,16,11,1,100),
        (1,128,7,34,0,16,11,1,100),(1,128,7,22,0,16,11,1,100),
        (1,128,7,24,0,16,11,1,100),(1,128,7,3,0,16,11,1,100),
        (1,128,7,6,0,16,11,1,100),(1,128,7,4,0,16,11,1,100)]
    profiles=[(*p,0 if p[6]==21 else 12)for p in profiles]+[
        (1,128,7,255,0,16,11,1,100,0),(1,128,7,255,0,0,11,1,100,1),
        (1,128,7,19,0,16,11,1,100,12)]
    stages=[]
    for reaction in(425,429,430):
        for p in profiles:
            hit,kind,amount,sid,secondary,flags,skillset,formula,brave,mp=p
            load(sid,secondary);draws=0
            o.uc.mem_write((unit&0x1fffff)+0x24,bytes((brave,)))
            o.uc.mem_write((unit&0x1fffff)+(0x8c if reaction==430 else 0x8b),bytes((128 if reaction==430 else 16 if reaction==425 else 1,)))
            o.uc.mem_write(action&0x1fffff,bytes((hit,)));o.uc.mem_write((action&0x1fffff)+4,struct.pack('<H',amount))
            o.uc.mem_write((action&0x1fffff)+0x25,bytes((kind,)))
            o.uc.mem_write((ability&0x1fffff)+0x14,bytes((skillset,)))
            o.uc.mem_write((ability&0x1fffff)+0x25,bytes((formula,)))
            o.uc.mem_write((ability&0x1fffff)+0x36,bytes((flags,)))
            o.uc.mem_write((ability&0x1fffff)+0x2b,bytes((mp,)));o.call('finalize',0)
            queued=struct.unpack('<H',o.uc.mem_read((action&0x1fffff)+14,2))[0]
            hp=struct.unpack('<H',o.uc.mem_read((unit&0x1fffff)+0x28,2))[0]
            stages.append([reaction,*p,queued,hp,draws])
    self_triggers=[]
    for mp in(0,12):
        load();draws=0;o.write(0x80192d94,unit)
        o.uc.mem_write((unit&0x1fffff)+0x8c,b'\x80')
        o.uc.mem_write((ability&0x1fffff)+0x25,b'\x01');o.uc.mem_write((ability&0x1fffff)+0x2b,bytes((mp,)))
        o.call('finalize',0);queued=struct.unpack('<H',o.uc.mem_read((action&0x1fffff)+14,2))[0]
        self_triggers.append([mp,queued,draws])
    effects=[]
    for reaction in(425,429,430):
        sid=19 if reaction==425 else 21
        profiles=[(100,255,0,255,0),(100,31,0,255,0)]
        if reaction==425:
            profiles += [(100,19,0,255,0),(100,255,1,255,0),(100,255,2,255,0),
                (100,24,0,255,0),(100,4,0,255,0),(100,7,0,255,0),
                (100,255,0,255,1),(100,255,0,19,0)]
        else:
            profiles += [(v,255,0,255,0)for v in(0,1,7,9,10,96,97,98,99)]
            if reaction==429:profiles += [(9,21,0,255,0),(7,21,0,255,0),(9,22,0,255,0)]
            else:profiles += [(100,4,0,255,0),(100,7,0,255,0)]
        for value,existing,trait,late_sid,late_trait in profiles:
            load(existing,1);at=unit&0x1fffff;draws=0
            # Both inputs share value; check the unchanged member as well.
            o.uc.mem_write(at+0x24,bytes((100 if reaction==430 else value,value,value)))
            innate=mask(sid)if trait&2 else bytes(5);immune=mask(sid)if trait&1 else bytes(5)
            current=bytes(a|b for a,b in zip(innate,mask(existing)if existing!=255 else bytes(5)))
            o.uc.mem_write(at+0x4e,innate+immune+current);o.uc.mem_write(at+0x1bb,current)
            o.write(0x80192d9c,reaction);o.call('perform');packet=bytes(o.uc.mem_read(action&0x1fffff,44))
            if late_trait:o.uc.mem_write(at+0x53,mask(sid))
            if late_sid!=255:
                now=bytes(a|b for a,b in zip(current,mask(late_sid)))
                o.uc.mem_write(at+0x58,now);o.uc.mem_write(at+0x1bb,now)
            o.call('finalize',0)
            after=list(o.uc.mem_read(at+0x58,5));bf=[o.uc.mem_read(at+0x24,1)[0],o.uc.mem_read(at+0x26,1)[0]]
            packed=next(((b<<8)|packet[27+b]for b in range(5)if packet[27+b]),0)
            effects.append([reaction,value,existing,trait,late_sid,late_trait,packet[0],packet[22],packet[23],packed,*bf,*after,draws])
    transparency=[]
    for active in(0,1):
        load();o.uc.mem_write((actor&0x1fffff)+0x5a,bytes((16 if active else 0,)))
        o.uc.mem_write((ability&0x1fffff)+0x1e,bytes((17,23,31,41)));o.call('transparent')
        after=list(o.uc.mem_read((ability&0x1fffff)+0x1e,4))
        o.uc.mem_write((ability&0x1fffff)+0x1c,b'\x64');o.uc.mem_write(action&0x1fffff,b'\x01')
        o.uc.mem_write((action&0x1fffff)+0x2a,struct.pack('<h',100));draws=0;o.call('evade')
        transparency.append([active,*after,draws])
    removal=[]
    for acted,armed,innate in((0,1,0),(1,0,0),(1,1,0),(1,1,1)):
        load(secondary=1);at=unit&0x1fffff
        o.uc.mem_write(at+0x58,mask(19)if not innate else bytes(5));o.uc.mem_write(at+0x4e,mask(19)if innate else bytes(5))
        o.uc.mem_write(at+0x1bb,mask(19)if not innate else bytes(5))
        # Offsets are documented source fields, not native layout estimates.
        o.uc.mem_write(at+0x188,bytes((acted,)));o.uc.mem_write(at+0x27,bytes((armed,)))
        result=o.call('remove',unit)
        removal.append([acted,armed,innate,result,o.uc.mem_read((action&0x1fffff)+0x22,1)[0]])
    targets=[]
    for reaction in(425,429,430):
        load(secondary=1);o.write(0x80192d9c,reaction);o.write(0x801d8100,0xffffffff)
        result=o.call('targets',10,0x801d8000,0x801d8100,0x801d8200)
        count=struct.unpack('<I',o.uc.mem_read(0x1d8100,4))[0];target=o.uc.mem_read(0x1d8200,1)[0]
        assert(result,count,target)==(1,1,10);targets.append([reaction,result,count,target])
    lines=['/* Generated from unmodified retail MIPS; local-only. */']
    for name,rows in(('source_reaction_stages',stages),('source_reaction_effects',effects),('source_transparent',transparency),('source_removal',removal)):
        lines+=['static const u16 %s[][%d]={'%(name,len(rows[0]))]
        lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(stages=stages,effects=effects,targets=targets,self_triggers=self_triggers,transparency=transparency,removal=removal,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),isolated=isolated,
        scope='Unmodified finalizer/pre- and post-reaction dispatch, effect preparation and status commit. Isolated graphics/equipment/reward bookkeeping; controlled zero BIOS RNG. No full original UI/AI/animation replay.'),indent=2)+'\n')
    print('Brave/Faith/Transparent oracle:',len(stages),'stages,',len(effects),'effects,',len(targets),'self-target lists')

if __name__=='__main__':main()
