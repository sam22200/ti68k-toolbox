#!/usr/bin/env python3
"""Original dual Charge initialization and per-hand power/status/critical arithmetic."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-charge-hands/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(init='battle_action_init_current_ability_strike_data',weapon='battle_action_store_used_weapon',
        base='battle_formula_calculate_base_xa',charge='battle_formula_apply_charge',
        physical='battle_formula_apply_physical_xa_modifying_statuses_support',
        critical='battle_formula_calculate_critical_hit',random='battle_formula_get_random_0_7fff',
        recoil='battle_formula_calculate_knockback')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0
    values=[];random_calls=[0];recoil_calls=[0]
    def external(uc,address,size,data):
        if address==s.FUNCTIONS['random'][0]:
            random_calls[0]+=1;value=values.pop(0)
        else:recoil_calls[0]+=1;value=0
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for name in ('random','recoil'):
        at=s.FUNCTIONS[name][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    assert d.at(0x80065cb4+8,1)==b'\x0a'
    initial=[]
    for index in range(8):
        o.uc.mem_write(actor&0x1fffff,bytes(0x1c0));o.uc.mem_write(ability&0x1fffff,bytes(0x50))
        o.uc.mem_write((actor&0x1fffff)+0x16f,b'\x08')
        o.uc.mem_write((actor&0x1fffff)+0x170,struct.pack('<H',0x196+index))
        o.uc.mem_write((actor&0x1fffff)+0x1d,bytes((1,255,11)))
        o.uc.mem_write((actor&0x1fffff)+0x91,b'\x01');o.write(0x8018f5f0,0);o.call('init',actor)
        row=[0x196+index,*d.at(0x80061044+2*index,2),o.uc.mem_read((ability&0x1fffff)+2,1)[0]]
        for hand in (0,1):
            o.uc.mem_write((ability&0x1fffff)+3,bytes((hand,)));o.call('weapon',actor+0x16e)
            row.append(o.uc.mem_read((ability&0x1fffff)+0x18,1)[0])
        initial.append(row)
    profiles=[(index,weapon,5,9,60,255,0,check,roll)
              for index in range(8)for weapon in (1,11)for check,roll in ((32767,0),(0,0),(0,32767))]
    profiles += [(7,weapon,pa,sp,brave,sid,sign,check,roll)
        for weapon,pa,sp,brave,sid,sign in ((0,1,1,1,255,0),(0,5,9,60,35,3),
                                           (19,7,4,40,4,3),(11,99,50,100,35,0))
        for check,roll in ((32767,0),(0,0),(0,32767))]
    cases=[]
    for index,weapon,pa,sp,brave,sid,sign,critcheck,critroll in profiles:
        o.uc.mem_write(actor&0x1fffff,bytes(0x1c0));o.uc.mem_write(target&0x1fffff,bytes(0x1c0))
        o.uc.mem_write(ability&0x1fffff,bytes(0x50))
        o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
        o.uc.mem_write((actor&0x1fffff)+0x36,bytes((pa,6,sp)));o.uc.mem_write((actor&0x1fffff)+0x24,bytes((brave,)))
        o.uc.mem_write((actor&0x1fffff)+6,b'\x80');o.uc.mem_write((target&0x1fffff)+6,b'\x80')
        o.uc.mem_write((target&0x1fffff)+8,struct.pack('<H',sign<<12))
        o.uc.mem_write((target&0x1fffff)+0x58,mask(sid)if sid!=255 else bytes(5))
        o.uc.mem_write((ability&0x1fffff)+0x18,bytes((weapon,)))
        o.uc.mem_write((ability&0x1fffff)+0x3e,d.at(s.battle.WEAPONS+8*weapon,8))
        o.uc.mem_write((ability&0x1fffff)+0x24,d.at(0x80061044+2*index+1,1))
        o.call('base');o.call('charge');o.call('physical')
        values[:]=[critcheck,critroll];random_calls[0]=recoil_calls[0]=0;o.call('critical')
        xa,ya=struct.unpack('<HH',o.uc.mem_read((ability&0x1fffff)+0x0e,4))
        # Critical is action-data offset1; no final HP/terrain/animation runs here.
        critical=o.uc.mem_read((target&0x1fffff)+0x18d,1)[0]
        assert random_calls[0]==(2 if critcheck==0 else 1)
        assert recoil_calls[0]==(1 if critcheck==0 else 0)
        cases.append([0x196+index,255 if not weapon else weapon,pa,sp,brave,sid,sign,
                      critcheck,critroll,xa*ya,critical])
    lines=['/* Generated unchanged original MIPS; local-only. */']
    for name,rows in [('source_charge_hands',initial),('source_charge_damage',cases)]:
        lines.append(f'static const u16 {name}[][{len(rows[0])}]={{')
        lines+=[' {'+','.join(map(str,r))+'},'for r in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(initial=initial,cases=cases,functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),
        battle_sha256=s.core_audit.sha(d.bat),decomp_commit=subprocess.check_output(
            ['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original Charge menu initializer/hand selection and base-XA, Charge power, physical status/support/zodiac and critical threshold/amplitude instructions. Random helper returns fixture values; recoil handler isolated/counted. No original full delayed-turn, HP, target rebuild, terrain recoil or animation trace.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original dual Charge:',len(initial),'initializers,',len(cases),'power/status/critical scalars',flush=True)


if __name__=='__main__':main()
