#!/usr/bin/env python3
"""Original HP Restore, MP Restore and Critical Quick boundaries and frozen effects."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-critical-reactions/oracle';out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(finalize='battle_action_finalize_attack_and_flag_reactions',
        chance='battle_action_calculate_chance_to_react',
        targets='battle_action_build_reaction_targets',perform='battle_action_perform_reaction_ability',stat='battle_unit_apply_stat_increment_decrement',handlers='battle_status_enable_special_flags')
    isolated=['battle_action_check_mp_switch_distribute_and_damage_split_usability',
        'battle_action_remove_broken_or_stolen_equipment','battle_action_add_war_funds',
        'battle_action_handle_steal_exp','battle_unit_apply_level_up_down_ability',
        'battle_action_add_poached_item_to_fur_shop_inventory','battle_status_remove_control']
    for k,n in names.items():bind_function(k,n,'battle',disc)
    for i,n in enumerate(isolated):bind_function('external'+str(i),n,'battle',disc)
    o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',disc);o.call('init')
    roll=[0]
    def external(uc,address,size,data):
        uc.reg_write(o.reg.UC_MIPS_REG_V0,roll[0]if address==0x8002230c else 0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in ['handlers']+['external'+str(i)for i in range(len(isolated))]:
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=0x8002230c,end=0x8002230c)
    unit,actor,ability=0x801908cc,0x801d4000,0x801938c0;action=unit+0x18c
    def load(sid=255,secondary=0):
        o.uc.mem_write(unit&0x1fffff,bytes(0x1c0));o.uc.mem_write(actor&0x1fffff,bytes(0x1c0))
        bits=mask(sid)if sid!=255 else bytes(5)
        o.uc.mem_write((unit&0x1fffff)+0x58,bits);o.uc.mem_write((unit&0x1fffff)+0x1bb,bits)
        o.uc.mem_write((unit&0x1fffff)+0x24,b'\x64');o.uc.mem_write((unit&0x1fffff)+0x28,struct.pack('<4H',100,100,10,20))
        o.uc.mem_write((unit&0x1fffff)+0x30,bytes([6]*9));o.uc.mem_write((unit&0x1fffff)+0x22,b'\x01')
        o.uc.mem_write((actor&0x1fffff)+0x22,b'\x01');o.uc.mem_write(ability&0x1fffff,bytes(0x50))
        o.write(0x80192d94,actor);o.write(0x80192d98,unit);o.write(0x80192d90,action)
        o.write(0x8018f5f0,secondary);o.write(0x8018f5fc,0);o.uc.mem_write(s.TILES&0x1fffff,bytes(8))
    stages=[]
    # hp,max,hit,type,damage,status,secondary,critical trait,Brave.
    profiles=[(100,100,1,128,79,255,0,0,100),(100,100,1,128,80,255,0,0,100),
        (100,100,1,128,81,255,0,0,100),(20,100,1,128,0,255,0,0,100),
        (21,100,1,128,0,255,0,0,100),(1,100,1,128,1,255,0,0,100),
        (2,100,1,128,1,255,0,0,100),(100,100,1,128,100,255,0,0,100),
        (1,1,1,128,0,255,0,0,100),(1,4,1,128,0,255,0,0,100),
        (1,5,1,128,0,255,0,0,100),(20,99,1,128,0,255,0,0,100),
        (20,99,1,128,1,255,0,0,100),(201,999,1,128,1,255,0,0,100),
        (201,999,1,128,2,255,0,0,100),(20,100,1,64,0,255,0,0,100),
        (20,100,0,128,7,255,0,0,100),(20,100,1,128,1,35,0,0,100),
        (20,100,1,128,1,30,0,0,100),(20,100,1,128,1,37,0,0,100),
        (20,100,1,128,1,22,0,0,100),(20,100,1,128,1,34,0,0,100),
        (20,100,1,128,1,24,0,0,100),(20,100,1,128,1,255,1,0,100),
        (20,100,1,128,1,255,0,0,0),(20,100,1,128,1,255,0,1,100),
        (100,100,1,128,1,255,0,2,100),(100,100,1,128,1,255,0,3,100),
        (100,100,1,128,1,23,0,0,100)]
    def traits(mode,sid):
        t=unit&0x1fffff
        innate=mask(23)if mode&2 else bytes(5)
        immune=mask(23)if mode&1 else bytes(5)
        current=bytes(a|b for a,b in zip(innate,mask(sid)if sid!=255 else bytes(5)))
        o.uc.mem_write(t+0x4e,innate+immune+current)
    draws=0
    def count_rng(uc,address,size,data):
        nonlocal draws
        draws+=1
    o.uc.hook_add(unicorn.UC_HOOK_CODE,count_rng,begin=0x8002230c,end=0x8002230c)
    for reaction in (0x1af,0x1b0,0x1b1):
        for profile in profiles:
            hp,mx,hit,kind,damage,sid,secondary,mode,brave=profile
            load(sid,secondary);traits(mode,sid);draws=0
            o.uc.mem_write((unit&0x1fffff)+0x28,struct.pack('<4H',hp,mx,4,20))
            o.uc.mem_write((unit&0x1fffff)+0x24,bytes((brave,)))
            o.uc.mem_write((unit&0x1fffff)+0x8c,bytes((64>>(reaction-0x1af),)))
            o.uc.mem_write(action&0x1fffff,bytes((hit,)))
            o.uc.mem_write((action&0x1fffff)+4,struct.pack('<h',damage))
            o.uc.mem_write((action&0x1fffff)+0x25,bytes((kind,)))
            o.uc.mem_write((ability&0x1fffff)+0x25,b'\x01');o.call('finalize',0)
            queued=struct.unpack('<H',o.uc.mem_read((action&0x1fffff)+0x0e,2))[0]
            after_hp=struct.unpack('<H',o.uc.mem_read((unit&0x1fffff)+0x28,2))[0]
            critical=o.uc.mem_read((unit&0x1fffff)+0x5a,1)[0]&1
            stages.append([reaction,*profile,queued,after_hp,critical,draws])
    effects=[]
    # hp,maxhp,mp,maxmp,ct,status,critical trait,late hp,late mp.
    profiles=[(20,100,4,20,37,255,0,65535,65535),
        (1,5,0,0,0,255,0,65535,65535),
        (199,999,998,999,254,255,0,65535,65535),
        (20,100,20,20,255,255,0,65535,65535),
        (10,100,4,20,37,3,0,65535,65535),
        (20,100,4,20,37,31,0,65535,65535),
        (100,100,20,20,37,255,2,65535,65535),
        (20,100,4,20,37,255,0,10,2)]
    for reaction in (0x1af,0x1b0,0x1b1):
        for profile in profiles:
            hp,mx,mp,mm,ct,sid,mode,late_hp,late_mp=profile
            load(sid,1);traits(mode,sid);o.write(0x80192d9c,reaction);draws=0
            o.uc.mem_write((unit&0x1fffff)+0x28,struct.pack('<4H',hp,mx,mp,mm))
            o.uc.mem_write((unit&0x1fffff)+0x39,bytes((ct,)))
            result=o.call('perform');v=bytes(o.uc.mem_read(action&0x1fffff,44))
            if late_hp!=65535:o.uc.mem_write((unit&0x1fffff)+0x28,struct.pack('<H',late_hp))
            if late_mp!=65535:o.uc.mem_write((unit&0x1fffff)+0x2c,struct.pack('<H',late_mp))
            o.call('finalize',0)
            hp_after,_,mp_after,_=struct.unpack('<4H',o.uc.mem_read((unit&0x1fffff)+0x28,8))
            ct_after=o.uc.mem_read((unit&0x1fffff)+0x39,1)[0]
            healing=struct.unpack_from('<H',v,6)[0];mp_healing=struct.unpack_from('<H',v,10)[0]
            effects.append([reaction,*profile,result,v[0],v[0x25],healing,mp_healing,v[0x13],hp_after,mp_after,ct_after,draws])
    targets=[]
    for reaction in (0x1af,0x1b0,0x1b1):
        load(secondary=1);o.write(0x80192d9c,reaction)
        o.write(0x801d8100,0xffffffff);o.uc.mem_write(0x1d8200,b'\xff')
        result=o.call('targets',10,0x801d8000,0x801d8100,0x801d8200)
        count=struct.unpack('<I',o.uc.mem_read(0x1d8100,4))[0];target=o.uc.mem_read(0x1d8200,1)[0]
        assert (result,count,target)==(1,1,10)
        targets.append(dict(reaction=reaction,result=result,count=count,target=target))
    lines=['/* Generated from unchanged original MIPS; local-only. */']
    # The three original staging sweeps differ only by the queued source ID.
    # Store that common profile once, preserving all87 original calls in JSON.
    common=[]
    for rows in (stages[:29],stages[29:58],stages[58:]):
        normalized=[row[1:10]+[int(row[10]!=0)]+row[11:]for row in rows]
        if common:assert normalized==common
        common=normalized
    for name,rows in [('source_critical_stages',common),('source_critical_effects',effects)]:
        lines.append(f'static const u16 {name}[][{len(rows[0])}]={{')
        lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(stages=stages,effects=effects,targets=targets,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),isolated=isolated,
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        layout_notes='Battle CT is at+0x39; the reaction byte at+0x8c uses HP0x40/MP0x20/Quick0x10. Native staging derives Critical23 from HP with immutable traits rather than storing a new mutable flag.',
        scope='Unmodified retail finalizer and critical-reaction dispatch, actual HP/status resolution and resource/CT effect preparation plus final commit, with original self-target lists. BIOS rand returns zero; unrelated equipment/MP-switch/reward bookkeeping and status-specific graphics handlers are isolated. No full original UI/AI/secondary-animation replay.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Critical reaction oracle:',len(stages),'stages,',len(effects),'effects,',len(targets),'self-target lists',flush=True)


if __name__=='__main__':main()
