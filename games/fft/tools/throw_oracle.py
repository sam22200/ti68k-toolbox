#!/usr/bin/env python3
"""Bounded original Throw formula99, stock selector and target finalizer evidence."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask
STATUS=((),(26,),(35,),(4,),(21,),(22,),(27,),(3,),(35,4,21))
def main():
    import unicorn
    out=s.HERE/'campaign/x/core-throw/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for k,n in dict(formula='battle_formula_damage_sp_times_wp',finish='battle_action_finalize_target_current_action',
        remove='battle_status_remove',sort='battle_target_sort_list',random='battle_formula_get_random_0_7fff').items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    for k,n in dict(roll='main_util_roll_pass_fail',lib_rand='rand',init='main_status_init_check_data').items():o.function_hashes[k]=bind_function(k,n,'main',d)
    o.call('init');rng=1;draws=0
    def external(uc,address,size,data):
        nonlocal rng,draws
        value=0
        if address!=s.FUNCTIONS['sort'][0]:
            rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1;value=rng>>17
            if address==s.FUNCTIONS['roll'][0]:value=int((value*uc.reg_read(o.reg.UC_MIPS_REG_A0)>>15)>=uc.reg_read(o.reg.UC_MIPS_REG_A1))
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in('sort','random','roll','lib_rand'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0
    profiles=[]
    # Every throwable source item, then modifier/status/element boundaries.
    for item in range(128):
        it=d.item(item);w=d.at(s.battle.WEAPONS+8*item,8)
        if it['flags']&128 and w[1]&2:profiles.append((item,7,0,0,0,0,0,0,0))
    for item in(1,37,104,125,126,127):
        for mode in range(len(STATUS)):profiles.append((item,17,0,6,1,1,1,mode,0))
        for affinity in range(1,5):profiles.append((item,99,0,0,0,0,0,0,affinity))
    for up in range(4):profiles.append((122,7,0,0,0,up,0,0,0))
    profiles=[(*p,0,0,1)for p in profiles]
    for gear in(1,2,3):
        for side in(0,1,2):
            for seed in(1,32767):profiles.append((122,7,0,0,0,0,0,0,0,gear,side,seed))
    rows=[]
    for p in profiles:
        item,sp,az,tz,sex,up,defense,mode,affinity,gear,side,seed=p;outputs=[]
        for execute in(0,1):
            for ptr,sign,female in((actor,az,0),(target,tz,sex)):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+0x1a,bytes((255,)*7))
                o.uc.mem_write(at+6,bytes((64 if female else 128,)));o.uc.mem_write(at+8,struct.pack('<H',sign<<12))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999))
            a=actor&0x1fffff;t=target&0x1fffff;v=ability&0x1fffff;w=d.at(s.battle.WEAPONS+8*item,8)
            o.uc.mem_write(a+0x38,bytes((sp,)));o.uc.mem_write(a+0x90,bytes((16 if up==1 else 4 if up==3 else 0,32 if up==2 else 0)))
            o.uc.mem_write(t+0x90,bytes((8 if defense else 0,)));current=mask(*STATUS[mode])
            o.uc.mem_write(t+0x58,current);o.uc.mem_write(t+0x1bb,current)
            if gear:
                o.uc.mem_write(t+0x1c,bytes((232 if gear&2 else 255,255,255,255,128 if gear&1 else 255)))
                o.uc.mem_write(t+0x43,bytes((d.job(0x4a)['cev'],)))
                o.uc.mem_write(t+0x47,b'\x01');o.uc.mem_write(t+0x48,struct.pack('<H',(3,1,0)[side]<<8))
            if affinity:o.uc.mem_write(t+0x6d+affinity-1,bytes((w[6],)))
            o.uc.mem_write(v,bytes(80));o.uc.mem_write(v+0x30,d.at(s.battle.ABILITY_RANGE,14));o.uc.mem_write(v+0x3e,w)
            o.uc.mem_write(v+0x16,struct.pack('<H',382));o.uc.mem_write(v+0x18,bytes((item,)))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            o.write(0x80192d8c,0x80192da0);o.uc.mem_write(0x192da0,bytes(44))
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws=0
            o.call('remove');o.call('formula');o.call('finish');result=bytes(o.uc.mem_read(t+0x18c,44))
            outputs.append(dict(hit=result[0],chance=struct.unpack_from('<H',result,42)[0],damage=struct.unpack_from('<h',result,4)[0],
                healing=struct.unpack_from('<h',result,6)[0],kind=result[37],draws=draws,rng=rng))
        rows.append(dict(input=p,preview=outputs[0],execute=outputs[1]))
    lines=['/* Unmodified retail Throw formula99 and finalizer; no Catch producer. */','static const u8 source_throw_status[][5]={']
    lines+=['{'+','.join(map(str,mask(*ids)))+'},'for ids in STATUS]
    lines+=['};','static const struct {u16 amount,preview_amount,seed;u8 item,sp,az,tz,sex,up,defense,mode,affinity,gear,side,hit,kind,draws,preview_kind,chance;} source_throw[]={']
    for row in rows:
        p=row['input'];v=row['execute'];kind=3 if v['kind']&64 else 0 if v['hit']else 8
        amount=v['healing']if kind==3 else v['damage'];pre=row['preview'];pk=3 if pre['kind']&64 else 0 if pre['hit']else 8
        pa=pre['healing']if pk==3 else pre['damage'];values=[amount,pa,p[-1],*p[:-1],v['hit'],kind,v['draws'],pk,pre['chance']if pre['hit']else 0]
        lines.append('{'+','.join(map(str,values))+'},')
    lines+=['};'];(out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        scope='Original Throw formula99, physical/support/status/zodiac/element helpers and finalizer. Controlled RNG/sort. Human targets including accessory/shield/front/side/back evasion; Catch reaction remains a separate producer.'),indent=2)+'\n')
    print('Original Throw profiles:',len(rows),flush=True)
if __name__=='__main__':main()
