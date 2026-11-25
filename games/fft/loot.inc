// FFT's crystal offers the dead unit's abilities as well as its HP and MP, and
// taking one teaches EVERY ability it lists, not only the one picked -- for the
// jobs the taker has unlocked. A battle Unit carries the bits of its own two
// skill sets, so a crystal's set has to match one of them for its bits to have
// anywhere to land; a set the taker has never held is exactly the job FFT would
// refuse to teach from. The reaction/support byte is left alone: it packs Squire
// JP beside its bits and cannot be OR-ed.
static u8 crystal_abilities(Unit *u, const Unit *o)
{
    LearnBits n;
    u8 got = 0;
    if (o->set == u->set && (n = (LearnBits)(o->learned & ~u->learned)) != 0) { u->learned |= n; got = 1; }
#ifdef FFT_CHAPTER
    if (o->set == u->set2 && (n = (LearnBits)(o->learned & ~u->learned2)) != 0) { u->learned2 |= n; got = 1; }
#endif
    return got;
}

// Cold pickup effects, PARTY-owned in the native chapter.
void pick_up(u8 who, u8 obj)
{
    Unit *u = &st.unit[who], *o = &st.unit[obj];
    st.abil=NO_ABIL; // a loot message supersedes any movement-benefit resource label
    if (o->dc == DC_CRYSTAL) {
        u->hp = u->max_hp; u->mp = u->max_mp;
        st.pick = crystal_abilities(u, o) ? PICK_TAUGHT : 0xFF;
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

