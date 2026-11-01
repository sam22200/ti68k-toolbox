// Shared reward implementation; native ACTION owns its code.
static void add_jp(Unit *u, u16 jp)
{
    u->jp = u->jp + jp > 9999 ? 9999 : u->jp + jp;
    u->tjp = u->tjp + jp > 9999 ? 9999 : u->tjp + jp;
    u->gjp += jp;
}

#include "reward_value.h"
// EXP (at most 255), then FFT's level check: level +1, EXP 0, stats grown from the raw ones.
static void exp_gain(Unit *u,u8 e)
{
    u16 x=u->exp+e;u->exp=x>255?255:(u8)x;
    if (u->exp >= EXP_LEVEL) {
        if (u->level < 99) {
            u16 hp = u->hp, mp = u->mp;
            u->level++; u->exp = 0; st.lvup = 1;
            unit_stats(u);
            u->hp = hp < u->max_hp ? hp : u->max_hp;
            u->mp = mp < u->max_mp ? mp : u->max_mp;
        } else u->exp = 99;
    }
}
#ifdef FFT_CHAPTER
// Steal Exp (battle_action_handle_steal_exp): the target loses it, the thief gains it now.
static void steal_exp(u8 a,Unit *ut,u8 n){ut->exp=ut->exp>n?ut->exp-n:0;exp_gain(&st.unit[a],n);}
#endif
static void award_rewards(u8 a,u8 e)
{
    Unit *ua=&st.unit[a],*u;
    u16 jp;
    u8 i, blue = ua->team != TEAM_ENEMY;
    if (!ua->hp) return;
    st.gain = (u8)e;
    jp = (u16)(8 + 2 * job_level(ua) + (ua->level >> 2));
    st.jpg = (u8)jp;
    ua->gexp += (u16)e;
    exp_gain(ua,e);
    add_jp(ua, jp);
    for (i = 0, u = st.unit; i < NUNIT; i++, u++)
        if (u != ua && u->hp && (u->team != TEAM_ENEMY) == blue && jobs[u->job].row == jobs[ua->job].row)
            add_jp(u, jp >> 2);
}
void rewards(u8 a,u8 t,u8 kill){award_rewards(a,earned_exp(a,t,kill));}

