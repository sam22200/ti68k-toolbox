// The context a module (TIGCC DLL) receives from the runtime program that loaded it: a module
// cannot link to the program's symbols, so data and services travel as pointers.
#ifndef RTMOD_H
#define RTMOD_H
#include "../../runtime/core/rt.h"
typedef struct {
    u16 *counter;                                           // the program's state
    void (*rect)(s16 x, s16 y, s16 w, s16 h, u8 c);         // its runtime's drawing
    void (*text)(s16 x, s16 y, const char *s, u8 font, u8 c);
} ModCtx;
#define RTMOD_ID 0x46465431UL                               // "FFT1"
#endif
