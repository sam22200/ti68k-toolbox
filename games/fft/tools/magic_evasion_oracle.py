#!/usr/bin/env python3
"""Original formula08 magic evasion, presence and post-formula validity; local-only fixtures."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask

def next_random(seed):
    seed ^= (seed << 13) & 0xffffffff
    seed ^= seed >> 17
    seed ^= (seed << 5) & 0xffffffff
    return seed,seed >> 17

def main():
    import unicorn
    out=s.HERE/'campaign/x/core-magic-evasion/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(evade='battle_formula_calculate_magical_evade',damage='battle_formula_magical_damage',
        final='battle_action_finalize_target_current_action',weather='battle_map_get_effective_weather',
        random='battle_formula_get_random_0_7fff',sort='battle_target_sort_list',
        presence='battle_target_is_unit_untargetable_and_store_tile')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    for k,n in dict(roll='main_util_roll_pass_fail',init='main_status_init_check_data').items():
        o.function_hashes[k]=bind_function(k,n,'main',d)
    o.call('init');actor=0x801908cc;target=actor+10*0x1c0;ability=0x801938c0
    draws=[];rng=[1]
    def external(uc,address,size,data):
        if address==s.FUNCTIONS['random'][0]:raise AssertionError('Unexpected direct RNG path')
        if address==s.FUNCTIONS['roll'][0]:
            base=uc.reg_read(o.reg.UC_MIPS_REG_A0);ev=uc.reg_read(o.reg.UC_MIPS_REG_A1)
            rng[0],r=next_random(rng[0]);draws.append([base,ev,r]);value=int((r*base>>15)>=ev)
        else:value=0 # normal weather; graphical target-list sorting isolated
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in ('weather','random','roll','sort'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    # ability, accessory, right shield/weapon, left shield/weapon, status+1,
    # Defend, Transparent, Abandon, depth, seed16. No monster/mount/movement producers.
    profiles=[]
    for ab in (0xf,0x10):
        for gear in ((255,255,255),(232,141,138),(234,136,143),(136,232,1)):
            for flags in range(8):
                for seed in (1,9,73):profiles.append((ab,*gear,0,flags&1,(flags>>1)&1,(flags>>2)&1,0,seed))
    for sid in (1,2,4,5,7,8,10,15,17,19,20,21,22,30,31,35,37):
        for ab in (0xf,0x10):profiles.append((ab,232,141,138,sid+1,1,0,1,0,73))
    for sid in (255,17,20,21,22,37):
        for depth in (1,2):profiles.append((0x10,232,141,138,0 if sid==255 else sid+1,0,0,1,depth,9))
    def reset(p):
        ab,acc,right,left,status,defend,transparent,abandon,depth,seed=p
        for ptr in (actor,target):
            o.uc.mem_write(ptr&0x1fffff,bytes(0x1c0));o.uc.mem_write((ptr&0x1fffff)+6,b'\x80')
            o.uc.mem_write((ptr&0x1fffff)+0x28,struct.pack('<4H',999,999,999,999))
        o.uc.mem_write((actor&0x1fffff)+0x37,b'\x05')
        o.uc.mem_write((actor&0x1fffff)+0x58,mask(*([19]if transparent else[])))
        o.uc.mem_write((target&0x1fffff)+0x58,mask(*([status-1]if status else[]),*([6]if defend else[])))
        if status==3:o.uc.mem_write((target&0x1fffff)+0x28,bytes(2))
        for off,item,shieldoff in ((0x1c,acc,None),(0x1d,right,0x1e),(0x1f,left,0x20)):
            item=0 if item==255 else item
            flags=d.at(s.battle.ITEMS+12*item+3,1)[0]
            o.uc.mem_write((target&0x1fffff)+(shieldoff if shieldoff and flags&0x40 else off),bytes((item,)))
        o.uc.mem_write((target&0x1fffff)+0x8e,bytes((16 if abandon else 0,)))
        o.uc.mem_write((s.TILES&0x1fffff)+3,bytes((depth<<5,)))
        o.uc.mem_write(ability&0x1fffff,bytes(0x50));r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x12,b'FF')
        o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
        o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
        o.uc.mem_write((target&0x1fffff)+0x18c,b'\x01')
        o.uc.mem_write((target&0x1fffff)+0x18c+0x2a,struct.pack('<H',100))
        rng[0]=0x2545f491^((seed<<16)|seed);draws.clear()
    rows=[];records=[]
    for p in profiles:
        reset(p);o.write(0x8018f5fc,2);o.call('evade')
        accuracy=struct.unpack('<H',o.uc.mem_read((target&0x1fffff)+0x18c+0x2a,2))[0]
        preview_draws=list(draws)
        reset(p);o.write(0x8018f5fc,0);miss=o.call('evade');evadedraws=len(draws)
        reset(p);absent=o.call('presence',10,0x801d6000)
        o.write(0x8018f5fc,0);o.call('damage');o.call('final')
        raw=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44))
        hit=raw[0];damage=struct.unpack_from('<H',raw,4)[0]
        rows.append([*p,accuracy,1-miss,evadedraws,hit,damage,len(draws),absent])
        records.append(dict(input=p,preview_accuracy=accuracy,preview_draws=preview_draws,execute_draws=list(draws),absent=absent,hit=hit,damage=damage))
    (out/'fme00.bin').write_bytes(b'FME1'+struct.pack('>H',len(rows))+b''.join(struct.pack('>17H',*row)for row in rows))
    lines=['/* Original unmounted human magical-evasion, formula08 and finalizer profiles. */',
        'static const u16 source_magic_evasion[][17]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(cases=records,functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original unmodified MIPS magical-evade, formula08, presence predicate and finalizer; deterministic external pass/fail RNG supplied with its exact scaling. Normal weather, unmounted human equipment, no special movement producers or status cancellation. Target-list graphical sorting stubbed. Preview RNG intentionally remains pure in the port. No full target-list/HP/status/EXP/reaction/area/presentation certificate.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Original magical evasion:',len(rows),'profiles')

if __name__=='__main__':main()
