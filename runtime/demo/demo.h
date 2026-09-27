// Demo state and constants, shared with its unit tests.
#ifndef DEMO_H
#define DEMO_H
#include "../core/rt.h"

#define MAP_W 24
#define MAP_H 14
#define MIN_X 16                               // inside the wall ring
#define MIN_Y 16
#define MAX_X ((MAP_W - 2) * 16)
#define MAX_Y ((MAP_H - 2) * 16)

typedef struct { s16 x, y; u16 steps; } DemoState;
extern DemoState st;
s16 demo_cam(s16 p, s16 view, s16 size);

#endif
