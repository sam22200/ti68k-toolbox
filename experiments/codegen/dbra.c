#define USE_TI89
#include <tigcclib.h>
void a(unsigned char *p, short n) { while (n--) *p++ = 0; }
void b(unsigned char *p, short n) { short i; for (i = n - 1; i >= 0; i--) *p++ = 0; }
void c(unsigned char *p, unsigned short n) { while (n--) *p++ = 0; }
void d(unsigned char *p, short n) { for (--n; n != -1; n--) *p++ = 0; }
