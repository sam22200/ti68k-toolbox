#!/usr/bin/env python3
"""Bounded original area-AI hit accounting, HP-state scores, ranking and tile scan."""
import argparse,json,struct,subprocess
from pathlib import Path
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=s.HERE/'campaign/x/core-area-ai/oracle')
    out=ap.parse_args().out.resolve();out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(score='battle_ai_simulate_action_and_score',account='battle_ai_call_ability_processing',
        restore='battle_ai_restore_unit_state',init_action='battle_action_init_current_data',
        resolve='battle_action_resolve_ability_strike',commit='battle_action_finalize_attack_and_flag_reactions',
        absent='battle_ai_is_unit_untargetable',store_target='battle_action_store_target_stats_pointer_data',
        store_actor='battle_action_store_acting_unit_data',reaction='battle_reaction_prepare_next',
        higher='battle_ai_is_action_higher_ranked',tiles='battle_ai_evaluate_target_tiles',
        move='battle_ai_move_temp_unit_to_coords',range='battle_target_set_panels_for_action',
        priority='battle_ai_check_set_highest_unit_priority')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=500000
    o.function_hashes['vsync']=bind_function('vsync','VSync','main',d)
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    ai=0x8019f3c4;units=0x801908cc;action=ai+0x14;active=[0];visits=[]
    def put(addr,data):o.uc.mem_write(addr&0x1fffff,data)
    def u16(addr,n):put(addr,struct.pack('<H',n&65535))
    def external(uc,address,size,data):
        value=0
        if address==s.FUNCTIONS['resolve'][0]:
            work=uc.reg_read(o.reg.UC_MIPS_REG_A1);raw=bytearray(36)
            raw[1]=len(active);raw[2:18]=bytes(active)+bytes([255])*(16-len(active));put(work,bytes(raw))
        elif address==s.FUNCTIONS['reaction'][0]:value=0xffffffff
        elif address==s.FUNCTIONS['range'][0]:
            for layer in range(2):
                for y in range(3):
                    for x in range(4):put(s.TILES+8*(256*layer+4*y+x)+5,b'\x40')
            value=1
        elif address==s.FUNCTIONS['priority'][0]:
            coords=struct.unpack('<3H',o.uc.mem_read((action+0x14)&0x1fffff,6))
            visits.append(list(coords));put(ai+0x19bc,b'\x01')
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for k in ('vsync','restore','init_action','resolve','commit','absent','store_target','store_actor','reaction','move','range','priority'):
        at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    def reset():
        put(ai,bytes(0x19c8));put(units,bytes(21*448));put(ai+0x198c,bytes([255])*21)
        put(ai+0xe17,b'\x01');put(0x8018f5f4,bytes(8));put(ai+0xe2e,b'\0')
        put(action+8,b'\0\x15\x10\0');put(ai,b'\x15\0\x10\0')
    def hp_state(rows,after):
        for i,p in enumerate(rows):
            hp,maximum,following,enemy,hit,useful=p;value=following if after and hit and (i in active) else hp
            ptr=units+448*i;u16(ptr+0x28,value);u16(ptr+0x2a,maximum)
            bits=mask(2)if not value else mask(23)if value*5<=maximum else bytes(5)
            put(ptr+0x58,bits);put(ai+0x198c+i,bytes((i,)));put(ai+0x182c+16*i+8,bytes((enemy,)))
    def score():
        put(ai+0x19b7,b'\x01');put(ai+0xeee,b'\x02');u16(action+0x1e,0)
        assert o.call('score')==1
        return struct.unpack('<h',o.uc.mem_read((action+0x1e)&0x1fffff,2))[0]
    cases=[];rows_out=[]
    for case in range(12):
        reset();rows=[]
        for i in range(13):
            hp=25 if case in (4,5)and i%3==0 else 100
            enemy=int(i%3!=0);hit=(100,55,30,0)[(i+case)%4]
            following=0 if case in (2,3)and i%2 else (100 if case in (4,5)and not enemy else max(0,hp-35))
            useful=int((enemy or(hp<50 and following>hp))and case!=7)
            rows.append([hp,100,following,enemy,hit,useful])
        selected=8191 if case<8 else (1<<(case-8))|0x1200
        if case==6:selected=0
        active[:]=[i for i in range(13)if selected&(1<<i)]
        hp_state(rows,False);before=score()
        put(ai+0x19b7,b'\0');u16(action+0x1e,0);put(action+0x1d,b'\xff')
        for i,p in enumerate(rows):
            u16(units+448*i+0x18c+0x2a,p[4]);put(ai+0xc78+21+i,bytes((p[5],)))
        accepted=o.call('account',action)
        prefix=struct.unpack('<h',o.uc.mem_read((action+0x1e)&0x1fffff,2))[0]
        hit=o.uc.mem_read((action+0x1d)&0x1fffff,1)[0]
        hp_state(rows,True);after=score();delta=after-before+prefix
        expected_hit=hit if accepted else 0
        record=[selected,accepted,delta&65535,expected_hit]+sum(rows,[]);rows_out.append(record)
        cases.append(dict(mask=selected,targets=rows,accepted=accepted,score_delta=delta,hit=expected_hit,friendly_penalty=prefix))
    ranks=[]
    for p in ((20,80,4,20,80,4),(20,80,3,20,80,4),(19,100,0,20,10,9),(21,1,9,20,100,0),(20,81,4,20,80,4)):
        reset();pa,ha,da,pb,hb,db=p
        for ptr,pri,hit,x in ((action,pa,ha,0),(ai+0x34,pb,hb,1)):
            put(ptr+4,bytes((x,0,0,0)));put(ptr+0x1c,bytes((0,hit)));u16(ptr+0x1e,pri)
        put(ai+0x174,bytes((da,db)));ranks.append([*p,o.call('higher',action,ai+0x34)])
    reset();put(ai+4,b'\x04');put(ai+0xe3a,b'\x04\x03');put(action+0x12,b'\x05')
    # Three empty source panels, including the second level; no unit lookup path.
    u16(ai+0xbdc+2,0x5000);u16(ai+0xbdc+36+4,0x8000);visits.clear();o.call('tiles')
    assert visits==[[1,0,1],[3,0,1],[0,1,2]],visits
    (out/'fai00.bin').write_bytes(b'FAI1'+struct.pack('>H',len(rows_out))+b''.join(struct.pack('>82H',*row)for row in rows_out))
    lines=['/* Original immediate HP-state deltas and hostile area hit accounting. */','static const u16 source_area_rank[][7]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in ranks];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(cases=cases,ranks=ranks,empty_tile_visits=visits,functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original score-state phase, strike hit accounting, rank comparison and empty-panel scan. HP/current Dead/Critical state and prepared hit lists are supplied. Action preparation/effect/reaction/coordinate/range/VSync hooks isolated; no whole original delayed-turn AI chooser, MP utility, broad status weights or source future-event simulation certificate.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Original area AI:',len(cases),'HP/accounting profiles,',len(ranks),'ranks and3 empty panels')


if __name__=='__main__':main()
