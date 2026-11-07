// Shared target validation and projected-tile picking; no overlay loads.
static u8 aim_ok(u8 t)
{
    const Unit *u = &st.unit[t];
#ifdef FFT_CHAPTER
    if(unit_airborne(u))return 0;
    if(restore_ability(st.abil))return restore_applicable(t,st.abil);
    if(st.abil<512&&(magic_formula(abilities[st.abil].formula)||abilities[st.abil].kind==AB_CLEANSE))return magic_applicable(t,st.abil);
#endif
    if (st.abil != NO_ABIL && abilities[st.abil].kind == AB_REVIVE) return !u->hp;
    return u->hp != 0;
}

static u8 pickable(u8 c, u8 aim)
{
    u8 o;
    if (!aim) return st.reach[c] != 0xFF && st.reach[c] > 0;
    if(st.mode==M_CAST_RANGE)return st.reach[c]==1;
    o = unit_at_cell(c);
    return st.reach[c] == 1 && o && aim_ok(o - 1);
}

// [2nd] on a tile that cannot be picked: the tile drawn on the same pixels (in every view a
// canal tile at height 0 and the street tile 2 units higher one tile diagonally in front of
// it share 21-27 spots) when that one can be (+ 1; 0: none)
static u16 twin(u8 c, u8 aim)
{
    s16 u, v, x, y;
    u16 i;
    view_of(st.rot,TILE_X[c],TILE_Z[c],&u,&v);
    x = scene_x(st.rot, u, v); y = scene_y(u, v) - map_tiles[c].stand * (HU / 2);
    for (i = 0; i < MAP_N; i++) {
        if (i == c) continue;
        view_of(st.rot,TILE_X[i],TILE_Z[i],&u,&v);
        if (scene_x(st.rot, u, v) == x && scene_y(u, v) - map_tiles[i].stand * (HU / 2) == y && pickable(i, aim))
            return i + 1;
    }
    return 0;
}
