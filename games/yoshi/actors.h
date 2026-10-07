#ifndef YOSHI_ACTORS_H
#define YOSHI_ACTORS_H
#include "../../runtime/core/rt.h"
#define YA_COUNT 5
typedef struct {
    u16 x, y, home;
    s16 vx, vy;
    u8 sub, state, awake, timer, shot, defeated;
} YoshiActor;
typedef struct {
    YoshiActor actors[YA_COUNT];
    u16 swallow;
    u8 enabled, facing, mouth, length, timer, up, slot, holding, eggs, blocked;
} YoshiInteraction;
void actors_reset(void);
void actors_scenario(u16 n);
void actors_step(s16 direction);
void actors_render(void);
#endif
