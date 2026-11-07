#ifndef FFT_WEAPON_PROC_INC
#define FFT_WEAPON_PROC_INC
// A once-only, single-target weapon spell. Root command payment stays outside.
static u8 weapon_proc_commit(void)
{
    AbId spell=st.self&255,old=st.abil;u8 a=st.atk,t=st.tgt,original_target=st.tgt,kind,hit,changed=0,phase=st.action.phase;u16 amount;Unit *u=&st.unit[t];
    if(!spell||!u->hp||!st.unit[a].hp)return 0;
    st.abil=spell;
    if(magic_reflects_from(a,t,spell)){
        u8 found=reflect_recipient(t,st.unit[a].x,st.unit[a].z);
        if(found==255){st.abil=old;return 0;}t=found;u=&st.unit[t];st.tgt=t;st.action.phase=CTX_APPLIED;
    }
    kind=skill_kind(a,t,spell);hit=magic_hit(a,t,spell,1);
    reaction_before(t,0);
    if(hit&&kind!=AB_NULL&&magic_target(t)==MAGIC_VALID){
        u8 ko=!u->hp,redirected;amount=magic_amount(a,t,((u16)abilities[spell].element<<8)|abilities[spell].y);
        if((abilities[spell].element&128)&&(core_status_removable(&u->statuses,2)&128)){u->statuses.inflicted[2]&=127;status_changed();}
        redirected=reaction_resources(t,kind,amount);
        if(!redirected)u->hp=magic_hp(kind,u->hp,u->max_hp,amount);
        if(!u->hp&&!ko){u->kos++;status_knockout(t);}
        else if(kind==AB_DAMAGE&&!redirected)status_damage(t);
        if(kind==AB_DAMAGE&&!redirected)reaction_damage(t,amount);
        changed=(u8)!u->hp!=ko;
        if(phase==CTX_IMPACT){u8 e=earned_exp(a,t,kind==AB_DAMAGE&&!u->hp);if(e>st.primary.reward)st.primary.reward=e;}
    }
    st.abil=old;st.tgt=original_target;st.action.phase=phase;return changed;
}
#endif
