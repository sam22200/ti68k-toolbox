#!/usr/bin/env python3
"""Original Draw Out formulas, status filtering, per-target katana rolls and finalizer."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask


def geometry(d,out):
    from area_oracle import base
    for key,name in dict(green='battle_target_set_green_panels_for_action',listing='battle_target_list_units_on_panels').items():
        bind_function(key,name,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    records=[];rows=[]
    for kind in (0,1,2):
        for cx,cz in ((4,2),(0,2),(2,4),(2,0)):
            p=base(kind);p.update(cx=cx,cz=cz);p['actors'][0][:3]=[2,2,0]
            o.write(0x800e4e9c,5);o.write(0x800e4ea0,5);tiles=bytearray(4096)
            for i in range(512):tiles[8*i+6]=1
            for l in range(2):
                for i in range(25):
                    height,dh,flags=p['terrain'][25*l+i];at=8*(256*l+i)
                    tiles[at+2]=height;tiles[at+3]=dh;tiles[at+6]=flags
            o.uc.mem_write(s.TILES&0x1fffff,bytes(tiles))
            for i in range(21):
                at=(0x801908cc+448*i)&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+1,b'\xff')
            for i,(x,z,l,team,sid,absent) in enumerate(p['actors']):
                at=(0x801908cc+448*i)&0x1fffff;o.uc.mem_write(at+1,bytes((255 if absent else i,)))
                o.uc.mem_write(at+5,bytes((16 if team==1 else 0,)));o.uc.mem_write(at+0x1ba,bytes((16 if team==1 else 0,)))
                o.uc.mem_write(at+0x47,bytes((x,z,128 if l else 0)))
                if sid!=255:o.uc.mem_write(at+0x58,mask(sid))
            o.uc.mem_write(0x1d6000,struct.pack('<BBHHHBBBBHHHBB',0,19,83,0,0,45,0,5,0,cx,0,cz,0,0))
            o.write(0x8018f4f8,0);count=o.call('green',0x801d6000)
            n=o.call('listing',0x801d6100,0x801908cc);listed=list(o.uc.mem_read(0x1d6100,n))
            expected=sum(1<<i for i in listed);marks=bytearray(64);raw=o.uc.mem_read(s.TILES&0x1fffff,4096)
            for i in range(512):
                if raw[8*i+5]&128:marks[i>>3]|=1<<(i&7)
            record=bytes((5,5,0,cx,cz,0,0,0,8,3,17,85,0,0))+b''.join(bytes(r)for r in p['terrain'])+b''.join(bytes(r)for r in p['actors'])+struct.pack('>h',expected)+marks
            records.append(struct.pack('>H',len(record))+record);rows.append(dict(**p,mask=expected,order=listed,panels=count))
    (out/'fdo00.bin').write_bytes(b'FDO1'+struct.pack('>H',len(records))+b''.join(records))
    (out/'geometry.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes),indent=2)+'\n')


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-draw-out/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    geometry(d,out)
    names=dict(f20='battle_formula_draw_out_damage',f21='battle_formula_draw_out_mp_damage',
        f22='battle_formula_draw_out_status',f23='battle_formula_draw_out_heal',
        finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
        remove='battle_status_remove',random='battle_formula_get_random_0_7fff',
        variable='battle_script_get_variable',katana='battle_action_finalize_draw_out_katana_result')
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
    # Ability,MA,zodiac pair,target sex,MAUp,MDUp,status mode,seed,Mime.
    profiles=[]
    for ab in range(76,86):
        for mode in (0,1,2,3,4,5,6,7,8):profiles.append((ab,17,0,0,0,0,0,mode,1,0))
        for ma,az,tz,sex,up,defense in ((1,0,3,0,0,1),(99,0,6,1,1,0),(31,12,4,0,1,1)):
            profiles.append((ab,ma,az,tz,sex,up,defense,0,9,0))
        profiles.append((ab,17,0,0,0,0,0,0,10000,1))
    for ab in (80,82):
        for seed in (2,9,32,333,10000,32767):profiles.append((ab,17,0,0,0,0,0,0,seed,0))
    rows=[]
    for p in profiles:
        ab,ma,az,tz,sex,up,defense,mode,seed,mime=p
        r=d.at(s.battle.ABILITY_RANGE+14*ab,14);request=d.at(s.battle.STATUS_SET+6*r[11],6);outputs=[]
        for execute in(0,1):
            for ptr,sign,female,faith in((actor,az,0,0),(target,tz,sex,100)):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+6,bytes((64 if female else 128,)))
                o.uc.mem_write(at+8,struct.pack('<H',sign<<12));o.uc.mem_write(at+0x26,bytes((faith,)))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999));o.uc.mem_write(at+0x1a,bytes((255,)*7))
            a=actor&0x1fffff;t=target&0x1fffff;o.uc.mem_write(a+0x36,bytes((19,ma)))
            o.uc.mem_write(a+0x90,bytes((4 if up else 0,)));o.uc.mem_write(t+0x90,bytes((2 if defense else 0,)))
            ids={1:27,2:22,3:21,4:3,5:2,6:8,7:31}
            inflicted=mask(ids[mode])if mode in ids else bytes(5)
            innate=bytes(5);immune=request[1:]if mode==8 else bytes(5)
            o.uc.mem_write(t+0x4e,innate+immune+bytes(a|b for a,b in zip(innate,inflicted)))
            o.uc.mem_write(t+0x1bb,inflicted)
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write((ability&0x1fffff)+0x30,r)
            o.uc.mem_write((ability&0x1fffff)+0x46,request);o.uc.mem_write((ability&0x1fffff)+0x12,bytes((100,0)))
            o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            o.write(0x80192d8c,0x80192da0);o.uc.mem_write(0x192da0,bytes(44))
            o.uc.mem_write(a+3,bytes((0x5d if mime else 0x58,)));o.uc.mem_write(a+0x16f,b'\x13')
            o.uc.mem_write(a+0x176,bytes((ab-38,)))
            o.uc.mem_write((ability&0x1fffff)+0x3e,d.at(s.battle.WEAPONS+8*(ab-38),8))
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);o.write(0x800b6698,0);weather=0;rng=seed;draws=0
            o.call('remove');o.call('f%02x'%r[8]);o.call('finish');v=bytes(o.uc.mem_read(t+0x18c,44))
            o.call('katana',actor,0x801d7000,int(v[0]!=0));loss=struct.unpack('<H',o.uc.mem_read(0x192db0,2))[0]
            damage,healing=struct.unpack_from('<hh',v,4)
            outputs.append(dict(hit=v[0],chance=struct.unpack_from('<H',v,42)[0],damage=damage,healing=healing,
                add=list(v[27:32]),remove=list(v[32:37]),draws=draws,type=v[37],mp=struct.unpack_from('<h',v,8)[0],breaks=int(bool(loss&0x1000))))
        rows.append(dict(input=p,preview=outputs[0],execute=outputs[1]))
    groups=[]
    for ab in (76,81,82):
        for seed in (1,9,32,333,10000,32767):
            for dead in (0,3):
                r=d.at(s.battle.ABILITY_RANGE+14*ab,14);request=d.at(s.battle.STATUS_SET+6*r[11],6)
                a=actor&0x1fffff;t=target&0x1fffff;o.uc.mem_write(a,bytes(448));o.uc.mem_write(a+3,b'\x58');o.uc.mem_write(a+0x37,b'\x11')
                o.uc.mem_write(a+0x16f,b'\x13');o.uc.mem_write(a+0x176,bytes((ab-38,)))
                o.uc.mem_write(0x192da0,bytes(44));o.write(0x80192d8c,0x80192da0)
                o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write((ability&0x1fffff)+0x30,r)
                o.uc.mem_write((ability&0x1fffff)+0x3e,d.at(s.battle.WEAPONS+8*(ab-38),8))
                o.uc.mem_write((ability&0x1fffff)+0x46,request);o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
                rng=seed;draws=0;hits=0;effects=[];o.write(0x8018f5fc,0)
                for j in range(3 if ab==81 else 2):
                    o.uc.mem_write(t,bytes(448));o.uc.mem_write(t+0x28,struct.pack('<4H',999,999,999,999))
                    o.uc.mem_write(t+0x58,mask(2)if dead and(ab!=81 or j!=0)else bytes(5));o.uc.mem_write(t+0x1bb,mask(2)if dead and(ab!=81 or j!=0)else bytes(5))
                    o.uc.mem_write(t+0x18c,b'\x01'+bytes(41)+b'\x64\0')
                    o.call('remove');o.call('f%02x'%r[8]);o.call('finish');v=bytes(o.uc.mem_read(t+0x18c,44));hits+=bool(v[0]);effects.append(list(v))
                o.call('katana',actor,0x801d7000,hits);loss=struct.unpack('<H',o.uc.mem_read(0x192db0,2))[0]
                groups.append(dict(ab=ab,seed=seed,dead=dead,draws=draws,broken=int(bool(loss&0x1000)),effects=effects))
    lines=['/* Unmodified original Draw Out scalar/finalizer profiles. */',
        'static const struct {u16 seed,amount,packed;u8 ab,ma,az,tz,sex,up,defense,mode,mime,hit,kind,draws,broken;} source_draw_out[]={']
    for row in rows:
        p=row['input'];v=row['execute'];kind=3 if v['type']&64 else 11 if v['type']&32 else 10 if p[0]in(81,84)else 0
        amount=v['healing']if kind==3 else v['mp']if kind==11 else v['damage']
        packed=next(((i<<8)|m for i,m in enumerate(v['add'])if m),0)
        if kind==10:amount=packed
        values=[p[8],amount&65535,packed,p[0],*p[1:8],p[9],v['hit'],kind,v['draws'],v['breaks']]
        lines.append(' {'+','.join(map(str,values))+'},')
    lines+=['};','static const u16 source_draw_groups[][5]={']
    lines+=[' {'+','.join(str(g[k])for k in ('ab','seed','dead','draws','broken'))+'},'for g in groups]
    lines+=['};'];(out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,groups=groups,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        scope='Unmodified four Draw Out formulas and target/katana finalizers; controlled RNG, representative native supports, '
              'human unmounted targets and native source status filtering. No full original AI/menu/animation/reward dispatcher.'),indent=2)+'\n')
    print('Original Draw Out profiles:',len(rows))



if __name__=='__main__':main()
