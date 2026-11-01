// FORMULA owns bitmap/timer commit. The renderer receives status labels, not HP.
static u8 status_magic_commit(void)
{
    CoreStatusDelta d;u8 t=st.tgt;Unit *u=&st.unit[t];u16 result;
    if(!st.hit||!magic_applicable(t,st.abil))return 0;
    if(st.effect_kind==AB_CLEANSE)return status_cleanse(t,st.abil,st.dmg);
    // Quick fills CT (255); Persuade's 0x100 empties it.
    if(st.effect_kind==AB_QUICK){u8 old=u->ct;u->ct=(u8)st.dmg;if(old!=u->ct)status_changed();return old!=u->ct;}
    if(st.effect_kind==AB_FAITH){s16 v=(s16)u->faith-(s8)st.dmg;u8 old=u->faith;u->faith=(u8)(v<0?0:v>100?100:v);return old!=u->faith;}
    // Yell: Speed 1..50; Scream: PA/MA 1..99 and Speed by its stat Y, then its Brave.
    if(st.effect_kind==AB_SPEED||(st.effect_kind==AB_BRAVE&&abilities[st.abil].formula==0x3b)){
        u8 y=st.effect_kind==AB_SPEED?(u8)(0U-st.dmg):abilities[st.abil].element,old=u->sp;
        u->sp=u->sp+y>50?50:u->sp+y;result=old!=u->sp;
        if(st.effect_kind==AB_SPEED){if(result)status_changed();return (u8)result;}
        old=u->pa;u->pa=u->pa+y>99?99:u->pa+y;result|=old!=u->pa;
        old=u->ma;u->ma=u->ma+y>99?99:u->ma+y;result|=old!=u->ma;
        return status_brave(t,(u8)(0U-st.dmg))|(u8)result;
    }
    if(st.effect_kind==AB_BRAVE)return status_brave(t,(u8)(0U-st.dmg));
    if(st.effect_kind==AB_GOLEM){st.golem[u->team&1]=u->max_hp;status_changed();return 1;}
    if(st.effect_kind!=AB_STATUS)return 0;
    memset(&d,0,sizeof d);d.add[st.dmg>>8]=(u8)st.dmg;
    result=core_status_apply(&u->statuses,&d,u->status_flags,0,cj_status_rules(common),u->status_ct);
    if(!(core_status_current(&u->statuses,0)&8))st.pending[t].state=DEFER_NONE;
    unit_status_refresh(u);status_clock_update(t);if(result)status_changed();
    return result!=0;
}
static u8 status_scalar_commit(void)
{
    u8 changed;
    if(!core_action_impact(&st.action,st.atk))return 0;
    if(!st.hit){if(st.effect_kind==AB_REFLECT)core_action_queue(&st.action,st.tgt,REACTION_REFLECT,NUNIT);return 0;}
    changed=status_magic_commit();
    if(changed&&st.action.phase==CTX_IMPACT)st.primary.reward=earned_exp(st.atk,st.tgt,0);
    return changed;
}
