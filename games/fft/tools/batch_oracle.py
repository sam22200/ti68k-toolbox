#!/usr/bin/env python3
"""Original formula08 compound preparation with no intervening HP commits; local fixtures."""
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
    out=s.HERE/'campaign/x/core-batch/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(damage='battle_formula_magical_damage',final='battle_action_finalize_target_current_action',
        weather='battle_map_get_effective_weather',random='battle_formula_get_random_0_7fff',
        presence='battle_target_is_unit_untargetable_and_store_tile')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    for k,n in dict(roll='main_util_roll_pass_fail',init='main_status_init_check_data').items():
        o.function_hashes[k]=bind_function(k,n,'main',d)
    o.call('init');actor=0x801908cc;ability=0x801938c0;draws=[];rng=[1]
    def external(uc,address,size,data):
        if address==s.FUNCTIONS['random'][0]:raise AssertionError('Unexpected direct RNG path')
        value=0
        if address==s.FUNCTIONS['roll'][0]:
            base=uc.reg_read(o.reg.UC_MIPS_REG_A0);ev=uc.reg_read(o.reg.UC_MIPS_REG_A1)
            rng[0],r=next_random(rng[0]);draws.append([base,ev,r]);value=int((r*base>>15)>=ev)
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in ('weather','random','roll'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    rows=[];records=[]
    for variant in range(13):
        seed=(1,9,73)[variant%3];element=(128,8,224)[(variant//4)%3];power=255 if variant%4==3 else 14
        evade=variant%4!=2;ab=0x10 if evade else 0xf;raw=bytearray(d.at(s.battle.ABILITY_RANGE+14*ab,14));raw[7]=element;raw[10]=power
        inputs=[]
        for i in range(13):
            sid=255 if i==0 or variant==12 else (255,2,1,5,8,31,15,255,255,255,255,255)[i-1]
            gear=(255,255,255)if i==0 or not evade else ((232,141,138)if i%3 else (234,136,143))
            inputs.append([*gear,sid,70 if i==0 else(30+5*i),int(i==9),int(i==10),int(i==11),int(i==12),1 if i==7 else 2 if i==8 else 0])
            ptr=actor+i*448;off=ptr&0x1fffff;o.uc.mem_write(off,bytes(448));o.uc.mem_write(off+1,bytes((i,)))
            o.uc.mem_write(off+6,b'\x80');o.uc.mem_write(off+0x28,struct.pack('<4H',0 if sid==2 else 100,999,999,999))
            o.uc.mem_write(off+0x37,bytes((99 if power==255 and i==0 else 5,)))
            innate=mask(*([16]if i==8 else[]));inflicted=mask(*([sid]if sid!=255 else[]),*([16]if i==7 else[]))
            o.uc.mem_write(off+0x4e,innate+bytes(5)+bytes(a|b for a,b in zip(innate,inflicted)));o.uc.mem_write(off+0x1bb,inflicted)
            o.uc.mem_write(off+0x26,bytes((inputs[-1][4],)))
            for slot,item,shield in ((0x1c,gear[0],None),(0x1d,gear[1],0x1e),(0x1f,gear[2],0x20)):
                item=0 if item==255 else item;flags=d.at(s.battle.ITEMS+12*item+3,1)[0]
                o.uc.mem_write(off+(shield if shield and flags&64 else slot),bytes((item,)))
            o.uc.mem_write(off+0x6d,bytes(element if f else 0 for f in inputs[-1][5:9])+b'\0')
        rng[0]=0x2545f491^((seed<<16)|seed);draws.clear();expected=[]
        for i,p in enumerate(inputs):
            ptr=actor+i*448;off=ptr&0x1fffff;absent=o.call('presence',i,0x801d6000)
            if absent:expected.append([0,0,0,8]);continue
            o.uc.mem_write(ability&0x1fffff,bytes(0x50));o.uc.mem_write((ability&0x1fffff)+0x30,bytes(raw))
            o.uc.mem_write((ability&0x1fffff)+0x12,bytes((p[4],70)));o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.write(0x80192d94,actor);o.write(0x80192d98,ptr);o.write(0x80192d90,ptr+0x18c)
            o.uc.mem_write(off+0x18c,b'\x01');o.uc.mem_write(off+0x18c+0x2a,struct.pack('<H',100));o.write(0x8018f5fc,0)
            o.call('damage');o.call('final');packet=bytes(o.uc.mem_read(off+0x18c,44));hit=packet[0];kind=packet[0x25]
            effect=0 if kind&128 else 3 if kind&64 else 8;amount=struct.unpack_from('<H',packet,6 if kind&64 else 4)[0]
            if not hit:amount=0;effect=8
            elif packet[0x22]&128:effect|=128
            expected.append([1,hit,amount,effect])
        row=[seed,power,element,int(evade),rng[0]>>16,rng[0]&65535,len(draws)]
        for p,e in zip(inputs,expected):row+=p+e
        rows.append(row);records.append(dict(variant=variant,seed=seed,power=power,element=element,evade=evade,inputs=inputs,expected=expected,final_rng=rng[0],draws=list(draws)))
    (out/'fba00.bin').write_bytes(b'FBA1'+struct.pack('>H',len(rows))+b''.join(struct.pack('>'+str(len(row))+'H',*row)for row in rows))
    report=dict(cases=records,functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original MIPS presence/formula08/finalizer, thirteen source slots in ascending order, no HP commits between preparations. External xorshift pass/fail stream and normal weather supplied. Compound driver reconstructs observed ordinary strike order, not full original dispatcher/cost/HP/reaction/reward/presentation execution.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Original batch preparation:',len(rows),'groups of13 source slots')
if __name__=='__main__':main()
