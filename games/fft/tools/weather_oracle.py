#!/usr/bin/env python3
"""Original formula08 weather, Oil removal and Float/earth interactions; local-only fixtures."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-weather/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(calendar='battle_weather_randomize_from_calendar',variable='battle_script_get_variable',damage='battle_formula_magical_damage',weather='battle_map_get_effective_weather',
        random='battle_formula_get_random_0_7fff',final='battle_action_finalize_target_current_action',
        commit='battle_action_finalize_attack_and_flag_reactions',reaction='battle_action_dispatch_target_reaction_ability',
        split='battle_action_check_mp_switch_distribute_and_damage_split_usability',
        stat='battle_unit_apply_stat_increment_decrement')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    weather=[0]
    actor=0x801908cc;target=actor+10*0x1c0;ability=0x801938c0;rolls=[]
    def external(uc,address,size,data):
        if address==s.FUNCTIONS['random'][0]:raise AssertionError('Non-evadeable carrier rolled evasion')
        if address==s.FUNCTIONS['stat'][0]:
            # Stop the unchanged commit function after its HP/MP stores, before stats/statuses.
            uc.reg_write(o.reg.UC_MIPS_REG_PC,0x801ff000);uc.emu_stop();return
        if address==s.FUNCTIONS['roll'][0]:
            pair=[uc.reg_read(o.reg.UC_MIPS_REG_A0),uc.reg_read(o.reg.UC_MIPS_REG_A1)]
            assert pair==[100,19],pair;rolls.append(pair);value=1
        elif address==s.FUNCTIONS['variable'][0]:
            assert uc.reg_read(o.reg.UC_MIPS_REG_A0)==0x23;value=weather[0]
        else:value=0                  # unrelated reaction/MP-split isolation
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in ('variable','roll','random','reaction','split','stat'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    holy=d.at(s.battle.ABILITY_RANGE+14*0xf,14);assert holy[7]==2 and not(holy[6]&2)
    # MA,power,element,raw_weather,map_flags,oil_mode,float,absorb,null,
    # half,weak,strengthen,execute,attacker_faith,target_faith
    profiles=[(5,14,e,w,flags,0,0,0,0,0,0,0,1,70,70)
        for w in range(5)for flags in range(4)for e in (128,64,32,8,192,224,2,0)]
    profiles += [(5,14,e,3,0,oil,0,0,0,0,0,0,execute,70,70)
        for oil in range(6)for e in (128,8,136,2)for execute in (0,1)]
    profiles += [(5,14,e,4,0,1,1,absorb,null,half,weak,1,1,70,70)
        for e in (8,136,224,2)for absorb,null,half,weak in ((1,0,0,0),(1,1,1,1),(0,0,1,1),(0,0,0,0))]
    profiles += [(ma,255,e,w,snow,1,0,absorb,0,half,weak,1,1,70,70)
        for ma in (98,99)for e,w,snow in ((128,3,0),(64,4,0),(32,3,1),(192,3,0),(224,4,0))
        for absorb,half,weak in ((0,0,0),(0,1,0),(1,0,1))]
    rows=[];records=[]
    for p in profiles:
        ma,power,element,w,flags,oil,floating,absorb,null,half,weak,strength,execute,af,tf=p
        up=0;defense=shell=frog=0
        weather[0]=w;o.write(0x800b6698,flags);effective=o.call('weather')
        for ptr in (actor,target):
            o.uc.mem_write(ptr&0x1fffff,bytes(0x1c0));o.uc.mem_write((ptr&0x1fffff)+6,b'\x80')
            o.uc.mem_write((ptr&0x1fffff)+0x28,struct.pack('<4H',100,999,100,999))
        o.uc.mem_write((actor&0x1fffff)+0x37,bytes((ma,)))
        o.uc.mem_write((actor&0x1fffff)+0x90,bytes((4 if up else 0,)))
        o.uc.mem_write((target&0x1fffff)+0x90,bytes((2 if defense else 0,)))
        innate=mask(*([16]if oil in (2,3)else[]))
        inflicted=mask(*([16]if oil in (1,3,4,5)else[]),*([17]if floating or oil==5 else[]))
        immunity=mask(*([16]if oil==4 else[]))
        o.uc.mem_write((target&0x1fffff)+0x4e,innate+immunity+bytes(a|b for a,b in zip(innate,inflicted)))
        o.uc.mem_write((target&0x1fffff)+0x1bb,inflicted)
        o.uc.mem_write((actor&0x1fffff)+0x71,bytes((element if strength else 0,)))
        o.uc.mem_write((target&0x1fffff)+0x6d,bytes(element if f else 0 for f in (absorb,null,half,weak,0)))
        o.uc.mem_write(ability&0x1fffff,bytes(0x50));r=bytearray(holy);r[7]=element;r[10]=power
        o.uc.mem_write((ability&0x1fffff)+0x30,bytes(r));o.uc.mem_write((ability&0x1fffff)+0x12,bytes((tf,af)))
        o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',0xf))
        o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
        o.uc.mem_write((target&0x1fffff)+0x18c,b'\x01')
        o.uc.mem_write((target&0x1fffff)+0x18c+0x2a,struct.pack('<H',100))
        o.write(0x8018f5fc,0 if execute else 2);rolls.clear();o.call('damage');pre=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));o.call('final')
        raw=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));hit=raw[0];kind=raw[0x25]
        damage,healing=struct.unpack_from('<hh',raw,4);o.call('commit',10)
        hp=struct.unpack('<H',o.uc.mem_read((target&0x1fffff)+0x28,2))[0]
        assert len(rolls)==(execute if hit else 0),(p,rolls,raw)
        removal=raw[0x22]&0x80
        xa=struct.unpack('<H',o.uc.mem_read((ability&0x1fffff)+0xe,2))[0]
        assert hp==max(0,min(999,100-damage+healing))
        row=[*p,effective,hit,kind,damage&65535,healing&65535,hp,len(rolls),removal,xa,struct.unpack_from('<H',pre,6 if pre[0x25]&64 else 4)[0]]
        rows.append(row);records.append(dict(input=p,effective_weather=effective,hit=hit,kind=kind,damage=damage,healing=healing,hp=hp,draws=len(rolls),oil_removal=removal,xa=xa))
    (out/'fwq00.bin').write_bytes(b'FWQ1'+struct.pack('>H',len(rows))+b''.join(struct.pack('>25H',*row)for row in rows))
    report=dict(cases=records,functions=o.function_hashes,holy_range=list(holy),exe_sha256=s.core_audit.sha(d.exe),
        battle_sha256=s.core_audit.sha(d.bat),decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original formula08/weather normalization/elemental/Oil filter/Float/finalizer/signed HP-prefix instructions; script weather getter supplied and normal proc roll isolated. Human/unmounted, no spell status set. Synthetic multi-element/power255 probes expose sequential narrowing. Commit stops before stat/status/KO/reward handlers: removal is observed in the prepared packet, not claimed as a full original live status/campaign trace.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Original weather/Oil/Float:',len(rows),'profiles; raw Oil packets observed')


if __name__=='__main__':main()
