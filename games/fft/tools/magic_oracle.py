#!/usr/bin/env python3
"""Original Holy formula08 scalar pipeline on bounded ordinary-human profiles."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-magic/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(damage='battle_formula_magical_damage',weather='battle_map_get_effective_weather',
        random='battle_formula_get_random_0_7fff')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rolls=[]
    def external(uc,address,size,data):
        if address==s.FUNCTIONS['random'][0]:raise AssertionError('Holy unexpectedly rolled evasion')
        if address==s.FUNCTIONS['roll'][0]:
            pair=[uc.reg_read(o.reg.UC_MIPS_REG_A0),uc.reg_read(o.reg.UC_MIPS_REG_A1)]
            assert pair==[100,19],pair;rolls.append(pair);value=1
        else:value=0                     # dry/normal weather; no elemental traits
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in ('weather','roll','random'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    holy=d.at(s.battle.ABILITY_RANGE+14*0xf,14)
    assert holy[8]==8 and holy[10]==50 and not(holy[6]&2) and holy[1]==0
    records=[d.at(s.battle.ABILITY_RANGE+14*i,14)for i in range(512)]
    max_power=max(r[10]for r in records if r[8]==8);assert max_power==96,max_power
    # ma,power,af,tf,attack_up,defense_up,shell,frog/chicken,astatus,tstatus,
    # az,tz,afemale,tfemale,execute,gear
    profiles=[(ma,50,af,tf,0,0,0,0,0,0,0,0,0,0,0,0)
        for ma in(0,1,3,5,99)for af,tf in((0,0),(3,97),(40,60),(70,70),(100,100))]
    profiles += [(ma,50,70,70,bits&1,(bits>>1)&1,(bits>>2)&1,(bits>>3)&1,0,0,0,0,0,0,0,0)
        for ma in(1,5,99)for bits in range(16)]
    profiles += [(5,50,40,60,0,0,0,0,a,t,0,0,0,0,0,0)for a in range(4)for t in range(4)]
    profiles += [(ma,power,100,100,1,0,shell,1,0,0,az,tz,0,tfemale,0,0)
        for ma,power,shell,az,tz,tfemale in ((99,96,0,0,6,1),(99,96,0,0,6,0),
            (1,50,1,0,3,0),(1,50,1,12,0,0),(1,50,1,0,12,0),
            (5,50,0,0,4,0),(5,50,0,0,3,0))]
    profiles += [(5,50,70,70,0,0,0,extra,0,0,0,0,0,0,execute,gear)
                 for extra in(0,2,4,8)for execute,gear in((0,1),(1,1))]
    rows=[]
    for p in profiles:
        ma,power,af,tf,aup,dup,shell,extra,ast,tst,az,tz,afemale,tfemale,execute,gear=p
        o.uc.mem_write(actor&0x1fffff,bytes(0x1c0));o.uc.mem_write(target&0x1fffff,bytes(0x1c0))
        o.uc.mem_write(ability&0x1fffff,bytes(0x50));o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
        o.uc.mem_write((actor&0x1fffff)+0x37,bytes((ma,)))
        o.uc.mem_write((actor&0x1fffff)+0x90,bytes((4 if aup else 0,)))
        o.uc.mem_write((target&0x1fffff)+0x90,bytes((2 if dup else 0,)))
        for ptr,faith,sign,female,state in ((actor,af,az,afemale,ast),(target,tf,tz,tfemale,tst)):
            o.uc.mem_write((ptr&0x1fffff)+6,bytes((0x40 if female else 0x80,)))
            o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12))
            o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((faith,)))
            o.uc.mem_write((ptr&0x1fffff)+0x5c,bytes(((0x80 if state&1 else 0)|(0x40 if state&2 else 0),)))
        ids=([27]if shell else[])+([22]if extra==1 else[21]if extra==2 else[4]if extra==4 else[35]if extra==8 else[])
        bits=bytearray(mask(*ids));bits[4]|=(0x80 if tst&1 else 0)|(0x40 if tst&2 else 0)
        o.uc.mem_write((target&0x1fffff)+0x58,bytes(bits))
        if gear:
            o.uc.mem_write((target&0x1fffff)+0x1e,b'\x88');o.uc.mem_write((target&0x1fffff)+0x20,b'\x8f')
            o.uc.mem_write((target&0x1fffff)+0x58,bytes((bits[0]|2,*bits[1:]))) # Defend too
        r=bytearray(holy);r[10]=power;o.uc.mem_write((ability&0x1fffff)+0x30,bytes(r))
        o.uc.mem_write((ability&0x1fffff)+0x12,bytes((tf,af)));o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',0xf))
        o.uc.mem_write((target&0x1fffff)+0x18c,b'\x01')
        o.uc.mem_write((target&0x1fffff)+0x18c+0x2a,struct.pack('<H',100))
        o.write(0x8018f5fc,0 if execute else 2);rolls.clear();o.call('damage')
        damage=struct.unpack('<h',o.uc.mem_read((target&0x1fffff)+0x190,2))[0]
        hit=o.uc.mem_read((target&0x1fffff)+0x18c,1)[0]
        assert hit==1 and len(rolls)==execute,(p,hit,rolls)
        rows.append([*p,damage,hit,len(rolls)])
    lines=['/* Original Holy scalar pipeline; local-only human/unmodified-element fixtures. */',
           f'static const u16 source_magic[][{len(rows[0])}]={{']
    lines+=[' {'+','.join(map(str,r))+'},'for r in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(cases=rows,functions=o.function_hashes,holy_range=list(holy),source_formula08_max_power=max_power,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original formula08 Holy damage pipeline, ordinary male/female stats and unmodified elemental affinities. Weather getter returns normal; generic proc roll is observed/stubbed to pass. Full target-list/HP commit, elemental/innate-trait producers, monster zodiac, magic-evasion spells and final display/capping are not certified.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original Holy:',len(rows),'scalar/status/support/zodiac/Faith cases; no evasion, one execute proc draw',flush=True)


if __name__=='__main__':main()
