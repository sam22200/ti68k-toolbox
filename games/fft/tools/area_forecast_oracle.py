#!/usr/bin/env python3
"""Four bounded original formula08 preview groups; reuse compound input evidence."""
import argparse,json,struct,subprocess
from pathlib import Path
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=s.HERE/'campaign/x/core-area-forecast/oracle')
    out=ap.parse_args().out.resolve();out.mkdir(parents=True,exist_ok=True)
    previous=s.HERE/'campaign/x/core-batch/oracle/source.json'
    source=json.loads(previous.read_text());d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    assert source['exe_sha256']==s.core_audit.sha(d.exe)
    assert source['battle_sha256']==s.core_audit.sha(d.bat)
    names=dict(damage='battle_formula_magical_damage',final='battle_action_finalize_target_current_action',
        weather='battle_map_get_effective_weather',random='battle_formula_get_random_0_7fff',
        presence='battle_target_is_unit_untargetable_and_store_tile',sort='battle_target_sort_list')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    for k,n in dict(roll='main_util_roll_pass_fail',init='main_status_init_check_data').items():
        o.function_hashes[k]=bind_function(k,n,'main',d)
    o.call('init');actor=0x801908cc;ability=0x801938c0;rolls=[]
    def external(uc,address,size,data):
        if address==s.FUNCTIONS['random'][0]:raise AssertionError('Unexpected direct RNG path')
        value=0
        if address==s.FUNCTIONS['roll'][0]:
            rolls.append([uc.reg_read(o.reg.UC_MIPS_REG_A0),uc.reg_read(o.reg.UC_MIPS_REG_A1)])
            value=1 # Source preview still draws evasion; pure port omits those draws.
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value) # normal weather; graphical sort isolated
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in ('weather','random','roll','sort'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    rows=[];cases=[]
    for variant in (0,2,3,12):
        profile=source['cases'][variant];power=profile['power'];element=profile['element'];evade=profile['evade']
        inputs=profile['inputs'];ab=0x10 if evade else 0xf
        raw=bytearray(d.at(s.battle.ABILITY_RANGE+14*ab,14));raw[7]=element;raw[10]=power
        for i,p in enumerate(inputs):
            off=(actor+i*448)&0x1fffff;sid=p[3];o.uc.mem_write(off,bytes(448))
            o.uc.mem_write(off+1,bytes((i,)));o.uc.mem_write(off+6,b'\x80')
            o.uc.mem_write(off+0x28,struct.pack('<4H',0 if sid==2 else 100,999,999,999))
            o.uc.mem_write(off+0x37,bytes((99 if power==255 and i==0 else 5,)))
            innate=mask(*([16]if p[9]==2 else[]));inflicted=mask(*([sid]if sid!=255 else[]),*([16]if p[9]==1 else[]))
            o.uc.mem_write(off+0x4e,innate+bytes(5)+bytes(a|b for a,b in zip(innate,inflicted)))
            o.uc.mem_write(off+0x1bb,inflicted);o.uc.mem_write(off+0x26,bytes((p[4],)))
            for slot,item,shield in ((0x1c,p[0],None),(0x1d,p[1],0x1e),(0x1f,p[2],0x20)):
                item=0 if item==255 else item;flags=d.at(s.battle.ITEMS+12*item+3,1)[0]
                o.uc.mem_write(off+(shield if shield and flags&64 else slot),bytes((item,)))
            o.uc.mem_write(off+0x6d,bytes(element if flag else 0 for flag in p[5:9])+b'\0')
        expected=[];rolls.clear()
        for i,p in enumerate(inputs):
            ptr=actor+i*448;off=ptr&0x1fffff
            if o.call('presence',i,0x801d6000):expected.append([0,0,0,8]);continue
            o.uc.mem_write(ability&0x1fffff,bytes(0x50));o.uc.mem_write((ability&0x1fffff)+0x30,bytes(raw))
            o.uc.mem_write((ability&0x1fffff)+0x12,bytes((p[4],70)))
            o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.write(0x80192d94,actor);o.write(0x80192d98,ptr);o.write(0x80192d90,ptr+0x18c)
            o.uc.mem_write(off+0x18c,b'\x01');o.uc.mem_write(off+0x18c+0x2a,struct.pack('<H',100))
            o.write(0x8018f5fc,2);o.call('damage');o.call('final')
            packet=bytes(o.uc.mem_read(off+0x18c,44));kind=packet[0x25]
            effect=0 if kind&128 else 3 if kind&64 else 8
            hit=struct.unpack_from('<H',packet,0x2a)[0] if packet[0] and effect!=8 else 0
            amount=struct.unpack_from('<H',packet,6 if kind&64 else 4)[0] if hit else 0
            if not hit:effect=8
            elif packet[0x22]&128:effect|=128
            expected.append([1,hit,amount,effect])
        row=[profile['seed'],power,element,int(evade),0,0,0]
        for p,e in zip(inputs,expected):row+=p+e
        rows.append(row);cases.append(dict(variant=variant,power=power,element=element,evade=evade,inputs=inputs,expected=expected,source_preview_rolls=list(rolls)))
    (out/'fbf00.bin').write_bytes(b'FBF1'+struct.pack('>H',len(rows))+b''.join(struct.pack('>189H',*row)for row in rows))
    report=dict(cases=cases,functions=o.function_hashes,exe_sha256=source['exe_sha256'],battle_sha256=source['battle_sha256'],
        reused_compound_inputs_sha256=s.core_audit.sha(previous.read_bytes()),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original MIPS presence/formula08/finalizer in preview mode2, four groups of13 slots, no HP commits. Source preview evasion draws observed/stubbed pass; native preview deliberately consumes no battle RNG. Normal weather, unmounted ordinary humans, graphical sort isolated. Not original area UI, AI choice, future-target movement or a complete dispatcher trace.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original formula08 previews:',len(rows),'groups of13; source evasion draws isolated')


if __name__=='__main__':main()
