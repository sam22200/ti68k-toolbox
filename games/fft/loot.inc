// Cold pickup effects, PARTY-owned in the native chapter.
void pick_up(u8 who, u8 obj)
{
    Unit *u = &st.unit[who], *o = &st.unit[obj];
    st.abil=NO_ABIL; // a loot message supersedes any movement-benefit resource label
    if (o->dc == DC_CRYSTAL) {
        u->hp = u->max_hp; u->mp = u->max_mp;
        st.pick = 0xFF;
    } else {
        u8 k, n = 0, eq[EQ_N];
        for (k = 0; k < EQ_N; k++) if (o->eq[k] != 0xFF) eq[n++] = o->eq[k];
        st.pick = eq[(u32)rand15() * n >> 15];
#ifdef FFT_CHAPTER
        if (st.nfound < NUNIT) st.found[st.nfound++] = st.pick;   // the party's (save.inc)
#endif
    }
    o->dc = DC_GONE; o->x = 0xFF;
    st.mode = M_PICK; st.pick_t = 0;
}

