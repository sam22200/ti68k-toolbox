#include <string.h>
#include "art.h"

static const u8 *bank, *records;
static RtTilemap scenery;
static u16 count;
static const u32 *ring_shifted;
typedef struct { RtSprite sprite; s8 dx, dy; } CachedSprite;
static CachedSprite ring_cache[8];
static u16 word(const u8 *p) { return ((u16)p[0] << 8) | p[1]; }

u8 sonic_art_init(void)
{
    u16 size, tiles, table, offset, i;
    const u8 *p;
    bank = records = RT_NULL; ring_shifted = RT_NULL; count = 0;
    memset(&scenery, 0, sizeof scenery);
    memset(ring_cache, 0, sizeof ring_cache);
    bank = rt_file("sonart", &size);
    if (!bank || size < 32 || memcmp(bank, "SNA1", 4)) return 0;
    tiles = word(bank + 8); count = word(bank + 10);
    table = word(bank + 12); offset = word(bank + 14);
    if (word(bank + 4) != 52 || word(bank + 6) != 64 || !tiles || tiles > 256 ||
        count != ART_COUNT || (u32)table != 32UL + 52 * 64 + ((u32)tiles << 6) ||
        (u32)offset != (u32)table + ((u32)count << 3) || offset > size) return 0;
    if ((word(bank + 16) & 3) || (u32)word(bank + 16) + 6400 > size) return 0;
    ring_shifted = (const u32 *)(bank + word(bank + 16));
    records = bank + table;
    p = records;
    for (i = 0; i < count; i++, p += 8) {
        u16 bytes;
        if ((p[0] != 8 && p[0] != 16 && p[0] != 32) || !p[1] || p[1] > 48 ||
            word(p + 4) < offset || (word(p + 4) & (p[0] == 8 ? 0 : 1))) return 0;
        bytes = (p[0] >> 3) * p[1];
        if ((u32)word(p + 4) + bytes * 3U > word(bank + 16)) return 0;
    }
    scenery.map = bank + 32; scenery.w = 52; scenery.h = 64;
    scenery.tiles = (const u16 *)(bank + 32 + 52 * 64); scenery.ntiles = tiles;
    for (i = 0; i < 8; i++) {
        CachedSprite *c = ring_cache + i;
        u16 bytes;
        p = records + ((ART_RING_0_RIGHT + (i << 1)) << 3);
        c->sprite.w = p[0]; c->sprite.h = p[1]; c->dx = p[2]; c->dy = p[3];
        bytes = (p[0] >> 3) * p[1];
        c->sprite.light = bank + word(p + 4);
        c->sprite.dark = (const u8 *)c->sprite.light + bytes;
        c->sprite.mask = (const u8 *)c->sprite.dark + bytes;
    }
    return 1;
}

void sonic_art_ring(u8 frame, s16 x, s16 y)
{
    const CachedSprite *c = ring_cache + frame;
    draw_sprite((x >> 1) - st.camx + c->dx, (y >> 1) - st.camy + c->dy, &c->sprite);
}

/* A fresh spill overlaps heavily. Compose its identical 8x8 rings once in
 * native rows instead of paying for up to 32 clipped ExtGraph calls. The
 * extraction guarantees dark == opacity for the four ring animation poses. */
u8 sonic_art_burst(void)
{
    const CachedSprite *c = ring_cache + ((st.logic >> 3) & 3);
    s16 left = 32767, top = 32767, right = -32767, bottom = -32767;
    u16 i, n = 0;
    u32 light[32], dark[32], mask[32];
    RtSprite sprite;
    for (i = 0; i < SON_LOST; i++) {
        const SonParticle *p = st.lost + i;
        s16 x, y;
        if (!p->life) continue;
        n++; x = p->x >> 1; y = p->y >> 1;
        if (x < left) left = x;
        if (x > right) right = x;
        if (y < top) top = y;
        if (y > bottom) bottom = y;
    }
    if (n < 8 || right - left > 24 || bottom - top > 24) return 0;
    /* Match the individual renderer's visibility test, including the hidden
     * plane margins. Otherwise use its normal clipping path. */
    { s16 x0 = left - st.camx, x1 = right - st.camx;
      s16 y0 = top - st.camy, y1 = bottom - st.camy;
      if (x0 < -8 || x1 >= RT_W + 8 || y0 < -8 || y1 >= RT_H + 8) return 0;
    }
    memset(light, 0, sizeof light); memset(dark, 0, sizeof dark);
    for (i = 0; i < SON_LOST; i++) {
        const SonParticle *p = st.lost + i;
        const u32 *l, *d;
        u32 *out_l, *out_d;
        u16 dx, row;
        if (!p->life) continue;
        dx = (p->x >> 1) - left;
        l = ring_shifted + (((st.logic >> 3) & 3) * 400U) + (dx << 4);
        d = l + 8;
        row = (p->y >> 1) - top;
        out_l = light + row; out_d = dark + row;
        for (row = 0; row < 8; row++) {
            u32 shape = *d++;
            *out_l = (*out_l & ~shape) | *l++;
            *out_d++ |= shape; out_l++;
        }
    }
    for (i = 0; i < 32; i++) mask[i] = ~dark[i];
    sprite.w = 32; sprite.h = bottom - top + 8;
    sprite.light = light; sprite.dark = dark; sprite.mask = mask;
    draw_sprite(left + c->dx - st.camx, top + c->dy - st.camy, &sprite);
    return 1;
}

void sonic_art_world(void)
{
    draw_tilemap(&scenery, st.camx, st.camy);
}

void sonic_art_draw(u16 id, s16 x, s16 y)
{
    const u8 *p;
    u16 bytes;
    RtSprite sprite;
    if (id >= count) return;
    p = records + (id << 3);
    sprite.w = p[0]; sprite.h = p[1];
    x = (x >> 1) - st.camx + (s8)p[2];
    y = (y >> 1) - st.camy + (s8)p[3];
    if (x >= RT_W || y >= RT_H || x + sprite.w <= 0 || y + sprite.h <= 0) return;
    bytes = (sprite.w >> 3) * sprite.h;
    sprite.light = bank + word(p + 4);
    sprite.dark = (const u8 *)sprite.light + bytes;
    sprite.mask = (const u8 *)sprite.dark + bytes;
    draw_sprite(x, y, &sprite);
}

void sonic_art_player(void)
{
    static const u8 walk[] = {0, 1, 2, 3, 4, 5, 0, 1};
    static const u8 roll[] = {0, 1, 2, 3, 4, 0, 1, 2};
    u16 id = ART_SONIC_1_RIGHT;
    s16 speed = st.speed < 0 ? -st.speed : st.speed;
    if (!st.hurt && st.invuln && !(st.invuln & 8)) return;
    if (st.hurt) id = ART_SONIC_85_RIGHT;
    else if (st.flags & S_ROLL) id = ART_SONIC_46_RIGHT + ((u16)roll[(st.logic >> 2) & 7] << 1);
    else if (!(st.flags & S_AIR) && (rt_keys & K_DOWN) && speed < 128) id = ART_SONIC_57_RIGHT;
    else if (speed >= 1536) id = ART_SONIC_30_RIGHT + ((st.logic >> 1) & 3) * 2U;
    else if (speed) id = ART_SONIC_6_RIGHT + ((u16)walk[(st.logic >> (speed > 768 ? 2 : 3)) & 7] << 1);
    id += (st.flags & S_LEFT) != 0;
    sonic_art_draw(id, st.x, st.y);
}
