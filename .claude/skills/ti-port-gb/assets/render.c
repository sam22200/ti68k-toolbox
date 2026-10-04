// Bubble Ghost renderer: the GB screen rebuilt from VRAM, the I/O registers and the OAM.
// The BG and window maps are cached as plane pairs (256 x 256 each, a GB tile row's two
// bytes through BGP = the light and dark plane bytes), redrawn per dirty cell; sprites are
// the 40 OAM entries, converted per frame through OBP0/OBP1 with their flips, drawn back to
// front in DMG priority (smaller x in front, then the lower index), BG-priority honoured.
// In a hall: GB rows 16..95 at TI rows 0..79 and the port's HUD below (decision 5); other
// screens: a 100-row band of the 144 (screens.c chooses its top).
#include <stdlib.h>
#include "vars.h"
#include "render.h"
#ifdef RT_CYCLES
#include "../../tools/m68kbench/bench.h"
#define Z_BEGIN(id, name) do { static u8 named; if (!named) { BENCH_NAME(id, name); named = 1; } BENCH_BEGIN(id); } while (0)
#define Z_END(id) BENCH_END(id)
#else
#define Z_BEGIN(id, name)
#define Z_END(id)
#endif

#define MAP_BYTES (32 * 8 * 32)        // a 256 x 256 plane
static u8 *map_l[2], *map_d[2];        // 9800 and 9C00
static u8 dirty[2][128];               // a bit per cell
static u8 all_dirty[2], any_dirty, spr_dirty;
static u8 cache_lcdc = 0xFF, cache_bgp;  // the tile addressing and palette the cache was drawn with
u8 in_play;                            // play.c: a hall is on screen (the HUD layout)
u8 screen_kind;                        // screens.c: the screen outside a hall
u8 secret_blink;                       // hazards.c: frames left of the score's blink
// GB rows shown per screen: a band, or two stitched (decision of 2026-10-03, see README):
// first row, rows taken, then the second band's first row (100 rows in all)
static const u8 bands[][3] = {
    { 22, 100, 0 },                    // SK_LOGO1: Pony Canyon, rows 40..103
    { 35, 100, 0 },                    // SK_LOGO2: the Infogrames licence, 32..134
    { 6, 90, 103 },                    // SK_TITLE: the logo 8..95, then PUSH START (no copyright)
    { 32, 100, 0 },                    // SK_TABLE: the five entries (no title box)
    { 30, 100, 0 },                    // SK_OVER: GAME OVER, CONTINUE / START
};
static u8 hud_l[20 * RT_PBYTES], hud_d[20 * RT_PBYTES];
static u8 hud_key[6];

void vram_dirty(u16 a)
{
    if (a >= 0x9800 && a < 0xA000) {
        u16 c = (u16)(a - 0x9800);
        dirty[c >> 10][(c & 1023) >> 3] |= (u8)(1 << (c & 7));
        any_dirty = 1;
    } else {                            // tile data: every cell, and the sprite tiles
        all_dirty[0] = all_dirty[1] = 1;
        if (a < 0x9000) spr_dirty = 1;
    }
}

void render_reset(void)
{
    u16 k;
    all_dirty[0] = all_dirty[1] = spr_dirty = 1;
    cache_lcdc = 0xFF;
    for (k = 0; k < 6; k++) hud_key[k] = 0;
    in_play = 0;
    secret_blink = 0;
}

// planes of one 8-pixel row through a palette: c = lo | hi << 1, shade = (pal >> 2c) & 3
static void through(u8 lo, u8 hi, u8 pal, u8 *pl, u8 *pd)
{
    u8 c[4], l = 0, d = 0, k;
    if (pal == 0xE4) { *pl = lo; *pd = hi; return; }
    c[0] = (u8)(~lo & ~hi); c[1] = (u8)(lo & ~hi); c[2] = (u8)(~lo & hi); c[3] = (u8)(lo & hi);
    for (k = 0; k < 4; k++) {
        u8 s = (u8)((pal >> (2 * k)) & 3);
        if (s & 1) l |= c[k];
        if (s & 2) d |= c[k];
    }
    *pl = l; *pd = d;
}

static u16 tile_addr(u8 t)
{
    if (IO(LCDC) & 0x10) return (u16)(0x8000 + ((u16)t << 4));
    return (u16)(0x9000 + (s16)(s8)t * 16);
}

static void draw_cell(u8 m, u16 c)
{
    const u8 *src = gb_vram + tile_addr(gb_vram[(m ? 0x1C00 : 0x1800) + c]) - 0x8000;
    u16 o = (u16)((c >> 5) * 8 * 32 + (c & 31));
    u8 *l = map_l[m] + o, *d = map_d[m] + o, y, pal = IO(BGP);
    if (pal == 0xE4)                                           // the usual palette: as they are
        for (y = 0; y < 8; y++, l += 32, d += 32) { *l = *src++; *d = *src++; }
    else
        for (y = 0; y < 8; y++, l += 32, d += 32, src += 2) through(src[0], src[1], pal, l, d);
}

static void update_maps(void)
{
    u16 c;
    u8 m, lcdc = IO(LCDC), used[2];
    if (!(lcdc & 0x80)) return;                                // LCD off: white, draw later
    if ((lcdc & 0x10) != cache_lcdc || IO(BGP) != cache_bgp) {
        cache_lcdc = lcdc & 0x10;
        cache_bgp = IO(BGP);
        all_dirty[0] = all_dirty[1] = 1;
    }
    used[0] = used[1] = 0;                                     // the maps on screen only
    if (lcdc & 0x01) used[(lcdc & 0x08) ? 1 : 0] = 1;
    if (lcdc & 0x20) used[(lcdc & 0x40) ? 1 : 0] = 1;
    for (m = 0; m < 2; m++) {
        if (!used[m]) continue;
        if (all_dirty[m]) {
            for (c = 0; c < 1024; c++) draw_cell(m, c);
            all_dirty[m] = 0;
            for (c = 0; c < 128; c++) dirty[m][c] = 0;
            continue;
        }
        if (!any_dirty) continue;
        for (c = 0; c < 128; c++)
            if (dirty[m][c]) {
                u8 b, bits = dirty[m][c];
                for (b = 0; b < 8; b++) if (bits & (1 << b)) draw_cell(m, (u16)(c * 8 + b));
                dirty[m][c] = 0;
            }
    }
    any_dirty = (u8)(!used[0] || !used[1]);                    // an unused map keeps its marks
}

// GB screen rows gy.. into TI rows ty.. (n of them): the BG (SCY, SCX = 0), then the window
static void put_rows(u8 gy, u8 ty, u8 n)
{
    u8 lcdc = IO(LCDC), r, bm = (lcdc & 0x08) ? 1 : 0, wm = (lcdc & 0x40) ? 1 : 0;
    u8 win = (lcdc & 0xA0) == 0xA0 && IO(WX) == 7, wy = IO(WY), on = (lcdc & 0x81) == 0x81;
    u8 *dl = (u8 *)rt_light + ty * RT_PBYTES, *dd = rt_dark ? (u8 *)rt_dark + ty * RT_PBYTES : 0;
    for (r = 0; r < n; r++, gy++, dl += RT_PBYTES, dd = dd ? dd + RT_PBYTES : 0) {
        u32 *ol = (u32 *)dl, *od = (u32 *)dd;
        const u32 *sl, *sd;
        if (win && gy >= wy) {
            u16 o = (u16)((u8)(gy - wy) * 32);
            sl = (const u32 *)(map_l[wm] + o); sd = (const u32 *)(map_d[wm] + o);
        } else if (on) {
            u16 o = (u16)((u8)(gy + IO(SCY)) * 32);
            sl = (const u32 *)(map_l[bm] + o); sd = (const u32 *)(map_d[bm] + o);
        } else {                                               // LCD or BG off: white
            ol[0] = ol[1] = ol[2] = ol[3] = ol[4] = 0;
            if (od) od[0] = od[1] = od[2] = od[3] = od[4] = 0;
            continue;
        }
        ol[0] = sl[0]; ol[1] = sl[1]; ol[2] = sl[2]; ol[3] = sl[3]; ol[4] = sl[4];
        if (od) { od[0] = sd[0]; od[1] = sd[1]; od[2] = sd[2]; od[3] = sd[3]; od[4] = sd[4]; }
    }
}

// ---------------------------------------------------------------- sprites
// sprite tiles converted once per palette (OBP0, OBP1) and flip (none, x, y, both): 8 rows of
// light, dark, mask in blocks of 256 tiles (6 KB) allocated when a combination first shows;
// redone when the sprite tiles or the palettes change
static u8 *spr_cache[8];               // [flip << 1 | palette]
static u8 spr_ok[8][32], spr_pal[2], rev[256];

static const u8 *spr_tile(u8 p, u8 f, u8 t)
{
    u8 b = (u8)(f << 1 | p), *e;
    if (!spr_cache[b] && !(spr_cache[b] = malloc(256 * 24))) return RT_NULL;
    e = spr_cache[b] + (u16)t * 24;
    if (!(spr_ok[b][t >> 3] & (1 << (t & 7)))) {
        const u8 *src = gb_vram + ((u16)t << 4);
        u8 y;
        for (y = 0; y < 8; y++) {
            const u8 *r = src + 2 * ((f & 2) ? 7 - y : y);
            u8 lo = r[0], hi = r[1], m, l, d;
            if (f & 1) { lo = rev[lo]; hi = rev[hi]; }
            m = (u8)~(lo | hi);                                // colour 0 is transparent
            through(lo, hi, spr_pal[p], &l, &d);
            e[y] = (u8)(l & ~m); e[8 + y] = (u8)(d & ~m); e[16 + y] = m;
        }
        spr_ok[b][t >> 3] |= (u8)(1 << (t & 7));
    }
    return e;
}

// one OAM entry at screen (sx, sy), GB row gy: from the cache; BG priority (flag 80): hidden
// over BG colours 1-3, read from the BG map cache (the BG only, as on the DMG)
static void sprite8(s16 sx, s16 sy, u8 gy, u8 tile, u8 flags, u8 cut)
{
    u8 f = (u8)(((flags >> 5) & 1) | ((flags >> 5) & 2)), y;
    const u8 *e = spr_tile((flags & 0x10) ? 1 : 0, f, tile);
    RtSprite sp;
    if (!e || cut == 0xFF) return;
    sp.w = 8; sp.h = 8;
    if (cut) {                                                 // rows over the 10-per-line limit
        u8 light[8], dark[8], mask[8];
        for (y = 0; y < 8; y++)
            if (cut & (1 << y)) { light[y] = dark[y] = 0; mask[y] = 0xFF; }
            else { light[y] = e[y]; dark[y] = e[8 + y]; mask[y] = e[16 + y]; }
        if (flags & 0x80) { static u8 l2[24]; for (y = 0; y < 8; y++) { l2[y] = light[y]; l2[8 + y] = dark[y]; l2[16 + y] = mask[y]; } e = l2; }
        else { sp.light = light; sp.dark = dark; sp.mask = mask; draw_sprite(sx, sy, &sp); return; }
    }
    if (!(flags & 0x80) || sx <= -8 || sx >= 160) {
        sp.light = e; sp.dark = e + 8; sp.mask = e + 16;
        draw_sprite(sx, sy, &sp);
        return;
    }
    {
        u8 light[8], dark[8], mask[8], m = (IO(LCDC) & 0x08) ? 1 : 0;
        u8 row = (u8)(gy + IO(SCY)), i = (u8)((sx < 0 ? 0 : sx) >> 3), sh = (u8)((sx < 0 ? 0 : sx) & 7);
        u8 pad = (u8)(sx < 0 ? -sx : 0);
        const u8 *ml = map_l[m] + i, *md = map_d[m] + i;
        for (y = 0; y < 8; y++, row++) {
            u16 o = (u16)row << 5, bg = (u16)((ml[o] | md[o]) << 8);
            u8 hide;
            if (i < 31) bg |= (u8)(ml[o + 1] | md[o + 1]);
            hide = (u8)((u16)(bg << sh) >> 8);
            if (pad) hide = (u8)(hide >> pad);
            mask[y] = (u8)(e[16 + y] | hide); light[y] = (u8)(e[y] & ~hide); dark[y] = (u8)(e[8 + y] & ~hide);
        }
        sp.light = light; sp.dark = dark; sp.mask = mask;
        draw_sprite(sx, sy, &sp);
    }
}

static void draw_sprites(s16 dy, u8 skip_hud)
{
    u16 key[40], t;                                            // x << 8 | index
    u8 cut[40];                                                // rows hidden by the line limit
    u8 n = 0, i, j, h = (IO(LCDC) & 0x04) ? 16 : 8;
    if (!(IO(LCDC) & 0x02) || !(IO(LCDC) & 0x80)) return;
    if (spr_dirty || spr_pal[0] != IO(OBP0) || spr_pal[1] != IO(OBP1)) {
        for (i = 0; i < 32; i++) for (j = 0; j < 8; j++) spr_ok[j][i] = 0;
        spr_pal[0] = IO(OBP0); spr_pal[1] = IO(OBP1);
        spr_dirty = 0;
    }
    Z_BEGIN(13, "render: sprite sort");
    {                                                          // the DMG shows 10 per line, the
        u8 cnt[144 + 16], k;                                   // first by OAM index (any x)
        for (k = 0; k < sizeof(cnt); k++) cnt[k] = 0;
        for (i = 0; i < 40; i++) {
            u8 y0 = gb_oam[4 * i];
            cut[i] = 0;
            if (y0 == 0 || y0 >= 160) continue;
            for (k = 0; k < h; k++) {
                u8 line = (u8)(y0 + k);                        // screen row + 16
                if (line < 16 || line >= 160) continue;
                if (++cnt[line] > 10) cut[i] |= (u8)(1 << k);
            }
        }
    }
    for (i = 0; i < 40; i++) {
        u8 y = gb_oam[4 * i], x = gb_oam[4 * i + 1];
        if (y == 0 || y >= 160 || x == 0 || x >= 168) continue;
        if (skip_hud && i >= 4 && i < 8) continue;             // the window HUD's lives icon
        t = (u16)(x << 8 | i);                                 // back to front: descending keys
        for (j = n++; j > 0 && key[j - 1] < t; j--) key[j] = key[j - 1];
        key[j] = t;
    }
    Z_END(13);
    for (i = 0; i < n; i++) {
        const u8 *e = &gb_oam[4 * (key[i] & 0xFF)];
        if (h == 16) {                                         // 8 x 16: two tiles (unused here)
            u8 t = (u8)(e[2] & 0xFE), fl = (e[3] & 0x40) ? 1 : 0;
            sprite8((s16)e[1] - 8, (s16)e[0] - 16 + dy, (u8)(e[0] - 16), (u8)(t + fl), e[3], 0);
            sprite8((s16)e[1] - 8, (s16)e[0] - 8 + dy, (u8)(e[0] - 8), (u8)(t + 1 - fl), e[3], 0);
        } else sprite8((s16)e[1] - 8, (s16)e[0] - 16 + dy, (u8)(e[0] - 16), e[2], e[3], cut[key[i] & 0xFF]);
    }
}

// ---------------------------------------------------------------- the HUD (decision 5)
static char *put_str(char *s, const char *t) { while (*t) *s++ = *t++; return s; }
static char *put_num(char *s, u16 v, u8 digits)
{
    char *e = s + digits;
    *e = 0;
    while (digits--) { s[digits] = (char)('0' + v % 10); v /= 10; }
    return e;
}

static void render_hud(void)
{
    void *l = rt_light, *d = rt_dark;
    char buf[40], *s;
    u16 k, w;
    for (k = 0; k < sizeof(hud_l); k++) hud_l[k] = hud_d[k] = 0;
    rt_light = hud_l;
    rt_dark = hud_d;
    s = put_str(buf, "HALL ");
    s = put_num(s, (u16)(hall_id - 1), 2);
    s = put_str(s, "   SCORE ");
    if (secret_blink & 4) s = put_str(s, "      ");                  // the secret: a blink
    else { s = put_num(s, W16(SCORE), 5); s = put_str(s, "0"); }
    s = put_str(s, "   LIVES ");
    put_num(s, lives, 1);
    draw_text(2, 2, buf, F_SMALL, C_BLACK);
    rt_light = l;
    rt_dark = d;
    w = (u16)(2 + (((u16)bonus * 3) >> 1));                    // 0x68 -> 156 px
    for (k = 12 * RT_PBYTES; k < 17 * RT_PBYTES; k += RT_PBYTES) {
        u16 x;
        for (x = 2; x < 158; x++) {
            u8 bit = (u8)(0x80 >> (x & 7));
            hud_l[k + (x >> 3)] |= bit;
            if (x < w) hud_d[k + (x >> 3)] |= bit;
        }
    }
}

void game_render(void)
{
    u16 k;
    if (!gb_wram) { draw_clear(); draw_text(4, 40, "NO BGROM FILE", F_MEDIUM, C_BLACK); return; }
    if (!map_l[0]) {
        u8 m;
        u16 k2;
        for (m = 0; m < 2; m++) { map_l[m] = malloc(MAP_BYTES); map_d[m] = malloc(MAP_BYTES); }
        if (!map_l[0] || !map_d[0] || !map_l[1] || !map_d[1]) return;
        all_dirty[0] = all_dirty[1] = spr_dirty = 1;
        for (k2 = 0; k2 < 256; k2++) {
            u8 b = 0, x;
            for (x = 0; x < 8; x++) if (k2 & (1 << x)) b |= (u8)(0x80 >> x);
            rev[k2] = b;
        }
    }
    Z_BEGIN(10, "render: map cache");
    update_maps();
    Z_END(10);
    if (in_play) {
        u8 *pl, *pd;
        Z_BEGIN(11, "render: rows");
        put_rows(16, 0, 80);
        Z_END(11);
        if (secret_blink) secret_blink--;
        if (hud_key[0] != hall_id || hud_key[1] != W8(SCORE) || hud_key[2] != W8(SCORE + 1)
            || hud_key[3] != lives || hud_key[4] != bonus || !hud_key[5] || secret_blink) {
            hud_key[0] = hall_id; hud_key[1] = W8(SCORE); hud_key[2] = W8(SCORE + 1);
            hud_key[3] = lives; hud_key[4] = bonus; hud_key[5] = 1;
            render_hud();
        }
        pl = (u8 *)rt_light + 80 * RT_PBYTES;
        pd = rt_dark ? (u8 *)rt_dark + 80 * RT_PBYTES : 0;
        for (k = 0; k < 20 * RT_PBYTES; k++) { pl[k] = hud_l[k]; if (pd) pd[k] = hud_d[k]; }
        Z_BEGIN(12, "render: sprites");
        draw_sprites(-16, 1);
        Z_END(12);
    } else if (screen_kind == SK_END) {                        // the sky, then the port's HUD
        u8 *pl, *pd;
        put_rows(24, 0, 80);
        render_hud();
        pl = (u8 *)rt_light + 80 * RT_PBYTES;
        pd = rt_dark ? (u8 *)rt_dark + 80 * RT_PBYTES : 0;
        for (k = 0; k < 20 * RT_PBYTES; k++) { pl[k] = hud_l[k]; if (pd) pd[k] = hud_d[k]; }
        draw_sprites(-24, 1);
    } else {
        const u8 *b = bands[screen_kind];
        put_rows(b[0], 0, b[1]);
        if (b[1] < 100) put_rows(b[2], b[1], (u8)(100 - b[1]));
        draw_sprites(-(s16)b[0], 0);                           // sprites: the first band only
    }
}
