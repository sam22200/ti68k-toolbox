// Pure reaction eligibility shared by every effect owner. No image loads.
#ifndef FFT_REACTION_LIVE_H
#define FFT_REACTION_LIVE_H
#if MOD_CORE
static u8 reaction_critical_ready(u8 who)
{
    const Unit *u=&st.unit[who];
    // Critical23 is derived after HP settles; innate overrides immunity.
    // Unsigned constant division uses GCC4TI's reciprocal, checked in codegen.
    return u->brave&&((u->statuses.innate[2]&1)||
        (!(u->statuses.immune[2]&1)&&u->hp<=u->max_hp/5U));
}
static u8 reaction_live(u8 who)
{
    const Unit *u=&st.unit[who];
    if(!u->hp||u->x>=MAP_W||u->z>=MAP_H||UNIT_CELL(u)>=MAP_N||
       core_reaction_blocked(&u->statuses,0,0,0,0))return 0;
    // Source deep-water restriction. Movement-trait/mount producers remain gated.
    if(W->surface){
        const u8 *col=W->surface+((u16)(W->grid_row[u->z]+u->x)<<3)+4*node_layer(UNIT_CELL(u));
        // PREVENT_REACTION above already excluded every byte2 bit in0x0e.
        if((col[1]>>5)>=2&&!(u->status_flags&STATUS_RIDER))return 0;
    }
    return 1;
}
#else
#define reaction_live(who) ((u8)M.target_query(who,254))
#define reaction_critical_ready(who) ((u8)M.target_query(who,253))
#endif
#endif
