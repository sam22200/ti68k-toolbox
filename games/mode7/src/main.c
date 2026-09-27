/* Mode 7 - Demo 2 (David Coz, 2005): start, memory, interrupts and the frame loop.
 * Reconstructed from the binary (see mode7.h). Original addresses in the comments. */
#define MIN_AMS 101                       /* "AMS 1.01 or higher needed" */
#define SAVE_SCREEN                       /* the startup code saves and restores LCD_MEM */

#include "mode7.h"

/* the layout is the original's: compile-time checks against the offsets of the disassembly */
#define CHECK(name, cond) extern char name[(cond) ? 1 : -1]
CHECK(chk_ship, sizeof(Ship) == 0x26 && offsetof(Ship, drag) == 0x24);
CHECK(chk_world, sizeof(World) == 0x7f4 && offsetof(World, cell_count) == 0x614
      && offsetof(World, visfaces) == 0x71a && offsetof(World, pangle) == 0x7f0);
CHECK(chk_game, sizeof(Game) == 0x88e && offsetof(Game, world) == 0x46 && offsetof(Game, angle) == 0x83e
      && offsetof(Game, tiles16) == 0x85e && offsetof(Game, old_int1) == 0x88a);
CHECK(chk_data, sizeof(Instance) == 8 && sizeof(Model) == 178);

/* ------------------------------------------------------------------------ globals */
short start_x = 220, start_y = 160, start_angle = 192;      /* 0x36f2: ship start */
volatile long timer;                      /* 0x36f8 auto-int 5 ticks, for the frame time */
volatile long ticks;                      /* 0x36ea counted with it, never read */
volatile short reset_counters = 1;        /* 0x36fe the first int 5 clears the two below */
volatile short counting = 1;              /* 0x3700 */
volatile short unused3702, unused5ca4;    /* 0x3702, 0x5ca4 (set to 0, never read) */
INT_HANDLER old_int5;                     /* 0x36ee */

short frame_count;                        /* 0x0594 counted every frame, never read */
short fps_count;                          /* 0x1596 frames since the frame time was printed */
long frame_time;                          /* the only BSS variable: ticks of the last frame */

/* 0x3606: timer interrupt, counts the ticks then chains to AMS's handler */
DEFINE_INT_HANDLER(Int5)
{
    if (reset_counters) {
        reset_counters = 0;
        unused3702 = reset_counters;
        unused5ca4 = unused3702;
    } else if (counting) {
        ticks++;
        timer++;
    }
    ExecuteHandler(old_int5);
}

/* ------------------------------------------------------------------------ memory */
/* 0x2f88: the floor buffers; 0 if the heap is full */
static short AllocMode7(Game *g)
{
    g->tex_far_h = g->tex_near_h = g->tiles16_h = g->tiles8_h = g->vscreen_h = g->sky_h = H_NULL;
#ifdef ORIGINAL
    if (!(g->tex_far_h = HeapAlloc(0x4000))) return 0;
    if (!(g->tex_near_h = HeapAlloc(0x4000))) return 0;
#else
    if (!(g->tex_far_h = HeapAlloc(0x8000))) return 0;   /* both textures, 256-byte rows */
#endif
    if (!(g->tiles16_h = HeapAlloc(NB_TILES << 8))) return 0;
    if (!(g->tiles8_h = HeapAlloc(NB_TILES << 6))) return 0;
#ifdef ORIGINAL
    if (!(g->vscreen_h = HeapAlloc(0x1E00))) return 0;
#else
    if (!(g->vscreen_h = HeapAlloc(GRAYDBUFFER_SIZE))) return 0;   /* GrayDBuf's second buffer */
#endif
    if (!(g->sky_h = HeapAlloc(0xA8C))) return 0;
    return 1;
}

/* 0x315e: pointers to the blocks, then the 3D world is built.
 * Fixed: the original takes them with HeapDeref, unlocked; GrayOn's HeapAllocHigh can then
 * compress the heap and move the blocks under the pointers (it did with this rebuild: the
 * near floor showed another block). HLock keeps them in place (HeapFree accepts locked ones). */
static void DerefAll(Game *g)
{
    char buf[200];
    sprintf(buf, "Set_Pointers");
    DrawStr(100, 0, buf, A_REPLACE);
#ifdef ORIGINAL
    g->tex_far = HLock(g->tex_far_h);
    g->tex_near = HLock(g->tex_near_h);
#else
    g->tex_near = HLock(g->tex_far_h);    /* near: columns 0..127, far: 128..255 */
    g->tex_far = g->tex_near + 128;
#endif
    g->tiles16 = HLock(g->tiles16_h);
    g->tiles8 = HLock(g->tiles8_h);
    g->vscreen = HLock(g->vscreen_h);
    g->sky = HLock(g->sky_h);
    sprintf(buf, "Set_Pointers 3D");
    DrawStr(100, 10, buf, A_REPLACE);
    Deref3D(&g->world);
}

/* 0x3be8 */
static void FreeAll(Game *g)
{
    if (g->tex_far_h) HeapFree(g->tex_far_h);
    if (g->tex_near_h) HeapFree(g->tex_near_h);
    if (g->tiles16_h) HeapFree(g->tiles16_h);
    if (g->tiles8_h) HeapFree(g->tiles8_h);
    if (g->vscreen_h) HeapFree(g->vscreen_h);
    if (g->sky_h) HeapFree(g->sky_h);
    Free3D(&g->world);
}

/* 0x32fa */
static void NoMemory(Game *g)
{
    ClrScr();
    DrawStr(0, 0, "Not Enough Memory !", A_NORMAL);
    FreeAll(g);
    ngetchx();
}

/* ------------------------------------------------------------------------ start, stop */
/* 0x3ce8: everything, with a progress line per step; 0 if out of memory */
static short Init(Game *g)
{
    char buf[200];
    ClrScr();
    FontSetSys(F_4x6);
    sprintf(buf, "Init ...");
    DrawStr(0, 10, buf, A_REPLACE);
    sprintf(buf, "Allocate memory mode7 ...");
    DrawStr(0, 10, buf, A_REPLACE);
    if (!AllocMode7(g)) goto no_memory;
    sprintf(buf, "Allocate memory mode7 ... OK");
    DrawStr(0, 10, buf, A_REPLACE);
    InitShip(&g->ship);
    sprintf(buf, "Allocate memory 3D stuff ...");
    DrawStr(0, 20, buf, A_REPLACE);
    if (!Init3D(&g->world, 0, &g->camx, &g->camy, &g->angle)) {
no_memory:
        NoMemory(g);
        return 0;
    }
    sprintf(buf, "Allocate memory 3D stuff ... OK");
    DrawStr(0, 20, buf, A_REPLACE);
    DerefAll(g);
    g->ship.dt = 4;
    sprintf(buf, "Init Sky ... ");
    DrawStr(0, 30, buf, A_REPLACE);
    InitSky(g);
#ifndef BENCH
    pokeIO(0x600017, 0xCC);               /* timer start value (AMS's own; not restored at exit) */
#endif
    g->shipx = 0x200;
    g->shipy = 0x200;
    sprintf(buf, "Tile Conversion... ");
    DrawStr(0, 40, buf, A_REPLACE);
    ConvertTiles(g);
    sprintf(buf, "Create 8*8 Tiles... ");
    DrawStr(0, 50, buf, A_REPLACE);
    CreateTiles8(g);
#ifndef ORIGINAL
    EncodeTiles(g);
#endif
    g->shipx = 600;                       /* only for this first camera update: ReadKeys sets them */
    g->shipy = 520;                       /* from the ship before the first frame is drawn */
    g->angle = 0;
    g->unused844 = 30;
    ResetTextures();
    UpdateCamera(g);
#ifndef BENCH
    old_int5 = GetIntVec(AUTO_INT_5);
    SetIntVec(AUTO_INT_5, Int5);
    g->old_int1 = GetIntVec(AUTO_INT_1);
    SetIntVec(AUTO_INT_1, DUMMY_HANDLER); /* before GrayOn, which chains its handler to it */
    GrayOn();
#ifdef ORIGINAL
    PortSet(g->vscreen, 239, 127);        /* DrawStr now writes into the virtual screen */
#else
    /* optimised: draw straight into the hidden planes (no copy): the block allocated for the
     * virtual screen becomes GrayDBuf's second buffer; both are cleared once, since every frame
     * rewrites the whole 128x100 view and nothing is drawn beside it */
    GrayDBufInit(g->vscreen);
    {
        short i;
        for (i = 0; i < 2; i++) {
            unsigned char *p = GrayDBufGetPlane(i, DARK_PLANE);
            ClearPlanes(p, p + 0xF00);
        }
    }
    g->vscreen = GrayDBufGetHiddenPlane(DARK_PLANE);
#endif
#endif
#if defined(BENCH) && !defined(ORIGINAL)
    ClearPlanes(g->vscreen, g->vscreen + 0xF00);
#endif
    return 1;
}

#ifndef BENCH
/* 0x3b9c */
static void Quit(Game *g)
{
    GrayOff();
    SetIntVec(AUTO_INT_5, old_int5);
    SetIntVec(AUTO_INT_1, g->old_int1);
    PortRestore();
    FreeAll(g);
}
#endif

/* ------------------------------------------------------------------------ frame loop */
static short saved[8];                    /* the frame time digits, 16 x 8 pixels */

/* one frame of 0x01f0's loop, drawn into the virtual screen then copied to light and dark */
static void Frame(Game *g, void *light, void *dark)
{
    char buf[20];
    unsigned char *vs = g->vscreen;
    short i;

    fps_count++;
    frame_count++;
#ifdef ORIGINAL
    BZ(Z_CLEAR); ClearPlanes(vs, vs + 0xF00); EZ(Z_CLEAR);
#endif
    frame_time = timer;
    BZ(Z_SKY); DrawSky(g); EZ(Z_SKY);
    BZ(Z_CAMERA); UpdateCamera(g); EZ(Z_CAMERA);
#ifdef ORIGINAL
    BZ(Z_FAR); Mode7Far(vs + 59 * 30, g->tex_far, g->far_u, g->far_v, g->angle); EZ(Z_FAR);
    BZ(Z_NEAR); Mode7Near(vs + 98 * 30, g->tex_near, g->near_u, g->near_v, g->angle); EZ(Z_NEAR);
    BZ(Z_DOUBLE);
    {                                     /* the near floor was drawn every other line */
        unsigned char *p = vs + 98 * 30;
        for (i = 20; i; i--, p -= 60) {
            memcpy(p + 30, p, 16);
            memcpy(p + 0xF00 + 30, p + 0xF00, 16);
        }
    }
    EZ(Z_DOUBLE);
#else
    /* optimised floor (render.s): no clearing, the near rows doubled as they are drawn */
    BZ(Z_FAR); Mode7FarFast(vs + 59 * 30, g->tex_far, g->far_u, g->far_v, g->angle); EZ(Z_FAR);
    BZ(Z_NEAR); Mode7NearFast(vs + 98 * 30, g->tex_near, g->near_u, g->near_v, g->angle); EZ(Z_NEAR);
#endif
    BZ(Z_SPRITE); DrawShip(g); EZ(Z_SPRITE);
    BZ(Z_3D); Render3D(&g->world, vs); EZ(Z_3D);
    frame_time = timer - frame_time;
    BZ(Z_TEXT);
    {
        short *digits = (short *)(DARK_FIRST ? vs + 0xF00 : vs);    /* in the light plane */
        if (fps_count > 7) {              /* every 8 frames: print the frame time (ticks) */
            fps_count = 0;
            sprintf(buf, "%ld", frame_time);
#if DARK_FIRST
            PortSet(digits, 239, 127);
#endif
            DrawStr(0, 0, buf, A_REPLACE);
            for (i = 0; i < 8; i++) saved[i] = digits[i * 15];
        }
        for (i = 0; i < 8; i++) digits[i * 15] = saved[i];      /* and keep it on screen */
    }
    EZ(Z_TEXT);
#ifdef ORIGINAL
    BZ(Z_COPY); CopyPlane(vs, light); CopyPlane(vs + 0xF00, dark); EZ(Z_COPY);
#else
    (void)light; (void)dark;
#endif
}

#ifndef BENCH
/* 0x01f0 */
static void GameLoop(Game *g)
{
    while (!_keytest(RR_ESC)) {
        ReadKeys(g);
#ifdef ORIGINAL
        Frame(g, GrayGetPlane(LIGHT_PLANE), GrayGetPlane(DARK_PLANE));
#else
        g->vscreen = GrayDBufGetHiddenPlane(DARK_PLANE);
        Frame(g, NULL, NULL);
        GrayDBufToggle();                 /* shown from the next plane switch, no copy */
#endif
    }
}

/* 0x01a2 */
void _main(void)
{
    Game g;
    ClrScr();
    DrawStr(50, 40, "LoAdInG", A_NORMAL);
    if (Init(&g)) {
        GameLoop(&g);
        Quit(&g);
    }
}
#else
/* ------------------------------------------------------------------------ benchmark (ti-cycles) */
/* ti-cycles --arg N: scenario N, a fixed input, frames measured zone by zone; a PNG of the last
 * frame (--png), and for scenario 1 the pixels covered by the 3D objects.
 *   0  start line at rest, 16 frames
 *   1  start line, 8 distant "mountains" (16 triangles, ~10 % of the view), 16 frames
 *   2  full throttle from the start line, 160 frames (the textures move) */
static unsigned char planes[2][0xF00];
static const char *zone_names[] = { 0, "frame", "clear planes", "ship physics", "sky",
    "texture far (8x8)", "texture near (16x16)", "camera+texture", "mode7 far", "mode7 near",
    "near line doubling", "ship sprite", "3D total", "3D cells", "3D faces list", "3D projection",
    "3D fill", "copy to planes", "frame time text", "3D fill: scanlines" };

/* pixels of the 128x100 view drawn by the 3D objects alone: rendered on a light background
 * and on a dark one, a pixel counts if either render changed it (white faces too) */
static short Coverage(Game *g)
{
    unsigned char *vs = g->vscreen;
    static unsigned char diff[1600];
    short pass, r, b, c = 0;
    for (pass = 0; pass < 2; pass++) {
        memset(vs, pass ? 0 : 0xFF, 0xF00);
        memset(vs + 0xF00, pass ? 0xFF : 0, 0xF00);
        Render3D(&g->world, vs);
        for (r = 0; r < 100; r++)
            for (b = 0; b < 16; b++) {
                unsigned char d = pass ? ~vs[r * 30 + b] & vs[0xF00 + r * 30 + b] : vs[r * 30 + b] & ~vs[0xF00 + r * 30 + b];
                d = ~d;                        /* the bits that are no longer the background */
                if (pass) d |= diff[r * 16 + b]; else diff[r * 16 + b] = d;
                if (pass) while (d) { c += d & 1; d >>= 1; }
            }
    }
    return c;
}

/* scenario 1: the start line, 8 distant "mountains" 150 to 240 3D units (600 to 960 world
 * units) ahead, at the edge of the 3D range (VisibleCells looks 4 cells, 256 units, around) */
static const short far_x[8] = { 355, 375, 395, 415, 360, 380, 400, 440 };
static const short far_y[8] = { 160, 120, 205, 80, 250, 40, 290, 170 };

void FillTriRef(short, short, short, short, short, short, short, unsigned char *);
void FillTriNew(short, short, short, short, short, short, short, unsigned char *);
/* scenario 4: random triangles through both fillers, the first that differs */
static void CompareFill(void)
{
    short i, k, v[7];
    unsigned char *ta = malloc(0x1E00), *tb = malloc(0x1E00);   /* not static: keeps the heap
                                                                   layout of the other scenarios */
    for (i = 0; i < 2000; i++) {
        for (k = 0; k < 6; k++) v[k] = (rand() % 300) - 100;
        v[6] = rand() & 3;
        memset(ta, 0x55, 0x1E00); memset(tb, 0x55, 0x1E00);
        FillTriRef(v[0], v[1], v[2], v[3], v[4], v[5], v[6], ta);
        FillTriNew(v[0], v[1], v[2], v[3], v[4], v[5], v[6], tb);
        if (memcmp(ta, tb, 0x1E00)) {
            for (k = 0; k < 7; k++) BENCH_VALUE(v[k]);
            for (k = 0; k < 0x1E00 && ta[k] == tb[k]; k++) ;
            BENCH_VALUE(k); BENCH_VALUE(ta[k]); BENCH_VALUE(tb[k]);
            return;
        }
    }
    BENCH_VALUE(-1);
}

void _main(void)
{
    Game g;
    short scenario = BENCH_ARG, i, n = 16, throttle = 0;
    for (i = 1; i < (short)(sizeof zone_names / sizeof *zone_names); i++) BENCH_NAME(i, zone_names[i]);
    if (scenario == 4) { CompareFill(); return; }
    if (!Init(&g)) return;
    if (scenario == 1) BenchScene(&g.world, 8, far_x, far_y, 140, 80);
    if (scenario == 2) { n = 160; throttle = 1; }
    for (i = 0; i < n; i++) {
        BZ(Z_FRAME);
        BZ(Z_SHIP); DriveShip(&g, 0, throttle); EZ(Z_SHIP);
        Frame(&g, planes[0], planes[1]);
        EZ(Z_FRAME);
    }
    BENCH_DARK_FIRST(DARK_FIRST);
    BENCH_SHOT(g.vscreen);
    if (scenario == 1) BENCH_VALUE(Coverage(&g));
}
#endif
