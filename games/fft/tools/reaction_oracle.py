#!/usr/bin/env python3
"""Original Counter/Auto-Potion staging, item selection, Brave and EXP contexts."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-reactions/oracle';out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(finalize='battle_action_finalize_attack_and_flag_reactions',
        potion='battle_action_select_auto_potion_item',chance='battle_action_calculate_chance_to_react',
        exp='battle_unit_update_attacker_earned_experience',handlers='battle_status_enable_special_flags')
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
        (1,0x80,7,30,0),(1,0x80,7,37,0),(1,0x80,7,255,1)]
    for reaction in (0x1ba,0x1b9):
        for hit,kind,damage,sid,secondary in cases:
            for flags in (0,0x10)if reaction==0x1ba else (0,):
                load(sid,secondary);o.uc.mem_write((unit&0x1fffff)+0x8d,bytes((8 if reaction==0x1ba else 16,)))
                o.uc.mem_write(action&0x1fffff,bytes((hit,)));o.uc.mem_write((action&0x1fffff)+4,struct.pack('<h',damage))
                o.uc.mem_write((action&0x1fffff)+0x25,bytes((kind,)))
                o.uc.mem_write((ability&0x1fffff)+0x36,bytes((flags,)))
                o.uc.mem_write((ability&0x1fffff)+0x25,b'\x01');o.call('finalize',0)
                queued=struct.unpack('<H',o.uc.mem_read((action&0x1fffff)+0x0e,2))[0]
                hp=struct.unpack('<H',o.uc.mem_read((unit&0x1fffff)+0x28,2))[0]
                stages.append([reaction,hit,kind,damage,sid,secondary,flags,queued,hp])
    potions=[]
    for team in (0,0x10):
        for stock in range(8):
            load();o.uc.mem_write((unit&0x1fffff)+0x1ba,bytes((team,)))
            o.uc.mem_write(0x596e0+0xf0,bytes((int(bool(stock&1)),int(bool(stock&2)),int(bool(stock&4)))))
            result=o.call('potion',unit);potions.append([team,stock,65535 if result==0xffffffff else result])
    chances=[]
    for brave in (0,1,50,100):
        for r in (0,327,16383,16384,32767):
            load();roll[0]=r;o.uc.mem_write((unit&0x1fffff)+0x24,bytes((brave,)))
            chances.append([brave,r,o.call('chance',unit)])
    exp=[]
    for secondary in (0,1):
        load(secondary=secondary);o.call('exp',1)
        exp.append([secondary,o.uc.mem_read((ability&0x1fffff)+0x27,1)[0]])
    lines=['/* Generated from unchanged original MIPS; local-only. */']
    for name,rows in [('source_stages',stages),('source_potions',potions),('source_chance',chances),('source_exp',exp)]:
        lines.append(f'static const u16 {name}[][{len(rows[0])}]={{')
        lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(stages=stages,potions=potions,chances=chances,exp=exp,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),isolated=isolated,
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original finalizer with Counter/Auto-Potion dispatch and actual HP/status resolution, item selection, Brave threshold and EXP-context guard. External equipment/MP-switch/reward bookkeeping and graphics isolated; BIOS rand returns a fixture value. No complete original secondary-animation/weapon-target/JP/multistrike trace.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Reaction oracle:',len(stages),'stages,',len(potions),'stock,',len(chances),'Brave,',len(exp),'EXP',flush=True)


if __name__=='__main__':main()
