// Alundra-style traversal on the TI-89: one room, move, jump, terrain heights (RE_NOTES.md).
// Our own engine; the behaviour was measured and read in Alundra (PS1), the numbers adapted.
#ifndef ALUNDRA_H
#define ALUNDRA_H
#include "../../runtime/core/rt.h"
#include "gfx.h"                        // the disc's art and the scale (tools/extract.py, local)

// World units: x, y in 1/16 pixel (feet centre on the ground plane), z in 1/16 pixel up.
#define SUB 16
// The scale (SCALE_N / SCALE_D, Makefile SCALE): every size and speed below was set at the
// reference scale 22/42 (Alundra's 42 px -> 22) and follows the scale linearly; at 22/42 SC(v) = v
#define SC(v) ((s16)(((v) * 42L * SCALE_N + SCALE_D * 11L) / (SCALE_D * 22L)))   // folded at compile time (long: TI ints are 16-bit)
// Speeds: Alundra's in screen px per second (x 146, y 97.5: depth at 2/3) scaled like the
// sprite, at 32 steps per second; continuous, in 1/16 px
#define SPEED_X SC(38)                  // 2.375 px per step (Alundra 2.44 px per frame at 60 Hz)
#define SPEED_Y SC(26)                  // 1.625 px per step (Alundra 1.625)
#define DIAG_X SC(27)                   // x 0.707 per axis on a diagonal (Alundra)
#define DIAG_Y SC(18)
#define JUMP_VZ SC(72)                  // 4.5 px per step up
#define GRAVITY SC(14)                  // apex 13.9 px = 1.73 levels, 12 steps (Alundra 1.72, 0.35 s)
#define FALL_MAX SC(128)                // 8 px per step
#define NUDGE_X SC(12)                  // around a corner: Alundra 0.75 / 0.5 px per frame
#define NUDGE_Y SC(8)                   // (0.31 of the walk), scaled the same way
#define SHADOW_DY SC(3)                 // shadow centre above the feet (Alundra 5 px x 0.524)
#define FOOT_W SC(10)                   // foot box, pixels: x - 5 .. x + 4
#define FOOT_D SC(8)                    //                   y - 4 .. y + 3
#define TILE 16
// A height level is 16 px of the game: 8.375 screen px at 22/42, kept exact in 1/16 px so the
// game's image and the collision agree on every terrace
#define LEVEL_Z SC(134)                 // 1/16 px per level
#define LEVEL_PX(h) ((s16)(((h) * LEVEL_Z) >> 4))   // screen px of h levels
// A tile: its level (0-63) in the low bits; a wall (never walkable; the level is the height of
// its top); a stair (on foot, the feet follow the floor up or down two levels: Alundra's
// ramps, ~2 levels per tile)
#define T_WALL 0x80
#define T_STAIR 0x40
#define LV(t) ((t) & 63)
#define WALL (T_WALL | 3)               // the test room's walls, drawn 3 levels high
#define ROOM_Y 4                        // the test room's top: screen y of its row 0 (100 - 96)

// A world: its tiles and its image (two planes, 1 bit per pixel, MSB left, iwb bytes per row):
// the test room's drawn by its tiles (tools/bake.c → world.h), the village's the game's own
// (tools/extract.py → the data files alvil0, alvil1); tile row 0 at image row `top`. What
// hides the player, per image byte: a threshold (he is behind it when his y / 2, screen px, is
// below it: the rows in front of his, Alundra's draw order) and the mask of its pixels
// (alvil2, alvil3)
typedef struct {
    const u8 *cell;                     // w x h tiles, row-major
    u8 w, h;
    u8 top;
    const u8 *img[2];                   // light, dark
    const u8 *depth[2];                 // threshold, mask
    u16 iwb, ih;
} World;

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

extern World worlds[];                   // 0 the test room, 1 the village of Inoa
#define W_TEST 0
#define W_VILLAGE 1
extern const World *world;              // the current one
extern s16 al_camx, al_camy;            // the camera of the last frame drawn (image px)

u8 al_frame(void);                      // the player's image: 0 stand, 1-6 walk, 7-8 jump, 9-10 breathing
u8 al_tile(s16 px, s16 py);             // the tile under a pixel (outside = WALL)
void al_world(u8 n);                    // switch to world n (its start; the village needs
                                        // its data files, else the test room)
s16 al_floor(u16 x, u16 y);             // the floor under the foot box at (x, y): max height, 1/16 px
u8 al_block(u16 x, u16 y, s16 z);       // the foot box's blocked corners at (x, y), feet at z
#define al_free(x, y, z) (!al_block(x, y, z)) // the foot box fits
void al_start(void);                    // the test room from its start
void al_step(u32 keys);                 // one logic step with these keys held
u32 al_hash(void);                      // every state field (TI = PC check)

#endif
