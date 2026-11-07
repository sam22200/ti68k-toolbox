#!/usr/bin/env python3
"""Original prepare/MP boundaries; no native spell-effect claim."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-casting/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(mp='battle_action_check_and_consume_mp',prepare='battle_action_prepare_attack',
               handlers='battle_status_enable_special_flags')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    def external(uc,address,size,data):
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    at=s.FUNCTIONS['handlers'][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    unit=0x801908cc;ability=0x801938c0;source=0x801d6000;dest=source+32
    ab=0xf;range_at=s.battle.ABILITY_RANGE+14*ab
    raw=d.at(range_at,14);assert raw[12:]==bytes((6,56))
    def load(mp,menu=0,mime=0,half=0,preview=0,sid=255):
        o.uc.mem_write(unit&0x1fffff,bytes(0x1c0));o.uc.mem_write(ability&0x1fffff,bytes(0x50))
        o.uc.mem_write((unit&0x1fffff)+1,b'\x00')
        o.uc.mem_write((unit&0x1fffff)+3,bytes((0x5d if mime else 0x4f,)))
        o.uc.mem_write((unit&0x1fffff)+0x2c,struct.pack('<H',mp))
        o.uc.mem_write((unit&0x1fffff)+0x90,bytes((0x80 if half else 0,)))
        o.uc.mem_write((unit&0x1fffff)+0x16f,b'\x0a')
        o.uc.mem_write((unit&0x1fffff)+0x170,struct.pack('<H',ab))
        o.uc.mem_write((unit&0x1fffff)+0x58,mask(sid)if sid!=255 else bytes(5))
        o.uc.mem_write(0x65cb4+10,bytes((menu,)))
        o.uc.mem_write(s.TILES&0x1fffff,bytes(8))
        o.write(0x8018f5f0,0);o.write(0x8018f5fc,2 if preview else 0)
    payments=[]
    for cost,menu,mime,half,mp,preview,sid in [
        (56,0,0,0,mp,p,255)for mp in(0,55,56,999)for p in(0,1)]+[
        (cost,0,0,1,mp,p,255)for cost in(7,56)for mp in(2,3,28,999)for p in(0,1)]+[
        (56,menu,mime,0,0,0,255)for menu,mime in((0,1),(6,0),(10,0))]+[
        (56,0,0,0,999,p,sid)for sid in(12,22,37)for p in(0,1)]:
        load(mp,menu,mime,half,preview,sid)
        o.uc.mem_write(range_at&0x1fffff,raw[:13]+bytes((cost,)))
        result=o.call('mp',unit)
        after=struct.unpack('<H',o.uc.mem_read((unit&0x1fffff)+0x2c,2))[0]
        effective=o.uc.mem_read((ability&0x1fffff)+0x2b,1)[0]
        payments.append([cost,menu,mime,half,mp,preview,sid,after,effective,result])
    o.uc.mem_write(range_at&0x1fffff,raw)
    preparation=[]
    for ticks,persevere,short,non,phase in [(t,p,sc,nc,ph)for t,p,sc,nc in
        ((6,0,0,0),(7,0,1,0),(7,0,0,1),(7,0,1,1),(7,1,1,1),(127,0,1,0))for ph in(0,1)]:
        load(999);r=bytearray(raw);r[12]=ticks;r[5]=(r[5]&~4)|(4 if persevere else 0)
        o.uc.mem_write(range_at&0x1fffff,bytes(r))
        o.uc.mem_write((unit&0x1fffff)+0x15d,b'\xff')
        o.uc.mem_write((unit&0x1fffff)+0x92,bytes(((8 if short else 0)|(4 if non else 0),)))
        command=bytearray(20);command[1]=10;struct.pack_into('<H',command,2,ab);command[10]=5
        o.uc.mem_write(source&0x1fffff,bytes(command));o.uc.mem_write(dest&0x1fffff,bytes(20))
        result=o.call('prepare',source,dest,phase)
        charged=o.uc.mem_read((unit&0x1fffff)+0x15d,1)[0]
        moved,acted=o.uc.mem_read((unit&0x1fffff)+0x187,2)
        mp=struct.unpack('<H',o.uc.mem_read((unit&0x1fffff)+0x2c,2))[0]
        preparation.append([ticks,persevere,short,non,phase,result,charged,moved,acted,mp])
        assert mp==999 and not moved and acted==phase
        if phase:assert charged==(ticks if persevere else 0 if non else(ticks+1)//2 if short else ticks)
    # Retail Holy: arming leaves MP alone; the later resolution entry pays56.
    load(999);o.uc.mem_write(range_at&0x1fffff,raw)
    command=bytearray(20);command[1]=10;struct.pack_into('<H',command,2,ab);command[10]=5
    o.uc.mem_write(source&0x1fffff,bytes(command));o.call('prepare',source,dest,1)
    armed_mp=struct.unpack('<H',o.uc.mem_read((unit&0x1fffff)+0x2c,2))[0]
    cost_result=o.call('mp',unit);resolved_mp=struct.unpack('<H',o.uc.mem_read((unit&0x1fffff)+0x2c,2))[0]
    assert (armed_mp,cost_result,resolved_mp)==(999,0,943)
    lines=['/* Original preparation/MP routines; local-only fixtures. */']
    for name,rows in [('source_cast_payment',payments),('source_cast_preparation',preparation)]:
        lines.append(f'static const u16 {name}[][{len(rows[0])}]={{')
        lines+=[' {'+','.join(map(str,r))+'},'for r in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(payments=payments,preparation=preparation,holy_arm_then_pay=[armed_mp,cost_result,resolved_mp],
        functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),
        innate_half_mp_jobs=[j for j in range(160)if 0x1ce in d.job(j)['innate']],
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Unchanged preparation and MP instructions; external status handlers isolated, dry/unmounted actor. Retail Holy arm/pay plus synthetic cost/timing/support/menu/status boundaries. No spell damage, target list, real elapsed CT, empty-target payment, full battle or native spell certification.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original casting:',len(payments),'MP cases,',len(preparation),'preparation cases; Holy arms999 then pays to943',flush=True)


if __name__=='__main__':main()
