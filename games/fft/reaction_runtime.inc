#include "reaction_policy_runtime.inc"
#if MOD_CORE
#include "reaction_self.inc"
#endif
#include "reaction_restore.inc"
#if MOD_CORE
#include "reaction_select.inc"
#endif
#include "reflect_launch.inc"
static void reaction_launch(u8 kind)
{
#if MOD_CORE
    if(kind==6){reaction_self_launch(kind);return;}
#endif
    CoreAction *c=&st.action;u8 who=c->actor,potion;
    if(c->phase!=CTX_SECONDARY||st.primary.running||kind<1||(kind>5&&kind!=8))return;
    if(kind==3){reflect_launch();return;}
    st.primary.running=1;
    action_pose_start(who,kind==1||kind==8?c->root:who);
    if(W){W->hold=W->frz=W->frz_h=0;W->fc_key=0;}
    if(kind==1){c->left=hand_count(who)-1;c->cell=UNIT_CELL(&st.unit[c->root]);st.abil=NO_ABIL;attack_roll(who,c->root);}
    else if(kind==8){
        u16 r;st.abil=0x93;st.hit=magic_target(c->root)==MAGIC_VALID;
        if(st.hit){
            st.dmg=dash_amount(who,c->root,(rand15()>>13)+1);
            r=(u16)(muls16((s16)rand15(),st.unit[who].brave)>>15);
            if((u16)(muls16((s16)rand15(),st.unit[c->root].brave)>>15)<r)knockback(who,c->root);
        }
    }
    else if(kind==4){st.abil=c->reaction[who];st.hit=1;st.dmg=1;}
    else if(kind==5){
        const Unit *u=&st.unit[who];st.abil=reaction_id(c->reaction[who]);
        st.hit=!(core_status_current(&u->statuses,3)&1);
        st.effect_kind=st.abil==REACTION_GILGAME?AB_STEAL_GIL:st.abil==REACTION_HP_RESTORE?AB_HEAL:st.abil==REACTION_MP_RESTORE||st.abil==REACTION_ABSORB_MP?AB_MP_HEAL:AB_QUICK;
        st.dmg=st.abil==REACTION_GILGAME?(c->reaction[who]&0x3fffU):st.abil==REACTION_HP_RESTORE?u->max_hp-u->hp:st.abil==REACTION_MP_RESTORE?u->max_mp-u->mp:st.abil==REACTION_ABSORB_MP?(c->reaction[who]&255):255;
        if(!st.hit){st.dmg=0;st.effect_kind=AB_NULL;}
    }
    else{
        potion=reaction_potion(st.bag,st.unit[who].team==TEAM_ENEMY);
        st.abil=(AbId)(0x170+potion);restore_prepare(who,st.abil);
        if(st.unit[who].team!=TEAM_ENEMY){st.bag[potion]--;W->win_k[0]=0;}
    }
}
static void reaction_rewards(void)
{
    u8 reward=st.primary.reward;st.primary.reward=0;
    if(!reward)return;
    award_rewards(st.action.root,reward);
    if(st.gain)st.act_t=ACT_HIT+GAIN_T-1;
}
