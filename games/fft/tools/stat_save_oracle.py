#!/usr/bin/env python3
"""Original PA/MA/Speed Save staging and self-directed secondary effects."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-stat-save/oracle';out.mkdir(parents=True,exist_ok=True)
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
    cases=[(1,0x80,7,255,0),(0,0x80,7,255,0),(1,0x80,0,255,0),
        (1,0x40,0,255,0),(1,0x80,100,255,0),(1,0x80,7,35,0),
        (1,0x80,7,30,0),(1,0x80,7,37,0),(1,0x80,7,255,1),
        (1,0x80,7,22,0),(1,0x80,7,34,0),(1,0x80,7,24,0)]
    for reaction in (0x1a6,0x1a7,0x1a8):
        for hit,kind,damage,sid,secondary in cases:
            load(sid,secondary)
            o.uc.mem_write((unit&0x1fffff)+0x8b,bytes((128>>(reaction-0x1a6),)))
            o.uc.mem_write(action&0x1fffff,bytes((hit,)))
            o.uc.mem_write((action&0x1fffff)+4,struct.pack('<h',damage))
            o.uc.mem_write((action&0x1fffff)+0x25,bytes((kind,)))
            o.uc.mem_write((ability&0x1fffff)+0x25,b'\x01');o.call('finalize',0)
            queued=struct.unpack('<H',o.uc.mem_read((action&0x1fffff)+0x0e,2))[0]
            hp=struct.unpack('<H',o.uc.mem_read((unit&0x1fffff)+0x28,2))[0]
            stages.append([reaction,hit,kind,damage,sid,secondary,queued,hp])
    # Keep perform + its ordinary target finalizer unmodified. Apply its staged
    # stat byte through the exact original clamp helper, independently of any
    # native model or cached effective attributes.
    effects=[]
    for reaction in (0x1a6,0x1a7,0x1a8):
        for value in (1,5,49,50,98,99):
            load(secondary=1)
            o.write(0x80192d9c,reaction)
            o.uc.mem_write((unit&0x1fffff)+0x30,bytes((value,value,min(value,50))))
            o.call('perform')
            v=bytes(o.uc.mem_read(action&0x1fffff,44))
            o.call('stat',v[0x12],unit+0x32,50,1)
            o.call('stat',v[0x14],unit+0x30,99,1)
            o.call('stat',v[0x15],unit+0x31,99,1)
            after=bytes(o.uc.mem_read((unit&0x1fffff)+0x30,3))
            effects.append([reaction,value,v[0],v[0x12],v[0x14],v[0x15],v[0x25],*after])
    targets=[]
    for reaction in (0x1a6,0x1a7,0x1a8):
        load(secondary=1);o.write(0x80192d9c,reaction)
        o.write(0x801d8100,0xffffffff);o.uc.mem_write(0x1d8200,b'\xff')
        result=o.call('targets',10,0x801d8000,0x801d8100,0x801d8200)
        count=struct.unpack('<I',o.uc.mem_read(0x1d8100,4))[0]
        target=o.uc.mem_read(0x1d8200,1)[0]
        flags=o.uc.mem_read(0x5ebd0+reaction-0x1a6,1)[0]
        assert (result,count,target)==(1,1,10)
        targets.append(dict(reaction=reaction,result=result,count=count,target=target,behavior_flags=flags))
    lines=['/* Generated from unchanged original MIPS; local-only. */']
    for name,rows in [('source_stat_stages',stages),('source_stat_effects',effects)]:
        lines.append(f'static const u16 {name}[][{len(rows[0])}]={{')
        lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(stages=stages,effects=effects,targets=targets,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),isolated=isolated,
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Unmodified retail finalizer and PA/MA/Speed Save dispatch, actual HP/status resolution, reaction effect preparation, secondary validity and original stat clamp helper. BIOS rand returns zero; unrelated equipment/MP-switch/reward bookkeeping and status-specific graphics handlers are isolated. No full original UI/AI/secondary-animation replay.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Stat Save oracle:',len(stages),'stages,',len(effects),'effects,',len(targets),'self-target lists',flush=True)


if __name__=='__main__':main()
