// Metal Gear Solid (GBC) on the TI-89: our own engine with the behaviour measured on the ROM
// (README.md, milestone 1: VR Training, Sneaking, Practice Lv.01).
#ifndef MGS_H
#define MGS_H
#include "../../runtime/core/rt.h"

#define VIEW_H 100                      // the TI screen: the whole level width (160), 100 rows
#define LV_W 160
#define LV_H 240

enum { M_PLAY, M_SPOTTED, M_FAILED, M_CLEAR };
// guard script (measured: README § The guard)
enum { G_WAIT_TOP, G_WALK_DOWN, G_WAIT_BOTTOM, G_TURN_BOTTOM, G_WALK_UP };

typedef struct {
    u8 mode;
    u16 timer;                          // logic steps in this mode
    u16 steps;                          // logic steps since the level started (GB frames)
    // Snake: feet position in level pixels, direction 0 = up, clockwise to 7 = up-left
    u8 sx, sy, sdir;
    u8 turn, settle;                    // turning: a step every 2, then 2 before walking
    u8 moving, anim, animcnt, diag;     // walk cycle step 0..10 every 3, diagonal phase
    // the guard
    u8 gx, gy, gdir, gstate;
    u16 gtimer;
    u8 gsub, gsteps;                    // 1 px every 3 steps, pixels walked (animation)
    u16 camy;
} State;

extern State st;
extern const u8 *mgs_data;              // mgsdat (tiles, then sprites)

void mgs_step(u8 keys);                 // one GB frame of logic; keys = K_UP.. bits (u8)
u8 mgs_free(u8 x, u8 y);                // Snake's box at (x, y) touches no solid quadrant
u8 mgs_sees(void);                      // the guard sees Snake this step
void mgs_start(void);                   // the level from its start
u32 mgs_hash(void);                     // every state field (TI = PC check)

#endif
