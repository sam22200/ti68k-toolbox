// Flappy Bird state and constants, shared with the unit tests. Spec and scaling: README.md.
#ifndef FLAPPY_H
#define FLAPPY_H
#include "../../runtime/core/rt.h"

// Geometry (pixels). Playfield 0..GROUND_Y-1, land strip below.
#define GROUND_Y   88
#define BIRD_X     28              // hitbox left edge (fixed column)
#define BIRD_W     8               // hitbox (upstream 24x24 inside a 48x48 cell)
#define BIRD_H     6
#define START_Y    53              // hitbox top on READY
#define CEIL_FP    (-11 * 256)     // the bird may leave the top by 11 px, not further (8.8)
#define NPIPE      4
#define PIPE_W     16
#define PIPE_DIST  60
#define PIPE_X0    (RT_W + 80)     // first pipe on READY → PLAY
#define GAP        20              // opening height
#define GAP_MIN    13              // opening top = GAP_MIN + [0, GAP_RANGE)
#define GAP_RANGE  45

// Physics, 8.8 fixed point, y grows downwards, per frame at 32 fps
#define GRAVITY    63              // 0.32 px/f^2 upstream
#define FLAP_VY    (-532)          // 5.2 px/f upstream: same 9.8 px rise per flap
#define DROP_VY    845             // death drop, 8 px/f upstream
#define ROT_STEP   5               // degrees per frame, 2.7 upstream
#define ANG_FLAP   (-45)
#define ANG_MAX    85

// Timings (frames)
#define FLASH_T    2               // white flash on the hit
#define OVER_TEXT  16              // "game over" drops in after this
#define OVER_PANEL 29              // then the score panel slides in
#define OVER_INPUT 37              // then a press restarts

#define FLAP_KEYS  (K_A | K_UP | K_ENTER)

enum { S_TITLE, S_READY, S_PLAY, S_DEAD, S_OVER };

typedef struct {
    s16 x;                         // left edge, pixels
    u8 top;                        // opening top
    u8 scored;
} Pipe;

typedef struct {
    u8 mode;                       // S_*
    u8 auto_pilot;                 // flaps by itself (demo, bench)
    u8 night, sub;                 // background variant; 1.5 px/f scroll phase
    u16 t;                         // frames in this mode
    u16 clock;                     // frames since launch (animations)
    s16 y, vy;                     // bird hitbox top and speed, 8.8
    s8 angle;                      // degrees, ANG_FLAP..ANG_MAX
    u8 land, newbest;              // land scroll offset; best beaten this round
    u16 score, best;
    Pipe pipe[NPIPE];              // pipe[0] = leftmost, the only one tested (upstream rule)
} Flappy;

extern Flappy st;

void flappy_reset(void);           // READY with fresh pipes
u8 flappy_scroll(void);            // advance the 1.5 px/f phase, returns 1 or 2
u8 flappy_hit(void);               // bird vs pipe[0] and ground
void flappy_play_step(u8 flap);    // one PLAY frame
u8 flappy_medal(u16 score);        // 0 none, 1 bronze (10) .. 4 platinum (40)

#endif
