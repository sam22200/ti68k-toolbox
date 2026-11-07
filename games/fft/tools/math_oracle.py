#!/usr/bin/env python3
"""Unmodified retail Math targeting, height and Reflect/Counter dispatch helpers."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

def spell_profiles(d,out):
    import unicorn
    for key,name in dict(evade='battle_formula_calculate_magical_evade',react_chance='battle_action_calculate_chance_to_react').items():bind_function(key,name,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    for key,name in dict(roll='main_util_roll_pass_fail',init='main_status_init_check_data').items():
        o.function_hashes[key]=bind_function(key,name,'main',d)
    o.call('init');rng=1;draws=0;reaction_queries=0
    def external(uc,address,size,data):
        nonlocal rng,draws,reaction_queries
        if address==s.FUNCTIONS['react_chance'][0]:value=0;reaction_queries+=1
        else:
            base=uc.reg_read(o.reg.UC_MIPS_REG_A0);evade=uc.reg_read(o.reg.UC_MIPS_REG_A1)
            rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1
            value=int(((rng>>17)*base>>15)>=evade)
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in ('roll','react_chance'):
        address=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=address,end=address)
    actor=0x801908cc;target=actor+448*10;ability=0x801938c0;action=0x801d6500
    def reset(gear,skillset=21):
        for ptr in (actor,target):
            at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+6,b'\x80');o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999))
        at=target&0x1fffff
        o.uc.mem_write(at+0x1c,bytes((232 if gear&1 else 0,)))
        o.uc.mem_write(at+0x20,bytes((128 if gear&2 else 0,)))
        at=ability&0x1fffff;o.uc.mem_write(at,bytes(80));o.uc.mem_write(at+0x14,bytes((skillset,)))
        o.uc.mem_write(at+0x16,struct.pack('<H',20));o.uc.mem_write(at+0x30,d.at(s.battle.ABILITY_RANGE+14*20,14))
        o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,action)
        packet=bytearray(44);packet[0]=1;struct.pack_into('<H',packet,42,100);o.uc.mem_write(action&0x1fffff,bytes(packet))
    rows=[]
    for gear in range(4):
        for seed in (1,9,73):
            reset(gear);o.write(0x8018f5fc,2);rng=seed;draws=0;o.call('evade')
            accuracy=struct.unpack('<H',o.uc.mem_read((action&0x1fffff)+42,2))[0];preview_draws=draws
            reset(gear);o.write(0x8018f5fc,0);rng=seed;draws=0;miss=o.call('evade')
            rows.append(dict(gear=gear,seed=seed,accuracy=accuracy,hit=1-miss,draws=draws,preview_draws=preview_draws,rng=rng))
    guards=[]
    for skillset in (11,21):
        reset(0,skillset);o.call('reflect');packet=bytes(o.uc.mem_read(action&0x1fffff,44))
        assert packet[0]==(skillset==21)
        reset(0,skillset);reaction_queries=0;o.call('counter',0x1ac,2);packet=bytes(o.uc.mem_read(action&0x1fffff,44))
        reaction=struct.unpack_from('<H',packet,14)[0];assert reaction==(0 if skillset==21 else 0x1ac)
        assert reaction_queries==(0 if skillset==21 else 1)
        guards.append(dict(skillset=skillset,reflect_keeps_hit=skillset==21,reaction=reaction,chance_calls=reaction_queries))
    lines=['/* Original Math magical evasion, controlled source-order RNG. */',
        'static const struct {u8 gear,seed,chance,hit,draws;} source_math_evade[]={']
    lines+=[' {%d,%d,%d,%d,%d},'%(r['gear'],r['seed'],r['accuracy'],r['hit'],r['draws'])for r in rows];lines+=['};']
    (out/'spells.h').write_text('\n'.join(lines)+'\n')
    return dict(cases=rows,guards=guards,functions=o.function_hashes,
        scope='Unmodified Math magical-evasion and Reflect/Counter guards. External deterministic pass/fail RNG; the Counter chance callback is isolated to expose guard dispatch. No full original action/UI/AI replay.')

def main():
    out=s.HERE/'campaign/x/core-math/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for key,name in dict(calculator='battle_target_run_calculator',listing='battle_target_list_units_on_panels',
                         height='battle_unit_calculate_height_data',reflect='battle_action_check_reflect_reaction',
                         counter='battle_action_check_counter_reaction_usability').items():bind_function(key,name,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    disk,files=s.extract.disc_files(str(s.core_audit.DEFAULT_DISC))
    raw,meta,provenance=s.terrain(disk,files,22)
    w,h=meta['width'],meta['height'];o.write(0x800e4e9c,w);o.write(0x800e4ea0,h)
    rows=[];values=(0,1,2,3,4,5,7,11,25,99,100,101,251)
    # Same thirteen humans on actual Gariland terrain. All16 criteria per fixture.
    for fixture in range(6):
        o.uc.mem_write(s.TILES&0x1fffff,raw)
        actors=[]
        for i in range(21):
            at=(0x801908cc+448*i)&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+1,b'\xff')
        for i in range(13):
            x,z=i%5,i//5;sid=255;absent=0
            if fixture==1:sid=(1,15,5,2,8,255,255,255,255,255,255,255,255)[i]
            if fixture==2 and i&1:sid=17
            if fixture==3 and i in (4,5):x,z=3,0
            if fixture==3 and i==4:sid=17
            if fixture==4 and i in (2,8):absent=1
            if fixture==5:x,z=i%5,7+i//5 # slopes and canal
            at=(0x801908cc+448*i)&0x1fffff
            o.uc.mem_write(at+1,bytes((255 if absent else i,)));o.uc.mem_write(at+3,b'\x4a')
            ct=values[i];level=values[(i+fixture)%13];exp=values[(i+2*fixture)%13]
            o.uc.mem_write(at+0x21,bytes((exp,level)));o.uc.mem_write(at+0x39,bytes((ct,)))
            o.uc.mem_write(at+0x47,bytes((x,z,0)))
            if sid!=255:o.uc.mem_write(at+0x58,mask(sid))
            actors.append((x,z,ct,level,exp,sid,absent))
        heights=[]
        for i in range(13):
            if actors[i][6] or actors[i][5] in(1,15):heights.append(255);continue
            o.call('height',0x801d6200,i);heights.append(o.uc.mem_read(0x1d6205,1)[0])
        for code in range(16):
            action=bytearray(24);action[0]=0;action[1]=21
            struct.pack_into('<H',action,2,19);struct.pack_into('<HH',action,4,414+(code>>2),418+(code&3))
            o.uc.mem_write(0x1d6000,bytes(action));o.write(0x8018f4f8,0)
            n=o.call('calculator',0x801d6000);assert n!=0xffffffff;count=o.call('listing',0x801d6100,0x801908cc)
            listed=list(o.uc.mem_read(0x1d6100,count));bits=sum(1<<i for i in listed)
            rows.append(dict(fixture=fixture,code=code,actors=actors,heights=heights,mask=bits,marked=n))
    invalid=[]
    for a,b in ((413,418),(414,417),(418,419),(414,422),(65535,418)):
        action=bytearray(24);struct.pack_into('<HH',action,4,a,b);o.uc.mem_write(0x1d6000,bytes(action))
        result=o.call('calculator',0x801d6000);assert result==0xffffffff;invalid.append([a,b,result])
    lines=['/* Retail Math targets: real Gariland terrain,13 humans,all16 criteria. */',
        'static const struct {u8 fixture,code;u16 mask;} source_math[]={']
    lines += [' {%d,%d,%d},'%(r['fixture'],r['code'],r['mask']) for r in rows];lines+=['};',
        'static const u8 math_values[13]={'+','.join(map(str,values))+'};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    spells=spell_profiles(d,out)
    (out/'source.json').write_text(json.dumps(dict(cases=rows,spells=spells,invalid=invalid,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),terrain=provenance,
        scope='Unmodified retail targeting/listing and height routines on controlled humans; no original full UI or AI replay.'),indent=2)+'\n')
    print('Original Math footprints:',len(rows))
if __name__=='__main__':main()
