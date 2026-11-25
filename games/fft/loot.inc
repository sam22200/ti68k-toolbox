// A crystal offers the dead unit's abilities as well as its HP and MP, and the
// guide states the rule plainly (fft_guides/battle_mecanics.txt [6.1]): "you
// will gain ALL of the abilities on the list, not just the one you select" --
// the menu's choice is cosmetic. Its three conditions:
//
//   1. the dead unit must have had the ability learned -> o->learned;
//   2. the taker must have access to the EXACT job it was learned from, which a
//      skill set identifies: the guide's own example is that a (34) Holy Knight
//      cannot take Lightning Stab from a (05) Holy Knight;
//   3. "only humans can transfer abilities via crystal".
//
// Two ways this is more generous than the source, both deliberate. FFT rolls the
// list with a per-ability probability, so a crystal shows only some of what the
// dead unit knew; we offer all of them, because that table is a kilobyte of data
// for a difference the player cannot see. And we teach without the menu, since
// the menu changes nothing once every listed ability is granted anyway.
//
// The reaction/support byte is left alone: it packs Squire JP beside its bits.
static u8 crystal_abilities(Unit *u, const Unit *o)
{
    LearnBits n;
    u8 got = 0;
    if (unit_monster(u) || unit_monster(o)) return 0;
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

