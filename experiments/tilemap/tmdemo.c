// TileMap engine (ExtGraph's tilemap.a) scrolling a 512x320 grey map drawn straight into the
// GrayDBuf hidden planes, plus 6 semi-preshifted 16x16 grey sprites (preshift.h) on top.
// Build: ti-cc -o tmdemo tmdemo.c ../../tools/extgraph/lib/tilemap.a
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "extgraph.h"
#include "tilemap.h"
#include "preshift.h"

#define MW 32
#define MH 20
static unsigned char map[MH][MW];
static unsigned short tiles[4][32];                  // grey tiles, interlaced: light row, dark row
static unsigned short ball[32];                      // grey sprite, interlaced
static volatile unsigned short ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }

static void make_data(void)
{
    short r, x, y;
    for (r = 0; r < 16; r++) {
        tiles[1][2 * r] = (r & 1) ? 0xAAAA : 0x5555;                       // light checker
        tiles[2][2 * r + 1] = (r == 0 || r == 15) ? 0xFFFF : 0x8001;       // dark frame
        tiles[3][2 * r] = tiles[3][2 * r + 1] = (r > 3 && r < 12) ? 0x3FFC : 0; // black block
        { unsigned short hw = (r < 8 ? r : 15 - r) + 1;                  // disc, half width 1..8
          unsigned short m = (0xFFFF >> (8 - hw)) & (unsigned short)(0xFFFF << (8 - hw));
          unsigned short in = hw > 1 ? (0xFFFF >> (9 - hw)) & (unsigned short)(0xFFFF << (9 - hw)) : 0;
          ball[2 * r] = m;                                                  // light: whole disc
          ball[2 * r + 1] = (r == 0 || r == 15) ? m : m & (unsigned short)~in; }  // dark: rim -> black
    }
    for (y = 0; y < MH; y++) for (x = 0; x < MW; x++)
        map[y][x] = (x == 0 || y == 0 || x == MW - 1 || y == MH - 1) ? 2 : ((x ^ y) & 3) == 0 ? 3 : ((x + y) & 1);
}

void _main(void)
{
    INT_HANDLER old_int1 = GetIntVec(AUTO_INT_1);
    void *dbuf = malloc(GRAYDBUFFER_SIZE);
    char *big = malloc(GRAY_BIG_VSCREEN_SIZE);
    unsigned long *pball = malloc(SIZE_OF_PGSPRITE16x16);
    Plane pl;
    short cx = 0, cy = 0, dx = 1, dy = 1, k, contig = 1;
    long dgray = 0, dhid[2] = {0, 0};
    unsigned short frames = 0;
    if (!dbuf || !big || !pball) goto out;
    make_data();
    PreshiftGrayISprite16x16(ball, pball);
    pl.matrix = map; pl.width = MW; pl.sprites = tiles; pl.big_vscreen = big; pl.force_update = 1;
    SetIntVec(AUTO_INT_1, DUMMY_HANDLER);
    if (!GrayOn()) goto out;
    GraySetInt1Handler(tick_handler);
    dgray = (char *)GrayGetPlane(DARK_PLANE) - (char *)GrayGetPlane(LIGHT_PLANE);
    GrayDBufInit(dbuf);
    for (k = 0; k < 2; k++) {                          // both buffers: dark plane right after light?
        dhid[k] = (char *)GrayDBufGetHiddenPlane(DARK_PLANE) - (char *)GrayDBufGetHiddenPlane(LIGHT_PLANE);
        if (dhid[k] != -LCD_SIZE) contig = 0;             // dark plane just before light
        GrayDBufToggle();
    }
    ticks = 0;
    while (ticks < 256 * 8) {                          // 8 s
        void *l = GrayDBufGetHiddenPlane(LIGHT_PLANE), *d = GrayDBufGetHiddenPlane(DARK_PLANE);
        if (contig) DrawPlane(cx, cy, &pl, d, TM_GRPLC89, TM_G16B);   // dest = first plane in memory
        for (k = 0; k < 6; k++) {
            short x = 10 + ((frames * (k + 1) + k * 37) % 130), y = 5 + ((frames * 2 + k * 29) % 75);
            GrayPSprite16x16_OR_R(x, y, pball, l, d);
        }
        GrayDBufToggleSync();
        frames++;
        cx += dx; cy += dy;
        if (cx <= 0 || cx >= MW * 16 - 160) dx = -dx;
        if (cy <= 0 || cy >= MH * 16 - 100) dy = -dy;
    }
    GraySetInt1Handler(DUMMY_HANDLER);
    GrayOff();
out:
    SetIntVec(AUTO_INT_1, old_int1);
    free(pball); free(big); free(dbuf);
    ClrScr();
    printf_xy(0, 0, "HW%d planes contiguous: %s", (short)HW_VERSION, contig ? "yes" : "NO");
    printf_xy(0, 10, "%u frames in 8 s = %u fps", frames, frames / 8);
    printf_xy(0, 20, "GrayOn  d-l %ld", dgray);
    printf_xy(0, 30, "dbuf    d-l %ld %ld", dhid[0], dhid[1]);
    GKeyFlush();
    ngetchx();
}
