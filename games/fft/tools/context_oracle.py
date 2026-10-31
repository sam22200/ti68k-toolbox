#!/usr/bin/env python3
"""Original reaction eligibility and pending-unit priority; no emulator input."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-context/oracle';out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(blocked='battle_action_is_reaction_blocked',next='battle_reaction_prepare_next',
        counter='battle_action_store_counter_ability',mp='battle_action_check_and_consume_mp',
        strikes='battle_action_init_current_ability_strike_data')
    for key,name in names.items():bind_function(key,name,'battle',disc)
    o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',disc);o.call('init')
    prevention=bytes(o.uc.mem_read(0x662d0+4*5,5));print('Reaction prevention:',prevention.hex(),flush=True)
    unit,ability=0x801908cc,0x801938c0
    cases=[('none',bytes(5),0,0,0,0)]
    cases += [(f'id{i}',mask(i),0,0,0,0)for i in range(40)]
    cases += [(f'context{i}_{j}_{k}',bytes(5),i,0x1ba if j else 0,k,0)for i in (0,1)for j in (0,1)for k in (0,1)]
    cases += [('already_queued',bytes(5),0,0,0,0x1ba),
              ('queued_secondary',bytes(5),1,0,0,0x1ba),
              ('queued_stop',mask(30),0,0,0,0x1ba),
              ('queued_dontact',mask(37),0,0,0,0x1ba)]
    records=[];labels=[]
    for name,bits,secondary,current,proc,pending in cases:
        o.uc.mem_write(unit&0x1fffff,bytes(0x1c0));o.uc.mem_write((unit&0x1fffff)+0x58,bits)
        o.uc.mem_write((unit&0x1fffff)+0x19a,struct.pack('<H',pending))
        o.uc.mem_write(ability&0x1fffff,bytes(0x50))
        o.uc.mem_write((ability&0x1fffff)+6,struct.pack('<HH',current,proc))
        o.write(0x8018f5f0,secondary);o.uc.mem_write(s.TILES&0x1fffff,bytes(8))
        result=o.call('blocked',unit)
        records.append([*bits,secondary,current,proc,pending,result]);labels.append(name)
    # Isolate only reaction execution/validation after the pending actor is
    # selected. Scan order, skip rules, context and command save/restore run.
    def external(uc,address,size,data):
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in ('counter','mp','strikes'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    orders=[]
    for stopped in (255,1,12):
        o.uc.mem_write(unit&0x1fffff,bytes(21*0x1c0));o.write(0x8018f8c8,6);o.write(0x8018f5fc,0)
        for i in range(21):
            at=(unit&0x1fffff)+i*0x1c0
            o.uc.mem_write(at+1,bytes((i if i<13 else 255,)))
            if i in (0,1,6,12):o.uc.mem_write(at+0x19a,struct.pack('<H',0x1ba))
            if i==stopped:o.uc.mem_write(at+0x5b,b'\x02')
        row=[stopped]
        for k in range(4):
            result=o.call('next',0x801d6000)
            row.append(255 if result==0xffffffff else result)
        orders.append(row)
    lines=['/* Generated from unchanged original MIPS; local-only. */',
           'static const u8 source_prevention[5]={'+','.join(map(str,prevention))+'};']
    for name,rows in [('source_reaction',records),('source_priority',orders)]:
        lines.append(f'static const u16 {name}[][{len(rows[0])}]={{')
        lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(cases=records,labels=labels,prevention=prevention.hex(),priority=orders,
        functions=o.function_hashes,exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        isolated=['battle_action_store_counter_ability','battle_action_check_and_consume_mp',
            'battle_action_init_current_ability_strike_data'],
        scope='Original prevention initializer and reaction-block checks on dry terrain with no mount. Pending priority/skip/context runs; counter construction, MP validation and strike initialization return success in the runner. No reaction effects, Brave rolls, damage timing, Hamedo, Reflect exception, equipment or complete multistrike trace certificate.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original reaction policy:',len(records),'cases;',len(orders),'priority cases',flush=True)


if __name__=='__main__':main()
