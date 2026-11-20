// The companion image (MOD_COMPANION, campaign/COMPANION.md): the rules of non-canonical jobs,
// monsters and bosses, shared by every chapter. One entry, special(a, t, ab, query), reached
// from the resident and from every zone image through ModCtx. On the TI it runs from its own
// heap block (fft.c: special); on the PC it is compiled into the program.
#if MOD_PART(MOD_COMPANION)
// Only rules the battle core lacks; it never copies the core's arithmetic: it calls the
// resident through ModCtx (campaign/COMPANION.md).
static u16 __attribute__((unused)) companion_query(u8 a,u8 t,AbId ab,u8 query)
{
    if(query==COMPANION_PROBE)return (u16)(((u16)a<<12)^((u16)t<<8)^ab);
    return 0;
}
#endif
#if MOD_CORE && !defined(FFT_MODULES)
static u16 __attribute__((unused)) special(u8 a,u8 t,AbId ab,u8 query){return companion_query(a,t,ab,query);}
#endif
#if !MOD_CORE && MOD_PART(MOD_COMPANION)
#include "campaign/save.h"
u16 mod_entry(u8 op,u8 a)
{
    (void)a;if(op==254)return save_load(M.save_out);
    // The wide monster bodies draw from here: their renderer is far too big for the
    // resident, and this image is never swapped out of its block (fft.c: companion_body).
    if(op==253)return draw_monster_body(M.body_x,M.body_y,M.body_rot,a,M.body_cover);
    return companion_query(M.cast_actor,M.cast_ref,M.cast_ability,op);
}
#endif
