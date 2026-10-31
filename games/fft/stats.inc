// Same stat implementation in portable code and its native callers' images.
void unit_stats(Unit *u)
{
    const Job *j = &jobs[u->job];
    u16 s[5];
    u8 i, k;
    for (i = 0; i < 5; i++) {
        u32 v = u->raw[i];
        u8 g = j->growth[i] ? j->growth[i] : 1;
        for (k = 2; k <= u->level; k++) v += v / (u16)(g + k - 1);
        if (v > 0xFFFFFFUL) v = 0xFFFFFFUL;
        v = v * j->mult[i] / 100 >> 14;
        s[i] = v ? (u16)v : 1;
    }
    u->max_hp = s[0] > 999 ? 999 : s[0];
    u->max_mp = s[1] > 999 ? 999 : s[1];
    u->move = j->move; u->jump = j->jump;
    for (i = 0; i < EQ_N; i++) {                   // helmet / armour HP and MP, item attributes
        const Item *it;
        if (u->eq[i] == 0xFF) continue;
        it = &items[u->eq[i]];
        u->max_hp += it->hp; u->max_mp += it->mp;
        s[2] += it->sp; s[3] += it->pa; s[4] += it->ma;
        u->move += it->move; u->jump += it->jump;
    }
    if (u->max_hp > 999) u->max_hp = 999;
    if (u->max_mp > 999) u->max_mp = 999;
    if (u->jump > 7) u->jump = 7;
    u->sp = (u8)(s[2] > 50 ? 50 : s[2]);
    u->pa = (u8)(s[3] > 99 ? 99 : s[3]);
    u->ma = (u8)(s[4] > 99 ? 99 : s[4]);
    u->hp = u->max_hp; u->mp = u->max_mp;
#ifdef FFT_CHAPTER
    if(W&&st.mode==M_AREA)W->area_ui.dirty=1;
    // Cold pointer lookup preserves the hot Unit stride and supports scratch Units.
    for(i=0;i<NUNIT;i++)if(u==&st.unit[i]){
        cj_affinity(common,u->job,u->eq,st.affinity[i]);break;
    }
#endif
}
