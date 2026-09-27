// Codegen check: cost of signed vs unsigned and of small types (m68k-coff-tigcc-gcc -Os -mshort -S types.c)
short          div4_s(short x)            { return x / 4; }        // signed: needs rounding fix-up
unsigned short div4_u(unsigned short x)   { return x / 4; }        // unsigned: one shift
short          sar4_s(short x)            { return x >> 2; }       // explicit shift (rounds to -inf)
short          mod8_s(short x)            { return x % 8; }        // signed modulo
unsigned short mod8_u(unsigned short x)   { return x % 8; }        // unsigned: one and
short          div10_s(short x)           { return x / 10; }       // real division
unsigned short div10_recip(unsigned short x) { return (unsigned short)(((unsigned long)x * 6554) >> 16); }
short          range_s(short x)           { return x >= 10 && x < 50; }                 // two compares
short          range_u(short x)           { return (unsigned short)(x - 10) < 40; }     // one compare
extern unsigned char tabc[256]; extern short tabs[256];
short          sum_uchar(unsigned char n) { unsigned char i; short s = 0; for (i = 0; i < n; i++) s += tabc[i]; return s; }
short          sum_short(unsigned short n){ unsigned short i; short s = 0; for (i = 0; i < n; i++) s += tabc[i]; return s; }
short          idx_uchar(unsigned char a) { return tabs[a]; }      // byte index: zero-extend + shift
short          idx_short(unsigned short a){ return tabs[a & 255]; }
unsigned char  add_uchar(unsigned char a, unsigned char b) { return a + b; }   // wraps for free
long           mul_long_const(long x)     { return x * 30; }
short          mul_short_const(short x)   { return x * 30; }
short          mul_shifts(short x)        { return (x << 5) - (x << 1); }
