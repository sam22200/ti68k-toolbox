#ifndef YOSHI_DAMAGE_H
#define YOSHI_DAMAGE_H
#include "../../runtime/core/rt.h"
enum { YB_ATTACHED, YB_PENDING, YB_LOST, YB_RETURN, YB_FAILED };
typedef struct {
    u16 x, y, remaining, tick;
    s16 vx, vy;
    u8 xsub, ysub, mode, phase, age, invincible, recoil, recharge, enabled;
    u8 hits, rescues, leftward;
} YoshiBaby;
void damage_reset(void);
void damage_scenario(u16 n);
u8 damage_clock(void);
void damage_step(u16 old_feet);
void damage_render(void);
#endif
