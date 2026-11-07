// Shared range/menu preparation; native MOVEMENT owns the whole cold transition.
static u8 aim(AbId ab)
{
    if (W) W->upd_k += AIM_K;
    st.mode = M_AIM; st.abil = ab;
#ifdef FFT_CHAPTER
    if(st.unit[st.act].set==21&&ab!=NO_ABIL){
#if !MOD_CORE
        return 3;
#else
        math_aim();return 0;
#endif
    }
    if(ab<512&&cast_area_supported(&abilities[ab],cj_range(common,ab))&&!st.ai)st.mode=M_CAST_RANGE;
#endif
    if (W) W->win_k[0] = 0;
    if (ab == NO_ABIL || (abilities[ab].flags&AF_CHARGE) || (u8)(abilities[ab].formula - 0x2b) <= 1 || abilities[ab].formula == 0x25){
#if !MOD_CORE
        return 2; // resident resumes in TURN2 after MOVEMENT has returned
#else
        attack_range(st.act);
#endif
    }else skill_range(st.act, ab);
    return 0;                        // the preview: for this ability
}
