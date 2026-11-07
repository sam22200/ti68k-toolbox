// Existing status hooks; timing, application/immunity and other statuses are separate.
#ifndef FFT_STATUS_RULES_H
#define FFT_STATUS_RULES_H
static inline u8 status_physical_base(u8 attacker,u8 target)
{
    u8 base=100;
    if(attacker&ST_DARK)base>>=1;
    if(target&ST_DEFEND)base>>=1;
    return base;
}
#ifdef FFT_CHAPTER
// Source halves the attacker's base hit for Darkness or Confusion.
static u8 unit_physical_base(const Unit *a,const Unit *t)
{
    return status_physical_base((u8)(a->status|((core_status_current(&a->statuses,1)&16)?ST_DARK:0)),t->status);
}
#else
#define unit_physical_base(a,t) status_physical_base((a)->status,(t)->status)
#endif
#ifdef FFT_CHAPTER
#include "cast_rules.h"
#if MOD_CORE
u8 skill_cost(u8 who,AbId ab)
{
    const u8 *range=cj_range(common,ab);
    if(!range)return 0;
    return core_cast_cost(range[13],cj_action_menu(common,st.unit[who].set),
        st.unit[who].job==SOURCE_JOB_MIME,st.support[who]==SUPPORT_HALF_MP);
}
#endif
static inline u8 ability_range(u8 who,AbId ab)
{
    const Ability *b=&abilities[ab];
    if(b->flags&AF_ITEM&&b->formula!=99)return st.unit[who].job==SOURCE_JOB_MIME||
        cj_support(common,st.unit[who].job,st.support[who],474)?4:1;
    return b->formula==99?st.unit[who].move:b->range;
}
static inline u8 status_blocks_skill(u8 status,AbId ab,u8 set)
{
    const u8 *range;u8 menu;
    if(!(status&ST_SILENCE)||(range=cj_range(common,ab))==RT_NULL)return 0;
    menu=cj_action_menu(common,set);
    return (menu==0||menu==6)&&(range[5]&0x20)!=0;
}
static inline u8 skill_blocked(u8 who,AbId ab)
{
    const Unit *u=&st.unit[who];
    return (UNIT_FROG(u)&&ab!=ABIL_FROG)||status_blocks_skill(u->status,ab,u->set)||u->mp<skill_cost(who,ab);
}
#endif
#endif
