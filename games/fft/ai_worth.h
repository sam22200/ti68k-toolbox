// Immediate HP/status utility shared by candidate ranking and cold weapon plans.
#ifndef FFT_AI_WORTH_H
#define FFT_AI_WORTH_H
static s16 worth(const Unit *u, u16 hp, u8 status)
{
    s16 s;
    if (!hp) return -192;
    s = divs32_16((long)hp << 7, (short)u->max_hp);
    if ((u16)(hp * 5) <= u->max_hp) s -= 32;
    if (status & ST_POISON) s -= 26;
    if (status & ST_DARK) s -= 64;
    return s;
}

#endif
