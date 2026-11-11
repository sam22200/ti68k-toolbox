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
    for k,n in names.items():bind_function(k,n,'battle',d)
    table=lambda f:struct.unpack_from('<I',d.bat,0x8018f610-s.battle.BAT+4*f)[0]
    assert table(0x50)==s.FUNCTIONS['formula'][0] and table(0x3D)==s.FUNCTIONS['magical'][0]   # retail dispatch entries
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
    lines=['/* Retail monster status strikes, formula 0x50 with evasion. No source code modification. */',
        '#define STRIKE_JOB %d'%JOB,'#define STRIKE_ACC %d'%ACC,'#define STRIKE_SHIELD %d'%SHIELD,
        'static const struct {u16 seed,ab;u8 ma,az,tz,up,defense,gear,forecast,hit,draws,add[5];} source_strike[]={']
    for row in rows:
        p=row['input'];pre=row['preview'];exe=row['execute']
        lines.append(' {%d,%d,%s,%d,%d,%d,{%s}},'%(p[-1],p[0],','.join(map(str,p[1:-1])),
            pre['chance']if pre['hit']else 0,exe['hit'],len(exe['draws']),','.join(map(str,exe['add']))))
    lines+=['};','static const u8 source_magical_status[][5]={']
    lines+=[' {'+','.join(map(str,mask(*ids)))+'},'for ids in MODES]
    lines+=['};','static const struct {u16 seed,ab;u8 ma,az,tz,up,defense,mode,af,tf,forecast,hit,draws,add[5];} source_magical[]={']
    for row in mrows:
        p=row['input'];pre=row['preview'];exe=row['execute']
        lines.append(' {%d,%d,%s,%d,%d,%d,{%s}},'%(p[-1],p[0],','.join(map(str,p[1:-1])),
            pre['chance']if pre['hit']else 0,exe['hit'],len(exe['draws']),','.join(map(str,exe['add']))))
    lines+=['};'];(out/'source.h').write_text('\n'.join(lines)+'\n')
    (out/'source.json').write_text(json.dumps(dict(cases=rows,magical_cases=mrows,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        evade=dict(job=JOB,cev=cev,accessory=aev,shield=sev),
        scope='Original unmodified formula 0x50 with flags_4 0x12 records: physical evade stages '
              'with their draws, then the accuracy roll and the status request. Controlled RNG.'),indent=2)+'\n')
    print('status strike oracle: %d profiles, hits %d, draws %s, miss types %s, evade %s'%(len(rows),
        sum(r['execute']['hit'] for r in rows),sorted({len(r['execute']['draws']) for r in rows}),
        sorted({r['execute']['miss'] for r in rows}),(cev,aev,sev)),flush=True)
    print('0x3D oracle: %d profiles, hits %d, draws %s, adds %s, previews %s'%(len(mrows),
        sum(r['execute']['hit'] for r in mrows),sorted({len(r['execute']['draws']) for r in mrows}),
        sorted({(r['input'][0],tuple(r['execute']['add'])) for r in mrows if r['execute']['hit']}),
        sorted({r['preview']['chance'] for r in mrows})[:12]),flush=True)

if __name__=='__main__':main()
