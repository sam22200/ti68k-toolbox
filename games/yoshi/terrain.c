#include "terrain.h"
#include <string.h>

static const u8 *cells, *profiles;
static const u8 *images;
static s16 minimum_height;
static RtTilemap view;

static u16 be16(const u8 *p) { return ((u16)p[0] << 8) | p[1]; }

u8 terrain_ready(void) { return cells != RT_NULL; }

u8 terrain_init(void)
{
    const u8 *p;
    u16 size, count, po, vo, to, nt, io;
    cells = profiles = images = RT_NULL;
    minimum_height = 0;
    memset(&view, 0, sizeof(view));
    p = rt_file("yjterr", &size);
    if (!p || size < 32 || memcmp(p, "YTR4", 4)) return 0;
    count = be16(p + 12); nt = be16(p + 18);
    po = be16(p + 20); vo = be16(p + 22); to = be16(p + 24);
    io = be16(p + 28);
    if (be16(p + 4) != 80 || be16(p + 6) != 32 || be16(p + 8) != YT_TOP ||
        be16(p + 10) != 128 || !count || count > 256 || !nt || nt > 256 ||
        be16(p + 14) != 48 || be16(p + 16) != 24 || po != 4128 ||
        vo != po + (count << 5) || to != vo + 1152 ||
        (u32)to + ((u32)nt << 6) != io || be16(p + 30) != 80 ||
        (u32)io + 40960UL != be16(p + 26) || be16(p + 26) > size) return 0;
    /* Validate indices once, never in the frame's collision/render loops. */
    { const u8 *q = p + 32; u16 n = 4096;
      while (n--) if (*q++ >= count) return 0;
      q = p + vo; n = 1152;
      while (n--) if (*q++ >= nt) return 0;
    }
    cells = p + 32; profiles = p + po;
    { const u8 *q = profiles; u16 n = count, x;
      while (n--) {
          for (x = 2; x < 18; ++x)
              if ((s8)q[x] < minimum_height) minimum_height = (s8)q[x];
          q += 32;
      }
    }
    view.map = p + vo; view.w = 48; view.h = 24;
    view.tiles = (const u16 *)(p + to); view.ntiles = nt;
    images = p + io;
    tilemap_dirty();
    return 1;
}

static const u8 *profile(u16 x, u16 y)
{
    if (!cells || x >= 1280 || y < YT_TOP || y >= YT_BOTTOM) return RT_NULL;
    return profiles + ((u16)cells[(((y - YT_TOP) >> 4) << 7) + (x >> 4)] << 5);
}

u8 terrain_solid(u16 x, u16 y)
{
    const u8 *p = profile(x, y);
    return p && (p[0] & 2);
}

s16 terrain_floor(u16 x, s16 low, s16 high, u8 *angle)
{
    static const u8 feet[3] = {3, 8, 13};
    s16 best = YT_NONE, start = low - 32, end = high + 16;
    u16 row, last, fx;
    u8 foot;
    if (!cells || high < YT_TOP || low >= YT_BOTTOM) return best;
    if (start < YT_TOP) start = YT_TOP;
    if (end >= YT_BOTTOM) end = YT_BOTTOM - 1;
    last = ((u16)end - YT_TOP) >> 4;
    for (foot = 0; foot < 3; ++foot) {
        fx = x + feet[foot];
        if (fx >= 1280) continue;
        for (row = ((u16)start - YT_TOP) >> 4; row <= last; ++row) {
            const u8 *p = profiles + ((u16)cells[(row << 7) + (fx >> 4)] << 5);
            s16 top;
            if (!(p[0] & 7)) continue;
            top = YT_TOP + (row << 4) + (s8)p[2 + (fx & 15)];
            if (top >= low && top <= high && top < best) {
                best = top;
                if (angle) *angle = p[1];
            }
        }
    }
    return best;
}

/* A full native image window avoids TileMap's cold cache spikes. Dispatch
   once per plane, then copy ten overlapping aligned pairs per scanline.
   The 68000 permits long reads on even addresses; no row-call overhead. */
#define W10(op) op op op op op op op op op op
#ifdef __m68k__
#define ROWS(op) do { for(r=0;r<RT_H;++r) { W10(op) s+=30; d+=5; } } while(0)
static void image_plane(u8 *dst,const u8 *src,u16 x,u16 y)
{
    const u16 *s=(const u16 *)(src+(y<<6)+(y<<4)+((x>>4)<<1));
    u16 *d=(u16 *)dst, sh=x&15, r;
    if(!sh) ROWS(*d++=*s++;);
    else if(sh==8) {
        const u8 *b=(const u8 *)s+1;
        u8 *a=dst;
        for(r=0;r<RT_H;++r) { W10(*a++=*b++; *a++=*b++;) b+=60; a+=10; }
    } else if(sh<7) ROWS(*d++=(u16)(((*(const u32 *)s)<<sh)>>16); ++s;);
    else { u16 rs=16-sh; ROWS(*d++=(u16)(*(const u32 *)s>>rs); ++s;); }
}
#undef ROWS
#else
static void image_plane(u8 *dst,const u8 *src,u16 x,u16 y)
{
    u16 r,c;
    for(r=0;r<RT_H;++r) {
        const u8 *s=src+(y+r)*80+(x>>3);
        u8 sh=x&7;
        for(c=0;c<20;++c) dst[c]=sh?(u8)((s[c]<<sh)|(s[c+1]>>(8-sh))):s[c];
        dst+=RT_PBYTES;
    }
}
#endif
#undef W10
void terrain_render(u16 camx,u16 camy)
{
    if(!images) return;
    image_plane(rt_light,images,camx,camy);
    image_plane(rt_dark,images+20480,camx,camy);
}

/* Independent bit-by-bit image-window oracle, only used by PC tests. */
#ifndef __m68k__
void terrain_render_reference(u16 camx, u16 camy)
{
    u16 x,y,plane;
    if(!images) return;
    for(plane=0;plane<2;++plane) {
        const u8 *src=images+plane*20480;
        u8 *dest=plane?(u8 *)rt_dark:(u8 *)rt_light;
        for(y=0;y<RT_H;++y) {
            memset(dest+y*RT_PBYTES,0,RT_W>>3);
            for(x=0;x<RT_W;++x) {
                u16 sx=camx+x;
                if(src[(camy+y)*80+(sx>>3)] & (128>>(sx&7)))
                    dest[y*RT_PBYTES+(x>>3)] |= 128>>(x&7);
            }
        }
    }
}
#endif

/* Six-pixel Shy Guy hitboxes use one central support sample. Avoid the
   player's three-foot query and repeated row indexing for these tiny actors. */
s16 terrain_actor_floor(u16 center, s16 low, s16 high)
{
    s16 start = low - 32, end = high + 16, top, best = YT_NONE;
    const u8 *c;
    u16 sample = 2 + (center & 15);
    if (!cells || center >= 1280 || high < YT_TOP || low >= YT_BOTTOM) return best;
    if (start < YT_TOP) start = YT_TOP;
    if (end >= YT_BOTTOM) end = YT_BOTTOM - 1;
    start = YT_TOP + (((u16)start - YT_TOP) & 65520);
    c = cells + ((((u16)start - YT_TOP) >> 4) << 7) + (center >> 4);
    while (start <= end) {
        const u8 *p = profiles + ((u16)*c << 5);
        if (p[0] & 7) {
            top = start + (s8)p[sample];
            if (top >= low && top <= high && top < best) {
                best = top;
                /* Later rows cannot beat this surface once even their
                   minimum signed profile height reaches it. */
                if (best - minimum_height - 1 < end) end = best - minimum_height - 1;
            }
        }
        start += 16; c += 128;
    }
    return best;
}
