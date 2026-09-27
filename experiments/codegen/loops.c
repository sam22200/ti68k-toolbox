// Codegen check: which loop forms compile to dbra? (m68k-coff-tigcc-gcc -Os -mshort -S loops.c)
extern unsigned char buf[];
void l_up(void)     { short i; for (i = 0; i < 100; i++) buf[i] = 0; }
void l_down(void)   { short i; for (i = 99; i >= 0; i--) buf[i] = 0; }
void l_ptr(void)    { unsigned char *p = buf; short n = 100; while (n--) *p++ = 0; }
void l_ptr2(void)   { unsigned char *p = buf; short n = 99; do *p++ = 0; while (--n >= 0); }
void l_ptr3(void)   { unsigned char *p = buf; short n = 99; do *p++ = 0; while (--n != -1); }
