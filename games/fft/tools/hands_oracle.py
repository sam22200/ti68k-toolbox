#!/usr/bin/env python3
"""Original hand selection and ordinary melee XA/status/zodiac scalar fixtures."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    out=s.HERE/'campaign/x/core-hands/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(init='battle_action_init_current_ability_strike_data',weapon='battle_action_store_used_weapon',
        base='battle_formula_calculate_base_xa',physical='battle_formula_apply_physical_status_support_compatibility')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0
    o.write(0x80192d94,actor);o.write(0x80192d98,target)
    hands=[]
    for right,left in ((255,255),(1,255),(255,1),(1,5),(1,11)):
        for two in (0,1):
            o.uc.mem_write(actor&0x1fffff,bytes(0x1c0));o.uc.mem_write(ability&0x1fffff,bytes(0x50))
            o.uc.mem_write((actor&0x1fffff)+0x16f,b'\x01');o.uc.mem_write(0x65cb5,b'\x08')
            o.uc.mem_write((actor&0x1fffff)+0x1d,bytes((right,255,left)))
            o.uc.mem_write((actor&0x1fffff)+0x91,bytes((two,)))
            o.write(0x8018f5f0,0);o.call('init',actor)
            row=[right,left,two,o.uc.mem_read((ability&0x1fffff)+2,1)[0]]
            for hand in (0,1):
                o.uc.mem_write((ability&0x1fffff)+3,bytes((hand,)));o.call('weapon',actor+0x16e)
                row.append(o.uc.mem_read((ability&0x1fffff)+0x18,1)[0])
            hands.append(row)
    scalars=[]
    for weapon in (0,1,5,11,19):
        for pa,sp,brave,sleep,sign in ((5,9,60,0,0),(5,9,60,1,0),(7,4,40,0,3),(1,1,1,0,0)):
            o.uc.mem_write(actor&0x1fffff,bytes(0x1c0));o.uc.mem_write(target&0x1fffff,bytes(0x1c0))
            o.uc.mem_write(ability&0x1fffff,bytes(0x50))
            o.uc.mem_write((actor&0x1fffff)+0x36,bytes((pa,6,sp)))
            o.uc.mem_write((actor&0x1fffff)+0x24,bytes((brave,)))
            o.uc.mem_write((actor&0x1fffff)+6,b'\x80');o.uc.mem_write((target&0x1fffff)+6,b'\x80')
            o.uc.mem_write((target&0x1fffff)+8,struct.pack('<H',sign<<12))
            o.uc.mem_write((target&0x1fffff)+0x58,mask(35)if sleep else bytes(5))
            o.uc.mem_write((ability&0x1fffff)+0x18,bytes((weapon,)))
            o.uc.mem_write((ability&0x1fffff)+0x3e,d.at(s.battle.WEAPONS+8*weapon,8))
            o.call('base');o.call('physical')
            xa,ya=struct.unpack('<HH',o.uc.mem_read((ability&0x1fffff)+0x0e,4))
            scalars.append([255 if not weapon else weapon,pa,sp,brave,sleep,sign,min(999,xa*ya)])
    lines=['/* Generated unchanged original MIPS scalar/hand fixtures, local-only. */']
    for name,rows in [('source_hands',hands),('source_hand_damage',scalars)]:
        lines.append(f'static const u16 {name}[][{len(rows[0])}]={{')
        lines+=[' {'+','.join(map(str,r))+'},'for r in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(hands=hands,damage=scalars,functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),
        battle_sha256=s.core_audit.sha(d.bat),decomp_commit=subprocess.check_output(
            ['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original initializer/used-weapon and base-XA/status/support/zodiac instructions. Damage fixture multiplies their XA/YA without critical, element, weapon proc or original full target/animation/AI trace.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original hands:',len(hands),'selections,',len(scalars),'damage scalars',flush=True)


if __name__=='__main__':main()
