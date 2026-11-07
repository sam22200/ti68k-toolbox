#!/usr/bin/env python3
"""Retail Songs/Dances: unchanged formulas, target finalizer and recurring clock."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask


def geometry_profiles(o,out,d):
    for k,n in dict(green='battle_target_set_green_panels_for_action',listing='battle_target_list_units_on_panels').items():
        o.function_hashes[k]=bind_function(k,n,'battle',d)
    rows=[];base=0x1908cc
    for ab in (86,93):
        for mode in range(3):
            o.write(0x800e4e9c,5);o.write(0x800e4ea0,5);tiles=bytearray(4096)
            for j in range(512):tiles[8*j+6]=1
            for j in range(25):tiles[8*j+6]=0;tiles[8*(256+j)+6]=0
            o.uc.mem_write(s.TILES&0x1fffff,bytes(tiles))
            actors=[]
            for j in range(21):
                at=base+448*j;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+1,b'\xff')
                if j>=11:continue
                team=j%3;sid=(255,5,2,8,31,35,30,1,15,10,12)[j]if mode else 255
                absent=bool(mode==2 and j==10);actors.append([team,sid,int(absent)])
                o.uc.mem_write(at+1,bytes((255 if absent else j,)));o.uc.mem_write(at+5,bytes((16 if team==1 else 0,)))
                o.uc.mem_write(at+0x1ba,bytes((16 if team==1 else 0,)));o.uc.mem_write(at+0x47,bytes((j%5,j//5,0)))
                if sid!=255:o.uc.mem_write(at+0x58,mask(sid))
            o.uc.mem_write(0x1d6000,struct.pack('<BBHHHBBBBHHHBB',0,21,ab,0,0,0,0,5,0,0,0,0,0,0))
            o.write(0x8018f4f8,0);panels=o.call('green',0x801d6000)
            n=o.call('listing',0x801d6100,0x801908cc);listed=list(o.uc.mem_read(0x1d6100,n))
            rows.append(dict(ab=ab,mode=mode,actors=actors,mask=sum(1<<i for i in listed),order=listed,panels=panels))
    (out/'geometry.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes),indent=2)+'\n')
    with (out/'source.h').open('a')as f:
        f.write('static const u16 source_performance_masks[][3]={\n')
        for r in rows:f.write(' {'+','.join(map(str,[r['ab'],r['mode'],r['mask']]))+'},\n')
        f.write('};\n')


def clock_profiles(o,out):
    base=0x801908cc;phase=0x8018f518;rows=[]
    for ab in range(86,100):
        a=base&0x1fffff;o.uc.mem_write(a,bytes(21*448))
        for j in range(21):o.uc.mem_write(a+448*j+1,b'\xff')
        o.uc.mem_write(a+1,b'\0');o.uc.mem_write(a+0x28,struct.pack('<4H',999,999,999,999))
        o.uc.mem_write(a+0x58,mask(7));o.uc.mem_write(a+0x1bb,mask(7));o.uc.mem_write(a+0x15d,b'\x01')
        o.uc.mem_write(a+0x170,struct.pack('<H',ab));o.write(phase,2)
        o.write(0x8019389c,0);o.write(0x8018f51c,0);o.write(0x8018f514,0)
        events=[]
        for j in range(3):
            result=o.call('clock',0);ct=o.uc.mem_read(a+0x15d,1)[0]
            assert result==0x200,(ab,j,result,ct)
            events.append(dict(result=result,next_ct=ct))
        # The source end-turn credit counts Performing as an Act.
        o.uc.mem_write(a+0x186,b'\x01\0\0');o.uc.mem_write(a+0x39,b'\0')
        o.call('end',0);credit=o.uc.mem_read(a+0x39,1)[0]
        moved=o.call('move',base,0);still=o.uc.mem_read(a+0x58,1)[0]
        rows.append(dict(ab=ab,pulses=events,wait_credit=credit,movement_restriction=moved,status_after_move=still))
    a=base&0x1fffff;o.uc.mem_write(a,bytes(21*448))
    for j in range(21):o.uc.mem_write(a+448*j+1,b'\xff')
    for j in (0,6,12):
        at=a+448*j;o.uc.mem_write(at+1,bytes((j,)));o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999))
        o.uc.mem_write(at+0x18a,bytes((j,)));o.uc.mem_write(at+0x15d,bytes((255 if j==6 else 1,)))
        if j!=6:o.uc.mem_write(at+0x58,mask(7));o.uc.mem_write(at+0x170,struct.pack('<H',94))
    o.write(phase,2);o.write(0x8019389c,0);o.write(0x8018f51c,0);o.write(0x8018f514,0)
    wave=[o.call('clock',0)];o.uc.mem_write(a+6*448+0x39,b'\xff');wave.append(o.call('clock',0))
    o.uc.mem_write(a+6*448+0x186,b'\0');o.write(phase,9);wave.append(o.call('clock',0))
    assert wave==[0x200,0x106,0x20c],wave
    (out/'clock.json').write_text(json.dumps(dict(cases=rows,quick_wave=wave,functions=o.function_hashes,
        scope='Unchanged source CT state machine, last-ability CT reload, end-turn credit and Charge-only move cancellation. Expiry/upkeep and graphical status handlers isolated.'),indent=2)+'\n')
    with (out/'source.h').open('a')as f:
        f.write('static const u8 source_performance_clocks[][4]={\n')
        for r in rows:f.write(' {'+','.join(map(str,[r['ab'],r['pulses'][0]['next_ct'],r['wait_credit'],r['movement_restriction']]))+'},\n')
        f.write('};\nstatic const u16 source_performance_wave[]={'+','.join(map(str,wave))+'};\n')


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-performance/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(song='battle_formula_song',dance='battle_formula_dance',
        finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
        remove='battle_status_remove',random='battle_formula_get_random_0_7fff',
        clock='battle_action_run_between_turn_control',expire='battle_status_update_expiring',
        upkeep='battle_status_build_upkeep_action',handlers='battle_status_enable_special_flags',
        move='battle_status_remove_charging_ability_ct',end='battle_action_end_turn',
        stat='battle_unit_apply_stat_increment_decrement')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    for k,n in dict(roll='main_util_roll_pass_fail',lib_rand='rand',init='main_status_init_check_data',
                    action='main_status_set_action_state').items():o.function_hashes[k]=bind_function(k,n,'main',d)
    o.call('init')
    rng=1;draws=0
    def rand():
        nonlocal rng,draws
        rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1
        return rng>>17
    def external(uc,address,size,data):
        value=0
        if address in(s.FUNCTIONS['random'][0],s.FUNCTIONS['lib_rand'][0]):value=rand()
        elif address==s.FUNCTIONS['roll'][0]:
            value=int((rand()*uc.reg_read(o.reg.UC_MIPS_REG_A0)>>15)>=uc.reg_read(o.reg.UC_MIPS_REG_A1))
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in ('roll','random','lib_rand','sort','expire','upkeep','handlers'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0
    profiles=[]
    for ab in range(86,100):
        for mode in range(8):profiles.append((ab,17,23,70,mode,1))
        for pa,ma,brave in ((1,1,0),(99,99,100),(31,47,43)):profiles.append((ab,pa,ma,brave,0,333))
    for ab in (91,98):
        for seed in (2,9,32,77,333,10000,32767,65535):profiles.append((ab,17,23,70,0,seed))
    # Frog alone must toggle off when Nameless Dance selects it; Sleep would
    # bypass the formula before this selection in the full-status fixture.
    for seed in (1,2,9,24,32,77,333,10000,32767,65535):profiles.append((98,17,23,70,8,seed))
    rows=[]
    for p in profiles:
        ab,pa,ma,brave,mode,seed=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        req=d.at(s.battle.STATUS_SET+6*r[11],6);results=[]
        for execute in (0,1):
            for ptr in (actor,target):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999))
                o.uc.mem_write(at+0x1a,bytes((255,)*7));o.uc.mem_write(at+6,b'\x80')
            a=actor&0x1fffff;t=target&0x1fffff;o.uc.mem_write(a+0x24,bytes((brave,0)))
            o.uc.mem_write(a+0x36,bytes((pa,ma)));o.uc.mem_write(t+0x26,b'\x64')
            sid={1:35,2:3,3:2,4:8,5:31,8:22}.get(mode)
            inf=mask(sid)if sid is not None else req[1:]if mode==7 else bytes(5)
            o.uc.mem_write(t+0x58,inf);o.uc.mem_write(t+0x1bb,inf)
            if mode==6:o.uc.mem_write(t+0x53,req[1:])
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write((ability&0x1fffff)+0x30,r)
            o.uc.mem_write((ability&0x1fffff)+0x46,req);o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.uc.mem_write(t+0x18c,b'\x01'+bytes(41)+b'\x64\0')
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            o.write(0x80192d8c,0x80192da0);o.uc.mem_write(0x192da0,bytes(44));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws=0
            if execute and ab<93:
                # Source global-friendly lists include the caster before this later slot.
                o.write(0x80192d98,actor);o.write(0x80192d90,actor+0x18c)
                o.uc.mem_write(a+0x18c,b'\x01'+bytes(41)+b'\x64\0')
                o.call('remove');o.call('song');o.call('finish')
                o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            o.call('remove');o.call('song'if ab<93 else'dance');o.call('finish');v=bytes(o.uc.mem_read(t+0x18c,44))
            results.append(dict(hit=v[0],chance=struct.unpack_from('<H',v,42)[0],
                channels=list(struct.unpack_from('<4h',v,4)),sp=v[18],ct=v[19],pa=v[20],ma=v[21],
                add=list(v[27:32]),remove=list(v[32:37]),kind=v[37],draws=draws))
        rows.append(dict(input=p,preview=results[0],execute=results[1]))
    assert any(row['input'][4]==8 and row['execute']['remove'][2]&2 for row in rows)
    lines=['/* Unmodified retail Song/Dance formula/finalizer observations. */',
        'static const struct {u16 seed,amount,packed;u8 ab,pa,ma,brave,mode,hit,chance,draws,preview_hit,preview_chance;} source_performance[]={']
    for row in rows:
        p=row['input'];v=row['execute'];ab=p[0]
        amount=v['channels'][3]if ab==86 else v['channels'][1]if ab==87 else v['channels'][2]if ab==93 else v['channels'][0]if ab==94 else (255 if ab==92 else 256 if ab==99 else (0-((v['sp']or v['pa']or v['ma'])&127))&65535 if ab in(88,89,90)else (v['sp']or v['pa']or v['ma']))
        packed=next(((i<<8)|b for i,b in enumerate(v['add'])if b),0)
        if ab==98 and v['remove'][2]&2:packed=0x0202
        if ab in(91,98):amount=packed
        values=[p[5],amount,packed,*p[:5],v['hit'],v['chance'],v['draws'],row['preview']['hit'],row['preview']['chance']]
        lines.append(' {'+','.join(map(str,values))+'},')
    lines.append('};');(out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        scope='Unmodified formulas and finalizer; controlled RNG and isolated target sort. Human unmounted units, no full original AI/menu/reaction/reward dispatcher.'),indent=2)+'\n')
    clock_profiles(o,out)
    geometry_profiles(o,out,d)
    print('Original Song/Dance formula profiles:',len(rows),flush=True)


if __name__=='__main__':main()
