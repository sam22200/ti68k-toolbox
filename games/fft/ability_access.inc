u8 can_use(u8 who, AbId ab)
{
#ifdef FFT_CHAPTER
    if (ab >= 512 || abilities[ab].kind == 255 || !unit_can_act(&st.unit[who]) || skill_blocked(who,ab)) return 0;
    // Other dual weapon-skill families still require their own source fixtures.
    if(hand_count(who)==2&&(abilities[ab].flags&AF_WEAPON)&&!(abilities[ab].flags&AF_CHARGE))return 0;
#endif
    return !(abilities[ab].flags & AF_ITEM) || st.unit[who].team != TEAM_PLAYER || st.bag[abilities[ab].bag];
}

