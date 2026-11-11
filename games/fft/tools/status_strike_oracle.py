#!/usr/bin/env python3
"""Original formula 0x50 for the monsters' evadeable status strikes (Eye Gouge, Poison Nail,
Beak; flags_4 0x12): physical evasion, then the (MA + X)% accuracy and the record's status
(campaign/MONSTER_STATUS_STRIKES.md). The Secret Fist oracle covers the accuracy stages."""
import json,struct
import surface_data as s
from common_tables import SHIELD_EVADE,ACCESSORY_EVADE
from status_state_oracle import bind_function,mask

SKILLS=(0x111,0x11B,0x132)
JOB=0x4A          # the check gives its target this job: the same class evade on both sides
ACC,SHIELD=232,128
# No attacker Frog/Berserk cases: neither can select this immediate command.
STATUS=((),(26,),(35,),(4,),(21,),(22,),(27,),(32,),(33,),(39,),(),(),(3,),
        (26,35,21),(26,4,22),(35,4),(24,))


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-status-strike/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(formula='battle_formula_hit_ma_x_percent_physical_evade',
        finish='battle_action_finalize_target_current_action',remove='battle_status_remove',
        sort='battle_target_sort_list',random='battle_formula_get_random_0_7fff')
    names['magical']='battle_formula_hit_ma_x_percent_magical_evade'
    names['apply']='battle_formula_apply_status_to_action'
    names['beaking']='battle_formula_lower_pa_y'
    names['goblin']='battle_formula_damage_caster_missing_hp'
    names['destruct']='battle_formula_damage_caster_missing_hp_self_sacrifice'
    for k,n in names.items():bind_function(k,n,'battle',d)
    table=lambda f:struct.unpack_from('<I',d.bat,0x8018f610-s.battle.BAT+4*f)[0]
    assert table(0x50)==s.FUNCTIONS['formula'][0] and table(0x3D)==s.FUNCTIONS['magical'][0] and table(0x38)==s.FUNCTIONS['apply'][0] and table(0x55)==s.FUNCTIONS['beaking'][0]
    assert table(0x4F)==s.FUNCTIONS['goblin'][0] and table(0x52)==s.FUNCTIONS['destruct'][0]   # retail dispatch entries
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    for k,n in dict(roll='main_util_roll_pass_fail',lib_rand='rand',init='main_status_init_check_data').items():
        o.function_hashes[k]=bind_function(k,n,'main',d)
    o.call('init');rng=1;draws=[]
    def rand():
        nonlocal rng
        rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff
        return rng>>17
    def external(uc,address,size,data):
        value=0
        if address in(s.FUNCTIONS['random'][0],s.FUNCTIONS['lib_rand'][0]):
            value=rand();draws.append(['random',value])
        elif address==s.FUNCTIONS['roll'][0]:
            bound=uc.reg_read(o.reg.UC_MIPS_REG_A0);chance=uc.reg_read(o.reg.UC_MIPS_REG_A1)
            r=rand();value=int((r*bound>>15)>=chance);draws.append([bound,chance,r])
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in('roll','random','sort','lib_rand'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    # ability, MA, zodiac pair, Attack UP, target Defense UP, gear (0 no evade source, 1 class
    # evade, 2 class + accessory + shield; the attacker in front), seed
    profiles=[]
    for ab in SKILLS:
        for ma,az,tz,up,defense in((6,0,0,0,0),(14,0,6,1,1),(31,4,4,0,0),(60,0,0,0,0)):
            profiles+=[(ab,ma,az,tz,up,defense,gear,seed)for gear in range(3)for seed in(1,2,73,255,4099,10000,32767,65535)]
    cev=d.job(JOB)['cev'];sev=d.at(SHIELD_EVADE+2*(SHIELD-0x80),1)[0]
    aev=d.at(ACCESSORY_EVADE+2*d.at(s.battle.ITEMS+12*ACC+4,1)[0],1)[0]
    assert cev and sev and aev,(cev,sev,aev)
    actor,target,ability=0x801908cc,0x801d4000,0x801938c0;rows=[]
    for p in profiles:
        ab,ma,az,tz,up,defense,gear,seed=p;outputs=[]
        raw=d.at(s.battle.ABILITY_RANGE+14*ab,14);request=d.at(s.battle.STATUS_SET+6*raw[11],6)
        assert raw[8]==0x50 and raw[6]&2 and request[0]==0x80
        for execute in(0,1):
            for ptr,sign in((actor,az),(target,tz)):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448))
                o.uc.mem_write(at+6,bytes((128,)))
                o.uc.mem_write(at+8,struct.pack('<H',sign<<12));o.uc.mem_write(at+0x26,bytes((70,)))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999))
                o.uc.mem_write(at+0x1a,bytes((255,)*7))
            a=actor&0x1fffff;t=target&0x1fffff;v=ability&0x1fffff
            o.uc.mem_write(a+0x37,bytes((ma,)))
            o.uc.mem_write(a+0x90,bytes((16 if up else 0,)))
            o.uc.mem_write(t+0x90,bytes((8 if defense else 0,)))
            if gear:
                o.uc.mem_write(t+0x43,bytes((cev,)))           # equipment_stats[7]: class physical evade
                o.uc.mem_write(t+0x47,b'\x01')                # attacker x0 -> target x1, facing west (front)
                o.uc.mem_write(t+0x48,struct.pack('<H',0x0100))
            if gear==2:
                o.uc.mem_write(t+0x1c,bytes((ACC,255,255,255,SHIELD)))
                o.uc.mem_write(t+0x40,bytes((aev,0,sev)))      # accessory, right shield, left shield evade
            o.uc.mem_write(v,bytes(80));o.uc.mem_write(v+0x30,raw);o.uc.mem_write(v+0x46,request)
            o.uc.mem_write(v+0x12,bytes((70,70)));o.uc.mem_write(v+0x16,struct.pack('<H',ab))
            o.uc.mem_write(v+0x18,b'\0');o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws.clear()
            o.call('remove');o.call('formula');o.call('finish')
            action=bytes(o.uc.mem_read(t+0x18c,44))
            outputs.append(dict(hit=action[0],chance=struct.unpack_from('<H',action,42)[0],
                add=list(action[27:32]),draws=list(draws),miss=action[2]))
        rows.append(dict(input=p,preview=outputs[0],execute=outputs[1]))
    # Formula 0x3D (Mind Blast: one random status of Confusion/Berserk on radius 1; the monsters'
    # Death Sentence): (MA + X)% without Faith, never evaded (flags_4 0). ability, MA, zodiac
    # pair, Magic Attack UP, target Magic Defense UP, target status mode, Faith pair, seed.
    MODES=((),(20,),(11,),(27,),(39,),(11,20))   # none, Berserk, Confusion, Shell, Death Sentence, both
    magical=[]
    for ab in(0x120,0x130):
        for ma,az,tz,up,defense in((6,0,0,0,0),(14,0,6,1,1),(31,4,4,0,0),(70,0,0,0,0)):
            magical+=[(ab,ma,az,tz,up,defense,mode,70,70,seed)for mode in range(len(MODES))for seed in(1,73,4099,65535)]
        magical+=[(ab,14,0,0,0,0,0,af,tf,seed)for af,tf in((0,0),(100,100),(3,97))for seed in range(2,12)]
    mrows=[]
    for p in magical:
        ab,ma,az,tz,up,defense,mode,af,tf,seed=p;outputs=[]
        raw=d.at(s.battle.ABILITY_RANGE+14*ab,14);request=d.at(s.battle.STATUS_SET+6*raw[11],6)
        assert raw[8]==0x3D and not raw[6]&2 and request[0]==(0x40 if ab==0x120 else 0x80)
        for execute in(0,1):
            for ptr,sign,faith in((actor,az,af),(target,tz,tf)):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448))
                o.uc.mem_write(at+6,bytes((128,)))
                o.uc.mem_write(at+8,struct.pack('<H',sign<<12));o.uc.mem_write(at+0x26,bytes((faith,)))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999))
                o.uc.mem_write(at+0x1a,bytes((255,)*7))
            a=actor&0x1fffff;t=target&0x1fffff;v=ability&0x1fffff
            o.uc.mem_write(a+0x37,bytes((ma,)))
            o.uc.mem_write(a+0x90,bytes((4 if up else 0,)))
            o.uc.mem_write(t+0x90,bytes((2 if defense else 0,)))
            current=mask(*MODES[mode])
            o.uc.mem_write(t+0x4e,bytes(10)+current);o.uc.mem_write(t+0x1bb,current)
            o.uc.mem_write(v,bytes(80));o.uc.mem_write(v+0x30,raw);o.uc.mem_write(v+0x46,request)
            o.uc.mem_write(v+0x12,bytes((tf,af)));o.uc.mem_write(v+0x16,struct.pack('<H',ab))
            o.uc.mem_write(v+0x1c,bytes((100,)));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws.clear()
            o.call('remove');o.call('magical');o.call('finish')
            action=bytes(o.uc.mem_read(t+0x18c,44))
            outputs.append(dict(hit=action[0],chance=struct.unpack_from('<H',action,42)[0],
                add=list(action[27:32]),draws=list(draws),miss=action[2]))
        mrows.append(dict(input=p,preview=outputs[0],execute=outputs[1]))
    # Odd Soundwave (0x11F, formula 0x38: the status application alone, a cancel set): no
    # evade, no accuracy, no draw; a miss when nothing is removed. Current statuses, innate ones.
    WAVE=((),(26,),(27,28),(17,18,19),(25,26,27,28,32,38),(24,10),(26,24),(3,),(26,))
    INNATE=((),(),(),(),(),(),(),(),(26,))
    wrows=[];raw=d.at(s.battle.ABILITY_RANGE+14*0x11F,14);request=d.at(s.battle.STATUS_SET+6*raw[11],6)
    assert raw[8]==0x38 and request==bytes((0x10,0,0,0x70,0x78,0x82))
    for mode in range(len(WAVE)):
        for seed in(1,4099):
            outputs=[]
            for execute in(0,1):
                for ptr in(actor,target):
                    at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+6,bytes((128,)))
                    o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999));o.uc.mem_write(at+0x1a,bytes((255,)*7))
                t=target&0x1fffff;v=ability&0x1fffff
                current=mask(*WAVE[mode]);innate=mask(*INNATE[mode])
                o.uc.mem_write(t+0x4e,innate+bytes(5)+current);o.uc.mem_write(t+0x1bb,current)
                o.uc.mem_write(v,bytes(80));o.uc.mem_write(v+0x30,raw);o.uc.mem_write(v+0x46,request)
                o.uc.mem_write(v+0x16,struct.pack('<H',0x11F));o.uc.mem_write(v+0x1c,bytes((100,)));o.uc.mem_write(0x193860,bytes(5))
                o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
                action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
                o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws.clear()
                o.call('remove');o.call('apply');o.call('finish')
                action=bytes(o.uc.mem_read(t+0x18c,44))
                outputs.append(dict(hit=action[0],remove=list(action[32:37]),add=list(action[27:32]),draws=len(draws)))
            assert outputs[0]['hit']==outputs[1]['hit'] and outputs[1]['draws']==0 and not any(outputs[1]['add'])
            wrows.append(dict(mode=mode,seed=seed,preview=outputs[0],execute=outputs[1]))
    # Beaking (0x135, formula 0x55): magical evasion (flags_4 0x12), (MA + X)% without Faith,
    # PA - Y. MA, zodiac pair, Magic Attack UP, target Magic Defense UP, gear (0 none, 1 the
    # accessory's and shield's magical evade), Faith pair, seed.
    msev=d.at(SHIELD_EVADE+2*(SHIELD-0x80)+1,1)[0];maev=d.at(ACCESSORY_EVADE+2*d.at(s.battle.ITEMS+12*ACC+4,1)[0]+1,1)[0]
    beak=[]
    for ma,az,tz,up,defense in((6,0,0,0,0),(14,0,6,1,1),(31,4,4,0,0),(70,0,0,0,0)):
        beak+=[(ma,az,tz,up,defense,gear,70,70,seed)for gear in(0,1)for seed in(1,2,73,255,4099,10000,32767,65535)]
    beak+=[(14,0,0,0,0,0,af,tf,seed)for af,tf in((0,0),(100,100))for seed in(3,77)]
    brows=[];raw=d.at(s.battle.ABILITY_RANGE+14*0x135,14)
    assert raw[8]==0x55 and raw[6]&2 and raw[10]==2
    for p in beak:
        ma,az,tz,up,defense,gear,af,tf,seed=p;outputs=[]
        for execute in(0,1):
            for ptr,sign,faith in((actor,az,af),(target,tz,tf)):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+6,bytes((128,)))
                o.uc.mem_write(at+8,struct.pack('<H',sign<<12));o.uc.mem_write(at+0x26,bytes((faith,)))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999));o.uc.mem_write(at+0x1a,bytes((255,)*7))
            a=actor&0x1fffff;t=target&0x1fffff;v=ability&0x1fffff
            o.uc.mem_write(a+0x37,bytes((ma,)));o.uc.mem_write(a+0x90,bytes((4 if up else 0,)))
            o.uc.mem_write(t+0x90,bytes((2 if defense else 0,)))
            if gear:
                o.uc.mem_write(t+0x1c,bytes((ACC,255,255,255,SHIELD)))
                o.uc.mem_write(t+0x44,bytes((maev,0,msev)))    # accessory, right shield, left shield magical evade
            o.uc.mem_write(v,bytes(80));o.uc.mem_write(v+0x30,raw)
            o.uc.mem_write(v+0x12,bytes((tf,af)));o.uc.mem_write(v+0x16,struct.pack('<H',0x135))
            o.uc.mem_write(v+0x1c,bytes((100,)));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws.clear()
            o.call('remove');o.call('beaking');o.call('finish')
            action=bytes(o.uc.mem_read(t+0x18c,44))
            outputs.append(dict(hit=action[0],chance=struct.unpack_from('<H',action,42)[0],pa=action[0x14],
                draws=list(draws),miss=action[2]))
        brows.append(dict(input=p,preview=outputs[0],execute=outputs[1]))
    assert maev and msev,(maev,msev)
    # Goblin Punch (0x10F, formula 0x4F): the 0x50 physical evade and (MA + X)% accuracy, then
    # damage = the caster's missing HP. MA, zodiac pair, Attack UP, Defense UP, gear, caster HP,
    # caster max HP, seed.
    goblin=[]
    for ma,az,tz,up,defense in((6,0,0,0,0),(14,0,6,1,1),(70,0,0,0,0)):
        goblin+=[(ma,az,tz,up,defense,gear,hp,mhp,seed)for gear in range(3)for hp,mhp in((1,250),(200,200),(37,120))for seed in(1,73,4099,65535)]
    grows=[];raw=d.at(s.battle.ABILITY_RANGE+14*0x10F,14)
    assert raw[8]==0x4F and raw[6]&2
    for p in goblin:
        ma,az,tz,up,defense,gear,hp,mhp,seed=p;outputs=[]
        for execute in(0,1):
            for ptr,sign in((actor,az),(target,tz)):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+6,bytes((128,)))
                o.uc.mem_write(at+8,struct.pack('<H',sign<<12));o.uc.mem_write(at+0x26,bytes((70,)))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999));o.uc.mem_write(at+0x1a,bytes((255,)*7))
            a=actor&0x1fffff;t=target&0x1fffff;v=ability&0x1fffff
            o.uc.mem_write(a+0x28,struct.pack('<2H',hp,mhp))
            o.uc.mem_write(a+0x37,bytes((ma,)));o.uc.mem_write(a+0x90,bytes((16 if up else 0,)))
            o.uc.mem_write(t+0x90,bytes((8 if defense else 0,)))
            if gear:
                o.uc.mem_write(t+0x43,bytes((cev,)));o.uc.mem_write(t+0x47,b'\x01');o.uc.mem_write(t+0x48,struct.pack('<H',0x0100))
            if gear==2:
                o.uc.mem_write(t+0x1c,bytes((ACC,255,255,255,SHIELD)));o.uc.mem_write(t+0x40,bytes((aev,0,sev)))
            o.uc.mem_write(v,bytes(80));o.uc.mem_write(v+0x30,raw)
            o.uc.mem_write(v+0x12,bytes((70,70)));o.uc.mem_write(v+0x16,struct.pack('<H',0x10F));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws.clear()
            o.call('remove');o.call('goblin');o.call('finish')
            action=bytes(o.uc.mem_read(t+0x18c,44))
            outputs.append(dict(hit=action[0],chance=struct.unpack_from('<H',action,42)[0],
                damage=struct.unpack_from('<h',action,4)[0],draws=list(draws)))
        grows.append(dict(input=p,preview=outputs[0],execute=outputs[1]))
    # Self Destruct (0x115, formula 0x52): others take the caster's missing HP and the status
    # (Oil) at 100%; the caster takes its own HP. No evade, no draw. caster HP, max HP, target
    # mode (0 none, 1 already Oil, 2 immune to Oil, 3 the caster itself).
    drows=[];raw=d.at(s.battle.ABILITY_RANGE+14*0x115,14);request=d.at(s.battle.STATUS_SET+6*raw[11],6)
    assert raw[8]==0x52 and not raw[6]&2 and request==bytes((0x80,0,0,0x80,0,0))
    for hp,mhp in((1,250),(200,200),(37,120),(300,999)):
        for mode in range(4):
            for ptr in(actor,target):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+6,bytes((128,)))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999));o.uc.mem_write(at+0x1a,bytes((255,)*7))
            a=actor&0x1fffff;victim=actor if mode==3 else target;t=victim&0x1fffff;v=ability&0x1fffff
            o.uc.mem_write(a+0x28,struct.pack('<2H',hp,mhp))
            oil=mask(16)
            if mode==1:o.uc.mem_write(t+0x4e+10,oil);o.uc.mem_write(t+0x1bb,oil)
            if mode==2:o.uc.mem_write(t+0x4e+5,oil)
            o.uc.mem_write(v,bytes(80));o.uc.mem_write(v+0x30,raw);o.uc.mem_write(v+0x46,request)
            o.uc.mem_write(v+0x16,struct.pack('<H',0x115));o.uc.mem_write(v+0x1c,bytes((100,)));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,victim);o.write(0x80192d90,victim+0x18c)
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0);rng=1;draws.clear()
            o.call('remove');o.call('destruct');o.call('finish')
            action=bytes(o.uc.mem_read(t+0x18c,44))
            assert not draws
            drows.append(dict(hp=hp,max_hp=mhp,mode=mode,hit=action[0],damage=struct.unpack_from('<h',action,4)[0],add=list(action[27:32])))
    # Direct-targeting strikes (flags_4 0x22), formula 0x50: Black Ink (0x11E, Darkness) and
    # Look of Devil (0x12D, one random status of Petrify/Darkness/Silence/Don't Move/Don't
    # Act). ability, MA, zodiac pair, Attack UP, Defense UP, gear, target status mode, seed index.
    DMODES=((),(10,),(12,36),(10,12,36,37))
    seed_of=lambda k:(k*0x9E3779B1&0xffffffff)|1
    direct=[]
    for ab in(0x11E,0x12D):
        for ma,az,tz,up,defense in((6,0,0,0,0),(14,0,6,1,1),(70,4,4,0,0)):
            direct+=[(ab,ma,az,tz,up,defense,gear,mode,seed)for gear in range(3)for mode in range(2 if ab==0x11E else 4)for seed in(3,8,21,40,55,77,90,123)]
    xrows=[]
    for p in direct:
        ab,ma,az,tz,up,defense,gear,mode,seed=p;outputs=[]
        raw=d.at(s.battle.ABILITY_RANGE+14*ab,14);request=d.at(s.battle.STATUS_SET+6*raw[11],6)
        assert raw[8]==0x50 and raw[6]==0x22 and request[0]==(0x80 if ab==0x11E else 0x40)
        for execute in(0,1):
            for ptr,sign in((actor,az),(target,tz)):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+6,bytes((128,)))
                o.uc.mem_write(at+8,struct.pack('<H',sign<<12));o.uc.mem_write(at+0x26,bytes((70,)))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999));o.uc.mem_write(at+0x1a,bytes((255,)*7))
            a=actor&0x1fffff;t=target&0x1fffff;v=ability&0x1fffff
            o.uc.mem_write(a+0x37,bytes((ma,)));o.uc.mem_write(a+0x90,bytes((16 if up else 0,)))
            o.uc.mem_write(t+0x90,bytes((8 if defense else 0,)))
            if gear:
                o.uc.mem_write(t+0x43,bytes((cev,)));o.uc.mem_write(t+0x47,b'\x01');o.uc.mem_write(t+0x48,struct.pack('<H',0x0100))
            if gear==2:
                o.uc.mem_write(t+0x1c,bytes((ACC,255,255,255,SHIELD)));o.uc.mem_write(t+0x40,bytes((aev,0,sev)))
            current=mask(*DMODES[mode])
            o.uc.mem_write(t+0x4e+10,current);o.uc.mem_write(t+0x1bb,current)
            o.uc.mem_write(v,bytes(80));o.uc.mem_write(v+0x30,raw);o.uc.mem_write(v+0x46,request)
            o.uc.mem_write(v+0x12,bytes((70,70)));o.uc.mem_write(v+0x16,struct.pack('<H',ab))
            o.uc.mem_write(v+0x18,b'\0');o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed_of(seed);draws.clear()
            o.call('remove');o.call('formula');o.call('finish')
            action=bytes(o.uc.mem_read(t+0x18c,44))
            outputs.append(dict(hit=action[0],chance=struct.unpack_from('<H',action,42)[0],
                add=list(action[27:32]),draws=list(draws),miss=action[2]))
        xrows.append(dict(input=p,preview=outputs[0],execute=outputs[1]))
    # Look of Fright (0x12E, formula 0x62): magical evasion (flags_4 0x22), (MA + X)% without
    # Faith, Brave - Y. Beaking's profiles.
    bind_function('fright','battle_formula_lower_brave_y_without_faith','battle',d)
    assert table(0x62)==s.FUNCTIONS['fright'][0]
    frows=[];raw=d.at(s.battle.ABILITY_RANGE+14*0x12E,14)
    assert raw[8]==0x62 and raw[6]==0x22 and raw[10]==10
    for p in beak:
        ma,az,tz,up,defense,gear,af,tf,seed=p;outputs=[]
        for execute in(0,1):
            for ptr,sign,faith in((actor,az,af),(target,tz,tf)):
                at=ptr&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+6,bytes((128,)))
                o.uc.mem_write(at+8,struct.pack('<H',sign<<12));o.uc.mem_write(at+0x26,bytes((faith,)))
                o.uc.mem_write(at+0x28,struct.pack('<4H',999,999,999,999));o.uc.mem_write(at+0x1a,bytes((255,)*7))
            a=actor&0x1fffff;t=target&0x1fffff;v=ability&0x1fffff
            o.uc.mem_write(a+0x37,bytes((ma,)));o.uc.mem_write(a+0x90,bytes((4 if up else 0,)))
            o.uc.mem_write(t+0x90,bytes((2 if defense else 0,)));o.uc.mem_write(t+0x24,bytes((70,)))
            if gear:
                o.uc.mem_write(t+0x1c,bytes((ACC,255,255,255,SHIELD)));o.uc.mem_write(t+0x44,bytes((maev,0,msev)))
            o.uc.mem_write(v,bytes(80));o.uc.mem_write(v+0x30,raw)
            o.uc.mem_write(v+0x12,bytes((tf,af)));o.uc.mem_write(v+0x16,struct.pack('<H',0x12E))
            o.uc.mem_write(v+0x1c,bytes((100,)));o.uc.mem_write(0x193860,bytes(5))
            o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
            action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write(t+0x18c,bytes(action))
            o.write(0x8018f5fc,0 if execute else 2);rng=seed;draws.clear()
            o.call('remove');o.call('fright');o.call('finish')
            action=bytes(o.uc.mem_read(t+0x18c,44))
            outputs.append(dict(hit=action[0],chance=struct.unpack_from('<H',action,42)[0],brave=action[0x16],
                draws=list(draws),miss=action[2]))
        frows.append(dict(input=p,preview=outputs[0],execute=outputs[1]))
    lines=['/* Retail monster status strikes, formula 0x50 with evasion. No source code modification. */',
        '#define STRIKE_JOB %d'%JOB,'#define STRIKE_ACC %d'%ACC,'#define STRIKE_SHIELD %d'%SHIELD,
        'static const struct {u16 seed,ab;u8 ma,az,tz,up,defense,gear,forecast,hit,draws,add[5];} source_strike[]={']
    for row in rows:
        p=row['input'];pre=row['preview'];exe=row['execute']
        lines.append(' {%d,%d,%s,%d,%d,%d,{%s}},'%(p[-1],p[0],','.join(map(str,p[1:-1])),
            pre['chance']if pre['hit']else 0,exe['hit'],len(exe['draws']),','.join(map(str,exe['add']))))
    lines+=['};','static const struct {u16 seed,damage;u8 ma,az,tz,up,defense,gear,hp,max_hp,forecast,hit,draws;} source_goblin[]={']
    for row in grows:
        p=row['input'];pre=row['preview'];exe=row['execute']
        lines.append(' {%d,%d,%s,%d,%d,%d},'%(p[-1],exe['damage'],','.join(map(str,p[:-1])),pre['chance']if pre['hit']else 0,exe['hit'],len(exe['draws'])))
    lines+=['};','static const struct {u16 hp,max_hp,damage;u8 mode,hit,add[5];} source_destruct[]={']
    lines+=[' {%d,%d,%d,%d,%d,{%s}},'%(r['hp'],r['max_hp'],r['damage'],r['mode'],r['hit'],','.join(map(str,r['add'])))for r in drows]
    lines+=['};','static const struct {u16 seed;u8 ma,az,tz,up,defense,gear,af,tf,forecast,hit,draws,pa;} source_beaking[]={']
    for row in brows:
        p=row['input'];pre=row['preview'];exe=row['execute']
        lines.append(' {%d,%s,%d,%d,%d,%d},'%(p[-1],','.join(map(str,p[:-1])),pre['chance']if pre['hit']else 0,exe['hit'],len(exe['draws']),exe['pa']))
    lines+=['};','static const struct {u8 current[5],innate[5],hit,remove[5];} source_wave[]={']
    for row in wrows:
        if row['seed']!=1:continue
        lines.append(' {{%s},{%s},%d,{%s}},'%(','.join(map(str,mask(*WAVE[row['mode']]))),','.join(map(str,mask(*INNATE[row['mode']]))),
            row['execute']['hit'],','.join(map(str,row['execute']['remove']))))
    lines+=['};','static const u8 source_magical_status[][5]={']
    lines+=[' {'+','.join(map(str,mask(*ids)))+'},'for ids in MODES]
    lines+=['};','static const struct {u16 seed,ab;u8 ma,az,tz,up,defense,mode,af,tf,forecast,hit,draws,add[5];} source_magical[]={']
    for row in mrows:
        p=row['input'];pre=row['preview'];exe=row['execute']
        lines.append(' {%d,%d,%s,%d,%d,%d,{%s}},'%(p[-1],p[0],','.join(map(str,p[1:-1])),
            pre['chance']if pre['hit']else 0,exe['hit'],len(exe['draws']),','.join(map(str,exe['add']))))
    lines+=['};','static const u8 source_direct_status[][5]={']
    lines+=[' {'+','.join(map(str,mask(*ids)))+'},'for ids in DMODES]
    lines+=['};','static const struct {u32 seed;u16 ab;u8 ma,az,tz,up,defense,gear,mode,forecast,hit,draws,add[5];} source_direct[]={']
    for row in xrows:
        p=row['input'];pre=row['preview'];exe=row['execute']
        lines.append(' {%dUL,%d,%s,%d,%d,%d,{%s}},'%(seed_of(p[-1]),p[0],','.join(map(str,p[1:-1])),
            pre['chance']if pre['hit']else 0,exe['hit'],len(exe['draws']),','.join(map(str,exe['add']))))
    lines+=['};','static const struct {u16 seed;u8 ma,az,tz,up,defense,gear,af,tf,forecast,hit,draws,brave;} source_fright[]={']
    for row in frows:
        p=row['input'];pre=row['preview'];exe=row['execute']
        lines.append(' {%d,%s,%d,%d,%d,%d},'%(p[-1],','.join(map(str,p[:-1])),pre['chance']if pre['hit']else 0,exe['hit'],len(exe['draws']),exe['brave']))
    lines+=['};'];(out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,magical_cases=mrows,wave_cases=wrows,beaking_cases=brows,direct_cases=xrows,fright_cases=frows,goblin_cases=grows,destruct_cases=drows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        evade=dict(job=JOB,cev=cev,accessory=aev,shield=sev),
        scope='Original unmodified formula 0x50 with flags_4 0x12 records: physical evade stages '
              'with their draws, then the accuracy roll and the status request. Controlled RNG.'),indent=2)+'\n')
    print('status strike oracle: %d profiles, hits %d, draws %s, miss types %s, evade %s'%(len(rows),
        sum(r['execute']['hit'] for r in rows),sorted({len(r['execute']['draws']) for r in rows}),
        sorted({r['execute']['miss'] for r in rows}),(cev,aev,sev)),flush=True)
    print('Goblin Punch oracle: %d profiles, hits %d, draws %s, damages %s'%(len(grows),sum(r['execute']['hit'] for r in grows),
        sorted({len(r['execute']['draws']) for r in grows}),sorted({r['execute']['damage'] for r in grows})),flush=True)
    print('Self Destruct oracle:',[(r['hp'],r['max_hp'],r['mode'],r['hit'],r['damage'],r['add'][2]) for r in drows],flush=True)
    print('Beaking oracle: %d profiles, hits %d, draws %s, pa %s, miss %s, magic evade %s'%(len(brows),sum(r['execute']['hit'] for r in brows),
        sorted({len(r['execute']['draws']) for r in brows}),sorted({r['execute']['pa'] for r in brows}),sorted({r['execute']['miss'] for r in brows}),(maev,msev)),flush=True)
    print('direct 0x50 oracle: %d profiles, hits %d, draws %s, adds %s, previews %s'%(len(xrows),sum(r['execute']['hit'] for r in xrows),
        sorted({len(r['execute']['draws']) for r in xrows}),sorted({(r['input'][0],tuple(r['execute']['add'])) for r in xrows if r['execute']['hit']}),
        sorted({(r['input'][0],r['input'][7],r['preview']['hit']) for r in xrows})),flush=True)
    print('Look of Fright oracle: %d profiles, hits %d, draws %s, brave %s, miss %s'%(len(frows),sum(r['execute']['hit'] for r in frows),
        sorted({len(r['execute']['draws']) for r in frows}),sorted({r['execute']['brave'] for r in frows}),sorted({r['execute']['miss'] for r in frows})),flush=True)
    print('Odd Soundwave oracle:',[(r['mode'],r['execute']['hit'],r['execute']['remove']) for r in wrows if r['seed']==1],flush=True)
    print('0x3D oracle: %d profiles, hits %d, draws %s, adds %s, previews %s'%(len(mrows),
        sum(r['execute']['hit'] for r in mrows),sorted({len(r['execute']['draws']) for r in mrows}),
        sorted({(r['input'][0],tuple(r['execute']['add'])) for r in mrows if r['execute']['hit']}),
        sorted({r['preview']['chance'] for r in mrows})[:12]),flush=True)

if __name__=='__main__':main()
