#!/usr/bin/env python3
"""Original MIPS CT gain and finite/innate Haste/Slow AT segments, no spell producer."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-haste-slow/oracle';out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(clock='battle_action_run_between_turn_control',check='battle_status_check_unit',
        order='battle_action_calculate_at_list',expire='battle_status_update_expiring',
        upkeep='battle_status_build_upkeep_action')
    for key,name in names.items():bind_function(key,name,'battle',disc)
    o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=3000000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',disc);o.call('init')
    unit=0x801908cc;phase=0x8018f518
    def isolated(uc,address,size,data):
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in ('expire','upkeep'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,isolated,begin=at,end=at)
    def reset(p=0):
        o.uc.mem_write(unit&0x1fffff,bytes(21*448));o.write(phase,p)
        o.write(0x8019389c,0);o.write(0x8018f51c,0);o.write(0x8018f514,0)
        for i in range(21):o.uc.mem_write((unit&0x1fffff)+i*448+1,b'\xff')
    def actor(i,speed,ct,rate=0,innate=0,dead=0,stop=0,sleep=0,ht=0,st=0,charge=255):
        at=(unit&0x1fffff)+i*448
        o.uc.mem_write(at+1,bytes((i,)));o.uc.mem_write(at+0x38,bytes((speed,ct)))
        current=bytearray(mask(*([2]if dead else[])));current[3]|=rate|innate
        if stop:current[3]|=2
        if sleep:current[4]|=0x10
        o.uc.mem_write(at+0x51,bytes((innate,)));o.uc.mem_write(at+0x58,bytes(current))
        o.uc.mem_write(at+0x61,bytes((ht,st,stop)));o.uc.mem_write(at+0x68,bytes((sleep,)))
        o.uc.mem_write(at+0x15d,bytes((charge,)));o.uc.mem_write(at+0x18a,bytes((i,)))
    gains=[]
    cases=[(sp,0,rate,0,0)for sp in(0,1,5,6,33,34,49,50)for rate in(0,8,4,12)]
    cases += [(sp,ct,rate,0,0)for sp,ct,rate in((50,250,8),(50,200,8),(34,250,8),(6,250,4),(1,98,4))]
    cases += [(sp,0,rate,1,0)for sp in(5,50)for rate in(8,4,12)]
    cases += [(5,40,rate,0,1)for rate in(8,4)]
    for sp,ct,rate,dead,stop in cases:
        reset();actor(0,sp,ct,rate,dead=dead,stop=stop);actor(12,0,254)
        chosen=o.call('clock',0)&255
        after=o.uc.mem_read((unit&0x1fffff)+0x39,1)[0]
        gains.append([sp,ct,rate,dead,stop,chosen,after])
    # Isolate one charge-decrement phase: tempo must not scale its countdown.
    charges=[]
    for rate in(0,8,4,12):
        reset(2);actor(0,6,0,rate,charge=2);actor(12,0,254,charge=1)
        result=o.call('clock',0);assert result==0x20c
        charges.append([rate,o.uc.mem_read((unit&0x1fffff)+0x15d,1)[0]])
    # [speed,ct,rate,hasteTimer,slowTimer,innateRate,StopWait,SleepWait]
    profiles=[(5,0,8,t,0,0,0,0)for t in(0,1,2,5,20,255)]
    profiles += [(5,0,4,0,t,0,0,0)for t in(0,1,2,5,20,255)]
    profiles += [(5,0,12,30,3,0,0,0),(5,0,12,3,30,0,0,0),
        (5,0,8,0,0,8,0,0),(5,0,4,0,0,4,0,0),(5,0,12,0,0,12,0,0),
        (5,0,8,2,0,0,3,0),(5,0,8,20,0,0,0,7),(5,0,8,255,0,0,20,0),
        (5,80,12,12,5,0,3,0),(5,80,12,12,5,0,0,3)]
    profiles += [(sp,ct,8,255,0,0,0,0)for sp,ct in((33,0),(34,0),(35,47),(35,48),(50,24),(50,25))]
    profiles += [(sp,ct,rate,255,255,0,0,0)for sp,ct,rate in((0,0,8),(0,0,4),(1,90,4),(6,255,8),(6,255,4))]
    orders=[]
    for p in profiles:
        sp,ct,rate,ht,st,innate,stop,sleep=p
        reset(1);actor(0,sp,ct,rate,innate,stop=stop,sleep=sleep,ht=ht,st=st);actor(1,6,50)
        o.uc.mem_write((unit&0x1fffff)+0x186,b'\x01');o.call('order',0x801d6000,1)
        data=bytes(o.uc.mem_read(0x1d6000,160));orders.append([*p,*[data[i*4]for i in range(6)]])
    lines=['/* Generated unchanged-MIPS Haste/Slow clock/AT fixtures; local only. */']
    for name,rows in [('source_tempo_gain',gains),('source_tempo_charge',charges),('source_tempo_order',orders)]:
        lines.append(f'static const u16 {name}[][{len(rows[0])}]={{')
        lines.extend(' {'+','.join(map(str,row))+'},'for row in rows);lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(gains=gains,charges=charges,orders=orders,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Unchanged original CT gain/selection/status check, one charge countdown and AT instructions. Expiry/upkeep callbacks isolated; no retail Haste/Slow spell or full original timed-status dispatcher trace. Prior original lifetime staging evidence reused separately.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original tempo:',len(gains),'gain profiles,',len(charges),'charge phases,',len(orders),'AT lists')


if __name__=='__main__':main()
