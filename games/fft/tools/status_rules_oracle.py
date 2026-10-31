#!/usr/bin/env python3
"""Small unchanged-MIPS status hook fixtures; no emulator input or live save."""
import argparse,json,re,struct,subprocess
from pathlib import Path
import surface_data as s


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=s.HERE/'campaign/x/core-status-rules/oracle')
    out=ap.parse_args().out;out.mkdir(parents=True,exist_ok=True)
    names={'dark':'battle_formula_calculate_dark_confuse',
           'defend':'battle_formula_apply_evasion_changes_due_to_statuses',
           'hit':'battle_formula_calculate_final_hit_percent',
           'restriction':'battle_action_check_and_consume_mp'}
    text=(s.ROOT/'sources/fft_decomp/target/battle.yaml').read_text()
    for key,name in names.items():
        m=re.search(r'addr: (0x[0-9a-f]+), size: (\d+), name: '+name+r', hash: ([0-9a-f]+)',text)
        assert m,name
        s.FUNCTIONS[key]=(int(m[1],16),int(m[2]),m[3])
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC));o=s.Original(disc)
    o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=100000
    # The executable's external BIOS rand veneer has no BIOS in this RAM-only
    # runner. Preview accuracy ignores its roll; supply one fixed BIOS result,
    # without patching or skipping any tested formula/restriction instructions.
    import unicorn
    def bios_rand(uc,address,size,data):
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0x4000)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    o.uc.hook_add(unicorn.UC_HOOK_CODE,bios_rand,begin=0x8002230c,end=0x8002230c)
    actor,target,result,ability=0x801d0000,0x801d0400,0x801d0800,0x801938c0
    o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,result)
    o.write(0x8018f5f0,1);o.write(0x8018f5fc,2) # non-primary preview: no MP subtraction
    rows=[];lines=['/* Generated from unchanged original MIPS; local-only. */',
        'static const struct { u8 dark,defend,side,shield,base,hit; } source_evades[]={']
    for dark in (0,1):
        for defending in (0,1):
            for side in (0,1,2):
                for shield in (255,0x80,0x8f):
                    o.uc.mem_write(actor&0x1fffff,bytes(0x1c0));o.uc.mem_write(target&0x1fffff,bytes(0x1c0))
                    o.uc.mem_write(result&0x1fffff,bytes(0x40));o.uc.mem_write(ability&0x1fffff,bytes(0x60))
                    cev=disc.job(0x4a)['cev'] if side==0 else 0
                    sev=disc.at(0x80063eb8+2*(shield-0x80),1)[0] if shield!=255 and side!=2 else 0
                    o.uc.mem_write((actor&0x1fffff)+0x59,bytes((0x20 if dark else 0,)))
                    o.uc.mem_write((target&0x1fffff)+0x58,bytes((2 if defending else 0,)))
                    o.uc.mem_write((ability&0x1fffff)+0x1c,b'\x64')
                    o.uc.mem_write((ability&0x1fffff)+0x1f,bytes((sev,)))
                    o.uc.mem_write((ability&0x1fffff)+0x21,bytes((cev,)))
                    o.uc.mem_write((result&0x1fffff)+0x2a,struct.pack('<H',100))
                    o.call('dark');o.call('defend');base=o.uc.mem_read((ability&0x1fffff)+0x1c,1)[0]
                    o.call('hit');hit=struct.unpack('<H',o.uc.mem_read((result&0x1fffff)+0x2a,2))[0]
                    row=[dark,defending,side,shield,base,hit];rows.append(row)
                    lines.append(' {'+','.join(map(str,row))+'},')
    lines+=['};','static const struct { u16 ability; u8 set,silence,blocked; } source_silence[]={']
    menus=disc.at(0x80065cb4,224);silence=[]
    for menu in (0,1,4,6,10):
        skillset=menus.index(menu)
        for ab in (1,9,0x93,0x170,0x196):
            for silenced in (0,1):
                o.uc.mem_write(actor&0x1fffff,bytes(0x1c0));o.uc.mem_write(ability&0x1fffff,bytes(0x60))
                o.uc.mem_write((actor&0x1fffff)+0x59,bytes((8 if silenced else 0,)))
                o.uc.mem_write((actor&0x1fffff)+0x16f,bytes((skillset,))+struct.pack('<H',ab))
                o.uc.mem_write((actor&0x1fffff)+0x2c,struct.pack('<H',999))
                o.uc.mem_write(s.TILES&0x1fffff,bytes(8))
                result_code=o.call('restriction',actor)
                assert result_code in (0,2),(ab,menu,silenced,result_code)
                row=[ab,skillset,silenced,int(result_code==2)];silence.append(row)
                lines.append(' {'+','.join(map(str,row))+'},')
    lines+=['};','']
    (out/'source.h').write_text('\n'.join(lines))
    report=dict(evades=rows,silence=silence,functions={k:o.function_hashes[k]for k in names},
        unicorn=o.version,exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        external_bios_rand='rand veneer returns fixed0x4000 via runner hook; preview ignores roll outcomes',
        scope='Original darkness/Defend/final accuracy with supplied class/shield evades; original commit restriction with normal terrain and enough MP, preview/non-primary context. No full PS1 turn, poison upkeep, confusion/team or immunity certification.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original status hooks:',len(rows),'evasion cases,',len(silence),'silence/menu cases')

if __name__=='__main__':main()
