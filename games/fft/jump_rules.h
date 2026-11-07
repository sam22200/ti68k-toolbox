// Jump selectors share one command; learned rows extend horizontal/vertical reach.
#ifndef FFT_JUMP_RULES_H
#define FFT_JUMP_RULES_H
enum { JUMP_FIRST=394, JUMP_END=406, JUMP_FORMULA=100 };
static inline u8 jump_ability(AbId ab){return (u16)(ab-JUMP_FIRST)<12;}
static inline u8 unit_airborne(const Unit *u){return u->status_flags&STATUS_AIRBORNE;}
#if MOD_CORE
static u16 jump_limits(u8 who)
{
    const Unit *u=&st.unit[who];const Ability *b=&abilities[JUMP_FIRST];
    u16 learned=u->learned&0x0fff;u8 range=1,vertical=2;
    if(u->job==SOURCE_JOB_MIME)return 0x1008;
    // Generic Jump's twelve source upgrade slots are contiguous and ordered.
    for(;learned;learned>>=1,b++)if(learned&1){
        if(b->range>range)range=b->range;
        if(b->vert>vertical)vertical=b->vert;
    }
    return (u16)vertical<<8|range;
}
#else
#define jump_limits(who) M.target_query(who,252)
#endif
// Native adaptation: target the same terrain layer, with no bridge/ceiling model.

#if MOD_CORE
static u8 jump_reaches(u8 who,u8 origin,u8 cell)
{
    u16 limits=jump_limits(who);s16 dx,dz,h;
    if(origin==cell||!map_tiles[cell].walk||node_layer(origin)!=node_layer(cell))return 0;
    dx=(s16)TILE_X[cell]-TILE_X[origin];dz=(s16)TILE_Z[cell]-TILE_Z[origin];
    h=(s16)map_tiles[cell].stand-map_tiles[origin].stand;
    return (dx<0?-dx:dx)+(dz<0?-dz:dz)<=(u8)limits&&(h<0?-h:h)<=(limits>>8);
}
#else
#define jump_reaches(who,origin,cell) ((u8)charge_query(who,origin,JUMP_FIRST,cell,0xffff))
#endif
#endif
