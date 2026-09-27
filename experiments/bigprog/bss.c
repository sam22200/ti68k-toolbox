#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
unsigned short buf[5000];
void _main(void){ unsigned short i, nz=0; for(i=0;i<5000;i++) if(buf[i]) nz++; buf[4999]++; clrscr(); printf("nonzero %u\nlast %u", nz, buf[4999]); ngetchx(); }
