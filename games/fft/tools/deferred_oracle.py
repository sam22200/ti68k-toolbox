#!/usr/bin/env python3
"""Original equal-tick charge ordering and unit-versus-tile coordinate binding."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-deferred/oracle';out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    symbols=dict(clock='battle_action_run_between_turn_control',check='battle_status_check_unit',
        target='battle_action_set_current_attacker_data')
    hooks=dict(expire='battle_status_update_expiring',upkeep='battle_status_build_upkeep_action')
    for key,name in {**symbols,**hooks}.items():bind_function(key,name,'battle',disc)
    o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=500000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',disc);o.call('init')
    unit=0x801908cc;phase=0x8018f518;events=[]
    def external(uc,address,size,data):
        key=next(k for k in hooks if s.FUNCTIONS[k][0]==address)
        i=uc.reg_read(o.reg.UC_MIPS_REG_A0)
        if key=='expire':events.append(['expire',i])
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in hooks:
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    def reset():
        o.uc.mem_write(unit&0x1fffff,bytes(21*0x1c0));o.write(phase,2)
        o.write(0x8019389c,0);o.write(0x8018f51c,3)
        for i in range(21):o.uc.mem_write((unit&0x1fffff)+i*0x1c0+1,b'\xff')
    def actor(i,ct=0,speed=1,ticks=255,status=bytes(5)):
        at=(unit&0x1fffff)+i*0x1c0
        o.uc.mem_write(at+1,bytes((i,)));o.uc.mem_write(at+0x38,bytes((speed,ct)))
        o.uc.mem_write(at+0x58,status);o.uc.mem_write(at+0x15d,bytes((ticks,)))
        o.uc.mem_write(at+0x18a,bytes((i,)))
    cases=[('ties',[1,7,12],None),('thirteen',list(range(13)),None),
           ('stopped',[0,1,7],0),('cancel_during_drain',[1,7,12],None),
           ('ready_turn_first',[1,7],None),('quick_after_charge',[1,7],None)]
    rows=[]
    for label,ids,stopped in cases:
        reset();actor(20,ct=50,speed=50)
        for i in ids:actor(i,ticks=1,status=mask(30)if i==stopped else bytes(5))
        if label=='ready_turn_first':o.write(phase,1);actor(20,ct=103)
        events.clear();returns=[]
        for _ in range(16):
            result=o.call('clock',0);kind=result&0xff00;i=result&255
            returns.append(result);events.append(['charge'if kind==0x200 else 'turn',i])
            if kind==0x200:
                if label=='cancel_during_drain' and i==1:
                    o.uc.mem_write((unit&0x1fffff)+7*0x1c0+0x15d,b'\xff')
                if label=='quick_after_charge' and i==1:actor(20,ct=255)
            else:break
        expected=[i for i in ids if i!=stopped and not(label=='cancel_during_drain' and i==7)]
        actual=[r&255 for r in returns if r&0xff00==0x200]
        if label=='ready_turn_first':assert returns==[0x114]
        elif label=='quick_after_charge':assert returns==[0x201,0x114]
        else:
            assert actual==expected,(label,returns)
            assert events.index(['expire',20])>max(events.index(['charge',i])for i in expected)
        rows.append(dict(name=label,returns=returns,events=list(events),
                         remaining=[o.uc.mem_read((unit&0x1fffff)+i*0x1c0+0x15d,1)[0]for i in ids]))
    targets=[]
    for kind in (5,6):
        for x,y in ((4,6),(9,3)):
            reset();a=unit&0x1fffff;t=a+0x1c0
            o.uc.mem_write(a+0x47,bytes((2,1,0)))
            o.uc.mem_write(t+0x47,bytes((x,y,0)))
            o.uc.mem_write(a+0x178,bytes((kind,1)))
            o.uc.mem_write(a+0x17a,struct.pack('<hhh',4,0,6))
            o.call('target',unit)
            dx,dy=struct.unpack('<hh',o.uc.mem_read(0x19389c+6,4))
            assert (dx,dy)==((x-2,y-1)if kind==6 else(2,5)),(kind,x,y,dx,dy)
            targets.append([kind,x,y,dx,dy])
    (out/'source.h').write_text('/* Original isolated clock order; local-only. */\n'+
        'static const u8 source_due_order[]={'+','.join(map(str,[r&255 for r in rows[1]['returns']if r&0xff00==0x200]))+'};\n')
    report=dict(clock=rows,targets=targets,functions=o.function_hashes,hooks=hooks,
        exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Unchanged original clock/status-check and attacker-coordinate instructions. Expiry/upkeep callbacks isolated; cancellation/Quick injected between returns. No original HP/reaction/MP/AoE/casting or full target validity certification. Quick255 ordering is research only, not enabled natively.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original deferred:',len(rows),'clock sequences,',len(targets),'target binding cases',flush=True)


if __name__=='__main__':main()
