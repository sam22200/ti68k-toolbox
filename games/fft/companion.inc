// The companion image (MOD_COMPANION, campaign/COMPANION.md): the rules of non-canonical jobs,
// monsters and bosses, shared by every chapter. One entry, special(a, t, ab, query), reached
// from the resident and from every zone image through ModCtx. On the TI it runs from its own
// heap block (fft.c: special); on the PC it is compiled into the program.
#if MOD_PART(MOD_COMPANION)
#include "magic_stages.h"          // the resident's magical XA/element stages, this image's copy
// Formula 0x4E (monster breaths, bombs, Souls): battle_formula_damage_ma_times_y, i.e.
// formula 0x08 without Faith: magical evade (core), MA x Y with the same element strengthen,
// support/status and absorb/null stages, the same status-proc draw (core), no status here.
static u16 truth_amount(u8 a,u8 t,AbId ab)
{
    const Ability *b=&abilities[ab];const Unit *ut=&st.unit[t];u8 element=b->element;s16 raw;
    if((st.affinity[t][1]&element)||((element&8)&&(core_status_current(&ut->statuses,2)&0x40)))return 0;
    raw=(s16)muls16(magic_xa(a,t,element,1,8),b->y);
    raw=magic_element_value(t,element,raw);
    return raw>999?999:(u16)raw;             // negative (absorbed) stays signed, as formula 0x08
}
static u16 __attribute__((unused)) companion_query(u8 a,u8 t,AbId ab,u8 query)
{
    if(query==COMPANION_PROBE)return (u16)(((u16)a<<12)^((u16)t<<8)^ab);
    if(query==COMPANION_AMOUNT&&ab<NABIL&&abilities[ab].formula==0x4E)return truth_amount(a,t,ab);
    return 0;
}
#endif
#if MOD_CORE && !defined(FFT_MODULES)
static u16 __attribute__((unused)) special(u8 a,u8 t,AbId ab,u8 query){return companion_query(a,t,ab,query);}
#endif
#if !MOD_CORE && MOD_PART(MOD_COMPANION)
u16 mod_entry(u8 op,u8 a){(void)a;return companion_query(M.cast_actor,M.cast_ref,M.cast_ability,op);}
#endif
