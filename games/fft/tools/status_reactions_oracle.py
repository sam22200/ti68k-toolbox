#!/usr/bin/env python3
"""Retail Caution/Dragon Spirit pre-impact and Regenerator post-impact reactions."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask

def main():
    import unicorn
    out=s.HERE/'campaign/x/core-status-reactions/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(finalize='battle_action_finalize_attack_and_flag_reactions',
        perform='battle_action_perform_reaction_ability',targets='battle_action_build_reaction_targets',
        handlers='battle_status_enable_special_flags')
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
    stages=[]
    for reaction in(426,427,428):
        for p in profiles:
            hit,kind,amount,sid,secondary,flags,skillset,formula,brave=p
            load(sid,secondary);draws=0
            o.uc.mem_write((unit&0x1fffff)+0x24,bytes((brave,)))
            o.uc.mem_write((unit&0x1fffff)+0x8b,bytes((8>>(reaction-426),)))
            o.uc.mem_write(action&0x1fffff,bytes((hit,)));o.uc.mem_write((action&0x1fffff)+4,struct.pack('<H',amount))
            o.uc.mem_write((action&0x1fffff)+0x25,bytes((kind,)))
            o.uc.mem_write((ability&0x1fffff)+0x14,bytes((skillset,)))
            o.uc.mem_write((ability&0x1fffff)+0x25,bytes((formula,)))
            o.uc.mem_write((ability&0x1fffff)+0x36,bytes((flags,)));o.call('finalize',0)
            queued=struct.unpack('<H',o.uc.mem_read((action&0x1fffff)+14,2))[0]
            hp=struct.unpack('<H',o.uc.mem_read((unit&0x1fffff)+0x28,2))[0]
            stages.append([reaction,*p,queued,hp,draws])
    effects=[]
    # Existing status, target status trait (immune1/innate2), late trait/status,
    # current timer. Cases expose refresh, cancellation and a prepared no-op.
    for reaction,sid in((426,6),(427,18),(428,25)):
        profiles=[(255,0,255,0,0),(sid,0,255,0,1),(255,1,255,0,0),
            (255,2,255,0,0),(3,0,255,0,0),(24,0,255,0,11),
            (4,0,255,0,0),(7,0,255,0,0),(31,0,255,0,0),
            (255,0,255,1,0),(255,0,sid,0,0)]
        for existing,trait,late_sid,late_trait,timer in profiles:
            load(existing,1);at=unit&0x1fffff;draws=0
            innate=mask(sid)if trait&2 else bytes(5);immune=mask(sid)if trait&1 else bytes(5)
            current=bytes(a|b for a,b in zip(innate,mask(existing)if existing!=255 else bytes(5)))
            o.uc.mem_write(at+0x4e,innate+immune+current);o.uc.mem_write(at+0x1bb,current)
            o.uc.mem_write(at+0x5d,bytes((timer,)*16));o.write(0x80192d9c,reaction)
            result=o.call('perform');packet=bytes(o.uc.mem_read(action&0x1fffff,44))
            if late_trait:o.uc.mem_write(at+0x53,mask(sid))
            if late_sid!=255:
                now=bytes(a|b for a,b in zip(current,mask(late_sid)))
                o.uc.mem_write(at+0x58,now);o.uc.mem_write(at+0x1bb,now)
            o.call('finalize',0)
            after=list(o.uc.mem_read(at+0x58,5));ct=list(o.uc.mem_read(at+0x5d,16))
            packed=next(((b<<8)|packet[27+b]for b in range(5)if packet[27+b]),0)
            effects.append([reaction,existing,trait,late_sid,late_trait,timer,result,packet[0],packet[0x25],packed,*after,ct[1],draws])
    targets=[]
    for reaction in(426,427,428):
        load(secondary=1);o.write(0x80192d9c,reaction);o.write(0x801d8100,0xffffffff)
        result=o.call('targets',10,0x801d8000,0x801d8100,0x801d8200)
        count=struct.unpack('<I',o.uc.mem_read(0x1d8100,4))[0];target=o.uc.mem_read(0x1d8200,1)[0]
        assert(result,count,target)==(1,1,10);targets.append([reaction,result,count,target])
    lines=['/* Generated from unmodified retail MIPS; local-only. */']
    for name,rows in(('source_status_stages',stages),('source_status_effects',effects)):
        lines+=['static const u16 %s[][%d]={'%(name,len(rows[0]))]
        lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(stages=stages,effects=effects,targets=targets,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),isolated=isolated,
        scope='Unmodified finalizer/pre- and post-reaction dispatch, effect preparation and status commit. Isolated graphics/equipment/reward bookkeeping; controlled zero BIOS RNG. No full original UI/AI/animation replay.'),indent=2)+'\n')
    print('Status reaction oracle:',len(stages),'stages,',len(effects),'effects,',len(targets),'self-target lists')

if __name__=='__main__':main()
