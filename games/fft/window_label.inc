// Cold cached-row labels; same text/availability policy in PC and RESULTS1.
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define window_defend CONTOUR_REF_st_names[0]
#else
#define window_defend "Defend"
#endif
static u8 window_label(char *s)
{
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define entries CONTOUR_REF_entries
#else
static const char *const entries[MN_N] = { "Move", "Act", "Wait", "Status", "Auto-battle" };
#endif


#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define automatic CONTOUR_REF_automatic
#else
static const char *const automatic[3] = { "Manual", "This unit", "All allies" };
#endif

    u8 k = W->win_n - 2, n = 0, m = W->win_k[0], c = C_BLACK;
    const Unit *u = &st.unit[W->win_k[2]];
    if (m == M_QUIT) {
        n = put_s(s, 0, k == 0 ? "Quit?" : k == 1 ? "Yes" : "No");
    } else if (m == M_ORDER) {                     // the next turns, the active unit first;
        const Unit *t = &st.unit[st.turns[k]];     // enemies in dark grey
        s[0] = (char)('1' + k); s[1] = ' ';
        n = put_s(s, 2, unit_name(st.turns[k]));
        if (t->team == TEAM_ENEMY) c = C_DGRAY;
    } else if (m == M_AUTO) {
        n = put_s(s, 0, automatic[k]);
    } else if (m == M_MENU) {
        n = put_s(s, 0, entries[k]);
        if (k <= MN_ACT && W->win_k[1] & (1 << k)) c = C_DGRAY;
    } else if (m == M_ACTS) {
        #ifdef FFT_CHAPTER
        if(k==2+u->defend)n=put_s(s,0,"Equip Change");else
#endif
        n = put_s(s, 0, k == 2 ? window_defend : k ? SKILL_NAME(u->set) : "Attack");
        if (k == 1 && W->win_k[1]) c = C_DGRAY;
    } else if (m == M_SKILLS) {                    // an item: how many are left
        AbId ab = known(W->win_k[2], k + W->win_k[1]);
        n = put_s(s, 0, ABIL_NAME(ab));
        if (abilities[ab].flags & AF_ITEM &&
#ifdef FFT_CHAPTER
            unit_home_team(u)!=TEAM_ENEMY
#else
            u->team==TEAM_PLAYER
#endif
        ) {
            s[n++] = ' '; n = put_n(s, n, ab>=512?battle_save.inv[abilities[ab].bag]:st.bag[abilities[ab].bag]);
        }
        if (abilities[ab].kind!=AB_MATH_SELECTOR&&!can_use(W->win_k[2], ab)) c = C_DGRAY;
    } else if (m == M_ACT) {
        n = put_s(s, 0, st.abil == NO_ABIL ? "Attack" : st.abil == ABIL_DEFEND ? window_defend : ABIL_NAME(st.abil));
    } else if (m == M_PICK) {                      // a crystal or a chest picked up
        #ifdef FFT_CHAPTER
        if(W->trap_message){
            const char *label=W->trap_message==1?"Degenerator":W->trap_message==2?"Deathtrap":W->trap_message==3?"Sleeping gas":"Steel needle";
            n=put_s(s,0,label);
        }else if(st.pick==254&&(u16)(st.abil-493)<4U){n=put_s(s,0,st.abil==493?"HP+":st.abil==494?"MP+":st.abil==495?"EXP+":"JP+");n=put_n(s,n,st.dmg);}
        else
#endif
        if (st.pick == 0xFF) n = put_s(s, 0, "HP MP full");
        else { n = put_s(s, 0, "Got "); n = put_s(s, n, items[st.pick].name); }
    }
    s[n] = 0;
    return c;
}
