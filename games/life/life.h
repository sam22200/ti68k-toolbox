// Game of Life state, shared with its unit tests.
#ifndef LIFE_H
#define LIFE_H
#include "../../runtime/core/rt.h"

#define GW 40                                  // 40x25 cells of 4x4 pixels, torus
#define GH 25
#define CELL 4
#define GEN_FRAMES 4                           // one generation every 4 frames (8/s)

typedef struct { u8 cell[GH][GW]; u16 gen; u8 tick; } LifeState;
extern LifeState st;
void life_step(void);

#endif
