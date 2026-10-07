#ifndef YOSHI_ITEMS_H
#define YOSHI_ITEMS_H
#include "../../runtime/core/rt.h"
#define YI_EGGS 6
#define YI_TRAIL 64
typedef struct { u16 x,y; } YoshiPoint;
typedef struct {
    u16 x,y;
    s16 vx,vy;
    u8 xs,ys,life,bounces;
} YoshiEgg;
typedef struct {
    YoshiPoint trail[YI_TRAIL], followers[YI_EGGS];
    YoshiEgg shots[YI_EGGS];
    u16 idle, hud[9];
    u8 collected[3], count, enabled, head, reserve, aim, phase, reverse, throwing, locked, aim_facing;
} YoshiItems;
void items_reset(void);
void items_scenario(u16 n);
void items_step(void);
void items_render(void);
void items_aim_render(void);
u8 items_collect(u16 x,u16 y,u16 width,u16 height);
#if defined(YJ_ITEM_TRACE) || !defined(__m68k__)
void items_trace(void (*emit)(u32));
#endif
#endif
