#ifdef FFT_CHAPTER
#include "weapon_range_mask.inc"
#endif
// walkable, within the vertical tolerance, the user's own only with AF_SELF
void skill_range(u8 who, AbId ab)
{
#ifdef FFT_CHAPTER
    if(!unit_can_act(&st.unit[who])){hl_seq++;memset(st.reach,0xff,sizeof st.reach);return;}
#endif
    const Ability *b = &abilities[ab];
    const Unit *u = &st.unit[who];
    s16 h=map_tiles[UNIT_CELL(u)].stand;u16 cell;
    u8 range=
#ifdef FFT_CHAPTER
        ability_range(who,ab);
#else
        b->range;
#endif
#ifdef FFT_CHAPTER
    if(b->formula==99)range=u->move;
#endif
#ifdef FFT_CHAPTER
    const u8 *raw=cj_range(common,ab);u8 cardinal=raw&&(raw[3]&16);
#endif
    hl_seq++;
    memset(st.reach, 0xFF, sizeof st.reach);
#ifdef FFT_CHAPTER
    if(b->flags&AF_CHARGE){weapon_range_mask(who);return;}
#endif
    for(cell=0;cell<MAP_N;cell++) {
            s16 dx=TILE_X[cell]-u->x,dz=TILE_Z[cell]-u->z,d=(dx<0?-dx:dx)+(dz<0?-dz:dz);
            const Tile *t=&map_tiles[cell];
#ifdef FFT_CHAPTER
            if(jump_ability(ab)){if(jump_reaches(who,UNIT_CELL(u),(u8)cell))st.reach[cell]=1;continue;}
            if(cardinal&&dx&&dz)continue;
#endif
            if(d>range||(cell==UNIT_CELL(u)&&!(b->flags&AF_SELF))||!t->walk)continue;
            if (b->vert != 0xFF && (t->stand > h ? t->stand - h : h - t->stand) > b->vert) continue;
            st.reach[cell]=1;
        }
}
