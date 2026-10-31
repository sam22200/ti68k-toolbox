// Shared reward implementation; native ACTION owns its code.
static void add_jp(Unit *u, u16 jp)
{
    u->jp = u->jp + jp > 9999 ? 9999 : u->jp + jp;
    u->tjp = u->tjp + jp > 9999 ? 9999 : u->tjp + jp;
    u->gjp += jp;
}

#include "reward_value.h"
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
    ua->exp += (u8)e;
    if (ua->exp >= EXP_LEVEL) {
        if (ua->level < 99) {                       // the stats grown from the raw ones
            u16 hp = ua->hp, mp = ua->mp;
            ua->level++; ua->exp = 0; st.lvup = 1;
            unit_stats(ua);
            ua->hp = hp < ua->max_hp ? hp : ua->max_hp;
            ua->mp = mp < ua->max_mp ? mp : ua->max_mp;
        } else ua->exp = 99;
    }
    add_jp(ua, jp);
    for (i = 0, u = st.unit; i < NUNIT; i++, u++)
        if (u != ua && u->hp && (u->team != TEAM_ENEMY) == blue && jobs[u->job].row == jobs[ua->job].row)
            add_jp(u, jp >> 2);
}
void rewards(u8 a,u8 t,u8 kill){award_rewards(a,earned_exp(a,t,kill));}

