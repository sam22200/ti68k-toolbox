#!/usr/bin/env python3
"""Bounded original-MIPS Sleep vulnerability, wake commit and AT fixtures."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-sleep/oracle';out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(xa='battle_formula_apply_target_xa_affecting_statuses',
        zodiac='battle_formula_apply_zodiac_compatibility',
        physical='battle_formula_apply_physical_status_support_compatibility',
        finalize='battle_action_finalize_attack_and_flag_reactions',
        order='battle_action_calculate_at_list',handlers='battle_status_enable_special_flags')
    # External reaction/equipment/reward subsystems are outside this fixture.
    # HP arithmetic, attack-type wake staging and bitmap/timer resolution execute.
    isolated=['battle_action_dispatch_target_reaction_ability',
        'battle_action_check_mp_switch_distribute_and_damage_split_usability',
        'battle_action_check_stat_save_and_restore_reaction_usability',
        'battle_action_remove_broken_or_stolen_equipment',
        'battle_action_add_war_funds','battle_action_handle_steal_exp',
        'battle_unit_apply_level_up_down_ability',
        'battle_action_add_poached_item_to_fur_shop_inventory',
        'battle_unit_update_attacker_earned_experience','battle_status_remove_control']
    for key,name in names.items():bind_function(key,name,'battle',disc)
    for i,name in enumerate(isolated):bind_function('external'+str(i),name,'battle',disc)
    o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',disc);o.call('init')
    def external(uc,address,size,data):
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in ['handlers']+['external'+str(i)for i in range(len(isolated))]:
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    unit,actor,ability=0x801908cc,0x801d4000,0x801938c0;action=unit+0x18c
    o.write(0x80192d94,actor);o.write(0x80192d98,unit);o.write(0x80192d90,action)
    def load(ids=(),innate=()):
        o.uc.mem_write(unit&0x1fffff,bytes(0x1c0));o.uc.mem_write(actor&0x1fffff,bytes(0x1c0))
        bits,traits=mask(*ids),mask(*innate)
        o.uc.mem_write((unit&0x1fffff)+0x4e,traits+bytes(5)+bytes(a|b for a,b in zip(bits,traits)))
        o.uc.mem_write((unit&0x1fffff)+0x1bb,bits)
        o.uc.mem_write((unit&0x1fffff)+0x5d,bytes([17]*16))
        o.uc.mem_write((unit&0x1fffff)+0x24,bytes((60,60,60)))
        o.uc.mem_write((unit&0x1fffff)+0x28,struct.pack('<4H',100,100,10,20))
        o.uc.mem_write((unit&0x1fffff)+0x30,bytes([6]*9))
        o.uc.mem_write(ability&0x1fffff,bytes(0x50))
        o.write(0x80192d98,unit);o.write(0x80192d90,action)
    xa=[]
    for bits in range(4):
        for raw in (1,5,7,9,11,31,99):
            for sign,female in ((0,0),(3,0),(4,0),(6,0),(6,1)):
                load([sid for b,sid in enumerate((35,4))if bits&(1<<b)])
                o.uc.mem_write((actor&0x1fffff)+6,b'\x80')
                o.uc.mem_write((unit&0x1fffff)+6,bytes((0x40 if female else 0x80,)))
                o.uc.mem_write((unit&0x1fffff)+8,struct.pack('<H',sign<<12))
                o.uc.mem_write((ability&0x1fffff)+0x0e,struct.pack('<H',raw))
                o.call('physical')
                value=struct.unpack('<H',o.uc.mem_read((ability&0x1fffff)+0x0e,2))[0]
                xa.append([bits,raw,sign,female,value])
    wakes=[];labels=[]
    cases=[('damage',1,0x80,7,0,0,0),('zero_damage',1,0x80,0,0,0,0),
        ('miss',0,0x80,7,0,0,0),('healing',1,0x40,0,7,0,0),
        ('mp_damage',1,0x20,0,0,7,0),('status_only',1,0,0,0,0,0),
        ('untyped_hp_damage',1,0,7,0,0,0),('innate_sleep',1,0x80,7,0,0,1),
        ('knockout',1,0x80,100,0,0,0)]
    o.write(0x8018f5f0,1);o.write(0x8018f5fc,2)
    for label,hit,kind,damage,heal,mp,innate in cases:
        load((35,),(35,)if innate else ())
        o.uc.mem_write(action&0x1fffff,bytes((hit,)))
        o.uc.mem_write((action&0x1fffff)+4,struct.pack('<3h',damage,heal,mp))
        o.uc.mem_write((action&0x1fffff)+0x25,bytes((kind,)))
        o.call('finalize',0)
        hp=struct.unpack('<H',o.uc.mem_read((unit&0x1fffff)+0x28,2))[0]
        sleep=bool(o.uc.mem_read((unit&0x1fffff)+0x5c,1)[0]&0x10)
        timer=o.uc.mem_read((unit&0x1fffff)+0x68,1)[0]
        wakes.append([hit,kind,damage,heal,mp,innate,hp,int(sleep),timer]);labels.append(label)
    orders=[]
    for stop,sleep in ((0,1),(0,60),(20,1),(1,60)):
        o.uc.mem_write(unit&0x1fffff,bytes(21*0x1c0))
        for i in range(21):o.uc.mem_write((unit&0x1fffff)+i*0x1c0+1,b'\xff')
        for i,ct in ((0,0),(1,50)):
            at=(unit&0x1fffff)+i*0x1c0;o.uc.mem_write(at+1,bytes((i,)))
            o.uc.mem_write(at+0x38,bytes((6,ct)));o.uc.mem_write(at+0x15d,b'\xff')
            o.uc.mem_write(at+0x18a,bytes((i,)))
        o.uc.mem_write((unit&0x1fffff)+0x186,b'\x01')
        at=(unit&0x1fffff)+0x1c0
        o.uc.mem_write(at+0x5b,bytes((2 if stop else 0,0x10)))
        o.uc.mem_write(at+0x63,bytes((stop,)));o.uc.mem_write(at+0x68,bytes((sleep,)))
        o.write(0x8018f51c,0);o.call('order',0x801d6000,1)
        data=bytes(o.uc.mem_read(0x1d6000,160));orders.append([stop,sleep,*[data[4*i]for i in range(6)]])
    lines=['/* Generated from unchanged original MIPS; local-only. */']
    for name,rows in [('source_xa',xa),('source_wakes',wakes),('source_orders',orders)]:
        lines.append(f'static const u16 {name}[][{len(rows[0])}]={{')
        lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(xa=xa,wakes=wakes,wake_labels=labels,orders=orders,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),isolated=isolated,
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original physical-status/support caller executes target-XA then zodiac (no equipped supports), original finalize HP/wake branch with actual bitmap/timer resolution and four turn-only AT lists. External reaction/equipment/reward/control and status graphics handlers isolated; no full PS1 battle, elemental/MP-switch/reaction or innate producer certificate.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Sleep original:',len(xa),'XA,',len(wakes),'wake,',len(orders),'AT cases',flush=True)


if __name__=='__main__':main()
