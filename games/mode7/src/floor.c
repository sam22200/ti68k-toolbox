/* Mode 7 - Demo 2 (David Coz, 2005): the floor textures, the camera, the sky and the ship sprite.
 * Reconstructed from the binary (see mode7.h). Original addresses in the comments.
 *
 * The Mode 7 routines (render.s) sample a 128x128 texture, rebuilt around the camera from the
 * 64x64 tile map: the near texture from 8x8 map cells of 16x16 tiles (4 world units per
 * texel), the far texture from 16x16 cells of 8x8 tiles (the same tiles shrunk: 8 units per
 * texel, twice the area for the distant rows). Only one texture is rebuilt per frame, in turn;
 * the other one's camera coordinates just follow the camera. */
#include "mode7.h"

static short frame_parity;                /* 0x1598 */
/* optimisation: the map cell each texture was last built from; a texture is rebuilt only when
 * its window moves to another cell (the original rebuilt one 16 KB texture every frame) */
static unsigned char *built_far, *built_near;

/* 0x310a: the 128x45 sky image into a buffer with the screen's 30-byte rows */
void InitSky(Game *g)
{
    short row, x;
    memset(g->sky, 0, 0xA8C);
    for (row = 0; row < 90; row++)
        for (x = 0; x < 16; x++)
            g->sky[row * 30 + x] = sky_image[row * 16 + x];
}

/* 0x4392: sky rows 0..44 of both planes */
void DrawSky(Game *g)
{
#ifdef ORIGINAL
    memcpy(g->vscreen, g->sky + 0x546, 0x546);
    memcpy(g->vscreen + 0xF00, g->sky, 0x546);
#else
    SkyCopy(g->sky, g->sky + 0x546, g->vscreen, 45);   /* optimised: 16 bytes per row, movem;
                                                          dark rows to the dark plane first */
#endif
}

/* 0x2c04: "Tile Conversion": the two 1-bit planes of each 16x16 tile to one byte per pixel */
void ConvertTiles(Game *g)
{
    short t, i, b;
    for (t = 0; t < NB_TILES; t++) {
        const unsigned char *src = tiles_gray + ((long)t << 6);
        unsigned char *dst = g->tiles16 + ((long)t << 8);
        for (i = 0; i < 32; i++, src++) {         /* 16 rows of 2 bytes */
            unsigned char a = src[0], b2 = src[32];
            short x0 = (i & 1) << 3;
            for (b = 0; b < 8; b++) {
                short mask = 0x80 >> b;
                dst[((long)(i >> 1) << 4) + x0 + b] = ((a & mask) ? 2 : 0) + ((b2 & mask) ? 1 : 0);
            }
        }
    }
}

/* 0x42c8: "Create 8*8 Tiles": each 8x8 pixel = (3 * a random pixel of its 2x2 block + the top-left
 * one + a random 0..2) / 4, a dithered shrink */
void CreateTiles8(Game *g)
{
    short t, y, x;
    for (t = 0; t < NB_TILES; t++) {
        signed char *src = (signed char *)g->tiles16 + ((long)t << 8);
        unsigned char *dst = g->tiles8 + ((long)t << 6);
        for (y = 0; y < 8; y++)
            for (x = 0; x < 8; x++) {
                unsigned short r1 = rand(), r2 = rand(), r3 = rand();
                short v = 3 * src[(2 * x + (r2 >> 14)) + ((long)(2 * y + (r1 >> 14)) << 4)]
                          + src[((long)(2 * y) << 4) + 2 * x];
                v += ((unsigned long)r3 * 3) >> 15;
                dst[((long)y << 3) + x] = v / 4;
            }
    }
}

#ifndef ORIGINAL
/* optimised floor: texels with their plane bits at the top of the byte (render.s) */
void EncodeTiles(Game *g)
{
    /* bit 7 goes to the plane at +0xF00 (light), bit 6 to the plane at vscreen (dark); pixel
     * value t: bit 0 light, bit 1 dark */
    static const unsigned char code[4] = { 0x00, 0x80, 0x40, 0xC0 };
    unsigned short i;
    for (i = 0; i < (NB_TILES << 8); i++) g->tiles16[i] = code[g->tiles16[i] & 3];
    for (i = 0; i < (NB_TILES << 6); i++) g->tiles8[i] = code[g->tiles8[i] & 3];
}
#endif

/* 0x3a3e: the camera 60 units behind the ship */
static void CameraFollow(Game *g)
{
    unsigned char a = g->angle;
    g->camx = ((sin128[a] * 60) >> 7) + g->shipx;
    g->camy = ((cos128[a] * -60) >> 7) + g->shipy;
}

/* 0x3078: far texture window: 16 tiles of 32 half units around a point ahead of the camera,
 * and the camera's position in it (half units, + 256 = the window's centre) */
static void SetFar(Game *g)
{
    unsigned char a = g->angle;
    short dx = (sin128[a] << 5) >> 5;     /* sic: a no-op scaling in the original */
    short dy = -((cos128[a] << 5) >> 5);
    short x = (g->camx >> 1) - dx - 256;
    short y = (g->camy >> 1) - dy - 256;
    short tx = x >> 5, ty = y >> 5;
    long ofs;
    g->far_tx = tx;
    g->far_ty = ty;
    g->far_u = (x & 31) + dx + 256;
    g->far_v = (y & 31) + dy + 256;
    ofs = (long)tx + (short)(ty << 6);
    g->unused856 = (char *)g->unused84e + (ofs << 2);
    g->map_far = track_map + ofs;
}

/* 0x3b0a: near texture window: 8 tiles of 64 units */
static void SetNear(Game *g)
{
    unsigned char a = g->angle;
    short dx = (sin128[a] * 48) >> 5;
    short dy = -((cos128[a] * 48) >> 5);
    short x = g->camx - dx - 256;
    short y = g->camy - dy - 256;
    short tx = x >> 6, ty = y >> 6;
    long ofs;
    g->near_tx = tx;
    g->near_ty = ty;
    g->near_u = (x & 63) + dx + 256;
    g->near_v = (y & 63) + dy + 256;
    ofs = (long)tx + (short)(ty << 6);
    g->unused85a = (char *)g->unused852 + (ofs << 2);
    g->map_near = track_map + ofs;
}

/* the textures must be rebuilt (a new run: the statics survive between runs on the TI) */
void ResetTextures(void)
{
    built_far = built_near = NULL;
}

/* 0x3a76 */
void UpdateCamera(Game *g)
{
    frame_parity++;
    CameraFollow(g);
    if (frame_parity & 1) {
        SetFar(g);
#ifdef ORIGINAL
        BZ(Z_TEX8); BuildTexture8(g->map_far, g->tiles8, g->tex_far); EZ(Z_TEX8);
#else
        if (g->map_far != built_far) {        /* optimised: only when the window moved */
            BZ(Z_TEX8); BuildTexture8W(g->map_far, g->tiles8, g->tex_far); EZ(Z_TEX8);
            built_far = g->map_far;
        }
#endif
        g->near_u += g->camx - g->prev_camx;
        g->near_v += g->camy - g->prev_camy;
    } else {
        SetNear(g);
#ifdef ORIGINAL
        BZ(Z_TEX16); BuildTexture16(g->map_near, g->tiles16, g->tex_near); EZ(Z_TEX16);
#else
        if (g->map_near != built_near) {        /* optimised: only when the window moved */
            BZ(Z_TEX16); BuildTexture16W(g->map_near, g->tiles16, g->tex_near); EZ(Z_TEX16);
            built_near = g->map_near;
        }
#endif
        g->far_u += (short)(g->camx - g->prev_camx) >> 1;
        g->far_v += (short)(g->camy - g->prev_camy) >> 1;
    }
    g->prev_camx = g->camx;
    g->prev_camy = g->camy;
    g->prev_shipx = g->shipx;
    g->prev_shipy = g->shipy;
}

/* 0x365e: the ship, 32x24, always at the same place on screen */
void DrawShip(Game *g)
{
    unsigned char *vs = g->vscreen;
#if DARK_FIRST
    Sprite32Gray(52, 73, 24, ship_dark, ship_light, ship_mask, ship_mask, vs, vs + 0xF00);
#else
    Sprite32Gray(52, 73, 24, ship_light, ship_dark, ship_mask, ship_mask, vs, vs + 0xF00);
#endif
}
