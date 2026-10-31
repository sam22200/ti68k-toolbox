#!/usr/bin/env python3
"""Original MIPS timer staging, timed bitmap commits and Poison/Regen staging."""
import json,struct,subprocess
from pathlib import Path
import surface_data as s
from status_state_oracle import bind_function,mask
import common_tables


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-status-lifetime/oracle';out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(filter=('battle_status_modify_inflictions','battle'),
        remove=('battle_status_set_inflicted_ct_and_transfer_last_used_ct','battle'),
        store=('battle_status_store_for_current_attack','battle'),
        handlers=('battle_status_enable_special_flags','battle'),
        tick=('battle_status_update_expiring','battle'),
        upkeep=('battle_status_apply_poison_and_regen','battle'),
        timer=('main_status_set_ct','main'))
    for k,(symbol,target)in names.items():
        if target=='battle':bind_function(k,symbol,target,disc)
    o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['timer']=bind_function('timer','main_status_set_ct','main',disc)
    def external_handler(uc,address,size,data):
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    o.uc.hook_add(unicorn.UC_HOOK_CODE,external_handler,begin=s.FUNCTIONS['handlers'][0],end=s.FUNCTIONS['handlers'][0])
    unit=0x801908cc;action=unit+0x18c;zero=bytes(5)
    o.write(0x80192d94,unit);o.write(0x80192d98,unit);o.write(0x80192d90,action)
    def load(inflicted,innate,immune,ct,present=1,maxhp=52):
        o.uc.mem_write(unit&0x1fffff,bytes(0x1c0))
        o.uc.mem_write((unit&0x1fffff)+1,bytes((0 if present else 255,)))
        o.uc.mem_write((unit&0x1fffff)+0x28,struct.pack('<HH',maxhp,maxhp))
        o.uc.mem_write((unit&0x1fffff)+0x4e,innate+immune+bytes(a|b for a,b in zip(innate,inflicted)))
        o.uc.mem_write((unit&0x1fffff)+0x5d,ct)
        o.uc.mem_write((unit&0x1fffff)+0x1bb,inflicted)
    ticks=[];applies=[];upkeeps=[];rows=dict(ticks=[],applies=[],upkeeps=[])
    cases=[]
    for i in range(15):
        for value,innate in [(0,zero),(1,zero),(2,zero),(255,zero),(1,mask(i+24))]:
            ct=bytearray(range(1,17));ct[i]=value
            cases.append((f'tick_{i}_{value}_{innate.hex()}',mask(i+24),innate,zero,bytes(ct)))
    cases += [('simultaneous',mask(*range(24,40)),zero,zero,bytes([1]*16)),
              ('mixed_innate',mask(*range(24,40)),mask(24,28,32,38),zero,bytes([1]*16)),
              ('none',zero,zero,zero,bytes(range(16)))]
    for label,inflicted,innate,immune,ct in cases:
        load(inflicted,innate,immune,ct);o.call('tick',0)
        afterct=bytes(o.uc.mem_read((unit&0x1fffff)+0x5d,16))
        remove=bytes(o.uc.mem_read((action&0x1fffff)+0x20,5));o.call('remove',0)
        after=bytes(o.uc.mem_read((unit&0x1fffff)+0x1bb,5))
        rec=inflicted+innate+immune+ct+afterct+remove+after;assert len(rec)==57
        ticks.append(rec);rows['ticks'].append(label)
    cases=[]
    for i in range(24,40):
        for label,inflicted,innate,immune,add,remove in [
            ('new',zero,zero,zero,mask(i),zero),
            ('repeat',mask(i),zero,zero,mask(i),zero),
            ('immune',zero,zero,mask(i),mask(i),zero),
            ('remove',mask(i),zero,zero,zero,mask(i)),
            ('innate_cure',mask(i),mask(i),zero,zero,mask(i))]:
            cases.append((f'{label}_{i}',inflicted,innate,immune,add,remove,0))
    for i,j in [(24,25),(25,24),(28,29),(29,28)]:
        cases.append((f'cancel_{i}_{j}',mask(i),zero,zero,mask(j),zero,0))
    cases.append(('ko_cancels_timed',mask(6,10,12,24,28),zero,zero,mask(2),zero,1))
    for label,inflicted,innate,immune,add,remove,mode in cases:
        ct=bytes([3]*16);load(inflicted,innate,immune,ct)
        o.uc.mem_write((action&0x1fffff)+0x1b,add+remove)
        total=o.call('filter',mode);o.call('remove',0);o.call('store',0,mode)
        after=bytes(o.uc.mem_read((unit&0x1fffff)+0x1bb,5))
        afterct=bytes(o.uc.mem_read((unit&0x1fffff)+0x5d,16))
        rec=inflicted+innate+immune+ct+add+remove+bytes((mode,))+struct.pack('>H',total)+after+afterct
        assert len(rec)==65;applies.append(rec);rows['applies'].append(label)
    cases=[]
    for ids in [(24,),(25,),(24,25)]:
        for maxhp in (0,1,7,8,9,52,999):cases.append((f'upkeep_{ids}_{maxhp}',mask(*ids),zero,1,maxhp))
        for block in (1,2,5,8,15):cases.append((f'blocked_{ids}_{block}',mask(*ids,block),zero,1,52))
        cases.append((f'absent_{ids}',mask(*ids),zero,0,52))
    cases += [('none',zero,zero,1,52),('innate_poison',zero,mask(24),1,52),('innate_regen',zero,mask(25),1,52)]
    for label,inflicted,innate,present,maxhp in cases:
        load(inflicted,innate,zero,bytes(16),present,maxhp);kind=o.call('upkeep',unit)
        dmg,heal=struct.unpack('<HH',o.uc.mem_read((action&0x1fffff)+4,4))
        rec=inflicted+innate+zero+bytes((present,))+struct.pack('>HBHH',maxhp,kind,dmg,heal)
        assert len(rec)==23;upkeeps.append(rec);rows['upkeeps'].append(label)
    blob=b'FSL1'+struct.pack('>3H',len(ticks),len(applies),len(upkeeps))+b''.join(ticks+applies+upkeeps)
    (out/'fsl00.bin').write_bytes(blob)
    report=dict(counts=dict(ticks=len(ticks),applies=len(applies),upkeeps=len(upkeeps)),rows=rows,
        functions=o.function_hashes,policy_sha256=s.core_audit.sha(common_tables.status_policy_bytes(disc)),
        exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original MIPS timer/bitmap and Poison/Regen staging. External status-specific handlers return without side effects; timer setters/removers execute unchanged. No full scheduler, HP commit, reactions, graphics or Death Sentence turn-counter certification.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print(report['counts'],flush=True)

if __name__=='__main__':main()
