#!/usr/bin/env python3
"""Original cast initialization/payment order and Charge-only recoil cancellation."""
import json,re,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-incantation/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(init='battle_action_init_current_data',mp='battle_action_check_and_consume_mp',
        strike='battle_action_init_current_ability_strike_data',bind='battle_action_set_current_attacker_data',
        recoil='battle_status_remove_charging_ability_ct',handlers='battle_status_enable_special_flags')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['status_init']=bind_function('status_init','main_status_init_check_data','main',d);o.call('status_init')
    unit=0x801908cc;events=[]
    def external(uc,address,size,data):
        if address==s.FUNCTIONS['mp'][0]:events.append(['payment',struct.unpack('<H',uc.mem_read((unit&0x1fffff)+0x2c,2))[0]]);return
        if address==s.FUNCTIONS['bind'][0]:events.append(['binding',struct.unpack('<H',uc.mem_read((unit&0x1fffff)+0x2c,2))[0]]);return
        if address==s.FUNCTIONS['strike'][0]:events.append(['strike_init',struct.unpack('<H',uc.mem_read((unit&0x1fffff)+0x2c,2))[0]])
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in ('mp','bind','strike','handlers'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    yaml=(s.ROOT/'sources/fft_decomp/target/battle.yaml').read_text()
    hamedo=int(re.search(r'addr: (0x[0-9a-f]+), name: g_battle_action_hamedo_flag',yaml)[1],16)
    def load():
        o.uc.mem_write(unit&0x1fffff,bytes(21*0x1c0));o.write(hamedo,0)
        o.uc.mem_write((unit&0x1fffff)+3,b'\x4f')
        o.uc.mem_write((unit&0x1fffff)+0x2c,struct.pack('<H',999))
        o.uc.mem_write((unit&0x1fffff)+0x47,bytes((2,2,0)))
        o.uc.mem_write((unit&0x1fffff)+0x16f,b'\x0a')
        o.uc.mem_write((unit&0x1fffff)+0x170,struct.pack('<H',0xf))
        o.uc.mem_write(s.TILES&0x1fffff,bytes(8));o.write(0x8018f5fc,0)
    initialization=[]
    for kind,present,hp in ((5,0,0),(6,1,52),(6,1,0),(6,0,0)):
        load();a=unit&0x1fffff;t=a+0x1c0
        o.uc.mem_write(t+1,bytes((1 if present else 255,)))
        o.uc.mem_write(t+0x28,struct.pack('<H',hp));o.uc.mem_write(t+0x47,bytes((8,4,0)))
        o.uc.mem_write(a+0x178,bytes((kind,1)));o.uc.mem_write(a+0x17a,struct.pack('<hhh',8,0,4))
        events.clear();result=o.call('init',0)
        mp=struct.unpack('<H',o.uc.mem_read(a+0x2c,2))[0]
        dx,dy=struct.unpack('<hh',o.uc.mem_read(0x19389c+6,4))
        assert (result,mp,dx,dy)==(0,943,6,2)
        assert events==[['payment',999],['strike_init',943],['binding',943]]
        initialization.append(dict(kind=kind,present=present,hp=hp,result=result,mp=mp,events=list(events)))
    recoil=[]
    for skillset in (8,10):
        for timer in (0,3,255):
            for moved in (0,1):
                load();a=unit&0x1fffff
                o.uc.mem_write(a+0x16f,bytes((skillset,)));o.uc.mem_write(a+0x15d,bytes((timer,)))
                o.uc.mem_write(a+0x58,b'\x08');o.uc.mem_write(a+0x47,bytes((3 if moved else 2,2,0)))
                o.uc.mem_write(a+0x1d,bytes((11 if moved else 1,255,255)))
                result=o.call('recoil',unit,0)
                assert result==(skillset==8 and timer!=255)
                recoil.append([skillset,timer,moved,result])
    lines=['/* Original initialization/recoil predicates; local-only. */',
        'static const u16 source_cast_recoil[][4]={']
    lines+=[' {'+','.join(map(str,r))+'},'for r in recoil];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(initialization=initialization,recoil=recoil,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        reused_target_evidence='x/core-deferred/oracle/source.json: original live-unit vs stored-tile coordinate cases',
        scope='Unchanged original action initializer/MP/binding and Charge-only recoil predicate. Strike initializer and external status handlers isolated; payment/binding hooks only observe. Empty/KO/absent centres are initializer evidence, not full spell target-list/effect/presentation/MP-refund traces.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original incantation:',len(initialization),'payment/binding sequences,',len(recoil),'Charge-only recoil cases',flush=True)


if __name__=='__main__':main()
