// Alundra-style traversal on the TI-89: one room, move, jump, terrain heights (RE_NOTES.md).
// Our own engine; the behaviour was measured and read in Alundra (PS1), the numbers adapted.
#ifndef ALUNDRA_H
#define ALUNDRA_H
#include "../../runtime/core/rt.h"

// World units: x, y in 1/16 pixel (feet centre on the ground plane), z in 1/16 pixel up.
#define SUB 16
#define ROOM_W 160                      // 10 tiles of 16
#define ROOM_H 96                       // 6 tiles of 16
// Speeds: Alundra's in screen px per second (x 146, y 97.5: depth at 2/3) scaled like the
// sprite (22 / 42 = 0.524), at 32 steps per second; continuous, in 1/16 px
#define SPEED_X 38                      // 2.375 px per step (Alundra 2.44 px per frame at 60 Hz)
#define SPEED_Y 26                      // 1.625 px per step (Alundra 1.625)
#define DIAG_X 27                       // x 0.707 per axis on a diagonal (Alundra)
#define DIAG_Y 18
#define JUMP_VZ 72                      // 4.5 px per step up
#define GRAVITY 14                      // apex 13.9 px = 1.73 levels, 12 steps (Alundra 1.72, 0.35 s)
#define FALL_MAX 128                    // 8 px per step
#define NUDGE_X 12                      // around a corner: Alundra 0.75 / 0.5 px per frame
#define NUDGE_Y 8                       // (0.31 of the walk), scaled the same way
#define SHADOW_DY 3                     // shadow centre above the feet (Alundra 5 px x 0.524)
#define FOOT_W 10                       // foot box, pixels: x - 5 .. x + 4
#define FOOT_D 8                        //                   y - 4 .. y + 3
#define TILE 16
#define MAP_W 10
#define MAP_H 6
#define LEVEL 8                         // screen px per height level
#define WALL 3                          // tile value of a wall (drawn 3 levels high)
#define ROOM_Y 4                        // screen y of the room's row 0 (100 - 96)

typedef struct {
    u16 x, y;                           // 1/16 px
    s16 z, vz;                          // 1/16 px, 1/16 px per step (up > 0)
    u8 grounded;
    u8 a_held;                          // [2nd] held last step (a jump needs a new press)
    u8 debug;                           // overlay on/off ([diamond])
    u8 dir;                             // facing: 0 down, 1 up, 2 left, 3 right
    u8 anim;                            // steps walked since the last stop
    u8 idle;                            // breathing cycle while standing (steps)
    u16 steps;
} State;

extern State st;

extern const u8 room[MAP_H][MAP_W];      // heights 0..2, WALL

u8 al_frame(void);                      // the player's image: 0 stand, 1-6 walk, 7-8 jump, 9-10 breathing
u8 al_tile(s16 px, s16 py);             // the tile under a pixel (outside = WALL)
s16 al_floor(u16 x, u16 y);             // the floor under the foot box at (x, y): max height, 1/16 px
u8 al_block(u16 x, u16 y, s16 z);       // the foot box's blocked corners at (x, y), feet at z
#define al_free(x, y, z) (!al_block(x, y, z)) // the foot box fits
void al_start(void);                    // the room from its start
void al_step(u32 keys);                 // one logic step with these keys held
u32 al_hash(void);                      // every state field (TI = PC check)

#endif
