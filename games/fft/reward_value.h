#ifndef FFT_REWARD_VALUE_H
#define FFT_REWARD_VALUE_H
static u8 earned_exp(u8 a,u8 t,u8 kill)
{
    Unit *ua = &st.unit[a], *ut = &st.unit[t];
    s16 e = (kill ? (ut->kos?20 / ut->kos:20) : 10) + ut->level - ua->level;
    if(e<1)e=1;
    return e>99?99:(u8)e;
}
#endif
