// Final Fantasy Tactics, Magic City Gariland: an isometric tech demo on the Portable Game
// Runtime (README.md). Our own engine; the battlefield's heights and its four pictures come
// from the local disc (tools/extract.py -> map.h, fftv0..3), and so do the units (tools/units.py
// -> units.h); the cursor, shadow and rotation greys are ours (tools/art.py).
#ifndef FFT_H
#define FFT_H
#include "../../runtime/core/rt.h"

typedef struct { u8 c[4]; u8 look[5]; u8 walk, stand, water; } Tile;   // map.h, extract.py
#include "art.h"
#include "units.h"
#include "map.h"

// Projection (scene pixels): a tile is a 24x12 diamond, one height unit is 6 px.
#define HW 12                          // half tile width
#define HH 6                           // half tile height
#define HU 6                           // pixels per height unit
#define MAXH 10                        // highest corner of the map (checked by the tests)
// Scene: the whole map for one orientation (fftv0..3 and the buffer the current one is copied to)
#define SC_BYTES 40                    // 320 px
#define SC_W (SC_BYTES * 8)
#define SC_H (HH * (MAP_W + MAP_H) + HU * MAXH + 8)
#define SC_OY (HU * MAXH + 4)          // scene y of the top vertex of view tile (0, 0) at h 0
#define SC_PLANE (SC_BYTES * SC_H)     // one plane

#define NUNIT 4
#define MOVE 4                         // FFT's squire: Move 4, Jump 3
#define JUMP 3
#define TURN_FRAMES 3                  // rotation animation: frames between two views
#define WALK_FRAMES 4                  // frames per tile when a unit walks
#define PATH_MAX 16

enum { M_BROWSE, M_TARGET, M_WALK };
enum { TEAM_PLAYER, TEAM_ENEMY };

typedef struct { u8 x, z, gfx, team, face; } Unit;   // face: world +x, -x, +z, -z

typedef struct {
    u8 rot;                            // orientation 0..3 (F5 +1, F1 -1)
    s8 turn;                           // rotation in progress: +1 / -1, 0 none
    u8 turn_t;                         // its frame, 1..TURN_FRAMES
    u8 cx, cz;                         // cursor, world tile
    u8 mode, sel;                      // M_*, selected unit
    u8 rep;                            // arrow key repeat
    u8 tick;                           // the units' animation clock (frames)
    u8 path_n, path_i, walk_t;         // walking: path, current step, frame in the step
    u8 path[PATH_MAX][2];
    Unit unit[NUNIT];
    s16 camx, camy;                    // camera, scene pixels of the current orientation
    u8 reach[MAP_W * MAP_H];           // M_TARGET: steps to each tile, 0xFF unreachable
} State;
extern State st;

// rotation of world tile (x, z) into view tile (u, v) and back, view size
void view_of(u8 rot, s16 x, s16 z, s16 *u, s16 *v);
void world_of(u8 rot, s16 u, s16 v, s16 *x, s16 *z);
u8 view_w(u8 rot);
u8 view_h(u8 rot);
// corner c (0..3: view (0,0) (1,0) (1,1) (0,1)) of view tile (u, v): its height
u8 corner_h(u8 rot, s16 u, s16 v, u8 c);
const Tile *tile_at(s16 x, s16 z);     // world tile or RT_NULL outside
s16 scene_x(u8 rot, s16 u, s16 v);     // scene position of view corner (u, v) at height 0
s16 scene_y(s16 u, s16 v);
u8 unit_at(s16 x, s16 z);              // unit index + 1, 0 = none
void compute_reach(u8 who);            // st.reach from unit who (BFS, Move/Jump)
u8 make_path(u8 who, u8 tx, u8 tz);    // st.path to a reachable tile, its length
u16 scene_checksum(void);              // the scene buffer (tests)
extern u8 *scene_l, *scene_d;          // the scene buffer planes (SC_BYTES x SC_H)
extern u8 scene_rot;                   // orientation it holds (0xFF: stale)

#endif
