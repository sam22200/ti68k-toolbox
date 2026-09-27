// Common header for the AI experiments: fixed-size types, PRNG, and the timing harness.
// The pure logic compiles both with GCC4TI (TI-89, __m68k__) and with the host gcc, so results
// (node counts, checksums) can be checked on the PC first and must match on the calculator.
// TI timing = bench7.c method: auto-int 1 replaced by a 256 Hz tick counter, int 5 silenced,
// cycles = ticks * 46875 (12 MHz / 256).
#ifndef AI_H
#define AI_H

#ifdef __m68k__
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;
typedef long s32;
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
#endif

// wyhash16 (performance §4): one mulu.w, good quality.
static u16 wy = 1;
static u16 rnd16(void)
{
    u32 h;
    wy += 0xfc15;
    h = (u32)wy * 0x2ab;
    return (u16)(h >> 16) ^ (u16)h;
}
// Lemire's fastrange: [0, n) without division (mulu.w + swap).
// A function, not a macro: with a u8 expression as argument GCC widened it and called __mulsi3.
static u16 RND(u16 n) { return (u16)(((u32)rnd16() * n) >> 16); }

#ifdef __m68k__
static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
static INT_HANDLER old1, old5;
static void timer_on(void)
{
    old1 = GetIntVec(AUTO_INT_1); old5 = GetIntVec(AUTO_INT_5);
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
}
static void timer_off(void) { SetIntVec(AUTO_INT_1, old1); SetIntVec(AUTO_INT_5, old5); }
// Starts on a tick edge; TICKS() is the elapsed count.
static unsigned long t0;
static void tstart(void) { unsigned long s = ticks; while (ticks == s); t0 = ticks; }
#define TICKS() (ticks - t0)
#define CYC(t) ((unsigned long)(t) * 46875UL)
#endif

#endif
