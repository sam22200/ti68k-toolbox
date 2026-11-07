#!/usr/bin/env python3
"""Original Earth Slash formula, elements, criticals and target finalizer."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask


def geometry(d,out):
    import copy
    from area_oracle import base
    for key,name in dict(green='battle_target_set_green_panels_for_action',
        listing='battle_target_list_units_on_panels',directional='battle_target_build_directional_attack_panels').items():
        bind_function(key,name,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    profiles=[]
    for kind in (0,1,2,3):
        for cx,cz in ((4,2),(0,2),(2,4),(2,0),(4,4),(2,2)):
            p=base(kind);p.update(actor=0,cx=cx,cz=cz,aoe=8,vert=2,f1=17,f2=21,f4=0)
            p['actors'][0][:3]=[2,2,0];p['actors'][1][:3]=[4,2,0]
            p['actors'][2][:3]=[3,2,0];profiles.append(p)
    for layer in (0,1):
        p=base(1);p.update(cx=4,cz=2,aoe=8,vert=2,f1=17,f2=21,f4=0)
        p['actors'][0][:3]=[2,2,layer]
        p['terrain'][12]=[2,0,0];p['terrain'][37]=[2,0,0]
        p['actors'][1][:3]=[4,2,0];p['actors'][2][:3]=[3,2,0]
        profiles.append(p)
    # Maximum native grid: reverse IDs, both floors, source reference height narrowing.
    p=base(1);p.update(w=8,h=16,cx=7,cz=8,aoe=8,vert=2,f1=17,f2=21,f4=0,
        terrain=[[2+4*l,0,0]for l in range(2)for i in range(128)])
    for i in range(13):p['actors'][i]=[i%8,8+i//8,i%2,i%3,255,0]
    profiles.append(p)
    records=[];rows=[]
    for p in profiles:
        w,h=p['w'],p['h'];area=w*h;o.write(0x800e4e9c,w);o.write(0x800e4ea0,h)
        tiles=bytearray(4096)
        for i in range(512):tiles[8*i+6]=1
        for l in range(2):
            for i in range(area):
                height,dh,flags=p['terrain'][l*area+i];at=8*(l*256+i)
                tiles[at+2]=height;tiles[at+3]=dh;tiles[at+6]=flags
        o.uc.mem_write(s.TILES&0x1fffff,bytes(tiles))
        for i in range(21):
            at=(0x801908cc+448*i)&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+1,b'\xff')
        for i,(x,z,l,team,sid,absent) in enumerate(p['actors']):
            at=(0x801908cc+448*i)&0x1fffff;o.uc.mem_write(at+1,bytes((255 if absent else i,)))
            o.uc.mem_write(at+5,bytes((16 if team==1 else 0,)));o.uc.mem_write(at+0x1ba,bytes((16 if team==1 else 0,)))
            o.uc.mem_write(at+0x47,bytes((x,z,128 if l else 0)))
            if sid!=255:o.uc.mem_write(at+0x58,mask(sid))
        o.uc.mem_write(0x1d6000,struct.pack('<BBHHHBBBBHHHBB',0,5,103,0,0,0,0,5,0,p['cx'],0,p['cz'],0,0))
        o.write(0x8018f4f8,0);count=o.call('green',0x801d6000)
        n=o.call('listing',0x801d6100,0x801908cc)
        listed=list(o.uc.mem_read(0x1d6100,n));expected=sum(1<<i for i in listed)
        distances=[abs(p['actors'][i][0]-p['actors'][0][0])+abs(p['actors'][i][1]-p['actors'][0][1])for i in listed]
        assert distances==sorted(distances)
        header=bytes(p[k]for k in ('w','h','actor','cx','cz','cl','follow','ref','aoe','vert','f1','f2','f4'))
        marks=bytearray(64)
        source_tiles=o.uc.mem_read(s.TILES&0x1fffff,4096)
        for i in range(512):
            if source_tiles[8*i+5]&128:marks[i>>3]|=1<<(i&7)
        record=header+b'\0'+b''.join(bytes(r)for r in p['terrain'])+b''.join(bytes(r)for r in p['actors'])+struct.pack('>h',expected)+marks
        records.append(struct.pack('>H',len(record))+record)
        rows.append(dict(**p,expected_mask=expected,source_order=listed,marked_source_tiles=count))
    (out/'flg00.bin').write_bytes(b'FLG1'+struct.pack('>H',len(records))+b''.join(records))
    (out/'geometry.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe)),indent=2)+'\n')
    print('Original line footprints:',len(rows))


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-line/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    geometry(d,out)
    names=dict(damage='battle_formula_damage_pa_plus_y_half_times_pa',finish='battle_action_finalize_target_current_action',
        sort='battle_target_sort_list',remove='battle_status_remove',random='battle_formula_get_random_0_7fff',
        variable='battle_script_get_variable',terrain='battle_action_get_elemental_ability_id')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    for k,n in dict(roll='main_util_roll_pass_fail',lib_rand='rand',init='main_status_init_check_data').items():
        o.function_hashes[k]=bind_function(k,n,'main',d)
    o.call('init');rng=1;draws=0;weather=0
    def rand():
        nonlocal rng,draws
        rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1
        return rng>>17
    def external(uc,address,size,data):
        value=0
        if address in(s.FUNCTIONS['random'][0],s.FUNCTIONS['lib_rand'][0]):value=rand()
        elif address==s.FUNCTIONS['roll'][0]:
            bound=uc.reg_read(o.reg.UC_MIPS_REG_A0);chance=uc.reg_read(o.reg.UC_MIPS_REG_A1)
            value=int((rand()*bound>>15)>=chance)
        elif address==s.FUNCTIONS['variable'][0]:
            assert uc.reg_read(o.reg.UC_MIPS_REG_A0)==0x23;value=weather
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in('roll','random','sort','lib_rand','variable'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0
    # PA, zodiac pair, target sex, physical supports, status, affinity, weather, seed.
    profiles=[]
    for pa in (1,13,31,99):
        for mode in (0,1,2,3,4,5,6,8,9,10):
            profiles.append((103,pa,17,0,100,0,0,0,0,0,mode,0,0,1))
    for az,tz,sex in ((0,3,0),(0,4,0),(0,6,0),(0,6,1),(12,4,0),(4,12,0)):
        profiles.append((103,23,17,70,70,az,tz,sex,0,0,0,0,0,1))
    for up in (1,2,3):
        for defense in (0,1,2):
            profiles.append((103,23,17,70,70,0,0,0,up,defense,1,0,0,1))
    for affinity in range(1,6):
        profiles.append((103,23,17,70,70,0,0,0,0,0,0,affinity,0,1))
    for mode in (0,5,6):
        for seed in (2,9,32,10000,32767):
            profiles.append((103,23,17,100,0,0,0,0,0,0,mode,0,0,seed))
    rows=[]
    for p in profiles:
        ab,pa,ma,af,tf,az,tz,sex,up,defense,mode,affinity,w,seed=p
        r=d.at(s.battle.ABILITY_RANGE+14*ab,14);request=d.at(s.battle.STATUS_SET+6*r[11],6);outputs=[]
        for execute in(0,1):
            for ptr,sign,female,faith in((actor,az,0,af),(target,tz,sex,tf)):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+6,bytes((64 if female else 128,)))
                o.uc.mem_write(at+8,struct.pack('<H',sign<<12));o.uc.mem_write(at+0x26,bytes((faith,)))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999));o.uc.mem_write(at+0x1a,bytes((255,)*7))
            a=actor&0x1fffff;t=target&0x1fffff;o.uc.mem_write(a+0x36,bytes((pa,ma)))
            o.uc.mem_write(a+0x90,bytes((0x10 if up==1 else 4 if up==3 else 0,0x20 if up==2 else 0)));o.uc.mem_write(t+0x90,bytes((8 if defense==1 else 2 if defense==2 else 0,)))
            ids={1:26,2:35,3:4,4:17,5:16,6:18,7:5,8:2,9:8,10:31}
            inflicted=mask(ids[mode])if mode in ids else request[1:]if mode in(9,10)else bytes(5)
            innate=bytes(5);immune=bytes(5)
            o.uc.mem_write(t+0x4e,innate+immune+bytes(a|b for a,b in zip(innate,inflicted)))
            o.uc.mem_write(t+0x1bb,inflicted)
            o.uc.mem_write(t+0x43,b'\x63');o.uc.mem_write(t+0x47,b'\x01\x00\x01')
            o.uc.mem_write(t+0x6d,bytes(r[7]if affinity==k else 0 for k in range(1,6)))
            if affinity==5:o.uc.mem_write(a+0x71,bytes((r[7],)))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write((ability&0x1fffff)+0x30,r)
            o.uc.mem_write((ability&0x1fffff)+0x46,request);o.uc.mem_write((ability&0x1fffff)+0x12,bytes((tf,af)))
            o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);o.write(0x800b6698,0);weather=w;rng=seed;draws=0
            o.call('remove');o.call('damage');o.call('finish');v=bytes(o.uc.mem_read(t+0x18c,44))
            damage,healing=struct.unpack_from('<hh',v,4)
            outputs.append(dict(hit=v[0],chance=struct.unpack_from('<H',v,42)[0],damage=damage,healing=healing,
                add=list(v[27:32]),remove=list(v[32:37]),draws=draws,type=v[37],critical=v[1]))
        rows.append(dict(input=p,preview=outputs[0],execute=outputs[1]))
    lines=['/* Original Earth Slash: unmodified formula31/finalizer. */',
        'static const struct {u16 seed,amount;u8 pa,az,tz,sex,up,defense,mode,affinity,weather,hit,kind,draws,critical;} source_line[]={']
    for row in rows:
        p=row['input'];v=row['execute'];kind=3 if v['type']&64 else 0
        amount=v['healing']if kind==3 else v['damage']
        values=[p[13],amount&65535,p[1],*p[5:13],v['hit'],kind,v['draws'],v['critical']]
        lines.append(' {'+','.join(map(str,values))+'},')
    lines+=['};'];(out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        scope='Unmodified original formula31 and post-formula target finalizer, including elemental/status filtering and controlled RNG; '
              'Controlled RNG and no single-target knockback (target_count0). No original area traversal, reaction dispatch or campaign simulation.'),indent=2)+'\n')
    print('Original Earth Slash profiles:',len(rows))


if __name__=='__main__':main()
