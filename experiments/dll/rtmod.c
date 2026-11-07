// A module for rtdll.c: draws through the context and changes the program's state.
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#include "rtmod.h"

DLL_INTERFACE
u16 mod_frame(const ModCtx *c);
DLL_ID RTMOD_ID
DLL_VERSION 1,0
DLL_EXPORTS mod_frame
DLL_IMPLEMENTATION

u16 mod_frame(const ModCtx *c)
{
    char s[8]; u16 v = ++*c->counter, i = 5;
    s[i] = 0;
    do s[--i] = (char)('0' + v % 10); while ((v /= 10) && i);
    c->rect(20, 30, 120, 40, C_BLACK);
    c->rect(22, 32, 116, 36, C_WHITE);
    c->text(30, 44, s + i, F_SMALL, C_BLACK);
    return v;
}
