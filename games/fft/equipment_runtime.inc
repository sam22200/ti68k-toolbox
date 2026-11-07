#include "discovery_move.inc"
// Cold shared progression and battle inventory. No file writes or heap allocation.
static void equipment_rebuild(u8 who)
{
    Unit *u=&st.unit[who];u16 hp=u->hp,mp=u->mp;
    unit_stats(u);u->hp=hp<u->max_hp?hp:u->max_hp;u->mp=mp<u->max_mp?mp:u->max_mp;
    W->fc_key=W->move_kind=0;W->ej=255;W->ai_k=0;
}
static void equipment_rebuild_pending(void)
{
    u8 i;u16 bits=W->rebuild;W->rebuild=0;
    for(i=0;i<NUNIT;i++)if(bits&((u16)1<<i))equipment_rebuild(i);
}
static u8 equipment_ready(u8 who)
{
    return who<NUNIT&&who==st.act&&!st.acted&&unit_on_field(&st.unit[who])&&
        st.unit[who].hp&&unit_can_act(&st.unit[who])&&!UNIT_FROG(&st.unit[who])&&
        unit_home_team(&st.unit[who])!=TEAM_ENEMY&&cj_support(common,st.unit[who].job,st.support[who],480);
}
// Complete loadout transaction: validate permissions, hands and net inventory first.
// A held copy may move between hands even when there are no unequipped copies.
static u8 equipment_commit(u8 who,const u8 *eq)
{
    Unit *u;u16 id;u8 k,old,new;u8 counts[256];
    if(!equipment_ready(who)||!eq)return 0;u=&st.unit[who];
    for(k=0;k<5;k++)if(!core_equipment_allowed(common,u->job,u->female,st.support[who],eq,k,eq[k]))return 0;
    memcpy(counts,battle_save.inv,256);
    for(id=1;id<255;id++){
        old=new=0;for(k=0;k<5;k++){old+=u->eq[k]==id;new+=eq[k]==id;}
        if(new>old&&new-old>counts[id])return 0;
        if(old>new&&counts[id]>99-(old-new))return 0;
        counts[id]=(u8)(counts[id]+old-new);
    }
    // IDs0 and255 represent empty, never a stocked/equippable item.
    for(k=0;k<5;k++)if(!eq[k])return 0;
    memcpy(battle_save.inv,counts,256);memcpy(u->eq,eq,5);
    if(st.pending[who].state==DEFER_CHARGING){st.pending[who].state=DEFER_NONE;unit_status_action(u,0);}
    equipment_rebuild(who);st.acted=1;st.mode=M_MENU;st.menu=st.moved?MN_WAIT:MN_MOVE;
    return 1;
}
static u8 movement_benefit(u8 who)
{
    Unit *u=&st.unit[who];u8 flags[3],kind=0;u16 n=0;
    if(W->nature_units){
#if MOD_CORE
        monster_refresh();
#else
        W->nature_pending|=1;
#endif
    }
    if(discovery_move(who))return 1;
    if(!u->hp||core_reaction_blocked(&u->statuses,0,0,0,0))return 0;
    cj_movement(common,u->job,st.movement[who],flags);
    if(flags[0]&1){kind=1;n=(u->max_hp+9)/10U;u->hp=action_hp(AB_HEAL,u->hp,u->max_hp,n);}
    else if(flags[1]&128){kind=2;n=(u->max_mp+9)/10U;u->mp=action_hp(AB_HEAL,u->mp,u->max_mp,n);}
    else if(flags[1]&96){
        n=st.path_n?st.path_n-1:0;
        if(flags[1]&12)n=(u16)(muls16((s16)rand15(),u->move)>>15)+1;
        if(n>99)n=99;
        if(flags[1]&64){kind=3;if(cj_support(common,u->job,st.support[who],464))n<<=1;u->gexp+=n;exp_gain(u,(u8)n);}
        else {kind=4;if(cj_support(common,u->job,st.support[who],463))n<<=1;add_jp(u,n);}
    }
    if(!kind)return 0;
    st.abil=492+kind;st.dmg=n;st.pick=254;st.pick_t=1;st.mode=M_PICK;
    return 1;
}
