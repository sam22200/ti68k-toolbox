// The world map (fftmap, campaign/ENGINE.md core milestone): Ivalice as a simplified geography
// (the user's choice: the original map's land and sea, our markers and roads), the locations the
// story has shown, Ramza travelling FFT's routes between them. fftmap() returns what the
// campaign does next (fftstory runs it, then the map again): 0 quit, 1 the management menus,
// 100+ a battle program's door (fftbat). The bank ffwm (tools/world_assets.py) holds the map's
// tiles (ExtGraph's TileMap engine draws them), the 43 locations, the 48 routes and the story
// list (a location shown by a story flag, entered as its battle until that battle is won);
// ffmg gives the font and Ramza's still sprite; the campaign save (save.h) the flags and
// the location, written back when the map closes.
// Keys: arrows choose a shown location, ENTER travels there (along the shortest chain of
// shown routes: FFT's costs) or enters the one Ramza stands on, ALPHA / DIAMOND the menus, ESC
// quits.
#include "../../../runtime/core/rt.h"
#include "../art.h"
#include "save.h"
#include <stdlib.h>
#include <string.h>

enum { LOC_N = 43, ROUTE_N = 48, NOTE_NONE = 0, NOTE_NOROAD, NOTE_LATER };
typedef struct {
    u8 at, sel;                          // Ramza's location, the chosen one
    u8 moving, leg, legs, step;          // travelling: leg of path[], its point
    u8 path[16];                         // route ids, bit 7: run end to start
    s16 x, y;                            // Ramza on the map (texture pixels)
    u8 note, changed, tick;
    u16 result;                          // what fftmap returns
} Wm;
Wm wm;
Save save;
static const u8 *bank, *mgb;
static RtTilemap map;
void rt_clamp_cam(const RtTilemap *m, s16 *cx, s16 *cy);   // (rt_core.c: draw_tilemap's camera)
#ifndef __m68k__
static u16 *tiles_host;                  // the PC's host-order copy of the tiles
#endif

static u16 b16(const u8 *p) { return (u16)((u16)p[0] << 8 | p[1]); }
static s16 s16v(const u8 *p) { return (s16)b16(p); }
static const u8 *part(u8 k) { return bank + b16(bank + 4 + 2 * k); }
static const u8 *loc(u8 i) { return part(2) + 8 * i; }
static s16 loc_x(u8 i) { return s16v(loc(i)); }
static s16 loc_y(u8 i) { return s16v(loc(i) + 2); }
static const char *loc_name(u8 i) { return (const char *)bank + b16(loc(i) + 6); }
static const u8 *route(u8 r) { return part(3) + 6 * r; }
static const u8 *menu_font(void) { return mgb + b16(mgb + 4 + 2 * 7); }
#define MGTEXT_TEXT_ONLY
#include "mgtext.inc"

static u8 flag(u8 k) { return (u8)(save.flags[k >> 3] >> (k & 7) & 1); }
// the story list's entry of location i (RT_NULL: none): location, flag + 1 that shows it,
// battle door (u16, 0 none), flag + 1 its victory sets
static const u8 *story(u8 i)
{
    const u8 *s = part(4) + 1;
    u8 n = part(4)[0];
    for (; n; n--, s += 6) if (s[0] == i) return s;
    return RT_NULL;
}
u8 shown(u8 i)
{
    const u8 *s = story(i);
    return i == wm.at || (s && s[1] && flag((u8)(s[1] - 1)));
}

// the cheapest chain of shown routes from wm.at to `to` (Dijkstra on FFT's route costs, 43
// nodes): into wm.path, 0 when none
u8 find_path(u8 to)
{
    u16 dist[LOC_N];
    u8 from[LOC_N], via[LOC_N], done[LOC_N], i, r, n = 0;
    memset(done, 0, sizeof done);
    for (i = 0; i < LOC_N; i++) dist[i] = 0xFFFF;
    dist[wm.at] = 0;
    for (;;) {
        u8 u = 0xFF;
        for (i = 0; i < LOC_N; i++) if (!done[i] && dist[i] != 0xFFFF && (u == 0xFF || dist[i] < dist[u])) u = i;
        if (u == 0xFF || u == to) break;
        done[u] = 1;
        for (r = 0; r < ROUTE_N; r++) {
            const u8 *rt = route(r);
            u8 a = rt[0], b = rt[1], v;
            if (a != u && b != u) continue;
            v = a == u ? b : a;
            if (!shown(v) || dist[u] + rt[2] >= dist[v]) continue;
            dist[v] = (u16)(dist[u] + rt[2]); from[v] = u; via[v] = (u8)(r | (a == u ? 0 : 0x80));
        }
    }
    if (dist[to] == 0xFFFF || to == wm.at) return 0;
    for (i = to; i != wm.at; i = from[i]) n++;
    if (n > sizeof wm.path) return 0;
    wm.legs = n;
    for (i = to; i != wm.at; i = from[i]) wm.path[--n] = via[i];
    return 1;
}

static const u8 *point(u8 leg, u8 k)     // point k of a path leg, in its running order
{
    const u8 *rt = route(wm.path[leg] & 0x7F);
    if (wm.path[leg] & 0x80) k = (u8)(rt[3] - 1 - k);
    return bank + b16(rt + 4) + 4 * k;
}

static void arrive(void)
{
    const u8 *rt = route(wm.path[wm.legs - 1] & 0x7F);
    wm.at = wm.path[wm.legs - 1] & 0x80 ? rt[0] : rt[1];
    wm.moving = 0; wm.x = loc_x(wm.at); wm.y = loc_y(wm.at);
    save.loc = (u8)(wm.at + 1); wm.changed = 1;
}

static void travel_step(void)            // a pixel a frame along the route's points
{
    const u8 *p = point(wm.leg, (u8)(wm.step + 1));
    s16 tx = s16v(p), ty = s16v(p + 2);
    wm.x += tx > wm.x ? 1 : tx < wm.x ? -1 : 0;
    wm.y += ty > wm.y ? 1 : ty < wm.y ? -1 : 0;
    if (wm.x != tx || wm.y != ty) return;
    if (++wm.step + 1 < route(wm.path[wm.leg] & 0x7F)[3]) return;
    wm.step = 0;
    if (++wm.leg == wm.legs) arrive();
}

// the shown location nearest in the direction pressed (dx, dy: -1, 0, 1)
static u8 nearest(s16 dx, s16 dy)
{
    u8 i, best = wm.sel;
    u32 bd = 0xFFFFFFFFUL;
    for (i = 0; i < LOC_N; i++) {
        s16 ex = (s16)(loc_x(i) - loc_x(wm.sel)), ey = (s16)(loc_y(i) - loc_y(wm.sel));
        s16 along = (s16)(ex * dx + ey * dy), across = (s16)(dx ? ey : ex);
        u32 d;
        if (i == wm.sel || !shown(i) || along <= 0) continue;
        if (across < 0) across = (s16)-across;
        if (across > 2 * along) continue;                    // within a broad cone
        d = (u32)((s32)along * along) + 4UL * (u32)((s32)across * across);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

void game_init(void)
{
    u16 size;
    bank = mgb = RT_NULL;                    // (statics survive between runs)
    memset(&wm, 0, sizeof wm);
    bank = rt_file("ffwm", &size);
    if (bank && memcmp(bank, "FWM1", 4)) bank = RT_NULL;
    mgb = rt_file("ffmg", &size);
    if (mgb && memcmp(mgb, "FMG1", 4)) mgb = RT_NULL;
    if (!rt_load(SAVE_NAME, &save, sizeof save) || memcmp(save.magic, SAVE_MAGIC, 4)) memset(&save, 0, sizeof save);
    if (bank) {
        u16 n = (u16)((part(1) - part(0)) / 2);
        map.map = part(1); map.w = bank[14]; map.h = bank[15];
        map.ntiles = (u16)(n / 32);
#ifdef __m68k__
        map.tiles = (const u16 *)part(0);
#else
        {
            u16 k;
            tiles_host = (u16 *)malloc(2UL * n);
            for (k = 0; k < n; k++) tiles_host[k] = b16(part(0) + 2 * k);
            map.tiles = tiles_host;
        }
#endif
    }
    rt_state = &wm; rt_state_size = sizeof wm;
}

void game_scenario(u16 n)                 // the save's location (n: test doors, Gariland + n)
{
    (void)n;
    wm.at = save.loc ? (u8)(save.loc - 1) : 6;        // a save before any: Gariland
    wm.sel = wm.at; wm.moving = 0; wm.note = 0; wm.result = 0;
    if (bank) { wm.x = loc_x(wm.at); wm.y = loc_y(wm.at); }
}

static u8 leave(u16 result)
{
    wm.result = result;
    if (wm.changed) rt_save(SAVE_NAME, &save, sizeof save);
    return 0;
}

u8 game_update(void)
{
    u32 key = rt_keys & ~rt_prev;
    wm.tick++;
    if (!bank || !mgb) return !(key & K_ESC);
    if (wm.moving) { travel_step(); travel_step(); return 1; }     // two pixels a frame
    if (wm.note) { if (key & (K_ENTER | K_A | K_B | K_ESC)) wm.note = 0; return 1; }
    if (key & K_ESC) return leave(0);
    if (key & (K_C | K_D)) return leave(1);
    if (key & K_LEFT) wm.sel = nearest(-1, 0);
    if (key & K_RIGHT) wm.sel = nearest(1, 0);
    if (key & K_UP) wm.sel = nearest(0, -1);
    if (key & K_DOWN) wm.sel = nearest(0, 1);
    if (key & (K_ENTER | K_A)) {
        if (wm.sel != wm.at) {
            if (find_path(wm.sel)) { wm.moving = 1; wm.leg = wm.step = 0; }
            else wm.note = NOTE_NOROAD;
        } else {
            const u8 *s = story(wm.at);
            u16 door = s ? b16(s + 2) : 0;
            if (door && !(s[4] && flag((u8)(s[4] - 1)))) return leave(door);   // its battle
            wm.note = NOTE_LATER;
        }
    }
    return 1;
}

// ---------------------------------------------------------------- drawing
static void pset(s16 x, s16 y, u8 c)
{
    u8 *l, *d, m;
    if ((u16)x >= 160 || (u16)y >= 100) return;
    l = (u8 *)rt_light + y * RT_PBYTES + (x >> 3); d = (u8 *)rt_dark + y * RT_PBYTES + (x >> 3);
    m = (u8)(0x80 >> (x & 7));
    if (c & 1) *l |= m; else *l &= (u8)~m;
    if (c & 2) *d |= m; else *d &= (u8)~m;
}

static void line(s16 x0, s16 y0, s16 x1, s16 y1)    // a black road, Bresenham
{
    s16 dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0;
    s16 sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx + dy;
    for (;;) {
        s16 e2;
        pset(x0, y0, C_BLACK);
        if (x0 == x1 && y0 == y1) return;
        e2 = (s16)(2 * e);
        if (e2 >= dy) { e += dy; x0 += sx; }
        if (e2 <= dx) { e += dx; y0 += sy; }
    }
}

static void marker(s16 x, s16 y, u8 town)     // white ring; a town black, a field dark grey
{
    draw_rect(x - 3, y - 3, 7, 7, C_WHITE);
    draw_rect(x - 2, y - 2, 5, 5, town ? C_BLACK : C_DGRAY);
    if (!town) draw_rect(x - 1, y - 1, 3, 3, C_BLACK);
}

static void ramza(s16 x, s16 y)              // ffmg's still sprite 40, feet on the marker
{
    const u8 *p = mgb + b16(mgb + 4 + 2 * 5) + 40 * 156;
#ifdef __m68k__
    RtSprite sp = { 16, 26, p, p + 52, p + 104 };
#else
    u16 r[3][26]; u8 i, j;
    RtSprite sp = { 16, 26, r[0], r[1], r[2] };
    for (i = 0; i < 3; i++) for (j = 0; j < 26; j++) r[i][j] = b16(p + i * 52 + 2 * j);
#endif
    draw_sprite(x - 8, y - 22, &sp);
}

void game_render(void)
{
    s16 cx, cy;
    u8 i, r, k;
    if (!bank || !mgb) { draw_clear(); draw_text(4, 40, "Missing ffwm / ffmg", 0, C_BLACK); return; }
    cx = (s16)(wm.x - 80); cy = (s16)(wm.y - 50);
    rt_clamp_cam(&map, &cx, &cy);
    draw_tilemap(&map, cx, cy);
    for (r = 0; r < ROUTE_N; r++) {                   // the shown routes
        const u8 *rt = route(r), *p = bank + b16(rt + 4);
        if (!shown(rt[0]) || !shown(rt[1])) continue;
        for (k = 0; k + 1 < rt[3]; k++, p += 4)
            line((s16)(s16v(p) - cx), (s16)(s16v(p + 2) - cy), (s16)(s16v(p + 4) - cx), (s16)(s16v(p + 6) - cy));
    }
    for (i = 0; i < LOC_N; i++) if (shown(i)) marker((s16)(loc_x(i) - cx), (s16)(loc_y(i) - cy), loc(i)[4] == 1);
    ramza((s16)(wm.x - cx), (s16)(wm.y - cy));
    if (!wm.moving) {
        RtSprite sp = { 32, HAND_H, hand_gfx[0], hand_gfx[1], hand_gfx[2] };
        draw_sprite((s16)(loc_x(wm.sel) - cx - 26), (s16)(loc_y(wm.sel) - cy - 7), &sp);
    }
    draw_rect(0, 91, 160, 9, C_WHITE); draw_rect(0, 90, 160, 1, C_BLACK);
    black(3, 93, loc_name(wm.sel));
    if (wm.note) {
        static const char *const notes[] = { "", "No road there yet", "Not ripped yet" };
        panel(20, 36, 120, 22);
        black(26, 40, notes[wm.note]);
        black(26, 48, "ENTER or SHIFT");
    }
}

u16 map_result(void) { return wm.result; }
