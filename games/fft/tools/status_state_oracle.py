#!/usr/bin/env python3
"""Original bitmap filtering/storage, with external non-bitmap handlers isolated."""
import argparse,json,re,struct,subprocess
from pathlib import Path
import surface_data as s
import common_tables


def bind_function(name,symbol,target,disc):
    text=(s.ROOT/'sources/fft_decomp/target'/f'{target}.yaml').read_text()
    m=re.search(r'addr: (0x[0-9a-f]+), size: (\d+), name: '+symbol+r', hash: ([0-9a-f]+)',text)
    assert m,symbol
    addr,size=int(m[1],16),int(m[2]);blob=disc.bat if target=='battle' else disc.exe
    offset=addr-(s.battle.BAT if target=='battle' else s.battle.EXE)
    sha=s.core_audit.sha(blob[offset:offset+size]);assert sha.startswith(m[3])
    s.FUNCTIONS[name]=(addr,size,m[3]);return sha


def mask(*ids):
    out=bytearray(5)
    for i in ids:out[i>>3]|=0x80>>(i&7)
    return bytes(out)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=s.HERE/'campaign/x/core-status-state/oracle')
    out=ap.parse_args().out;out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(filter=('battle_status_modify_inflictions','battle'),
        remove=('battle_status_set_inflicted_ct_and_transfer_last_used_ct','battle'),
        store=('battle_status_store_for_current_attack','battle'),
        handlers=('battle_status_enable_special_flags','battle'))
    for k,(name,target)in names.items():bind_function(k,name,target,disc)
    o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',disc)
    o.function_hashes['action']=bind_function('action','main_status_set_action_state','main',disc)
    o.call('init')
    policy=common_tables.status_policy_bytes(disc)
    assert bytes(o.uc.mem_read(0x662d0+6*5,5))==policy[640:645]
    import unicorn
    def external_handler(uc,address,size,data):
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    o.uc.hook_add(unicorn.UC_HOOK_CODE,external_handler,begin=s.FUNCTIONS['handlers'][0],end=s.FUNCTIONS['handlers'][0])
    unit=0x801908cc;action=unit+0x18c;actor=0x801d4000
    o.uc.mem_write(actor&0x1fffff,bytes(0x1c0));o.write(0x80192d94,actor)
    o.write(0x80192d98,unit);o.write(0x80192d90,action)
    zero=bytes(5);cases=[]
    for i in range(40):
        for label,inflicted,innate,immune,add,remove in (
            ('new',zero,zero,zero,mask(i),zero),
            ('repeat',mask(i),zero,zero,mask(i),zero),
            ('immune',zero,zero,mask(i),mask(i),zero),
            ('innate_cure',mask(i),mask(i),zero,zero,mask(i))):
            cases.append((f'{label}_{i}',inflicted,innate,immune,add,remove,0,0))
    cases += [
        ('frog_toggle',mask(22),zero,zero,mask(22),zero,0,0),
        ('haste_then_slow',mask(28),zero,zero,mask(29),zero,0,0),
        ('slow_then_haste',mask(29),zero,zero,mask(28),zero,0,0),
        ('haste_slow_together',zero,zero,zero,mask(28,29),zero,0,0),
        ('dead_blocks_poison',mask(2),zero,zero,mask(24),zero,0,0),
        ('dead_cancels_ailments',mask(6,10,12,24,28),zero,zero,mask(2),zero,0,0),
        ('mixed_cure',mask(10,12,24),mask(12),mask(10),zero,mask(10,12,24),0,1),
        ('repeat_timed_innate',mask(24),mask(24),zero,mask(24),mask(24),0,0),
        ('upper_bits',mask(34,39),zero,zero,mask(38),mask(34),0,0),
        ('four_supported',zero,zero,zero,mask(6,10,12,24),zero,0,0),
        ('removal_only',mask(22),zero,zero,mask(22),mask(22),0,1)]
    for context in (1,2,3):
        cases += [(f'context_{context}_{mode}',mask(6,10,24),zero,zero,bytes([255]*5),bytes([255]*5),context,mode)for mode in (0,1)]
    rows=[];records=[]
    for name,inflicted,innate,immune,add,remove,flags,mode in cases:
        o.uc.mem_write(unit&0x1fffff,bytes(0x1c0))
        o.uc.mem_write((unit&0x1fffff)+0x4e,innate+immune+bytes(a|b for a,b in zip(innate,inflicted)))
        o.uc.mem_write((unit&0x1fffff)+0x1bb,inflicted)
        o.uc.mem_write((unit&0x1fffff)+5,bytes((4 if flags&2 else 0,)))
        o.uc.mem_write((unit&0x1fffff)+0x182,bytes((128 if flags&1 else 0,)))
        o.uc.mem_write((action&0x1fffff)+0x1b,add+remove)
        total=o.call('filter',mode);filtered=bytes(o.uc.mem_read((action&0x1fffff)+0x1b,10))
        o.call('remove',0);o.call('store',0,mode)
        after=bytes(o.uc.mem_read((unit&0x1fffff)+0x1bb,5))
        rec=inflicted+innate+immune+add+remove+bytes((flags,mode))+filtered+struct.pack('>H',total)+after
        assert len(rec)==44;records.append(rec);rows.append(dict(name=name,total=total,filtered=filtered.hex(),after=after.hex()))
    (out/'fsp00.bin').write_bytes(b'FSP1'+struct.pack('>H',len(records))+b''.join(records))
    (out/'source.json').write_text(json.dumps(dict(cases=len(rows),case_bytes=44,rows=rows,
        functions={k:o.function_hashes[k]for k in (*names,'init','action')},
        policy_sha256=s.core_audit.sha(policy),exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original filter/remove/store bitmap instructions with supplied canonical unit state. External status-specific handlers return without side effects; CT setters execute but timers, team/control, HP and graphics are not native certification targets here. No instruction in the tested bitmap routines is modified.'),indent=2)+'\n')
    print('Original bitmap policy queries:',len(rows),flush=True)

if __name__=='__main__':main()
