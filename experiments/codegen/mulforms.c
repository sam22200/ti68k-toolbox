// Codegen check: which C forms of a 16-bit multiply compile to a single muls/mulu at -Os and -O2?
// (m68k-coff-tigcc-gcc -O2 -mshort -S mulforms.c) Result: all but f_loop at -O2 (→ __mulsi3).
short  f_short(short a, short b)            { return a * b; }                 // 16-bit result
long   f_widen(short a, short b)            { return (long)a * b; }           // 16x16 -> 32
long   f_widen2(short a, short b)           { return (long)a * (long)b; }
unsigned long f_uwiden(unsigned short a, unsigned short b) { return (unsigned long)a * b; }
long   f_loop(short a, short b, long n)     { long i, acc = 0; for (i = 0; i < n; i++) acc += (long)(short)((short)i ^ a) * b; return acc; }
static inline long muls16(short a, short b) { long r; asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r; }
long   f_asm_loop(short a, short b, long n) { long i, acc = 0; for (i = 0; i < n; i++) acc += muls16((short)i ^ a, b); return acc; }
long   f_const(short a)                     { return (long)a * 30; }          // constant
long   f_div(unsigned short a, unsigned short b) { return a / b; }
long   f_div32(long a, long b)              { return a / b; }
