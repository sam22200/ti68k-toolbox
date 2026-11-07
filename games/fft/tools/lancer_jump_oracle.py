#!/usr/bin/env python3
"""Bounded retail Jump timing and PA*WP formula, executed as unmodified MIPS."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

def main():
    out=s.HERE/'campaign/x/core-lancer-jump/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for key,name in dict(prepare='battle_action_prepare_attack',damage='battle_formula_damage_pa_times_wp').items():
        bind_function(key,name,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    unit,target,ability=0x801908cc,0x801d4000,0x801938c0
    times=[]
    for speed in (1,2,3,4,5,6,7,8,10,25,49,50,51,99):
        o.uc.mem_write(unit&0x1fffff,bytes(448));o.uc.mem_write((unit&0x1fffff)+0x38,bytes((speed,)))
        command=bytearray(24);command[1]=18;struct.pack_into('<H',command,2,405)
        o.uc.mem_write(0x1d6000,bytes(command));result=o.call('prepare',0x801d6000,0x801d6100,2)
        ticks=o.uc.mem_read((unit&0x1fffff)+0x15d,1)[0]
        resolved=struct.unpack('<H',o.uc.mem_read(0x1d6102,2))[0]
        assert ticks==max(1,50//speed) and resolved==394
        times.append([speed,ticks,result,resolved])
    rows=[]
    # Weaponless, sword, knife and polearm; two kinds of status multiplication,
    # Defense Up and compatibility boundaries. No native formula supplies answers.
    profiles=[(5,70,weapon,255,255,0,0,0,0)for weapon in(0,19,1,99)]
    profiles += [(pa,brave,weapon,ast,tst,defense,az,tz,sex)for pa,brave,weapon,ast,tst,defense,az,tz,sex in
        ((1,0,0,255,255,0,0,0,0),(99,100,99,255,255,0,0,0,0),
         (5,70,99,255,26,1,0,0,0),(7,70,19,255,4,0,0,0,0),
         (7,70,19,255,27,0,0,0,0),(7,70,19,255,21,0,0,0,0),
         (7,70,99,20,255,0,0,0,0),(7,70,99,22,255,0,0,0,0),
         (7,70,99,255,255,0,0,3,0),(7,70,99,255,255,0,0,6,0),
         (7,70,99,255,255,0,0,6,1),(7,70,99,255,255,0,12,0,0))]
    for pa,brave,weapon,ast,tst,defense,az,tz,sex in profiles:
        for ptr,zodiac,female,sid in((unit,az,0,ast),(target,tz,sex,tst)):
            at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+6,bytes((64 if female else 128,)))
            o.uc.mem_write(at+8,struct.pack('<H',zodiac<<12))
            if sid!=255:o.uc.mem_write(at+0x58,mask(sid))
        o.uc.mem_write((unit&0x1fffff)+0x36,bytes((pa,)));o.uc.mem_write((unit&0x1fffff)+0x24,bytes((brave,)))
        o.uc.mem_write((target&0x1fffff)+0x90,bytes((8 if defense else 0,)))
        o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write((ability&0x1fffff)+0x18,bytes((weapon,)))
        wp=d.at(s.battle.WEAPONS+8*weapon+4,1)[0]
        o.uc.mem_write((ability&0x1fffff)+0x3e,d.at(s.battle.WEAPONS+8*weapon,8))
        o.write(0x80192d94,unit);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
        o.call('damage');damage=struct.unpack('<h',o.uc.mem_read((target&0x1fffff)+0x190,2))[0]
        rows.append([pa,brave,weapon,ast,tst,defense,az,tz,sex,damage])
    lines=['/* Generated from retail MIPS. Local-only. */']
    for name,values in(('jump_times',times),('jump_damage_rows',rows)):
        lines+=['static const u16 %s[][%d]={'%(name,len(values[0]))]
        lines+=[' {'+','.join(map(str,row))+'},'for row in values];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(timing=times,damage=rows,functions=o.function_hashes,
        jump_rows=list(d.at(0x8006102c,24)),exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),
        scope='Unmodified preparation phase2 (no presentation/action-state handler) and formula0x64. No full source animation, AI or layered targeting replay.'),indent=2)+'\n')
    print('Original Jump:',len(times),'timing,',len(rows),'damage profiles')

if __name__=='__main__':main()
