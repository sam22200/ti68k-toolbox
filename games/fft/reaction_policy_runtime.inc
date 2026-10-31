// ACTION-owned response policy, packet suspension and preparation. No overlay calls.
static u8 reaction_live(u8 who)
{
    const Unit *u=&st.unit[who];
    if(!u->hp||u->x>=MAP_W||u->z>=MAP_H||UNIT_CELL(u)>=MAP_N||
       core_reaction_blocked(&u->statuses,0,0,0,0))return 0;
    // Source deep-water restriction. Movement-trait/mount producers remain gated.
    if(W->surface){
        const u8 *col=W->surface+((u16)(W->grid_row[u->z]+u->x)<<3)+4*node_layer(UNIT_CELL(u));
        if((col[1]>>5)>=2&&!(core_status_current(&u->statuses,2)&0x0e)&&!(u->status_flags&STATUS_RIDER))return 0;
    }
    return 1;
}
static u8 reaction_counter_flags(void)
{
    AbId ab=st.abil;const u8 *r;
    if(ab==ABIL_DEFEND)return 0;
    if(ab==NO_ABIL||(ab<512&&(abilities[ab].flags&AF_CHARGE)))ab=0;
    r=cj_range(common,ab);return r?r[6]&0x10:0;
}
static void reaction_offer(u8 who,u16 kind)
{
    CoreAction *c=&st.action;
    if(c->phase!=CTX_IMPACT||who==c->root||st.reaction[who]!=kind||
       (c->pending&((u16)1<<who))||!reaction_live(who))return;
    if(reaction_chance(st.unit[who].brave,rand15()))core_action_queue(c,who,kind,NUNIT);
}
