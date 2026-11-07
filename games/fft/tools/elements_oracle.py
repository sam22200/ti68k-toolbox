#!/usr/bin/env python3
"""Original formula08 elemental results, final caps and signed HP commit prefix."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-elements/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(damage='battle_formula_magical_damage',weather='battle_map_get_effective_weather',
        random='battle_formula_get_random_0_7fff',final='battle_action_finalize_target_current_action',
        commit='battle_action_finalize_attack_and_flag_reactions',reaction='battle_action_dispatch_target_reaction_ability',
        split='battle_action_check_mp_switch_distribute_and_damage_split_usability',
        stat='battle_unit_apply_stat_increment_decrement')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    actor=0x801908cc;target=actor+10*0x1c0;ability=0x801938c0;rolls=[]
    def external(uc,address,size,data):
        if address==s.FUNCTIONS['random'][0]:raise AssertionError('Non-evadeable carrier rolled evasion')
        if address==s.FUNCTIONS['stat'][0]:
            # Stop the unchanged commit function after its HP/MP stores, before stats/statuses.
            uc.reg_write(o.reg.UC_MIPS_REG_PC,0x801ff000);uc.emu_stop();return
        if address==s.FUNCTIONS['roll'][0]:
            pair=[uc.reg_read(o.reg.UC_MIPS_REG_A0),uc.reg_read(o.reg.UC_MIPS_REG_A1)]
            assert pair==[100,19],pair;rolls.append(pair);value=1
        else:value=0                  # normal weather; unrelated reaction/MP split isolated
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in ('weather','roll','random','reaction','split','stat'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    holy=d.at(s.battle.ABILITY_RANGE+14*0xf,14);assert holy[7]==2 and not(holy[6]&2)
    # MA,power,element,faith,defender-faith,up,defense,shell,frog,strengthen,absorb,null,half,weak,execute
    profiles=[(5,50,2,70,70,0,0,0,0,flags&1,(flags>>1)&1,(flags>>2)&1,(flags>>3)&1,(flags>>4)&1,execute)
        for flags in range(32)for execute in (0,1)]
    profiles += [(ma,power,element,af,tf,up,0,0,frog,1,absorb,0,half,weak,1)
        for ma,power,element,af,tf,up,frog,absorb,half,weak in (
        (1,50,2,70,70,0,0,0,1,1),(7,50,2,3,97,1,0,1,1,0),
        (99,50,2,100,100,1,1,0,0,1),(99,50,2,100,100,1,1,1,0,1),
        (99,96,0,100,100,1,1,0,0,0),(99,96,2,100,100,1,1,0,0,1),
        (99,96,2,100,100,1,1,1,0,1),(99,96,2,70,70,1,1,1,1,0),
        (5,50,128,70,70,0,0,1,1,1),(5,50,0,70,70,0,0,1,1,1))]
    profiles += [(5,50,2,70,70,1,1,1,1,1,0,0,1,1,1),
        (1,50,2,100,100,1,1,1,1,1,1,0,0,0,0),
        (98,255,2,70,70,1,0,0,1,1,1,0,1,0,1)]
    rows=[]
    for p in profiles:
        ma,power,element,af,tf,up,defense,shell,frog,strength,absorb,null,half,weak,execute=p
        for ptr in (actor,target):
            o.uc.mem_write(ptr&0x1fffff,bytes(0x1c0));o.uc.mem_write((ptr&0x1fffff)+6,b'\x80')
            o.uc.mem_write((ptr&0x1fffff)+0x28,struct.pack('<4H',100,999,100,999))
        o.uc.mem_write((actor&0x1fffff)+0x37,bytes((ma,)))
        o.uc.mem_write((actor&0x1fffff)+0x90,bytes((4 if up else 0,)))
        o.uc.mem_write((target&0x1fffff)+0x90,bytes((2 if defense else 0,)))
        o.uc.mem_write((target&0x1fffff)+0x58,mask(*([27]if shell else[]),*([22]if frog else[])))
        o.uc.mem_write((actor&0x1fffff)+0x71,bytes((element if strength else 0,)))
        o.uc.mem_write((target&0x1fffff)+0x6d,bytes(element if f else 0 for f in (absorb,null,half,weak,0)))
        o.uc.mem_write(ability&0x1fffff,bytes(0x50));r=bytearray(holy);r[7]=element;r[10]=power
        o.uc.mem_write((ability&0x1fffff)+0x30,bytes(r));o.uc.mem_write((ability&0x1fffff)+0x12,bytes((tf,af)))
        o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',0xf))
        o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
        o.uc.mem_write((target&0x1fffff)+0x18c,b'\x01')
        o.uc.mem_write((target&0x1fffff)+0x18c+0x2a,struct.pack('<H',100))
        o.write(0x8018f5fc,0 if execute else 2);rolls.clear();o.call('damage');o.call('final')
        raw=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));hit=raw[0];kind=raw[0x25]
        damage,healing=struct.unpack_from('<hh',raw,4);o.call('commit',10)
        hp=struct.unpack('<H',o.uc.mem_read((target&0x1fffff)+0x28,2))[0]
        assert len(rolls)==(execute if hit else 0),(p,rolls,raw)
        assert hp==max(0,min(999,100-damage+healing))
        rows.append([*p,hit,kind,damage&65535,healing&65535,hp,len(rolls)])
    lines=['/* Original elemental formula/finalizer and HP-prefix; no weather/Oil/Float. */',
        'static const u16 source_elements[][21]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(cases=rows,functions=o.function_hashes,holy_range=list(holy),exe_sha256=s.core_audit.sha(d.exe),
        battle_sha256=s.core_audit.sha(d.bat),decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original non-evadeable formula08 carrier, normal weather and no Oil/Float/mount/monster profile; elemental masks varied explicitly, including synthetic power/element combinations. Finalizer unchanged; commit runs only through signed HP/MP stores with reaction/MP-split isolated and stops before stat/status/KO/EXP handling. Not a full spell/campaign/presentation or reaction trace.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Original elements:',len(rows),'typed/capped/signed HP profiles')


if __name__=='__main__':main()
