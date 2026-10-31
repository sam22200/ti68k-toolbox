#!/usr/bin/env python3
"""Small original-MIPS clock, command restriction, evasion and CT-end fixtures."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    out=s.HERE/'campaign/x/core-control/oracle';out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(check='battle_status_check_unit',restrict='battle_unit_get_action_restriction',
        ctend='battle_action_end_turn',evade='battle_formula_apply_evasion_changes_due_to_statuses',
        order='battle_action_calculate_at_list')
    for key,name in names.items():bind_function(key,name,'battle',disc)
    o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',disc);o.call('init')
    unit=0x801908cc;ability=0x801938c0;o.write(0x80192d98,unit)
    def load(bits):
        o.uc.mem_write(unit&0x1fffff,bytes(0x1c0));o.uc.mem_write((unit&0x1fffff)+0x58,bits)
        o.uc.mem_write(s.TILES&0x1fffff,bytes(8)) # dry, ordinary lower cell
    controls=[];ctend=[]
    cases=[('none',bytes(5))]+[(f'id{i}',mask(i))for i in range(40)]
    cases += [(f'combo{i}',mask(*[sid for b,sid in enumerate((30,36,37))if i&(1<<b)]))for i in range(1,8)]
    for label,bits in cases:
        load(bits);flags=o.call('check',unit);restricted=o.call('restrict',unit)
        o.uc.mem_write(ability&0x1fffff,bytes(0x60));o.uc.mem_write((ability&0x1fffff)+0x1e,bytes([20]*4))
        o.call('evade');zero=bytes(o.uc.mem_read((ability&0x1fffff)+0x1e,4))==bytes(4)
        # Only freeze is cached from the source turn flags. Haste/Slow and mounts
        # have separate producers; Death is still HP-owned in the native core.
        code=(4 if flags&1 else 0)|(8 if bits[4]&8 else 0)|(16 if restricted else 0)|(32 if zero else 0)
        controls.append([*bits,code]);
    for ids in [(),(30,),(36,),(37,),(36,37),(4,),(7,)]:
        for moved in (0,1):
            for acted in (0,1):
                for ct in (0,19,59):
                    bits=mask(*ids);load(bits);o.uc.mem_write((unit&0x1fffff)+0x39,bytes((ct,)))
                    o.uc.mem_write((unit&0x1fffff)+0x186,bytes((1,moved,acted)))
                    o.call('ctend',0);after=o.uc.mem_read((unit&0x1fffff)+0x39,1)[0]
                    ctend.append([*bits,ct,moved,acted,after])
    orders=[]
    for wait in (1,20):
        o.uc.mem_write(unit&0x1fffff,bytes(21*0x1c0))
        for i in range(21):o.uc.mem_write((unit&0x1fffff)+i*0x1c0+1,bytes((255,)))
        for i,ct in [(0,0),(1,50)]:
            at=(unit&0x1fffff)+i*0x1c0;o.uc.mem_write(at+1,bytes((i,)))
            o.uc.mem_write(at+0x38,bytes((6,ct)));o.uc.mem_write(at+0x15d,b'\xff')
            o.uc.mem_write(at+0x18a,bytes((i,)))
        o.uc.mem_write((unit&0x1fffff)+0x186,b'\x01')
        o.uc.mem_write((unit&0x1fffff)+0x1c0+0x5b,b'\x02')
        o.uc.mem_write((unit&0x1fffff)+0x1c0+0x63,bytes((wait,)))
        o.write(0x8018f51c,0);o.call('order',0x801d6000,1)
        entries=bytes(o.uc.mem_read(0x1d6000,160));orders.append([wait,*[entries[i*4]for i in range(8)]])
    lines=['/* Generated from unchanged original MIPS; local-only. */']
    for name,rows in [('source_controls',controls),('source_ctend',ctend),('source_orders',orders)]:
        lines.append(f'static const u8 {name}[][{len(rows[0])}]={{')
        lines += [' {'+','.join(map(str,row))+'},'for row in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(controls=controls,ctend=ctend,orders=orders,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original MIPS status-check/restriction/evasion/end-turn functions, dry terrain and no mount. Freeze-set initializer executes unchanged. Not full Stop action scheduling, reactions, Sleep damage/wake or water/mount certification.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Controls',len(controls),'CT-end',len(ctend),flush=True)

if __name__=='__main__':main()
