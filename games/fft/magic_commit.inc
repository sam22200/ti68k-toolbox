// Shared magical HP/status/response commit; payment and rewards stay outside.
static u8 action_magic_commit(void)
{
    Unit *ut=&st.unit[st.tgt];u8 kind,death,proc_changed=0,ko=!ut->hp;
    kind=st.effect_kind&EFFECT_KIND_MASK;
    if(kind==AB_CATCH){if(unit_home_team(ut)!=TEAM_ENEMY&&st.dmg<255&&battle_save.inv[st.dmg]<99)battle_save.inv[st.dmg]++;return 0;}
#if MOD_CORE || MOD_PART(MOD_PERFORMANCE)
    if(st.abil<512&&(performance_formula(abilities[st.abil].formula)||st.abil==109||st.abil==116)&&performance_status_commit())return status_magic_commit();
#endif
    if(kind==AB_NULL)return 0;
    if(kind==AB_MP_DAMAGE||kind==AB_MP_HEAL){
        ut->mp=action_hp(kind==AB_MP_DAMAGE?AB_DAMAGE:AB_HEAL,ut->mp,ut->max_mp,st.dmg);return 0;
    }
    if(kind==AB_HP_MP_HEAL||kind==AB_HP_DAMAGE_MP_HEAL){
        ut->mp=action_hp(AB_HEAL,ut->mp,ut->max_mp,st.effect_mp);
        kind=kind==AB_HP_MP_HEAL?AB_HEAL:AB_DAMAGE;
    }
    if((st.effect_kind&EFFECT_OIL)&&(core_status_removable(&ut->statuses,2)&128)){
        ut->statuses.inflicted[2]&=127;status_changed();
    }
    if(kind==AB_DEATH&&!magic_heal(st.atk,st.tgt,st.abil,3))kind=AB_DAMAGE;
    u8 redirected=reaction_resources(st.tgt,kind,st.dmg);
    if(redirected)st.effect_kind=(st.effect_kind&~EFFECT_KIND_MASK)|AB_MP_DAMAGE;
    death=kind==AB_DEATH;if(!redirected)ut->hp=magic_hp(kind,ut->hp,ut->max_hp,st.dmg);
    if(death)kind=AB_DAMAGE;
    if(kind==AB_REVIVE&&ko&&ut->hp){
        ut->statuses.inflicted[0]&=0xdf;ut->dc=3;
        unit_status_refresh(ut);status_clock_update(st.tgt);status_changed();
    }
    if(!ut->hp&&!ko){ut->kos++;status_knockout(st.tgt);}
    else if(kind==AB_DAMAGE&&!redirected)status_damage(st.tgt);
    if((st.effect_kind&EFFECT_STATUS)&&ut->hp){
        u16 packed=abilities[st.abil].formula==0x20?st.effect_mp:magic_heal(st.atk,st.tgt,st.abil,3);
        if(packed){
            u8 byte=(u8)(packed>>8),mask=(u8)packed,sid=(u8)(byte<<3),bit=128;
            const u8 *row;
            while(!(mask&bit)){bit>>=1;sid++;}
            row=cj_status_rules(common)+((u16)sid<<4);
            if(sid==SID_DEAD){if(ut->hp){ut->hp=0;ut->kos++;status_knockout(st.tgt);}}
            else if(sid==SID_FROG&&(core_status_current(&ut->statuses,2)&2)){
                ut->statuses.inflicted[2]&=0xfd;
            }else{
                // The source request contains exactly one bit. Its filtered
                // cancellation row and timer reuse the common status rules.
                u8 remove[5],i;
                for(i=0;i<5;i++)remove[i]=row[6+i]&core_status_removable(&ut->statuses,i);
                core_status_remove(&ut->statuses,remove,ut->status_ct);
                ut->statuses.inflicted[byte]|=mask;
                core_status_timer_set(ut->status_ct,sid,0,cj_status_rules(common));
            }
            if(!(core_status_current(&ut->statuses,0)&9))st.pending[st.tgt].state=DEFER_NONE;
            unit_status_refresh(ut);status_clock_update(st.tgt);status_changed();proc_changed=1;
        }
    }
    if(kind==AB_DAMAGE&&!redirected)reaction_damage(st.tgt,st.dmg);
    return proc_changed||((u8)!ut->hp!=ko);
}
