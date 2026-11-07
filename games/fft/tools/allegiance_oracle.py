#!/usr/bin/env python3
"""Unchanged retail Steal Heart/Invitation formulas and allegiance storage."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

TALK=(109,116)


def social_profiles(o,out,d):
    unit,actor=0x801908cc,0x801d4000;u=unit&0x1fffff;a=actor&0x1fffff;rows=[]
    for sid in(9,34):
        for home in(0x08,0x10,0x84):
            for camp in(0x08,0x10,0x84):
                o.uc.mem_write(u,bytes(448));o.uc.mem_write(a,bytes(448))
                o.uc.mem_write(u+1,b'\0');o.uc.mem_write(u+5,bytes((home,)));o.uc.mem_write(u+0x1ba,bytes((home,)))
                o.uc.mem_write(a+5,bytes((camp,)));o.write(0x80192d94,actor)
                o.write(0x80192d98,unit);o.write(0x80192d90,unit+0x18c);o.write(0x8018f5fc,0)
                o.uc.mem_write(u+0x1a7,mask(sid));o.call('store',0,0);o.call('control',unit)
                flags=o.uc.mem_read(u+5,1)[0];initial=o.uc.mem_read(u+0x1ba,1)[0];ct=o.uc.mem_read(u+0x67,1)[0]
                current=bytes(o.uc.mem_read(u+0x58,5))
                o.uc.mem_write(u+0x1bb,bytes(5));o.call('main_remove',unit);o.call('control',unit)
                after=o.uc.mem_read(u+5,1)[0]
                rows.append([sid,home,camp,flags,initial,after,ct,*current])
    # Source repeated Charm resets its timer while keeping initial allegiance.
    o.uc.mem_write(u,bytes(448));o.uc.mem_write(a,bytes(448));o.uc.mem_write(u+5,b'\x10');o.uc.mem_write(u+0x1ba,b'\x10')
    o.uc.mem_write(a+5,b'\x08');o.write(0x80192d94,actor);o.write(0x80192d98,unit);o.write(0x80192d90,unit+0x18c)
    o.uc.mem_write(u+0x1a7,mask(34));o.call('store',0,0);o.call('control',unit)
    o.uc.mem_write(u+0x67,b'\x07');o.uc.mem_write(a+5,b'\x10');o.uc.mem_write(u+0x18c,bytes(44));o.uc.mem_write(u+0x1a7,mask(34))
    o.call('store',0,0);o.call('control',unit)
    repeat=[o.uc.mem_read(u+5,1)[0],o.uc.mem_read(u+0x1ba,1)[0],o.uc.mem_read(u+0x67,1)[0]]
    (out/'social.json').write_text(json.dumps(dict(cases=rows,repeated_charm=repeat,functions=o.function_hashes,
        columns='sid,initial flags,actor current flags,after infliction flags,new initial flags,after removal flags,charm timer,current[5]',
        scope='Unchanged original store and control restoration; graphical handlers isolated. Native blue converts use TEAM_PLAYER plus STATUS_NO_CONTROL; retention of original manual control on previously player-controlled recipients is outside the native model.'),indent=2)+'\n')
    with(out/'source.h').open('a')as f:
        f.write('static const u8 source_allegiance_teams[][12]={\n')
        for row in rows:f.write(' {'+','.join(map(str,row))+'},\n')
        f.write('};\nstatic const u8 source_allegiance_repeat[]={'+','.join(map(str,repeat))+'};\n')


def ownership_profiles(o,out,d):
    o.function_hashes['outcome']=bind_function('outcome','battle_action_check_battle_outcome','battle',d)
    o.function_hashes['item']=bind_function('item','battle_action_decrement_player_item_quantity','battle',d)
    base=0x1908cc;rows=[];stock=[]
    for initial in(0x10,0):
        o.uc.mem_write(base,bytes(21*448))
        for i in range(21):o.uc.mem_write(base+448*i+1,b'\xff')
        for i,current,home in((0,8,8),(6,0,initial)):
            at=base+448*i;o.uc.mem_write(at+1,bytes((i,)));o.uc.mem_write(at+5,bytes((current,)))
            o.uc.mem_write(at+0x1ba,bytes((home,)));o.uc.mem_write(at+0x28,struct.pack('<HH',300,300))
        o.uc.mem_write(base+6*448+0x58,mask(34))
        rows.append([initial,o.call('outcome')])
    for home,current in((8,16),(16,0),(132,16),(132,0)):
        o.uc.mem_write(base,bytes(448));o.uc.mem_write(base+5,bytes((current,)));o.uc.mem_write(base+0x1ba,bytes((home,)))
        o.uc.mem_write(0x596e0+240,b'\x03');o.write(0x8018f5fc,0)
        result=o.call('item',0x801908cc,240,1);left=o.uc.mem_read(0x596e0+240,1)[0]
        stock.append([home,current,result,left])
    (out/'ownership.json').write_text(json.dumps(dict(outcomes=rows,stock=stock,functions=o.function_hashes,
        scope='Unchanged original ordinary outcome and party-item decrement, current versus permanent team. Importance/script objectives remain separate.'),indent=2)+'\n')
    with(out/'source.h').open('a')as f:
        f.write('static const u8 source_allegiance_outcome[][2]={'+','.join('{'+','.join(map(str,r))+'}'for r in rows)+'};\n')
        f.write('static const u8 source_allegiance_stock[][4]={'+','.join('{'+','.join(map(str,r))+'}'for r in stock)+'};\n')


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-allegiance/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for k,n in dict(charm='battle_formula_opposite_sex_hit_ma_x_percent',talk='battle_formula_talk_skill_hit_ma_x_percent',upkeep='battle_status_build_upkeep_action',finish='battle_action_finalize_target_current_action',
            sort='battle_target_sort_list',remove='battle_status_remove',random='battle_formula_get_random_0_7fff').items():
        bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    o.function_hashes['control']=bind_function('control','battle_status_remove_control','battle',d)
    o.function_hashes['store']=bind_function('store','battle_status_store_for_current_attack','battle',d)
    o.function_hashes['handlers']=bind_function('handlers','battle_status_enable_special_flags','battle',d)
    o.function_hashes['main_remove']=bind_function('main_remove','main_status_store_current','main',d)
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['lib_rand']=bind_function('lib_rand','rand','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    rng=1;draws=0
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
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in('roll','random','sort','lib_rand','handlers'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    # ability, ma, actor sign, target sign, target female, Magic AttackUP, target mode, seed
    # target modes: 0 plain, 1 Sleep, 2 Berserk already, 3 Berserk immune, 4 Sleep immune
    profiles=[]
    for ab in TALK:
        profiles += [(ab,ma,az,tz,1 if ab==109 else 0,up,mode,1)for ma,az,tz,up in
            ((5,0,0,0),(14,0,6,0),(14,0,3,1),(31,4,4,0),(60,1,7,0),(99,0,0,1))for mode in range(11)]
        profiles += [(ab,20,0,0,0,0,0,seed)for seed in range(2,14)]
        profiles += [(ab,40,2,8,female,0,mode,seed)for female in(0,1)for mode in(0,1,3,4)for seed in(1,2,9,24,333)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,ma,az,tz,female,up,mode,seed=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        status=d.at(s.battle.STATUS_SET+6*r[11],6)
        outputs=[]
        for execute in(0,1):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            for ptr,sign,sex in((actor,az,0),(target,tz,female)):
                o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((64 if sex else 128,)))
                o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((70,)))
            if mode>=9:
                for ptr in(actor,target):o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((0 if mode==9 else 100,)))
            o.uc.mem_write((actor&0x1fffff)+0x37,bytes((ma,)));o.uc.mem_write((actor&0x1fffff)+0x90,bytes((4 if up else 0,)))
            o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HH',300,300))
            o.uc.mem_write((target&0x1fffff)+0x1a,bytes((255,)*7))
            current=bytearray(5)
            if mode==1:current[4]|=0x10
            if mode==2:current[4]|=0x20
            if mode==5:current[1]|=0x40
            if mode==6:current[1]|=0x80
            if mode==7:current[0]|=0x20
            if mode==8:current[3]|=1
            o.uc.mem_write((target&0x1fffff)+0x58,bytes(current));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(current))
            if mode==3:o.uc.mem_write((target&0x1fffff)+0x53+4,b'\x20')
            if mode==4:o.uc.mem_write((target&0x1fffff)+0x53+1,b'\x40')
            o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x46,status)
            o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws=0
            o.call('remove');o.call('charm'if ab==109 else'talk');o.call('finish')
            v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44))
            outputs += [v[0],struct.unpack_from('<h',v,42)[0]&0xffff,v[19],v[22],v[23],v[37],*v[27:32],draws]
        rows.append([*p,*outputs])
    # Only compared fields travel in the native header; every full observation
    # remains in source.json. Seed333 is encoded as0 to keep byte records.
    native=[]
    for row in rows:
        assert not any(row[i] for i in (10,11,12,22,23,24))
        packed=next(((i<<8)|row[26+i] for i in range(5)if row[26+i]),0)
        native.append([*row[:7],0 if row[7]==333 else row[7],row[8],row[9],row[20],row[31],packed>>8,packed&255])
    lines=['/* Original allegiance formula/finalizer profiles. */',
           'static const u8 source_allegiance[][%d]={'%len(native[0])]
    lines+=[' {'+','.join(map(str,row))+'},'for row in native];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        columns='ability,ma,actor sign,target sign,target female,Magic AttackUP,target mode,seed; '
                'then forecast and execute: hit,accuracy,ct change,brave change,faith change,attack type,infliction[5],draws',
        scope='Unchanged original two formulas and target finalizer. Controlled RNG; human targets, no Finger Guard.'),indent=2)+'\n')
    social_profiles(o,out,d)
    ownership_profiles(o,out,d)
    with(out/'source.h').open('a')as f:
        f.write('static const u8 source_allegiance_immunity[]={'+','.join(map(str,__import__('common_tables').allegiance_immunity(d)))+'};\n')
    print('Original allegiance profiles:',len(rows))

if __name__=='__main__':main()
