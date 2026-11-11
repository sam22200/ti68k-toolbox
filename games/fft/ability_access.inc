u8 can_use(u8 who, AbId ab)
{
#ifdef FFT_CHAPTER
    if (who>=NUNIT || ab >= NABIL || abilities[ab].kind == 255 || !unit_can_act(&st.unit[who]) || skill_blocked(who,ab)) return 0;
    if(abilities[ab].kind==AB_THROW_SELECTOR||abilities[ab].kind==AB_MATH_SELECTOR)return 0;
    if(MIME_ACTIVE(who))return 1; // accepted source action; no learning or stock requirement
    if((u16)(ab-0x100)<0x70U||unit_monster(&st.unit[who])){   // a monster: its own set only
        u8 k;if(!unit_monster(&st.unit[who]))return 0;
        for(k=0;k<4;k++)if(SET_ABILITY(st.unit[who].set,k)==ab)return (st.unit[who].learned&(1U<<k))!=0;
        return 0;
    }
    if(jump_ability(ab))return st.unit[who].set==18;
    if(ab>=512)return throw_available(who,ab);
    if(st.unit[who].set==21)return math_available(who,ab);
    // Other dual weapon-skill families still require their own source fixtures.
    if(hand_count(who)==2&&(abilities[ab].flags&AF_WEAPON)&&!(abilities[ab].flags&AF_CHARGE))return 0;
    if((abilities[ab].formula==0x24||draw_out_formula(abilities[ab].formula))&&magic_heal(who,who,ab,4)!=ab)return 0;
#endif
    return !(abilities[ab].flags & AF_ITEM) ||
#ifdef FFT_CHAPTER
        unit_home_team(&st.unit[who])==TEAM_ENEMY ||
#else
        st.unit[who].team!=TEAM_PLAYER ||
#endif
        st.bag[abilities[ab].bag];
}

