// The companion's queries (companion.inc, campaign/COMPANION.md): special(a, t, ab, query).
#ifndef FFT_COMPANION_H
#define FFT_COMPANION_H
#define COMPANION_PROBE 0          // mechanism check: a value built from every argument
#define COMPANION_AMOUNT 1         // the formula's amount (HP; the core applies kind and sign)
// Formulas whose arithmetic lives in the companion; the core keeps their generic rules.
static inline u8 companion_formula(u8 f){return f==0x4E;}
#endif
