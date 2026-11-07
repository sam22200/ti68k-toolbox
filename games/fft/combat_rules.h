#ifndef FFT_COMBAT_RULES_H
#define FFT_COMBAT_RULES_H
static u8 counter_magic_cost(u8 who,AbId ab)
{
    const u8 *r=cj_range(common,ab);
    return r?core_cast_cost(r[13],0,st.unit[who].job==SOURCE_JOB_MIME,
        cj_support(common,st.unit[who].job,st.support[who],SUPPORT_HALF_MP)):0;
}
static u8 counter_magic_ready(u8 who,AbId ab)
{
    const u8 *r=cj_range(common,ab);const Unit *u=&st.unit[who];
    return ab<512&&r&&(r[6]&64)&&abilities[ab].kind!=255&&(abilities[ab].flags&AF_CAST)&&
        !(u->status&ST_SILENCE&&r[5]&32)&&(!UNIT_FROG(u)||ab==ABIL_FROG)&&u->mp>=counter_magic_cost(who,ab);
}
static u16 distribute_targets(u8 who)
{
    u8 i;u16 mask=0;
    for(i=0;i<NUNIT;i++)if(i!=who){
        const Unit *u=&st.unit[i];
        if(u->hp&&u->hp<u->max_hp&&unit_on_field(u)&&!unit_petrified(u)&&!unit_airborne(u)&&
           ((unit_home_team(u)!=TEAM_ENEMY)==(st.unit[who].team!=TEAM_ENEMY)))mask|=(u16)1<<i;
    }
    return mask;
}
#endif
