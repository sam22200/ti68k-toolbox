// Impact commit, separate from presentation. Consumes ACT_HIT on a hit or miss.
// No nested overlay calls: ACTION uses its local evaluator and resident callbacks.
#ifdef FFT_CHAPTER
#if MOD_CORE
#include "reaction_runtime.inc"
#else
#include "reaction_policy_runtime.inc"
#endif
#if MOD_CORE
#include "strike_runtime.inc"
#endif
static u8 action_reaction_blocked(u8 target)
{
    CoreAction *c=&st.action;
    u8 blocked=core_reaction_blocked(&st.unit[target].statuses,c->phase>=CTX_SECONDARY,0,0,
        c->pending&((u16)1<<target)?c->reaction[target]:0);
    return blocked?blocked:!reaction_live(target);
}
// a skill's effects beyond its damage: the user drains it, a cross splash around the target
static void skill_extra(void)
{
    const Ability *b;
    Unit *a=&st.unit[st.atk],*t=&st.unit[st.tgt];
    if(st.abil==NO_ABIL)return;
    b=&abilities[st.abil];
    if(b->flags&AF_DRAIN){u16 n=st.dmg; if(n>a->max_hp-a->hp)n=a->max_hp-a->hp; a->hp+=n;}
    if(b->flags&AF_AREA){
        u8 i;
        // Cross-shaped Stasis Sword splash, including allies (no team filtering).
        for(i=0;i<NUNIT;i++){
            Unit *v=&st.unit[i];s16 dx=(s16)v->x-t->x,dz=(s16)v->z-t->z;
            u8 d=(u8)((dx<0?-dx:dx)+(dz<0?-dz:dz));u16 n;
            if(i==st.tgt||!v->hp||d!=1||unit_petrified(v))continue;
            if(st.reaction[i]==REACTION_COUNTER&&reaction_counter_flags())reaction_offer(i,REACTION_COUNTER);
            n=skill_amount(st.atk,i,st.abil,0);v->hp=action_hp(AB_DAMAGE,v->hp,v->max_hp,n);
            if(!v->hp){v->kos++;status_knockout(i);}
            else status_damage(i);
            reaction_offer(i,REACTION_POTION);
        }
    }
}
#endif
static void action_rewards(u8 kill)
{
#ifdef FFT_CHAPTER
    if(st.action.phase!=CTX_IMPACT)return;
    if(st.action.pending||st.action.left||st.primary.reward){
        u8 e=earned_exp(st.atk,st.tgt,kill);
        if(e>st.primary.reward)st.primary.reward=e;
        return;
    }
#endif
    rewards(st.atk,st.tgt,kill);
}

#ifdef FFT_CHAPTER
#include "magic_commit.inc"
#endif
static u8 action_commit(void)
{
#ifdef FFT_CHAPTER
    if(!core_action_impact(&st.action,st.atk))return 0;
    // Match the post-formula validity stage: do not skip preparation RNG/costs.
    if(unit_petrified(&st.unit[st.tgt])){st.hit=st.push=0;st.dmg=st.self=0;return 0;}
    if(st.reaction[st.tgt]==REACTION_COUNTER&&reaction_counter_flags())reaction_offer(st.tgt,REACTION_COUNTER);
#endif
    if(!st.hit){
#ifdef FFT_CHAPTER
        if(st.effect_kind==AB_REFLECT)core_action_queue(&st.action,st.tgt,REACTION_REFLECT,NUNIT);
#endif
        return 0;
    }
    Unit *ut=&st.unit[st.tgt],*ua=&st.unit[st.atk];
    u8 kind = st.abil == NO_ABIL ? AB_DAMAGE : st.abil == ABIL_DEFEND ? AB_DEFEND : abilities[st.abil].kind, ko = !ut->hp;
#ifdef FFT_CHAPTER
    if(restore_ability(st.abil)){
        u16 old_hp=ut->hp,old_mp=ut->mp;u8 changed=action_magic_commit();
        if(old_hp!=ut->hp||old_mp!=ut->mp)action_rewards(!ut->hp&&!ko);
        return changed;
    }
    if(st.abil<512&&magic_formula(abilities[st.abil].formula)){
        u16 old_hp=ut->hp;u8 changed=action_magic_commit();
        if((st.effect_kind&EFFECT_KIND_MASK)!=AB_NULL&&(abilities[st.abil].formula!=14||old_hp!=ut->hp))action_rewards(!ut->hp&&!ko);
        return changed;
    }
    if (kind == AB_RAISE) { if (ua->pa < 99) ua->pa++; action_rewards(0); return 0; }
    if (kind == AB_STEAL_EXP) { steal_exp(st.atk,ut,(u8)st.dmg); action_rewards(0); return 0; }
    if (kind == AB_BREAK) {
        // Source stat by ability id (0x2b); Magic Break's Orbonne adaptation lowers MA.
        u8 *v = st.abil == 0x90 ? &ut->pa : st.abil == 0x8f ? &ut->sp : &ut->ma;
        *v = *v > abilities[st.abil].y ? *v - abilities[st.abil].y : 1;
        action_rewards(0); return 0;
    }
#endif
    if (kind == AB_DEFEND) {
        #ifdef FFT_CHAPTER
        unit_status_action(ut,2);
        st.pending[st.atk].state=DEFER_NONE;
#else
        ut->status = action_status(kind,ut->status,0);
#endif
        return 0;
    }
#ifdef FFT_CHAPTER
    if(kind==AB_DAMAGE&&st.action.phase==CTX_IMPACT&&st.golem[ut->team&1]&&
       (st.abil==NO_ABIL||(abilities[st.abil].flags&AF_CHARGE))){
        u16 *g=&st.golem[ut->team&1];*g=st.dmg>=*g?0:*g-st.dmg;
        st.effect_kind=AB_GOLEM;st.push=0;return 0;   // whole hit absorbed: no HP, status or push
    }
#endif
    if (kind == AB_DAMAGE) {
        ut->hp = action_hp(kind,ut->hp,ut->max_hp,st.dmg);
        if (st.push) {
#ifdef FFT_CHAPTER
            unit_place(ut,st.push_cell);
#else
            ut->x=st.kx;ut->z=st.kz;
#endif
        }
        if (!ut->hp) {
            ut->kos++;
#ifdef FFT_CHAPTER
            status_knockout(st.tgt);
#endif
        }
#ifdef FFT_CHAPTER
        else status_damage(st.tgt);
#endif
    } else if (kind == AB_CURE) {
#ifdef FFT_CHAPTER
        unit_status_cure(ut,abilities[st.abil].y);
        status_clock_update(st.tgt);
#else
        ut->status = action_status(kind,ut->status,abilities[st.abil].y);
#endif
    }
    else {
        ut->hp = action_hp(kind,ut->hp,ut->max_hp,st.dmg);
        if (ko) ut->dc = 3;
    }
#ifdef FFT_CHAPTER
    skill_extra();
    if(kind==AB_DAMAGE)reaction_offer(st.tgt,REACTION_POTION);
#endif
    action_rewards(kind==AB_DAMAGE&&!ut->hp);
    if (st.self) {
        u16 hp=ua->hp;ua->hp=st.self>=hp?0:hp-st.self;
#ifdef FFT_CHAPTER
        if(hp&&!ua->hp){ua->kos++;status_knockout(st.atk);}
#endif
    }
    return (u8)!ut->hp != ko || !ua->hp;
}

