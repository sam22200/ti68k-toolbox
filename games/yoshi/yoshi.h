#ifndef YOSHI_PROBE_H
#define YOSHI_PROBE_H
#include "../../runtime/core/rt.h"
#include "actors.h"
#include "damage.h"
#include "items.h"

/* Terrain traversal and controlled probes; original pixels, signed 8.8 speeds. */
typedef struct {
    u16 x, x_sub, y, sub, camx, camy, anchor_x;
    s16 vx, vy;
    u8 grounded, flutter, phase_timer, cooldown, jump, skid, anchor;
    u8 native, won, angle, head_timer;
    YoshiInteraction action;
    YoshiBaby baby;
    YoshiItems items;
} YoshiMovement;

extern YoshiMovement yoshi;
void yoshi_reset(void);
void yoshi_step(s16 direction, u8 held, u8 pressed);
void yoshi_drop(u16 x, u16 y);
#endif
