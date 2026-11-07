// Source reaction staging shared by every scalar and batch effect owner.
#include "reaction_live.h"
static u8 reaction_counter_flags(void)
{
    AbId ab=st.abil;const u8 *r;
    if(ab==ABIL_DEFEND||(st.unit[st.atk].set==21&&ab<60&&math_spell(ab)))return 0;
    if(ab==NO_ABIL||(ab>=512&&ab<NABIL)||(ab<512&&(abilities[ab].flags&AF_CHARGE)))ab=0;
    r=cj_range(common,ab);return r?r[6]:0;
}
static u8 reaction_test(u8 who,u16 kind)
{
    CoreAction *c=&st.action;
    if(c->phase!=CTX_IMPACT||st.reaction[who]!=kind||(c->pending&((u16)1<<who))||!reaction_live(who))return 0;
    if(st.abil<NABIL&&abilities[st.abil].formula==7)return 0;
    return reaction_chance(st.unit[who].brave,rand15());
}
static void reaction_offer(u8 who,u16 kind,u16 request)
{
    CoreAction *c=&st.action;u8 self;
    self=kind==REACTION_FAITH_UP||kind==REACTION_SPLIT||kind==REACTION_DISTRIBUTE||kind==REACTION_COUNTER_MAGIC;
    if(who==c->root&&!self)return;
    if(reaction_test(who,kind)&&core_action_queue(c,who,kind,(u8)(NUNIT|(self?CTX_QUEUE_SELF:0)))==CTX_OK)c->reaction[who]=request;
}
static void reaction_before(u8 who,u8 physical)
{
    u16 id=st.reaction[who];u8 cost;
    if(id==REACTION_COUNTER_MAGIC&&(reaction_counter_flags()&64)){
        reaction_offer(who,id,0x0800U|st.abil);return;
    }
    if(physical&&reaction_counter(id)&&(reaction_counter_flags()&(id==REACTION_FLOOD?128:16))){reaction_offer(who,id,id);return;}
    if(reaction_mp(id)&&st.abil<NABIL&&(cost=skill_cost(st.atk,st.abil))!=0)reaction_offer(who,id,id==REACTION_ABSORB_MP?0x8000U|cost:id);
}
// These reactions run before resources settle. MP Switch spends all remaining
// MP on a shortfall, with no spill into HP; Split heals back only after survival.
static u8 reaction_resources(u8 who,u8 kind,u16 amount)
{
    Unit *u=&st.unit[who];u16 id=st.reaction[who],n;
    if(kind==AB_DAMAGE&&(s16)amount>0){
        if(id==REACTION_MP_SWITCH&&u->mp&&reaction_test(who,id)){
            n=amount>999?999:amount;u->mp=n>=u->mp?0:u->mp-n;return 1;
        }
        if(id==REACTION_SPLIT){n=((amount>999?999:amount)+1)>>1;reaction_offer(who,id,0x2000U|n);}
    }
    if(kind==AB_HEAL&&(s16)amount>0&&id==REACTION_DISTRIBUTE&&amount>u->max_hp-u->hp){
        n=amount-(u->max_hp-u->hp);if(n>999)n=999;reaction_offer(who,id,0x1000U|n);
    }
    return 0;
}
static void reaction_damage(u8 who,u16 amount)
{
    u16 id=st.reaction[who];
    if(id==REACTION_GILGAME){reaction_offer(who,id,0x4000U|(amount>999?999:amount));return;}
    if(reaction_critical(id)){if(!reaction_critical_ready(who))return;}
    else if(id==REACTION_REGENERATOR){if(!st.unit[who].brave)return;}
    else if(!reaction_stat(id))id=REACTION_POTION;
    reaction_offer(who,id,id);
}
