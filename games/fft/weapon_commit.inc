// Shared HP/reaction/status lifecycle, without another command or consumable cost.
#ifndef FFT_WEAPON_COMMIT_INC
#define FFT_WEAPON_COMMIT_INC
#include "allegiance.inc"
#if MOD_CORE
// The monolithic build already included the common reward and reaction helpers.
#else
#include "action_rewards.inc"
#endif
static u8 weapon_status_commit(u8 t,u16 packet)
{
    Unit *u=&st.unit[t];CoreStatusDelta d;u16 result;u8 sid=((packet>>8)&63)-1;
    memset(&d,0,sizeof d);
    if(packet&32768){memcpy(d.remove,cj_section(common,CJ_STATUS)+6*((packet>>8)&31)+1,5);sid=255;}
    else{if(sid==SID_CHARM&&(cj_allegiance_immune(common,u->job,u->eq)&2))return 0;d.add[sid>>3]=128>>(sid&7);}
    result=core_status_apply(&u->statuses,&d,u->status_flags,0,cj_status_rules(common),u->status_ct);
    if(!result)return 0;
    if(sid==SID_CHARM)unit_allegiance(u,st.unit[st.atk].team,0);
    if(sid==SID_DEAD){if(u->hp){u->hp=0;u->kos++;status_knockout(t);}}
    if(!(core_status_current(&u->statuses,0)&9))st.pending[t].state=DEFER_NONE;
    unit_status_refresh(u);status_clock_update(t);status_changed();return 1;
}
static u8 weapon_commit(void)
{
    u8 a=st.atk,t=st.tgt,kind=st.effect_kind&EFFECT_KIND_MASK,ko=!st.unit[t].hp,redirected,changed=0;u16 packet=st.self;Unit *u=&st.unit[t],*actor=&st.unit[a];
    if(!core_action_impact(&st.action,a))return 0;
    reaction_before(t,1);
    if(!st.hit||magic_target(t)!=MAGIC_VALID){st.hit=st.push=0;st.dmg=0;st.self=0;return 0;}
    if(kind==AB_NULL)return 0;
    if(kind==AB_DAMAGE&&st.action.phase==CTX_IMPACT&&st.golem[u->team&1]){
        u16 *g=&st.golem[u->team&1];*g=st.dmg>=*g?0:*g-st.dmg;st.effect_kind=AB_GOLEM;st.push=0;st.self=0;return 0;
    }
    if(st.effect_kind&EFFECT_OIL){u->statuses.inflicted[2]&=127;status_changed();}
    redirected=reaction_resources(t,kind,st.dmg);
    if(redirected)st.effect_kind=AB_MP_DAMAGE;
    else u->hp=magic_hp(kind,u->hp,u->max_hp,st.dmg);
    if(st.push&&kind==AB_DAMAGE&&!redirected&&u->hp)unit_place(u,st.push_cell);
    if(!u->hp&&!ko){u->kos++;status_knockout(t);}
    else if(kind==AB_DAMAGE&&!redirected)status_damage(t);
    if((packet&16384)&&!redirected&&actor->hp){
        u8 selfkind=packet&32768?AB_DAMAGE:AB_HEAL;u16 hp=actor->hp;
        actor->hp=action_hp(selfkind,actor->hp,actor->max_hp,st.dmg);
        if(hp&&!actor->hp){actor->kos++;status_knockout(a);}
    }
    if(u->hp&&(packet>>8)&63)changed=weapon_status_commit(t,packet);
    if(kind==AB_DAMAGE&&!redirected)reaction_damage(t,st.dmg);
    if(W->nature_units&&kind==AB_DAMAGE&&!redirected&&cj_weapon(common,current_weapon(a))[2]<7){
#if MOD_CORE
        nature_hit_commit();
#else
        W->nature_pending|=2;
#endif
    }
    if((packet&255)&&u->hp){
        u8 e=earned_exp(a,t,0);if(st.action.phase==CTX_IMPACT&&e>st.primary.reward)st.primary.reward=e;
#if MOD_CORE
        changed|=weapon_proc_commit();
#else
        W->nature_pending|=8;
#endif
    }else action_rewards(kind==AB_DAMAGE&&!u->hp);
    return changed||((u8)!u->hp!=ko)||!actor->hp;
}
#endif
