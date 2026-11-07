#ifdef FFT_CHAPTER
#include "weapon_range_mask.inc"
#endif
void attack_range(u8 who)
{
#ifdef FFT_CHAPTER
    hl_seq++;memset(st.reach,0xff,sizeof st.reach);
    if(unit_can_act(&st.unit[who]))weapon_range_mask(who);
#else
#if !MOD_CORE
    static const s8 dir_x[4]={1,-1,0,0},dir_z[4]={0,0,1,-1};
#endif
    const Unit *u=&st.unit[who];s16 h=map_tiles[UNIT_CELL(u)].stand;u8 d;
    hl_seq++;memset(st.reach,0xff,sizeof st.reach);
    for(d=0;d<4;d++){
        u16 cell=node_at(u->x+dir_x[d],u->z+dir_z[d],0);const Tile *t;
        if(cell>=MAP_N)continue;t=&map_tiles[cell];
        if(t->walk&&t->stand+6>=h&&t->stand<=h+5)st.reach[cell]=1;
    }
#endif
}
