/* Mode 7 - Demo 2, by David Coz (2005, TIGCC 0.96 beta 5): reconstructed sources.
 *
 * Decompiled from the published binary (modebin.89y, unpacked from its PPG). Names, types and
 * comments are ours; the logic, the constants and the memory layout are the original's
 * (offsets of the original structures in the comments, so that this file can be read next to
 * the disassembly ../mode7.s). The original ran from a 0x890-byte Game struct on the stack.
 *
 * Units:
 * - world unit: 1/4 texel of the near floor texture. A map tile is 16 texels = 64 units, the
 *   track map 64x64 tiles = 4096 units. The ship position is kept in 1/8 world units.
 * - angles: 0..255 per turn (high byte of the 8.8 ship angle); sin128/cos128 are * 128.
 * - screen: the game draws into a 240x128 "virtual screen" with the LCD layout (30 bytes per
 *   row), two planes of 0xF00 bytes, copied to the grayscale planes once per frame. The view is
 *   the left 128x100 pixels: sky rows 0..44, far floor 45..59, near floor 60..99.
 */
#ifndef MODE7_H
#define MODE7_H

#define USE_TI89                          /* the original is built for the three models */
#define USE_TI92PLUS
#define USE_V200
#include <tigcclib.h>

/* ------------------------------------------------------------------------ data (data.c) */
typedef struct { signed char type; char pad; short x, y; unsigned char angle, pad2; } Instance;
typedef struct { short x, y, z; } Vertex;
typedef struct { short v[3]; short color; } Face;         /* colour: bit 0 light, bit 1 dark */
typedef struct {
    signed char nverts, nfaces;
    Vertex v[16];                                          /* +0x02 */
    Face f[10];                                            /* +0x62 */
} Model;                                                   /* 178 bytes */

extern const short sin128[256], cos128[256];
extern unsigned char track_map[64 * 64];
extern const unsigned char tiles_gray[], sky_image[];
extern const unsigned char ship_light[], ship_dark[], ship_mask[];
extern const signed char accel_curve[];
extern const short wall_point[75][2], wall_normal[75][2];
extern const unsigned char wall_length[];
extern const signed char cell_walls_count[64], cell_walls[512];
extern const short level_info[1][3];
extern const Instance instances[1][100];
extern const Model models[5];

#define NB_TILES 92                  /* the original keeps it in a global short (0x36fc) */

/* ------------------------------------------------------------------------ game state */
typedef struct {
    long x, y;                       /* 0x00 position, 1/8 world unit */
    short unused08[4];
    unsigned short angle;            /* 0x10 8.8, high byte = heading 0..255 */
    long speed;                      /* 0x12 */
    short unused16[2];
    short accel;                     /* 0x1a throttle gain (100) */
    short turn_base;                 /* 0x1c turn rate at rest (1200) */
    short turn_loss;                 /* 0x1e turn rate lost per speed step (35) */
    short brake;                     /* 0x20 brake strength (150) */
    short dt;                        /* 0x22 time step, 6 at init then 4 */
    short drag;                      /* 0x24 air drag (1000) */
} Ship;                              /* 0x26 bytes */

typedef struct { short x, y; } Point2;

typedef struct {
    short nobj;                      /* 0x000 object instances in the level */
    short nverts;                    /* 0x002 */
    short unused004;
    short nfaces;                    /* 0x006 */
    short nvisfaces;                 /* 0x008 faces in visfaces[] this frame */
    short focal;                     /* 0x00a projection focal (43) */
    short cam_z;                     /* 0x00c eye height (20) */
    short level;                     /* 0x00e never written by the original (see Init3D) */
    short cx, cy;                    /* 0x010 projection centre (64, 40) */
    Vertex *verts;                   /* 0x014 world vertices (models placed) */
    HANDLE verts_h;                  /* 0x018 */
    Face *faces;                     /* 0x01a */
    HANDLE faces_h;                  /* 0x01e */
    Point2 proj[200];                /* 0x020 projected vertices of this frame */
    short clip[200];                 /* 0x340 1 = behind the eye */
    char unused4d0[256];             /* 0x4d0 cleared every frame, never read */
    signed char vis[30][2];          /* 0x5d0 visible 256-unit cells (x, y) */
    short nvis;                      /* 0x60c */
    short *cell_faces;               /* 0x60e 16 face numbers per cell */
    HANDLE cell_faces_h;             /* 0x612 */
    signed char cell_count[256];     /* 0x614 faces per cell, 16x16 cells */
    char *drawn;                     /* 0x714 per face: already in visfaces[] */
    HANDLE drawn_h;                  /* 0x718 */
    short visfaces[100];             /* 0x71a */
    short *cache;                    /* 0x7e2 per vertex: index in proj[], -1 = not yet */
    HANDLE cache_h;                  /* 0x7e6 */
    short *px, *py;                  /* 0x7e8 camera position (Game.camx, camy) */
    unsigned char *pangle;           /* 0x7f0 heading (Game.angle) */
} World;                             /* 0x7f4 bytes */

typedef struct {
    short camx, camy;                /* 0x00 camera, world units, 60 behind the ship */
    short far_u, far_v;              /* 0x04 camera in the far texture */
    short near_u, near_v;            /* 0x08 camera in the near texture */
    short far_tx, far_ty;            /* 0x0c top-left map tile of the far texture */
    short near_tx, near_ty;          /* 0x10 same, near texture */
    short prev_camx, prev_camy;      /* 0x14 */
    short shipx, shipy;              /* 0x18 ship, world units */
    short prev_shipx, prev_shipy;    /* 0x1c */
    Ship ship;                       /* 0x20 */
    World world;                     /* 0x46 */
    char unused83a[4];
    unsigned char angle;             /* 0x83e heading of the ship and the camera */
    char unused83f[5];
    short unused844;                 /* 0x844 set to 30, never read */
    char unused846[8];
    void *unused84e, *unused852;     /* 0x84e never set; the two below are computed from them */
    void *unused856, *unused85a;     /* 0x856 */
    unsigned char *tiles16;          /* 0x85e 92 tiles of 16x16 bytes (pixel 0..3) */
    HANDLE tiles16_h;
    unsigned char *tiles8;           /* 0x864 the same tiles shrunk to 8x8 */
    HANDLE tiles8_h;
    unsigned char *map_near;         /* 0x86a top-left map cell of the near texture */
    unsigned char *map_far;          /* 0x86e same, far texture */
    unsigned char *tex_far;          /* 0x872 128x128 bytes: 16x16 tiles of 8x8 */
    HANDLE tex_far_h;
    unsigned char *tex_near;         /* 0x878 128x128 bytes: 8x8 tiles of 16x16 */
    HANDLE tex_near_h;
    unsigned char *vscreen;          /* 0x87e 240x128, two planes (light, then dark at +0xF00) */
    HANDLE vscreen_h;
    unsigned char *sky;              /* 0x884 sky, 30 bytes per row: dark rows 0..44, light 45..89 */
    HANDLE sky_h;
    INT_HANDLER old_int1;            /* 0x88a */
} Game;                              /* 0x88e bytes */

/* Plane order of the virtual screen. The original: light plane at vscreen, dark at +0xF00. The
 * optimised build draws straight into GrayDBuf's hidden planes, dark first, light at +0xF00:
 * the texel codes, the sky order, the sprite planes and the face colours are swapped instead of
 * the routines. P0 / P1 = the colour bit that goes to the plane at vscreen / at +0xF00. */
#ifdef ORIGINAL
#define DARK_FIRST 0
#else
#define DARK_FIRST 1
#endif

/* ------------------------------------------------------------------------ globals (main.c) */
extern volatile long timer;          /* 0x36f8 auto-int 5 ticks (the timer runs at 0xCC) */
extern short start_x, start_y, start_angle;

/* ------------------------------------------------------------------------ asm (render.s) */
/* stack parameters, as the original */
void Mode7Far(void *dest, unsigned char *tex, short u, short v, unsigned char angle) __attribute__((stkparm));
void Mode7Near(void *dest, unsigned char *tex, short u, short v, unsigned char angle) __attribute__((stkparm));
void BuildTexture8(unsigned char *map, unsigned char *tiles, unsigned char *tex) __attribute__((stkparm));
void BuildTexture16(unsigned char *map, unsigned char *tiles, unsigned char *tex) __attribute__((stkparm));
void ClearPlanes(void *light, void *dark) __attribute__((stkparm));
void CopyPlane(void *src, void *dest) __attribute__((stkparm));
void Sprite32Gray(short x, short y, short h, const void *light, const void *dark,
                  const void *mask_light, const void *mask_dark, void *plane0, void *plane1) __attribute__((stkparm));
void HLine(void *plane, short x1, short x2, short y, short mode) __attribute__((stkparm));
void Mode7FarFast(void *dest, unsigned char *tex, short u, short v, unsigned char angle) __attribute__((stkparm));
void Mode7NearFast(void *dest, unsigned char *tex, short u, short v, unsigned char angle) __attribute__((stkparm));
void BuildTexture8W(unsigned char *map, unsigned char *tiles, unsigned char *tex) __attribute__((stkparm));
void BuildTexture16W(unsigned char *map, unsigned char *tiles, unsigned char *tex) __attribute__((stkparm));
void SkyCopy(const void *src0, const void *src1, void *dest, short rows) __attribute__((stkparm));
void Span2(void *plane0, short x1, short x2, short y, short color) __attribute__((stkparm));
long TriSpans(void *vs, short y, short yend, short fa, short fb, short sa, short sb, short color) __attribute__((stkparm));

/* ------------------------------------------------------------------------ benchmark zones */
/* built with -DBENCH, the program runs under tools/bin/ti-cycles and marks its zones */
#ifdef BENCH
#include "../../../tools/m68kbench/bench.h"
#define BZ(id) BENCH_BEGIN(id)
#define EZ(id) BENCH_END(id)
#else
#define BZ(id)
#define EZ(id)
#endif
enum { Z_FRAME = 1, Z_CLEAR, Z_SHIP, Z_SKY, Z_TEX8, Z_TEX16, Z_CAMERA, Z_FAR, Z_NEAR, Z_DOUBLE,
       Z_SPRITE, Z_3D, Z_CELLS, Z_FACES, Z_PROJECT, Z_FILL, Z_COPY, Z_TEXT, Z_SPANS };

/* ------------------------------------------------------------------------ C modules */
/* floor.c */
void InitSky(Game *g);
void DrawSky(Game *g);
void ConvertTiles(Game *g);
void CreateTiles8(Game *g);
void EncodeTiles(Game *g);
void UpdateCamera(Game *g);
void ResetTextures(void);
void DrawShip(Game *g);
/* ship.c */
void InitShip(Ship *s);
void ReadKeys(Game *g);
void DriveShip(Game *g, short turn, short throttle);
/* world3d.c */
short Init3D(World *w, short level, short *px, short *py, unsigned char *pangle);
void Deref3D(World *w);
void Free3D(World *w);
void Render3D(World *w, unsigned char *vscreen);
void BenchScene(World *w, short n, const short *x, const short *y, short b, short h);

#endif
