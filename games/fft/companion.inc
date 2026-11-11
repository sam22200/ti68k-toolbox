// The companion image (MOD_COMPANION, campaign/COMPANION.md): the rules of non-canonical jobs,
// monsters and bosses, shared by every chapter. One entry, special(a, t, ab, query), reached
// from the resident and from every zone image through ModCtx. On the TI it runs from its own
// heap block (fft.c: special); on the PC it is compiled into the program.
#define COMPANION_PROBE 0          // mechanism check: a value built from every argument
#if MOD_PART(MOD_COMPANION)
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
u16 mod_entry(u8 op,u8 a){(void)a;return companion_query(M.cast_actor,M.cast_ref,M.cast_ability,op);}
#endif
