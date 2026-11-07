#!/usr/bin/env python3
"""Bounded original MIPS Quick/charge priority, CT consumption and AT fixtures."""
import json,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-scheduler/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(clock='battle_action_run_between_turn_control',check='battle_status_check_unit',
        expire='battle_status_update_expiring',upkeep='battle_status_build_upkeep_action',
        order='battle_action_calculate_at_list')
    for key,name in names.items():bind_function(key,name,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=3000000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    base=0x801908cc;phase=0x8018f518;events=[]
    def external(uc,address,size,data):
        key='expire'if address==s.FUNCTIONS['expire'][0]else'upkeep'
        events.append([key,uc.reg_read(o.reg.UC_MIPS_REG_A0)])
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in ('expire','upkeep'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    def reset(p=1):
        o.uc.mem_write(base&0x1fffff,bytes(21*448));o.write(phase,p)
        o.write(0x8019389c,0);o.write(0x8018f51c,0);o.write(0x8018f514,0)
        for i in range(21):o.uc.mem_write((base&0x1fffff)+i*448+1,b'\xff')
        events.clear()
    def actor(i,ct,speed=1,charge=255,ids=()):
        at=(base&0x1fffff)+i*448
        o.uc.mem_write(at+1,bytes((i,)));o.uc.mem_write(at+0x38,bytes((speed,ct)))
        o.uc.mem_write(at+0x58,mask(*ids));o.uc.mem_write(at+0x15d,bytes((charge,)))
        o.uc.mem_write(at+0x18a,bytes((i,)))
    picks=[]
    for p,ct0,ct1,stop in [(1,254,255,0),(1,255,255,0),(1,200,199,0),
                           (1,255,254,1),(9,254,255,0),(9,255,255,0)]:
        reset(p);actor(0,ct0,ids=(30,)if stop else());actor(12,ct1)
        result=o.call('clock',0);chosen=result&255
        assert result&0xff00==0x100
        after=o.uc.mem_read((base&0x1fffff)+chosen*448+0x39,1)[0]
        picks.append([p,ct0,ct1,stop,chosen,after])
    reset(2);actor(1,0,charge=1);actor(7,0,charge=1);actor(12,50,speed=50)
    first=o.call('clock',0);assert first==0x201
    actor(2,255);actor(10,255);wave=[]
    for expected in (0x102,0x10a,0x207):
        result=o.call('clock',0);assert result==expected,(result,expected);wave.append(result)
        if result&0xff00==0x100:
            # An injected completed turn resumes the original documented state9.
            at=(base&0x1fffff)+(result&255)*448
            assert o.uc.mem_read(at+0x39,1)==b'\0', (picks, result, o.uc.mem_read(at+0x38,4).hex(), o.uc.mem_read(phase&0x1fffff,4).hex())
            o.uc.mem_write(at+0x186,b'\0');o.write(phase,9)
    assert not any(e[0]=='expire'for e in events)
    # The original itself yields NONE0xff00 after1201 phases instead of hanging.
    stalls=[]
    for ids,speed in [((30,),6),((35,),6),((),0)]:
        reset();actor(0,0,speed=speed,ids=ids);actor(12,0,speed=speed,ids=ids)
        result=o.call('clock',0);assert result==0xff00,(ids,speed,result,events[:5])
        stalls.append(dict(status=list(ids),speed=speed,result=result,phase=o.uc.mem_read(phase&0x1fffff,4).hex(),expiry_calls=sum(e[0]=='expire'for e in events)))
    orders=[]
    for speed,ct in ((6,255),(0,0),(0,255)):
        reset();actor(0,ct,speed=speed);actor(1,50,speed=6)
        o.uc.mem_write((base&0x1fffff)+0x186,b'\x01');o.write(0x8018f51c,0)
        o.call('order',0x801d6000,1)
        data=bytes(o.uc.mem_read(0x1d6000,160));orders.append([speed,ct,*[data[4*i]for i in range(8)]])
    text=['/* Original MIPS scheduler/AT expectations; generated local evidence. */']
    for name,rows in [('source_clock_picks',picks),('source_clock_orders',orders)]:
        text.append(f'static const u16 {name}[][{len(rows[0])}]={{')
        text += [' {'+','.join(map(str,row))+'},'for row in rows];text.append('};')
    (out/'source.h').write_text('\n'.join(text)+'\n')
    report=dict(picks=picks,first_charge=first,quick_wave=wave,stalls=stalls,orders=orders,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Unchanged original clock/check/AT instructions; expiry/upkeep callbacks isolated. Quick CT255 and completed-turn state9 are fixture inputs. Stalled source yields are measured, not complete original status effects or a retail Quick producer.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))


if __name__=='__main__':main()
