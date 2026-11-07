void compute_reach(u8 who)
{
    u16 i;
#ifdef FFT_CHAPTER
    if(!unit_can_move(&st.unit[who])){
        hl_seq++;memset(st.reach,0xff,sizeof st.reach);st.reach[UNIT_CELL(&st.unit[who])]=0;
        W->from_who=0;return;
    }
#endif
    hl_seq++;
    bfs(who, bfs_from);                           // the steps kept for make_path
    W->from_who = who + 1;
    W->upd_k += REACH_K;
    for (i = 0; i < NUNIT; i++)                   // can't stop on a unit (a crystal or a chest:
        if (i != who && st.unit[i].x < MAP_W && st.unit[i].z < MAP_H
            && unit_at_cell(UNIT_CELL(&st.unit[i])))       // can, it is picked up)
            st.reach[UNIT_CELL(&st.unit[i])] = 0xFF;
    for (i = 0; i < MAP_N; i++)            // nor on a tree (crossed only)
        if (map_tiles[i].walk == 2
#ifdef FFT_CHAPTER
            ||(W->move_kind&&(W->hw[i]&64))
#endif
            ) st.reach[i] = 0xFF;
}
