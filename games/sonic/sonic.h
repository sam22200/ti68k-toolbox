#ifndef SONIC_H
#define SONIC_H
#include "../../runtime/core/rt.h"

/* Simulation uses original MD pixels and signed 8.8 velocities at ~60 Hz.
 * Positions split integer/fraction to avoid 32-bit arithmetic in the hot path.
 * Rendering halves every distance, with two simulation steps per drawn frame. */
#define SON_END 1536
#define SON_WORLD_W 1664
#define SON_BRIDGE_L 1088
#define SON_BRIDGE_R 1280
#define SON_BRIDGE_Y 896
#define SON_TOP_SPEED 1536
#define SON_ACCEL 12
#define SON_BRAKE 128
#define SON_GRAVITY 56
#define SON_JUMP_SPEED 1664

#define S_LEFT 1
#define S_AIR 2
#define S_ROLL 4
#define S_BRIDGE 8
#define S_ROLLJUMP 16

#define SON_RINGS 24
#define SON_ENEMIES 3
#define SON_SHOTS 4
#define SON_LOST 32
#define E_MOTO 0x40
#define E_BUZZ 0x22
#define E_CHOP 0x2b

typedef struct {
    s16 x, y, vx, vy, origin_y;
    u8 fx, fy, kind, phase, timer, direction, fired, flash;
} SonEnemy;
typedef struct {
    s16 x, y, vx, vy;
    u8 fx, fy, life, delay, parent;
} SonParticle;

typedef struct {
    s16 x, y, vx, vy, speed;
    u16 camx, camy, logic, resets;
    u8 fx, fy, flags, angle, jumping, finished, debug, ready;
    u8 rings, hurt, invuln, notice, objects, kills;
    u8 ring_state[SON_RINGS];
    SonEnemy enemies[SON_ENEMIES];
    SonParticle shots[SON_SHOTS], lost[SON_LOST];
} Sonic;
extern Sonic st;
extern const u8 *son_objects;
extern u8 son_object_count;

void sonic_start(s16 x, s16 y);
void sonic_step(u16 keys, u8 jump_pressed);
u8 sonic_solid(s16 x, s16 y, u8 sides);
s16 sonic_floor(s16 x, s16 low, s16 high, u8 *angle);
u32 sonic_hash(void);
void sonic_objects_reset(void);
void sonic_objects_step(void);
void sonic_objects_render(void);
u32 sonic_objects_hash(u32 hash);
void sonic_hurt(s16 source_x);
#endif
