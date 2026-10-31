// walkable, within the vertical tolerance, the user's own only with AF_SELF
void skill_range(u8 who, AbId ab)
{
#ifdef FFT_CHAPTER
    if(!unit_can_act(&st.unit[who])){hl_seq++;memset(st.reach,0xff,sizeof st.reach);return;}
#endif
    const Ability *b = &abilities[ab];
    const Unit *u = &st.unit[who];
    s16 h=map_tiles[UNIT_CELL(u)].stand;u16 cell;
    hl_seq++;
    memset(st.reach, 0xFF, sizeof st.reach);
    for(cell=0;cell<MAP_N;cell++) {
            s16 dx=TILE_X[cell]-u->x,dz=TILE_Z[cell]-u->z,d=(dx<0?-dx:dx)+(dz<0?-dz:dz);
            const Tile *t=&map_tiles[cell];
            if(d>b->range||(cell==UNIT_CELL(u)&&!(b->flags&AF_SELF))||!t->walk)continue;
            if (b->vert != 0xFF && (t->stand > h ? t->stand - h : h - t->stand) > b->vert) continue;
            st.reach[cell]=1;
        }
}
