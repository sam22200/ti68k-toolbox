#!/usr/bin/env python3
"""Bounded original-MIPS strike initialization, phase order and best-EXP cases."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-strikes/oracle';out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    symbols=dict(init='battle_action_init_current_ability_strike_data',
        resume='battle_state_handle_resume_attack_phase_state',
        exp='battle_unit_update_attacker_earned_experience')
    hooks=dict(cursor='battle_state_handle_free_cursor_input',menus='battle_menu_update_menus_and_event_speed',
        selected='battle_menu_get_selected_command_address',casting='battle_unit_get_casting_misc_data',
        post='battle_action_resume_attack_phase_control',continued='battle_action_set_damage_display_type_based_on_ability',
        invalidate='battle_gfx_invalidate_sp2_vram_slot',stored='battle_action_store_acting_unit_data',
        anim='battle_unit_call_set_animation_based_on_status',targets='battle_unit_update_anim_display_for_all_targets',
        source='battle_unit_get_source_misc_data',phase='battle_state_announce_next_charged_action',
        rewards='battle_action_handle_post_action_xp_jp_ability')
    for key,name in {**symbols,**hooks}.items():bind_function(key,name,'battle',disc)
    o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=200000
    stats,misc,ability=0x801908cc,0x801d5000,0x801938c0
    event=[];phase_result=[0];post_result=[0]
    def external(uc,address,size,data):
        key=next(k for k in hooks if s.FUNCTIONS[k][0]==address)
        value=0
        if key in ('casting','source'):value=misc
        if key=='selected':value=0x801d6000
        if key=='post':value=post_result[0]
        if key=='phase':
            event.append(10+struct.unpack('<I',uc.mem_read(0x96214,4))[0]);value=phase_result[0]
        if key=='continued':event.append(1)
        if key=='rewards':event.append(20)
        uc.reg_write(o.reg.UC_MIPS_REG_V0,value)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    for key in hooks:
        at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
    initial=[]
    for menu in (8,10,9,2):
        for right,left in ((255,255),(1,255),(255,2),(1,2)):
            for two in (0,1):
                o.uc.mem_write(stats&0x1fffff,bytes(0x1c0));o.uc.mem_write(ability&0x1fffff,bytes(0x50))
                o.uc.mem_write((stats&0x1fffff)+0x16f,b'\x01')
                o.uc.mem_write(0x65cb5,bytes((menu,)))
                o.uc.mem_write((stats&0x1fffff)+0x1d,bytes((right,255,left)))
                o.uc.mem_write((stats&0x1fffff)+0x91,bytes((two,)))
                o.write(0x8018f5f0,0);o.call('init',stats)
                result=list(o.uc.mem_read((ability&0x1fffff)+2,4))
                initial.append([menu,right,left,two,*result])
    phases=[]
    for phase,more,blocked,pending in ((1,1,0,1),(1,0,0,1),(2,1,0,1),(2,0,0,0),(1,1,1,1)):
        o.uc.mem_write(misc&0x1fffff,bytes(0x400));o.write(misc+0x134,stats)
        o.uc.mem_write((misc&0x1fffff)+0x1a4,bytes((more,)))
        o.write(0x80096214,phase);o.write(0x80098db8,1);o.write(0x80098d84,255);o.write(0x80098d70,0)
        phase_result[0]=pending;post_result[0]=blocked;event.clear();o.call('resume')
        count=o.uc.mem_read((misc&0x1fffff)+0x1b5,1)[0]
        phases.append(dict(phase=phase,more=more,blocked=blocked,next_pending=pending,events=list(event),count=count))
    assert [p['events']for p in phases]==[[1],[12],[1],[12,20],[]],phases
    xp=[];target=0x801d4000
    for levels,outcomes,kos in (((1,1,1),(1,1),(0,0)),((20,30,1),(1,1),(0,0)),
        ((20,1,30),(1,1),(0,0)),((20,1,1),(1,2),(0,1)),
        ((20,20,20),(1,2),(0,1)),((20,20,20),(1,2),(0,2)),
        ((99,1,1),(1,1),(0,0)),((1,99,1),(1,1),(0,0)),
        ((20,1,30),(0,1),(0,0)),((20,30,1),(1,0),(0,0))):
        o.uc.mem_write(stats&0x1fffff,bytes(0x1c0));o.uc.mem_write(target&0x1fffff,bytes(0x1c0))
        o.uc.mem_write(ability&0x1fffff,bytes(0x50));o.write(0x8018f5f0,0)
        o.write(0x80192d94,stats);o.write(0x80192d98,target)
        o.uc.mem_write((stats&0x1fffff)+0x22,bytes((levels[0],)))
        for i in range(2):
            o.uc.mem_write((target&0x1fffff)+0x22,bytes((levels[i+1],)))
            o.uc.mem_write((target&0x1fffff)+0x15c,bytes((kos[i],)))
            o.call('exp',outcomes[i])
        xp.append([*levels,*outcomes,*kos,o.uc.mem_read((ability&0x1fffff)+0x27,1)[0]])
    lines=['/* Generated unchanged original instructions; local-only fixture inputs. */']
    for name,rows in [('source_strikes',initial),('source_strike_exp',xp)]:
        lines.append(f'static const u16 {name}[][{len(rows[0])}]={{')
        lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(initial=initial,phases=phases,exp=xp,functions=o.function_hashes,
        exe_sha256=s.core_audit.sha(disc.exe),battle_sha256=s.core_audit.sha(disc.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        hooks=hooks,scope='Original strike initializer and best-EXP instructions; resume phase/continue branches with UI/animation/getters/announcement/reward callbacks isolated. Not full Two Swords formulas, original retargeting/knockback or an end-to-end original double-strike battle.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original strikes:',len(initial),'initializers,',len(phases),'phase branches,',len(xp),'best-EXP pairs',flush=True)


if __name__=='__main__':main()
