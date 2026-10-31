// A binding stores a compact tile or a stable actor slot, never screen pixels.
#ifndef FFT_CAST_TARGET_H
#define FFT_CAST_TARGET_H
static inline u16 cast_target_cell(u8 follow,u8 ref)
{
    const Unit *u;
    if(!follow)return ref<MAP_N?ref:MAP_N;
    if(ref>=NUNIT)return MAP_N;
    u=&st.unit[ref];
    if(u->x>=MAP_W||u->z>=MAP_H||DC_OBJ(u->dc)||
       (core_status_current(&u->statuses,0)&4))return MAP_N;
    return UNIT_CELL(u)<MAP_N?UNIT_CELL(u):MAP_N;
}
#if !defined(FFT_MODULES) && (MOD_CORE || MOD_PART(MOD_COMMAND) || MOD_PART(MOD_AREA_UI))
static u8 cast_can_order(u8 a,u8 ref,AbId ab,u8 follow)
{
    const Ability *b;const u8 *r;u16 c;s16 dx,dz,h;
    if(follow>1||!action_available(a,a)||!can_use(a,ab))return 0;
    b=&abilities[ab];r=cj_range(common,ab);
    if(!(b->flags&AF_CAST)||!r||(r[5]&4))return 0;
    if((b->flags&AF_AREA)&&(!W||!W->surface||!cast_area_supported(b,r)))return 0;
    c=cast_target_cell(follow,ref);if(c>=MAP_N||!map_tiles[c].walk)return 0;
    if(c==UNIT_CELL(&st.unit[a])&&!(b->flags&AF_SELF))return 0;
    dx=(s16)TILE_X[c]-st.unit[a].x;dz=(s16)TILE_Z[c]-st.unit[a].z;
    if((dx<0?-dx:dx)+(dz<0?-dz:dz)>b->range)return 0;
    h=(s16)map_tiles[c].stand-map_tiles[UNIT_CELL(&st.unit[a])].stand;
    return b->vert==255||(h<0?-h:h)<=b->vert;
}
#endif
#endif
