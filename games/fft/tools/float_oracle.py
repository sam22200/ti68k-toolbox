#!/usr/bin/env python3
"""Original Float formula/finalizer and actual status-driven movement configuration."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

SPELLS=(38,)


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-float/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(hostile='battle_formula_hit_faith_ma_x_percent',friendly='battle_formula_friendly_magic_accuracy',
        finish='battle_action_finalize_target_current_action',sort='battle_target_sort_list',
        remove='battle_status_remove',random='battle_formula_get_random_0_7fff',protect='battle_formula_apply_target_xa_affecting_statuses')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    o.function_hashes['roll']=bind_function('roll','main_util_roll_pass_fail','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    rng=1;draws=0
    def rand():
        nonlocal rng,draws
        rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1
        return rng>>17
    def external(uc,address,size,data):
        value=0
        if address==s.FUNCTIONS['random'][0]:value=rand()
        elif address==s.FUNCTIONS['roll'][0]:
            bound=uc.reg_read(o.reg.UC_MIPS_REG_A0);chance=uc.reg_read(o.reg.UC_MIPS_REG_A1)
            value=int((rand()*bound>>15)>=chance)
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in('roll','random','sort'):
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    # ab,MA,actorFaith,targetFaith,AttackUp,DefenseUp,Shell,actorSign,targetSign,female,mode,gear
    profiles=[(ab,5,70,70,0,0,0,0,0,0,0,0)for ab in SPELLS]
    for ab in SPELLS:
        profiles += [(ab,ma,af,tf,up,down,shell,az,tz,female,mode,gear)for ma,af,tf,up,down,shell,az,tz,female,mode,gear in
            ((0,100,100,0,0,0,0,0,0,0,0),(99,100,100,1,1,1,0,0,0,0,0),
             (5,0,70,0,0,0,0,0,0,0,0),(5,70,70,1,1,1,0,3,0,0,0),
             (5,70,70,0,0,0,0,6,1,0,0),(5,70,70,0,0,0,12,0,0,0,0),
             (5,70,70,0,0,0,0,0,0,1,0),(5,70,70,0,0,0,0,0,0,2,0),
             (5,70,70,0,0,0,0,0,0,3,0),(5,70,70,0,0,0,0,0,0,0,1))]
    # Chance clamp precedes evasion; also observe failed nonzero accuracy rolls.
    profiles += [(38,99,100,100,0,0,0,0,0,0,0,1),
                 (38,5,100,100,0,0,0,0,0,0,0,1),
                 (38,5,100,100,0,0,0,0,0,0,0,0),
                 (38,5,10,10,0,0,0,0,0,0,0,0),
                 (38,5,10,10,0,0,0,0,0,0,0,0)]
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,ma,af,tf,up,down,shell,az,tz,female,mode,gear=p;r=d.at(s.battle.ABILITY_RANGE+14*ab,14)
        outputs=[]
        for execute in(0,1):
            for ptr in(actor,target):o.uc.mem_write(ptr&0x1fffff,bytes(448))
            o.uc.mem_write(ability&0x1fffff,bytes(80));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            for ptr,faith,sign,sex in((actor,af,az,0),(target,tf,tz,female)):
                o.uc.mem_write((ptr&0x1fffff)+1,b'\0');o.uc.mem_write((ptr&0x1fffff)+6,bytes((64 if sex else 128,)))
                o.uc.mem_write((ptr&0x1fffff)+8,struct.pack('<H',sign<<12));o.uc.mem_write((ptr&0x1fffff)+0x26,bytes((faith,)))
            o.uc.mem_write((actor&0x1fffff)+0x37,bytes((ma,)));o.uc.mem_write((actor&0x1fffff)+0x90,bytes((4 if up else 0,)))
            o.uc.mem_write((target&0x1fffff)+0x90,bytes((2 if down else 0,)));o.uc.mem_write((target&0x1fffff)+0x28,struct.pack('<HH',500,999))
            o.uc.mem_write((target&0x1fffff)+0x1a,bytes((255,)*7))
            if gear:o.uc.mem_write((target&0x1fffff)+0x1c,b'\xe8')
            status=d.at(s.battle.STATUS_SET+6*r[11],6)
            current=bytearray(5)
            if shell:current[3]|=16
            if mode in(2,3):current=bytearray(a|b for a,b in zip(current,status[1:]))
            o.uc.mem_write((target&0x1fffff)+0x58,bytes(current));o.uc.mem_write((target&0x1fffff)+0x1bb,bytes(current))
            if mode==1:o.uc.mem_write((target&0x1fffff)+0x53,status[1:])
            if mode==3:o.uc.mem_write((target&0x1fffff)+0x4e,status[1:])
            o.uc.mem_write((ability&0x1fffff)+0x30,r);o.uc.mem_write((ability&0x1fffff)+0x46,status)
            o.uc.mem_write((ability&0x1fffff)+0x12,bytes((tf,af)));o.uc.mem_write((ability&0x1fffff)+0x16,struct.pack('<H',ab))
            o.uc.mem_write((target&0x1fffff)+0x18c,b'\1');o.uc.mem_write((target&0x1fffff)+0x1b6,struct.pack('<H',100))
            o.write(0x8018f5fc,0 if execute else 2);rng=1;draws=0
            o.call('remove');o.call('friendly'if r[8]==11 else'hostile');o.call('finish')
            v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));packed=0
            if v[0]:
                for b in range(5):
                    if v[27+b]:packed=(b<<8)|v[27+b];break
            outputs += [v[0],struct.unpack_from('<H',v,42)[0],packed,draws]
        rows.append([*p,*outputs])
    # No fake movement implementation: execute the original config producer and
    # ordinary adjacent-panel propagation, with absent other actors.
    for key,name in dict(movement='battle_move_store_unit_movement_to_scratchpad',
            height='battle_unit_get_effective_height',flags='battle_move_set_tile_flags_for_pathfinding',
            one='battle_move_check_and_spread_one_tile_around').items():
        o.function_hashes[key]=bind_function(key,name,'battle',d)
    mover=o;mover.instruction_limit=1000000
    mover.write(0x8018f4f0,0x801d6000);mover.write(0x8018f4e8,0x801d1000)
    mover.write(0x800e4e9c,3);mover.write(0x800e4ea0,1)
    for j in range(21):
        mover.uc.mem_write(0x1908cc+448*j,bytes(448));mover.uc.mem_write(0x1908cd+448*j,b'\xff')
    mover.uc.mem_write(0x1908cd,b'\0');mover.uc.mem_write(0x1908cf,b'\x4a');mover.uc.mem_write(0x1908d2,b'\x80')
    movement=[];configs=[]
    for floating in(0,1):
        for wet_from,wet_to in((0,0),(0,1),(1,0),(1,1)):
            for move in(1,2,3):
                mover.uc.mem_write(0x1d3000,bytes(128));mover.write(0x801d3000,s.TILES);mover.write(0x801d3008,0x80192dd8)
                mover.uc.mem_write(0x190906,bytes((move,3)));mover.uc.mem_write(0x190926,bytes((floating*64,)))
                for j in range(512):mover.uc.mem_write((s.TILES&0x1fffff)+8*j,bytes((0,0,2,0,0,0,1,0)))
                for j,wet in enumerate((wet_from,wet_to,0)):
                    mover.uc.mem_write((s.TILES&0x1fffff)+8*j,bytes((17 if wet else 0,0,2,wet<<5,0,0,0,0)))
                mover.call('movement',0)
                config=bytes(mover.uc.mem_read(0x1d0000,40));cost=bytes(mover.uc.mem_read(0x1d5000,64))
                if not wet_from and not wet_to and move==1:
                    configs.append(dict(float=floating,config=list(config),terrain_costs=list(cost),height=mover.call('height',actor)))
                mover.call('flags',1);mover.call('one')
                left=mover.uc.mem_read(0x192dd8+5,1)[0]
                movement.append([floating,wet_from,wet_to,move,move+1-left if left else 255])
    o.function_hashes.update(mover.function_hashes)
    lines=['/* Generated original formula0a/0b; controlled xorshift draws, local only. */','static const u16 source_float[][20]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines+=['};','static const u8 source_float_movement[][5]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in movement];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,movement=movement,configurations=configs,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        scope='Unchanged Float formula, status filtering/finalizer with controlled RNG and isolated target sort. Original human movement configuration, ordinary adjacent propagation and effective-height helper; not full campaign/render/AI or equipped movement abilities.'),indent=2)+'\n')
    print('Original Float:',len(rows),'profiles,',len(movement),'adjacent moves,',len(configs),'configurations')


if __name__=='__main__':main()
