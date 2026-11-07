#ifndef FFT_RESTORATION_AI_H
#define FFT_RESTORATION_AI_H
// Immediate adapted MP utility only for units with an enabled learned cast.
static u8 restore_magic_user(u8 t)
{
    const Unit *u=&st.unit[t];u8 k;
    for(k=0;k<SET_N;k++)if((u->learned&((LearnBits)1<<k))&&
        (abilities[SET_ABILITY(u->set,k)].flags&AF_CAST))return 1;
    return 0;
}
#endif
