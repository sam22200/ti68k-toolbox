#!/usr/bin/env python3
"""Small original-MIPS trap lookup/raw-growth fixture; no map census or PS1 UI."""
import argparse
import itertools
import json
from pathlib import Path
import struct
import surface_data as s
from status_state_oracle import bind_function


def main():
    import unicorn
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=s.HERE/'campaign/x/tile-traps/oracle')
    out=ap.parse_args().out.resolve();out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names={'lookup':('battle_map_set_item_trap_data','battle'),
           'find_unit':('battle_unit_find_at_tile','battle'),
           'variable':('battle_script_get_variable','battle'),
           'resources':('battle_action_check_mp_switch_distribute_and_damage_split_usability','battle'),
           'post_damage':('battle_action_check_stat_save_and_restore_reaction_usability','battle'),
           'blocked':('battle_action_is_reaction_blocked','battle'),
           'restriction':('battle_unit_get_action_restriction','battle'),
           'drain':('battle_reaction_prepare_next','battle'),
           'growth':('main_unit_apply_level_growth','main'),
           'refresh':('main_unit_refresh_stats_and_statuses','main'),
           'random':('rand','main')}
    for key,(name,target)in names.items():
        if target=='battle':bind_function(key,name,target,disc)
    o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=100000
    for key,(name,target)in names.items():
        if target=='main':bind_function(key,name,target,disc)
    draws=0;blocked=0;unit=0x801908cc
    def external(uc,address,size,data):
        nonlocal draws
        value=0
        if address==s.FUNCTIONS['find_unit'][0]:value=0xffffffff
        if address==s.FUNCTIONS['random'][0]:draws+=1;value=17
        if address==s.FUNCTIONS['blocked'][0]:
            value=blocked or struct.unpack('<H',uc.mem_read((unit&0x1fffff)+0x19a,2))[0]!=0
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in ['find_unit','variable','refresh','random','blocked','restriction']:
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    lookup=[]
    # Flags, both layers, no match and first-record precedence. All items disabled
    # so the unrelated item-found flag writer is not part of this lookup oracle.
    for flags,layer in itertools.product([0,1,2,4,8,15,16,17,31],range(2)):
        rows=bytes((0x22,flags|32|(layer<<7),0,0))+bytes((0xff,48,0,0))*3
        o.uc.mem_write(0x193910,rows)
        for query_layer in range(2):
            ptr=o.call('lookup',2,2,query_layer)&0x1fffff
            result=bytes(o.uc.mem_read(ptr,6));trap=result[4]if result[0]&2 else 255
            lookup.append((flags,layer,query_layer,trap))
    growth=[]
    for down,level,g,value in itertools.product(range(2),(1,2,99),(0,6,100),(0,508530,0xffffff)):
        o.uc.mem_write(unit&0x1fffff,bytes(448))
        o.uc.mem_write((unit&0x1fffff)+0x22,bytes((level,)))
        o.uc.mem_write((unit&0x1fffff)+0x72,value.to_bytes(3,'little')*5)
        o.uc.mem_write((unit&0x1fffff)+0x81,bytes((g,100))*5)
        draws=0;o.call('growth',unit,down)
        result=bytes(o.uc.mem_read((unit&0x1fffff)+0x72,15))
        values=[int.from_bytes(result[3*i:3*i+3],'little')for i in range(5)]
        assert values==values[:1]*5 and draws==5
        growth.append((down,level,g,value,values[0],draws))
    reactions=[]
    flags={445:(2,1),446:(3,128),422:(0,128),428:(0,2),441:(2,16),439:(2,64),431:(1,64)}
    cases=[(445,20,100,5,100,0),(445,20,100,5,0,0),(445,20,100,0,100,0),
           (445,0,2,5,100,0),(445,20,100,5,100,1),
           (446,20,100,20,100,0),(446,20,100,20,0,0),(446,0,2,20,100,0),
           (422,20,100,20,100,0),(422,20,100,20,0,0),(422,0,2,20,100,0),
           (428,20,100,20,0,0),(441,20,100,20,100,0),(439,20,100,20,100,0),
           (431,20,30,20,100,0),(431,20,30,20,0,0),(431,20,100,20,100,0),
           (422,20,100,20,100,1),(422,20,10,20,100,0),(446,20,10,20,100,0)]
    for rid,amount,hp,mp,brave,blocked in cases:
        initial_hp,initial_mp=hp,mp
        units=bytearray(21*448)
        for i in range(21):units[i*448+1]=255
        units[1]=0;units[0x24]=brave;byte,mask=flags[rid];units[0x8b+byte]=mask
        struct.pack_into('<4H',units,0x28,hp,100 if amount else 4,mp,20)
        action=0x18c;units[action]=1;units[action+0x25]=128
        struct.pack_into('<H',units,action+4,amount)
        o.uc.mem_write(unit&0x1fffff,bytes(units));o.uc.mem_write(0x1938c0,bytes(80))
        o.write(0x80192d98,unit);o.write(0x80192d90,unit+action)
        o.write(0x8018f5f0,0);o.write(0x8018f5fc,0);o.write(0x8018f8c8,0)
        draws=0;o.call('resources')
        packet=bytes(o.uc.mem_read((unit&0x1fffff)+action,44))
        hp=max(0,hp-struct.unpack_from('<H',packet,4)[0]);mp=max(0,mp-struct.unpack_from('<H',packet,8)[0])
        o.uc.mem_write((unit&0x1fffff)+0x28,struct.pack('<H',hp))
        o.uc.mem_write((unit&0x1fffff)+0x5a,bytes((int(hp<=(100 if amount else 4)//5),)))
        if hp:o.call('post_damage')
        skipped=o.call('drain',0x801d7000)==0xffffffff
        reactions.append((rid,amount,initial_hp,initial_mp,brave,blocked,hp,mp,draws,int(skipped)))
    report=dict(lookup=lookup,growth=growth,reactions=reactions,
                functions={k:s.FUNCTIONS[k]for k in names},
                exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),
                scope='Original lookup, raw arithmetic and20 resource/post-damage reaction boundaries plus acting-slot drain. Occupancy/map-variable, eligibility and RNG/derived refresh isolated. Not a full trap finalizer or battle replay.')
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    lines=['// Local original-MIPS fixture; never commit generated source data.',
           'static const u8 trap_lookup_oracle[][4]={']
    lines+=['{'+','.join(map(str,row))+'},'for row in lookup]
    lines+=['};','static const struct {u8 down,level,growth;u32 before,after;u8 draws;} trap_growth_oracle[]={']
    lines+=['{'+','.join(map(str,row))+'},'for row in growth];lines+=['};']
    lines+=['static const u16 trap_reaction_oracle[][10]={']
    lines+=['{'+','.join(map(str,row))+'},'for row in reactions];lines+=['};']
    (out/'fixture.h').write_text('\n'.join(lines)+'\n')
    print(f'Original MIPS: {len(lookup)} trap lookups, {len(growth)} raw-growth boundaries, {len(reactions)} reaction boundaries')


if __name__=='__main__':main()
