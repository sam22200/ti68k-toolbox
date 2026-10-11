// Final Fantasy Tactics, Magic City Gariland: isometric tech demo (README.md for the design).
// World tiles (x, z) are rotated into view tiles (u, v) for each of the four orientations; the
// scene of each orientation is the game's own textured map drawn on the PC (tools/extract.py)
// with the depth of its pixels, four ZX0-packed data files read in place. The current one is
// unpacked (during the turn that reaches it) and copied into a buffer larger than the screen
// (the reach rings are drawn there); every frame copies the camera's window and draws the
// units through a cover mask made from that depth.
// The rotation itself is animated with polygons projected at intermediate angles, each face in
// the mean grey of its pixels in the views (map.h).
#include "fft.h"
#include <stdlib.h>
#include <string.h>

#ifdef RT_CYCLES                        // zones under ti-cycles: 3 view copy, 4 units, 5 cursor
#include "../../tools/m68kbench/bench.h" // and HUD, 6 scene, 7 rotation frame, 8 view unpacking
#define ZB(n) BENCH_BEGIN(n)
#define ZE(n) BENCH_END(n)
#else
#define ZB(n)
#define ZE(n)
#endif

// Planes are big-endian 16-bit words (MSB = leftmost pixel): native on the TI, composed on
// the PC. RMW16: word = word & ~k | v.
#ifdef __m68k__
#define RD16(p, i) (((const u16 *)(p))[i])
#define WR16(p, i, v) (((u16 *)(p))[i] = (v))
#define RD32(p, i) (*(const u32 *)((const u8 *)(p) + 2 * (i)))
#define WR32(p, i, v) (*(u32 *)((u8 *)(p) + 2 * (i)) = (v))
#define RMW16(p, k, v) (*(u16 *)(p) = (*(u16 *)(p) & ~(k)) | (v))
// 16 x 16 -> 32 multiply and 32 / 16 division in one instruction (ti68k-performance.md §4,
// verified): plain C widens shared shorts and calls __mulsi3 / __divsi3
static inline long muls16(short a, short b)
{ long r; asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r; }
static inline short divs32_16(long n, short d)
{ asm("divs.w %1,%0" : "+d"(n) : "dmi"(d)); return (short)n; }
#else
#define RD16(p, i) ((u16)((p)[2 * (i)] << 8 | (p)[2 * (i) + 1]))
#define WR16(p, i, v) ((p)[2 * (i)] = (u8)((v) >> 8), (p)[2 * (i) + 1] = (u8)(v))
#define RD32(p, i) ((u32)RD16(p, i) << 16 | RD16(p, (i) + 1))
#define WR32(p, i, v) (WR16(p, i, (u16)((v) >> 16)), WR16(p, (i) + 1, (u16)(v)))
#define RMW16(p, k, v) do { u16 o_ = (u16)((p)[0] << 8 | (p)[1]); o_ = (u16)((o_ & ~(k)) | (v)); \
                            (p)[0] = (u8)(o_ >> 8); (p)[1] = (u8)o_; } while (0)
static long muls16(short a, short b) { return (long)a * b; }
static short divs32_16(long n, short d) { return (short)(n / d); }
#endif

#ifdef __m68k__
#include "../../lib/unpack68k.h"         // zx0_asm: ~42-73 cycles per output byte
#define zx0_unpack zx0_asm
#else
// ZX0 v2 (einar-saukas/ZX0), the PC's copy of lib/unpack68k.s (experiments/compress/unpack.c)
static int zx0_bit(const u8 **s, u16 *bb)
{
    u16 t;
    if (*bb == 0x8000) *bb = (u16)(*(*s)++ << 8 | 0x80);
    t = *bb; *bb = (u16)(*bb << 1);
    return (t & 0x8000) != 0;
}
static u16 zx0_gamma(const u8 **s, u16 *bb)
{
    u16 x = 1;
    while (!zx0_bit(s, bb)) { x += x; if (zx0_bit(s, bb)) x++; }
    return x;
}
static u8 *zx0_unpack(const u8 *s, u8 *d)
{
    u16 dist = 1, n, bb = 0x8000;
    for (;;) {
        for (n = zx0_gamma(&s, &bb); n; n--) *d++ = *s++;     // literals
        if (!zx0_bit(&s, &bb)) {                                // repeat the last offset
            for (n = zx0_gamma(&s, &bb); n; n--, d++) *d = d[-dist];
            if (!zx0_bit(&s, &bb)) continue;
        }
        for (;;) {                                              // new offset
            u16 lo;
            n = 1;
            while (!zx0_bit(&s, &bb)) { n += n; if (!zx0_bit(&s, &bb)) n++; }
            if (n == 256) return d;
            lo = *s++;
            dist = (u16)((n << 7) - (lo >> 1));
            n = 1;
            if (!(lo & 1)) do { n += n; if (zx0_bit(&s, &bb)) n++; } while (!zx0_bit(&s, &bb));
            for (n++; n; n--, d++) *d = d[-dist];
            if (!zx0_bit(&s, &bb)) break;
        }
    }
}
#endif

State st;
u8 *scene_l, *scene_d;
u8 scene_rot;
// the four views (fftv0..3, ZX0, read in place), unpacked one at a time into vbuf: light and
// dark planes, then twice (the frontmost pixels of each byte, then the next ones) their view
// diagonal and their mask (SC_PLANE each)
#define VIEW_BYTES (6UL * SC_PLANE)          // 52,320: above a 16-bit int
static const u8 *views[4];
static const u8 *ugfx;                  // the units' frames (fftu, units.h), read in place
static u8 *vbuf, vbuf_rot;

// Work memory, allocated with the scene (globals and statics are stored in the program file
// on the TI: -mno-bss, and the TI-89 refuses programs above 24 KB)
#define TW (RT_W / 2)                   // the rotation frames: half resolution
#define TH (RT_H / 2)
#define COVER_H UNIT_SH                // the shadow lies inside the sprite's box
#define GLINTS 250                      // glints per phase at most
#define HUD_ROWS 14                     // the top strip (rows 0-7), text to be turned (8-13)
#define WIN_ROWS 27                     // the menu or a unit's numbers, at the top right
#define WIN_DONE 5                      // its box, then its four lines: one per frame
typedef struct {                        // a unit's cover mask, and its covered rows r0..r1
    s16 x, y;                           // among 2 .. UNIT_SH - 3 (r0 > r1: none)
    u8 depth, rot, r0, r1;
    u16 cm[COVER_H];
} Cover;
typedef struct {
    Cover cover[NUNIT];                 // per unit, for its feet, its depth and the view
    s16 span_l[TH + 8], span_r[TH + 8];
    u8 queue[MAP_W * MAP_H], from[MAP_W * MAP_H];
    u8 half[2][TW / 8 * TH];
    u16 dbl[256];                       // byte -> its bits doubled
    u8 order[MAP_W * MAP_H], count[256];
    struct { s16 x, y; } glint[4][GLINTS];  // the water's glints of each phase (scene pixels)
    u8 glints[4];
    s16 key[MAP_W * MAP_H], gx[MAP_W + 1][MAP_H + 1], gy[MAP_W + 1][MAP_H + 1];
    u16 hud[2][HUD_ROWS * RT_PBYTES / 2];   // the top strip (the turns, the view and height)
    char hud_s[8];                      // what it shows
    u8 hud_t[ORDER_N], hud_late;
    u16 label[UNIT_GFX_N][5];           // the turns' labels (Ra, De, Sq, Ch), drawn once
    u16 win[2][WIN_ROWS * RT_PBYTES / 2];   // the menu or a status window, and what it shows:
    u8 win_k[3], win_w, win_h, win_n;   // mode, moved | acted << 1 (M_AIM: the attacker), unit;
                                        // its width in words, its rows; its part to draw next
                                        // (WIN_DONE: all drawn)
    u16 pan[2][2][PANEL_H];             // the active unit's panel (left), the cursor's (right):
    u16 pan_k[2][3];                    // light, dark; unit + 1 (0: none), HP, MP
    u8 arrow[4][2][8];                  // the facing arrows (view -v, +u, +v, -u): fill, outline
    u8 pname[UNIT_GFX_N][PANEL_H];      // the names turned a quarter, a row per panel row (5 bits)
    u8 ringed[MAP_W * MAP_H], ring_n;   // the tiles whose reach ring is in the scene, how many,
    u8 ring_i;                          // the next tile to look at (MAP_W * MAP_H: all drawn)
    u32 num[2][7];                      // an attack's number (or Miss): black, its white outline
    u8 num_w, num_ok;                   // its width; drawn for this attack
} Work;
static Work *W;
#define span_l (W->span_l)
#define span_r (W->span_r)
#define queue (W->queue)
#define bfs_from (W->from)
#define half (W->half)
#define dbl (W->dbl)
#define order (W->order)
#define key (W->key)
#define gx (W->gx)
#define gy (W->gy)
#define HUD_L ((u8 *)W->hud[0])
#define HUD_D ((u8 *)W->hud[1])

// ---------------------------------------------------------------- the map in four orientations
u8 view_w(u8 rot) { return rot & 1 ? MAP_H : MAP_W; }
u8 view_h(u8 rot) { return rot & 1 ? MAP_W : MAP_H; }

void view_of(u8 rot, s16 x, s16 z, s16 *u, s16 *v)
{
    switch (rot) {
    case 0: *u = x; *v = z; break;
    case 1: *u = MAP_H - 1 - z; *v = x; break;
    case 2: *u = MAP_W - 1 - x; *v = MAP_H - 1 - z; break;
    default: *u = z; *v = MAP_W - 1 - x; break;
    }
}

void world_of(u8 rot, s16 u, s16 v, s16 *x, s16 *z)
{
    switch (rot) {
    case 0: *x = u; *z = v; break;
    case 1: *x = v; *z = MAP_H - 1 - u; break;
    case 2: *x = MAP_W - 1 - u; *z = MAP_H - 1 - v; break;
    default: *x = MAP_W - 1 - v; *z = u; break;
    }
}

const Tile *tile_at(s16 x, s16 z)
{
    if ((u16)x >= MAP_W || (u16)z >= MAP_H) return RT_NULL;
    return &map_tiles[z * MAP_W + x];
}

// view corner c of a tile is world corner (c - rot) & 3 (both numbered (0,0) (1,0) (1,1) (0,1))
u8 corner_h(u8 rot, s16 u, s16 v, u8 c)
{
    s16 x, z;
    if ((u16)u >= view_w(rot) || (u16)v >= view_h(rot)) return 0;
    world_of(rot, u, v, &x, &z);
    return map_tiles[z * MAP_W + x].c[(c - rot) & 3];
}

s16 scene_x(u8 rot, s16 u, s16 v) { return (s16)((u - v + view_h(rot)) * HW); }
s16 scene_y(s16 u, s16 v) { return (s16)(SC_OY + (u + v) * HH); }

u8 unit_at(s16 x, s16 z)
{
    u8 i;
    for (i = 0; i < NUNIT; i++)
        if (st.unit[i].x == x && st.unit[i].z == z) return i + 1;
    return 0;
}

// ---------------------------------------------------------------- the battle's units
// FFT's model (tools/battle.py, checked on the original at Ramza's first turn): raw stats of 24
// bits drawn from the unit type's base and variance, grown per level, times the job's
// multiplier; then the equipment. Once per battle: the 32-bit divisions do not matter.
// Our own 32-bit xorshift for the battle's draws, seeded from the runtime's: the runtime's
// wyhash16 (a 16-bit state) ties two draws in a row (after a 4 % critical roll its next draw,
// the critical's bonus, was never 0: criticals seen 3.8 % for the original's 2.0 %)
static u32 rng32 = 1;
static void rand_seed(u16 s) { rng32 = 0x2545F491UL ^ ((u32)s << 16 | s); }
static u16 rand15(void)                                   // 0..32767, as the PS1's rand()
{
    rng32 ^= rng32 << 13; rng32 ^= rng32 >> 17; rng32 ^= rng32 << 5;
    return (u16)(rng32 >> 17);
}

void unit_stats(Unit *u)
{
    const Job *j = &jobs[u->job];
    u16 s[5];
    u8 i, k;
    for (i = 0; i < 5; i++) {
        u32 v = u->raw[i];
        u8 g = j->growth[i] ? j->growth[i] : 1;
        for (k = 2; k <= u->level; k++) v += v / (u16)(g + k - 1);
        if (v > 0xFFFFFFUL) v = 0xFFFFFFUL;
        v = v * j->mult[i] / 100 >> 14;
        s[i] = v ? (u16)v : 1;
    }
    u->max_hp = s[0] > 999 ? 999 : s[0];
    u->max_mp = s[1] > 999 ? 999 : s[1];
    u->move = j->move; u->jump = j->jump;
    for (i = 0; i < EQ_N; i++) {                   // helmet / armour HP and MP, item attributes
        const Item *it;
        if (u->eq[i] == 0xFF) continue;
        it = &items[u->eq[i]];
        u->max_hp += it->hp; u->max_mp += it->mp;
        s[2] += it->sp; s[3] += it->pa; s[4] += it->ma;
        u->move += it->move; u->jump += it->jump;
    }
    if (u->max_hp > 999) u->max_hp = 999;
    if (u->max_mp > 999) u->max_mp = 999;
    if (u->jump > 7) u->jump = 7;
    u->sp = (u8)(s[2] > 50 ? 50 : s[2]);
    u->pa = (u8)(s[3] > 99 ? 99 : s[3]);
    u->ma = (u8)(s[4] > 99 ? 99 : s[4]);
    u->hp = u->max_hp; u->mp = u->max_mp;
}

void battle_units(void)
{
    u8 i, k;
    for (i = 0; i < NUNIT; i++) {
        const UnitDef *d = &unit_defs[i];
        Unit *u = &st.unit[i];
        u16 day = d->day;
        u->x = d->x; u->z = d->z; u->gfx = d->gfx; u->team = d->team; u->face = d->face;
        u->job = d->job; u->level = d->level; u->ct = 0; u->dc = 3; u->female = d->type == 1;
        u->brave = d->brave ? d->brave : (u8)(45 + ((30UL * rand15()) >> 15));
        u->faith = d->faith ? d->faith : (u8)(45 + ((30UL * rand15()) >> 15));
        if (!day) day = (u16)(1 + ((365UL * rand15()) >> 15));
        for (k = 0; k < 12 && day >= zodiac_days[k]; k++) ;
        u->zodiac = (u8)((k + 9) % 12);
        for (k = 0; k < 5; k++)
            u->raw[k] = ((u32)gen_base[d->type][k] << 14) + (((u32)rand15() * gen_var[d->type][k]) >> 1);
        for (k = 0; k < EQ_N; k++)                 // one of the two candidates (FFT: of n)
            u->eq[k] = d->eq[k][d->eq[k][1] != 0xFF ? rand15() >> 14 : 0];
        unit_stats(u);
    }
}

// ---------------------------------------------------------------- turn order
// FFT's clock (RE_NOTES.md § Turn order, checked on the original's turns): each tick every unit's
// CT grows by its Speed (at most 254); a unit with CT >= 100 acts, the highest first, a tie to
// the lowest index; its CT loses 100 when picked and gains 20 for each of Move and Act left
// unused at the end, at most 60. A unit at 0 HP keeps its clock: each pick lowers its death
// counter, then it turns into a crystal or a chest and its clock stops.
u8 ct_next(void)
{
    for (;;) {
        u8 i, best = 0xFF, max = 99;
        Unit *u;
        for (i = 0, u = st.unit; i < NUNIT; i++, u++)
            if (u->dc != DC_GONE && u->ct > max) { max = u->ct; best = i; }
        if (best == 0xFF) {                        // a tick
            for (i = 0, u = st.unit; i < NUNIT; i++, u++)
                if (u->dc != DC_GONE) { u16 c = u->ct + u->sp; u->ct = (u8)(c > 254 ? 254 : c); }
            continue;
        }
        u = &st.unit[best];
        u->ct = (u8)(max >= 200 ? max - 200 : max - 100);
        if (u->hp) return best;
        u->dc = u->dc ? u->dc - 1 : DC_GONE;
    }
}

void ct_end(u8 who, u8 moved, u8 acted)
{
    u8 c = st.unit[who].ct;
    if (!moved) c += 20;
    if (!acted) c += 20;
    st.unit[who].ct = c > 60 ? 60 : c;
}

// FFT's AT list: the active unit, then each living unit's next turns, its clock run alone
// (no bonus at their end), sorted by tick then by the CT reached (the higher first), a tie in
// unit order. At each turn change only (11 x ORDER_N divisions).
void turn_order(void)
{
    u16 tk[ORDER_N];
    u8 i, j, n = 1, t;
    Unit *u;
    st.turns[0] = st.act; tk[0] = 0;
    for (i = 0, u = st.unit; i < NUNIT; i++, u++) {
        u16 tick = 0, ct = u->ct;
        if (!u->hp || u->dc == DC_GONE) continue;
        for (t = 0; t < ORDER_N - 1; t++) {
            u16 k;
            if (ct < 100) {                        // the ticks to 100 at once
                u16 k2 = (u16)((u16)(100 - ct + u->sp - 1) / u->sp);
                tick += k2; ct += k2 * u->sp;
            }
            if (tick >= 256) break;
            ct = ct >= 200 ? ct - 200 : ct - 100;
            k = (u16)(tick << 8 | (100 - ct));
            for (j = n; j > 1 && tk[j - 1] > k; j--)
                if (j < ORDER_N) { tk[j] = tk[j - 1]; st.turns[j] = st.turns[j - 1]; }
            if (j >= ORDER_N) break;               // later than every turn kept
            tk[j] = k; st.turns[j] = i;
            if (n < ORDER_N) n++;
        }
    }
    for (; n < ORDER_N; n++) st.turns[n] = 0xFF;
}

// the next turn: the cursor and the camera go to its unit; enemies and the guest wait, a
// player's unit opens its menu
static void turn_start(void)
{
    u8 me;
    st.act = ct_next();
    st.cx = st.unit[st.act].x; st.cz = st.unit[st.act].z;
    me = st.unit[st.act].team == TEAM_PLAYER;
    st.wait_t = me ? 0 : AI_WAIT;
    st.mode = me ? M_MENU : M_BROWSE;
    st.menu = MN_MOVE; st.moved = st.acted = 0;
    turn_order();
}

static void turn_end(u8 moved, u8 acted)
{
    ct_end(st.act, moved, acted);
    turn_start();
}

// ---------------------------------------------------------------- drawing targets
// Every drawing call takes scene coordinates; the target maps them to its own pixels (origin
// ox, oy) and clips to pixels [x0, x1) and rows [y0, y1).
typedef struct { u8 *l, *d; s16 stride, ox, oy, x0, x1, y0, y1; } Dst;
static Dst D;

static void dst_scene(void)
{
    D.l = scene_l; D.d = scene_d; D.stride = SC_BYTES; D.ox = D.oy = 0;
    D.x0 = 0; D.x1 = SC_W; D.y0 = 0; D.y1 = SC_H;
}

static void dst_screen(s16 camx, s16 camy)
{
    D.l = rt_light; D.d = rt_dark; D.stride = RT_PBYTES; D.ox = camx; D.oy = camy;
    D.x0 = 0; D.x1 = RT_W; D.y0 = 0; D.y1 = RT_H;
}

// A masked sprite of up to 24 columns (u32 rows, bit 31 = column 0): dest = dest & ~m | src & m,
// written as three 16-bit words per plane and row, each clipped
static void blit24(s16 x, s16 y, s16 rows, const u32 *l, const u32 *d, const u32 *m)
{
    s16 w, sh, r = 0, k;
    u16 ck[3];
    u8 *pl, *pd;
    x -= D.ox; y -= D.oy;
    if (x >= D.x1 || x + 24 <= D.x0) return;
    if (y < D.y0) { r = D.y0 - y; y = D.y0; }
    if (y + rows - r > D.y1) rows = D.y1 - y + r;
    if (r >= rows) return;
    w = x >> 4; sh = x & 15;
    for (k = 0; k < 3; k++) {
        s16 lo = (w + k) << 4, a = lo < D.x0 ? D.x0 : lo, b = lo + 16 > D.x1 ? D.x1 : lo + 16;
        ck[k] = a < b ? (u16)((0xFFFFU >> (a - lo)) & (0xFFFFU << (lo + 16 - b))) : 0;
    }
    pl = D.l + y * D.stride + 2 * w; pd = D.d + y * D.stride + 2 * w;
    for (; r < rows; r++, pl += D.stride, pd += D.stride) {
        u32 mm = m[r] >> sh, ll = l[r] >> sh, dd = d[r] >> sh;
        u16 k0 = ck[0] & (u16)(mm >> 16), k1 = ck[1] & (u16)mm;
        if (k0) { RMW16(pl, k0, (u16)(ll >> 16) & k0); RMW16(pd, k0, (u16)(dd >> 16) & k0); }
        if (k1) { RMW16(pl + 2, k1, (u16)ll & k1); RMW16(pd + 2, k1, (u16)dd & k1); }
        if (sh > 8) {
            u16 k2 = ck[2] & (u16)(m[r] << (16 - sh));
            if (k2) {
                RMW16(pl + 4, k2, (u16)(l[r] << (16 - sh)) & k2);
                RMW16(pd + 4, k2, (u16)(d[r] << (16 - sh)) & k2);
            }
        }
    }
}

// Convex polygon (n <= 4 points, target coordinates) filled with an 8x8 pattern aligned to the
// target. Pixel (x, y) is inside when its centre is. Spans in 16-bit words.

static const u16 lmask[16] = { 0xFFFF, 0x7FFF, 0x3FFF, 0x1FFF, 0x0FFF, 0x07FF, 0x03FF, 0x01FF,
    0x00FF, 0x007F, 0x003F, 0x001F, 0x000F, 0x0007, 0x0003, 0x0001 };   // pixels >= x & 15

// rows ymin.. of the target from span_l/span_r, an 8 x 8 pattern of 16-bit rows aligned to
// the target's pixels (the rotation frames)
static void fill_spans(s16 ymin, s16 rows, const u16 *pl, const u16 *pd)
{
    u8 *ql = D.l + ymin * D.stride, *qd = D.d + ymin * D.stride;
    const s16 *sa = span_l, *sb = span_r;
    s16 stride = D.stride, py8 = (ymin + D.oy) & 7;
    for (; rows > 0; rows--, ql += stride, qd += stride, py8 = (py8 + 1) & 7) {
        s16 xa = *sa++, xb = *sb++, n;
        u16 rl, rd, k, kr;
        u8 *al, *ad;
        if (xa < D.x0) xa = D.x0;
        if (xb > D.x1) xb = D.x1;
        if (xa >= xb) continue;
        rl = pl[py8]; rd = pd[py8];
        al = ql + ((xa >> 3) & ~1); ad = qd + ((xa >> 3) & ~1);
        k = lmask[xa & 15];
        kr = (u16)(0xFFFFU << (15 - ((xb - 1) & 15)));
        n = ((xb - 1) >> 4) - (xa >> 4);
        if (!n) k &= kr;
        RMW16(al, k, rl & k); RMW16(ad, k, rd & k);
        if (!n) continue;
        for (al += 2, ad += 2; --n > 0; al += 2, ad += 2) { WR16(al, 0, rl); WR16(ad, 0, rd); }
        RMW16(al, kr, rl & kr); RMW16(ad, kr, rd & kr);
    }
}

static void fill_poly(const s16 *px, const s16 *py, u8 n, const u16 *pl, const u16 *pd)
{
    s16 ymin = 32767, ymax = -32768, y, i, rows;
    for (i = 0; i < n; i++) {
        s16 yy = py[i] - D.oy;
        if (yy < ymin) ymin = yy;
        if (yy > ymax) ymax = yy;
    }
    if (ymin < D.y0) ymin = D.y0;
    if (ymax > D.y1) ymax = D.y1;
    rows = ymax - ymin;
    if (rows <= 0) return;
    {
        s16 *a = span_l, *b = span_r;
        for (y = rows; y > 0; y--) { *a++ = 32767; *b++ = -32768; }
    }
    for (i = 0; i < n; i++) {
        s16 j = i + 1 < n ? i + 1 : 0;
        s16 x0 = px[i] - D.ox, y0 = py[i] - D.oy, x1 = px[j] - D.ox, y1 = py[j] - D.oy;
        s16 ya, yb, *sl, *sr, dx8;
        s32 x, dx;
        if (y0 == y1) continue;
        if (y0 > y1) { s16 t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
        ya = y0 < ymin ? ymin : y0;
        yb = y1 > ymax ? ymax : y1;
        if (ya >= yb) continue;
        dx8 = divs32_16((s32)(x1 - x0) << 8, y1 - y0);            // 8.8 slope: |dx| < 128
        dx = (s32)dx8 << 8;
        x = ((s32)x0 << 16) + (muls16(dx8, (s16)(2 * (ya - y0) + 1)) << 7) + 0x8000;  // row centre
        sl = span_l + (ya - ymin); sr = span_r + (ya - ymin);
        for (y = yb - ya; y > 0; y--, x += dx, sl++, sr++) {
            s16 xi = (s16)(x >> 16);
            if (xi < *sl) *sl = xi;
            if (xi > *sr) *sr = xi;
        }
    }
    fill_spans(ymin, rows, pl, pd);
}

#define RINGS(m) ((m) == M_TARGET || (m) == M_AIM)   // the modes that show st.reach

// pixels of the scene in front of view diagonal depth inside a box (byte columns, rows):
// restored from the view (a reach ring drawn behind a building), but for a pattern of them:
// a hidden ring stays visible over the building, set apart from the seen ones
static const u8 hide_pat[2] = { 0xCC, 0x33 };   // 2-pixel dashes (picked over the whole ring
                                                // and a checkerboard)
static void uncover(s16 x, s16 y, s16 w, s16 rows, u8 depth)
{
    s16 b0 = x >> 3, b1 = (x + w - 1) >> 3, r, b;
    if (b0 < 0) b0 = 0;
    if (b1 >= SC_BYTES) b1 = SC_BYTES - 1;
    if (y < 0) { rows += y; y = 0; }
    if (y + rows > SC_H) rows = SC_H - y;
    for (r = 0; r < rows; r++) {
        // three pointers, the other planes at constant offsets (at most 3 SC_PLANE: a 16-bit
        // displacement): nine pointers or six indexed arrays cost ~18k a ring
        u16 o = (u16)((y + r) * SC_BYTES + b0);
        const u8 *v = vbuf + o, *t = vbuf + 2 * SC_PLANE + o;   // view: light, dark; depths
        u8 *q = scene_l + o, hp = (u8)~hide_pat[(y + r) & 1];    // scene: light, dark
        for (b = b0; b <= b1; b++, v++, t++, q++) {
            u8 m = (u8)(((t[0] > depth ? t[SC_PLANE] : 0) | (t[2 * SC_PLANE] > depth ? t[3 * SC_PLANE] : 0)) & hp);
            if (!m) continue;
            q[0] = (u8)((q[0] & ~m) | (v[0] & m));
            q[SC_PLANE] = (u8)((q[SC_PLANE] & ~m) | (v[SC_PLANE] & m));
        }
    }
}

// The scene of orientation rot with the current reach rings: the view copied from its file,
// then a dotted ring on each reachable tile, behind what stands in front of it
static u8 scene_seq;                    // hl_seq it was made for
static u8 hl_seq;                       // bumped when the highlights may have changed

static void unpack_view(u8 rot)
{
    if (vbuf_rot == rot) return;
    zx0_unpack(views[rot], vbuf);
    vbuf_rot = rot;
}

// is scene pixel (x, y) in front of everything nearer than view diagonal depth (not covered)?
static u8 seen(s16 x, s16 y, u8 depth)
{
    const u8 *t1 = vbuf + 2 * SC_PLANE, *m1 = t1 + SC_PLANE, *t2 = m1 + SC_PLANE, *m2 = t2 + SC_PLANE;
    u16 o = (u16)(y * SC_BYTES + (x >> 3));
    u8 b = (u8)(0x80 >> (x & 7));
    return !((t1[o] > depth && m1[o] & b) || (t2[o] > depth && m2[o] & b));
}

// The water's glints of orientation rot: 2-pixel dashes on a lattice of the scene's pixels
// (every other row, one even column in 8, staggered by row) that slides 2 px right each phase
// (a current), on the water tiles' tops where the view shows them (not under a bank or a roof)
static void make_glints(u8 rot)
{
    u8 i;
    memset(W->glints, 0, sizeof W->glints);
    for (i = 0; i < MAP_W * MAP_H; i++) {
        s16 u, v, x, y, r, px;
        if (!map_tiles[i].water) continue;
        view_of(rot, i % MAP_W, i / MAP_W, &u, &v);
        x = scene_x(rot, u, v) - HW; y = scene_y(u, v) - map_tiles[i].stand * (HU / 2);
        for (r = 1; r < 11; r++) {
            s16 py = y + r, k = r < 6 ? r : 11 - r;
            if ((py & 1) || (u16)py >= SC_H) continue;
            for (px = (x + 10 - 2 * k + 1) & ~1; px + 1 < x + 14 + 2 * k; px += 2) {
                u8 ph = (u8)(((px >> 1) + 3 * (py >> 1)) & 3), n;
                if (px < 0 || px + 1 >= SC_W) continue;
                n = W->glints[ph];
                if (n >= GLINTS || !seen(px, py, (u8)(u + v)) || !seen(px + 1, py, (u8)(u + v))) continue;
                W->glint[ph][n].x = px; W->glint[ph][n].y = py;
                W->glints[ph] = (u8)(n + 1);
            }
        }
    }
}

// this frame's glints on the screen, white (a phase every 8 frames)
static void draw_glints(void)
{
    u8 ph = (u8)((st.tick >> 3) & 3), n = W->glints[ph], i;
    for (i = 0; i < n; i++) {
        s16 x = W->glint[ph][i].x - st.camx, y = W->glint[ph][i].y - st.camy, k;
        if ((u16)y >= RT_H || x < 0 || x + 1 >= RT_W) continue;
        for (k = x; k <= x + 1; k++) {
            u16 o = (u16)(y * RT_PBYTES + (k >> 3));
            u8 b = (u8)~(0x80 >> (k & 7));
            ((u8 *)rt_light)[o] &= b; ((u8 *)rt_dark)[o] &= b;
        }
    }
}

// a ring's box in the scene: its tile's diamond at its standing height
static void ring_box(u8 rot, u8 i, s16 *x, s16 *y, u8 *depth)
{
    s16 u, v;
    view_of(rot, i % MAP_W, i / MAP_W, &u, &v);
    *x = scene_x(rot, u, v) - HW; *y = scene_y(u, v) - map_tiles[i].stand * (HU / 2);
    *depth = (u8)(u + v);
}

// a new view, or many rings to drop (a Move's): copied whole (130k); a few (an attack's): only
// their boxes put back from the view (~7k each; the frame an attack starts went over the
// budget with the whole copy). The new rings are then drawn RING_STEP a frame (~18k each:
// the four of an attack in one frame made it 381k with ten units on screen)
#define RING_BOXES 12
#define RING_STEP 2
static void ensure_scene(u8 rot)
{
    u8 i, d, k;
    s16 x, y;
    if (scene_rot == rot && scene_seq == hl_seq) {
        if (W->ring_i >= MAP_W * MAP_H) return;
        goto rings;
    }
    unpack_view(rot);
    if (scene_rot != rot || W->ring_n > RING_BOXES) {
        if (scene_rot != rot) make_glints(rot);
        memcpy(scene_l, vbuf, 2 * SC_PLANE);
    } else
        for (i = 0; i < MAP_W * MAP_H; i++) {
            s16 r, b0, b1, rows = 12;
            if (!W->ringed[i]) continue;
            ring_box(rot, i, &x, &y, &d);
            b0 = x < 0 ? 0 : x >> 3; b1 = (x + 23) >> 3;
            if (b1 >= SC_BYTES) b1 = SC_BYTES - 1;
            if (y < 0) { rows += y; y = 0; }
            if (y + rows > SC_H) rows = SC_H - y;
            for (r = 0; r < rows; r++) {           // 3-4 bytes a row: no ROM memcpy
                u16 o = (u16)((y + r) * SC_BYTES + b0), e = o + (b1 - b0);
                for (; o <= e; o++) { scene_l[o] = vbuf[o]; scene_d[o] = vbuf[SC_PLANE + o]; }
            }
        }
    scene_rot = rot; scene_seq = hl_seq;
    memset(W->ringed, 0, sizeof W->ringed);
    W->ring_n = W->ring_i = 0;
rings:
    dst_scene();
    if (!RINGS(st.mode)) { W->ring_i = MAP_W * MAP_H; return; }
    for (k = 0; W->ring_i < MAP_W * MAP_H && k < RING_STEP; W->ring_i++) {
        i = W->ring_i;
        if (st.reach[i] == 0xFF) continue;
        W->ringed[i] = 1; W->ring_n++; k++;
        ring_box(rot, i, &x, &y, &d);
        blit24(x, y, 12, hi_l, hi_d, hi_m);
        uncover(x, y, 24, 12, d);
    }
}

u16 scene_checksum(void)
{
    u16 a = 0, b = 0, i;
    for (i = 0; i < (u16)(SC_BYTES * SC_H); i++) {
        a = (a + scene_l[i]) % 255; b = (b + a) % 255;
        a = (a + scene_d[i]) % 255; b = (b + a) % 255;
    }
    return (u16)(b << 8 | a);
}

// ---------------------------------------------------------------- reach and paths
static const s8 dir_x[4] = { 1, -1, 0, 0 }, dir_z[4] = { 0, 0, 1, -1 };

static void bfs(u8 who, u8 *from)
{
    u8 qh = 0, qt = 0, i;
    Unit *un = &st.unit[who];
    for (i = 0; i < MAP_W * MAP_H; i++) st.reach[i] = 0xFF;
    st.reach[un->z * MAP_W + un->x] = 0;
    queue[qt++] = un->z * MAP_W + un->x;
    while (qh < qt) {
        u8 c = queue[qh++], x = c % MAP_W, z = c / MAP_W, d;
        const Tile *t = &map_tiles[c];
        if (st.reach[c] >= un->move) continue;
        for (d = 0; d < 4; d++) {
            s16 nx = x + dir_x[d], nz = z + dir_z[d];
            const Tile *n = tile_at(nx, nz);
            u8 o, nc;
            s16 dh;
            if (!n || !n->walk) continue;
            nc = (u8)(nz * MAP_W + nx);
            if (st.reach[nc] != 0xFF) continue;
            dh = (s16)n->stand - t->stand;
            if (dh > 2 * un->jump || dh < -2 * un->jump) continue;
            o = unit_at(nx, nz);                                    // enemies block, allies pass
            if (o && (st.unit[o - 1].team == TEAM_ENEMY) != (un->team == TEAM_ENEMY)) continue;
            st.reach[nc] = st.reach[c] + 1;
            if (from) from[nc] = c;
            queue[qt++] = nc;
        }
    }
}

void compute_reach(u8 who)
{
    u8 i;
    hl_seq++;
    bfs(who, RT_NULL);
    for (i = 0; i < NUNIT; i++)                                     // can't stop on a unit
        if (i != who) st.reach[st.unit[i].z * MAP_W + st.unit[i].x] = 0xFF;
}

u8 make_path(u8 who, u8 tx, u8 tz)
{
    u8 c = tz * MAP_W + tx, n, i;
    bfs(who, bfs_from);
    n = st.reach[c];
    if (n == 0xFF || n >= PATH_MAX) { st.path_n = 0; return 0; }
    for (i = n + 1; i-- > 0;) {
        st.path[i][0] = c % MAP_W; st.path[i][1] = c / MAP_W;
        c = bfs_from[c];
    }
    st.path_n = n + 1;                                             // path[0] = the start
    compute_reach(who);
    return st.path_n;
}

// ---------------------------------------------------------------- attack
// FFT's Attack with the battle's weapons (RE_NOTES.md § Attack: read in the decompilation,
// checked on the original's previews and attacks). A sword or knife strikes the four tiles
// around the unit whose standing height is at most 3 below or 2.5 above its own; the target
// facing the attacker evades with its class evade (no shields, accessories or Weapon Guard
// at Gariland), from the side or the back it is always hit. XA: PA (a sword, bare hands) or
// (PA + Speed) / 2 (a knife), changed by the signs' compatibility, then on a critical hit
// (4 %) XA + 0..XA - 1 and the target pushed a tile away; damage XA * WP (bare hands: PA *
// Brave / 100), at most 999.
void attack_range(u8 who)
{
    const Unit *u = &st.unit[who];
    s16 h = map_tiles[u->z * MAP_W + u->x].stand;
    u8 d;
    hl_seq++;
    memset(st.reach, 0xFF, sizeof st.reach);
    for (d = 0; d < 4; d++) {
        s16 x = u->x + dir_x[d], z = u->z + dir_z[d];
        const Tile *t = tile_at(x, z);
        if (t && t->walk && t->stand + 6 >= h && t->stand <= h + 5) st.reach[z * MAP_W + x] = 1;
    }
}

// from the target: the attack's dominant axis against its facing (a tie: either axis)
u8 attack_side(u8 a, u8 t)
{
    const Unit *ua = &st.unit[a], *ut = &st.unit[t];
    s16 dx = (s16)ua->x - ut->x, dz = (s16)ua->z - ut->z;
    s16 ax = dx < 0 ? -dx : dx, az = dz < 0 ? -dz : dz, c = 0;
    if (!ax && !az) return SIDE_BACK;
    if (ax >= az) c += dir_x[ut->face] * (dx < 0 ? -1 : dx > 0);
    if (az >= ax) c += dir_z[ut->face] * (dz < 0 ? -1 : dz > 0);
    return c > 0 ? SIDE_FRONT : c < 0 ? SIDE_BACK : SIDE_SIDE;
}

u8 hit_chance(u8 a, u8 t)
{
    return attack_side(a, t) == SIDE_FRONT ? 100 - jobs[st.unit[t].job].cev : 100;
}

u16 attack_damage(u8 a, u8 t, u8 crit, u16 r)
{
    const Unit *ua = &st.unit[a], *ut = &st.unit[t];
    const Item *w = ua->eq[EQ_WEAPON] != 0xFF ? &items[ua->eq[EQ_WEAPON]] : RT_NULL;
    s16 xa = w && w->knife ? (ua->pa + ua->sp) >> 1 : ua->pa, ya;
    u8 c = zodiac_compat[ua->zodiac > ut->zodiac ? ua->zodiac - ut->zodiac : ut->zodiac - ua->zodiac];
    u16 p = (u16)(xa * 25), d;
    if (c == 3) c = ua->female == ut->female ? 4 : 5;       // opposite signs: worst, best
    if (c == 1) xa -= p / 100;
    else if (c == 2) xa += p / 100;
    else if (c == 4) xa -= p / 50;
    else if (c == 5) xa += p / 50;
    if (xa < 1) xa = 1;
    if (crit) xa += (s16)(((u32)r * (u16)xa) >> 15);
    ya = w ? w->wp : (s16)((u16)(ua->pa * ua->brave) / 100);
    if (ya < 1) ya = 1;
    d = (u16)(xa * ya);
    return d > 999 ? 999 : d;
}

// the hit rolled (rand() * 100 / 32768 >= the evade), then the critical, the damage and the
// knockback: one tile away along the dominant axis (a tie: z), onto a walkable, free, dry tile
// no higher (FFT compares the tiles' facing edges), not when the blow kills; a fall of more
// than Jump levels costs (levels - Jump) * max HP / 10
void attack(u8 a, u8 t)
{
    Unit *ua = &st.unit[a], *ut = &st.unit[t];
    s16 dx = (s16)ut->x - ua->x, dz = (s16)ut->z - ua->z;
    u8 ev = 100 - hit_chance(a, t);
    st.atk = a; st.tgt = t; st.act_t = 0; st.mode = M_ACT;
    if (W) W->num_ok = 0;
    ua->face = (dx < 0 ? -dx : dx) > (dz < 0 ? -dz : dz) ? (dx > 0 ? 0 : 1) : (dz > 0 ? 2 : 3);
    st.hit = (u8)(((u32)rand15() * 100 >> 15) >= ev);
    st.crit = st.push = 0; st.dmg = 0;
    if (!st.hit) return;
    st.crit = (u8)(((u32)rand15() * 100 >> 15) < 4);
    st.dmg = attack_damage(a, t, st.crit, st.crit ? rand15() : 0);
    if (st.crit && st.dmg < ut->hp) {
        s16 kx = ut->x, kz = ut->z;
        const Tile *s = &map_tiles[ut->z * MAP_W + ut->x], *n;
        if ((dx < 0 ? -dx : dx) > (dz < 0 ? -dz : dz)) kx += dx > 0 ? 1 : -1;
        else kz += dz > 0 ? 1 : -1;
        n = tile_at(kx, kz);
        if (n && n->walk && !n->water && n->stand <= s->stand && !unit_at(kx, kz)) {
            s16 fall = ((s->stand - n->stand) >> 1) - ut->jump;
            st.push = 1; st.kx = (u8)kx; st.kz = (u8)kz;
            if (fall > 0) st.dmg += (u16)((u32)fall * ut->max_hp / 10);
            if (st.dmg > 999) st.dmg = 999;
        }
    }
}

// M_ACT, once per frame: the target loses its HP at ACT_HIT (pushed, KO at 0: the turn order
// again), the menu comes back at ACT_END
static void attack_step(void)
{
    Unit *ut = &st.unit[st.tgt];
    if (st.act_t == ACT_HIT && st.hit) {
        ut->hp = st.dmg >= ut->hp ? 0 : ut->hp - st.dmg;
        if (st.push) { ut->x = st.kx; ut->z = st.kz; }
        if (!ut->hp) turn_order();
    }
    if (++st.act_t >= ACT_END) {
        st.mode = M_MENU; st.menu = st.moved ? MN_WAIT : MN_MOVE;
        st.cx = st.unit[st.act].x; st.cz = st.unit[st.act].z;
    }
}

// ---------------------------------------------------------------- units on screen
// a unit's feet: scene position of its tile centre at its standing height, interpolated
// while walking (t / WALK_FRAMES of the way to the next tile, with a hop on height changes);
// depth: the diagonal it is drawn after (both tiles' while walking)
typedef struct { s16 x, y; u8 depth; } Feet;

static Feet unit_feet(u8 rot, u8 i)
{
    Feet f;
    s16 u, v;
    const Unit *un = &st.unit[i];
    view_of(rot, un->x, un->z, &u, &v);
    f.x = scene_x(rot, u, v);
    f.y = scene_y(u, v) + HH - map_tiles[un->z * MAP_W + un->x].stand * (HU / 2);
    f.depth = (u8)(u + v);
    if (st.mode == M_WALK && st.sel == i && st.path_i + 1 < st.path_n) {
        const u8 *a = st.path[st.path_i], *b = st.path[st.path_i + 1];
        s16 u2, v2, x2, y2, t = st.walk_t, hop;
        s16 sa = map_tiles[a[1] * MAP_W + a[0]].stand, sb = map_tiles[b[1] * MAP_W + b[0]].stand;
        view_of(rot, a[0], a[1], &u, &v);
        view_of(rot, b[0], b[1], &u2, &v2);
        f.x = scene_x(rot, u, v); f.y = scene_y(u, v) + HH - sa * (HU / 2);
        x2 = scene_x(rot, u2, v2); y2 = scene_y(u2, v2) + HH - sb * (HU / 2);
        hop = sa != sb ? (t * (WALK_FRAMES - t)) * 2 : 0;            // up to 8 px at mid-step
        f.x += (x2 - f.x) * t / WALK_FRAMES;
        f.y += (y2 - f.y) * t / WALK_FRAMES - hop;
        f.depth = (u8)(u + v > u2 + v2 ? u + v : u2 + v2);
    }
    return f;
}

// Cover mask of a unit: the pixels in front of it (the view's depth: diagonals greater than
// its own) in its sprite's 16 x COVER_H box, one bit per pixel. Kept per unit (W->cover) and
// made again only when its feet, its depth or the view change: a unit standing still costs nothing.
static void cover(u16 *cm, s16 fx, s16 fy, u8 depth)
{
    const u8 *t1 = vbuf + 2 * SC_PLANE, *m1 = t1 + SC_PLANE, *t2 = m1 + SC_PLANE, *m2 = t2 + SC_PLANE;
    s16 bx = fx - 8, by = fy - UNIT_FOOT, b0 = bx >> 3, sh = 8 - (bx & 7), r, k;
    for (r = 0; r < COVER_H; r++) {
        s16 y = by + r;
        u32 m = 0;
        if ((u16)y < SC_H) {
            u16 o = (u16)(y * SC_BYTES + b0);
            for (k = 0; k < 3; k++, o++) {
                m <<= 8;
                if ((u16)(b0 + k) >= SC_BYTES) continue;
                if (t1[o] > depth) m |= m1[o];
                if (t2[o] > depth) m |= m2[o];
            }
        }
        cm[r] = (u16)(m >> sh);
    }
}

// FFT's battle idle (TYPE1.SEQ 6 / 7: frames 11 10 9 10 11 12 13 12 for 6 8 10 8 ticks at
// 60 Hz) and walk (8 / 9: the same frames for 2 4 6 4 ticks), in our frames (~30 per second)
static const u8 idle_seq[32] = { 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 3, 3, 3, 3 };
static const u8 walk_seq[16] = { 2, 1, 1, 0, 0, 0, 1, 1, 2, 3, 3, 4, 4, 4, 3, 3 };

// a unit's pose (units.h: P_*), 0 for the idle or the walk: KO lying, the attacker's swing,
// the target hit, kneeling with little HP left (FFT's Critical: HP <= max / 5)
static u8 unit_pose(u8 i)
{
    const Unit *un = &st.unit[i];
    if (st.mode == M_ACT) {
        if (i == st.atk && st.act_t < ACT_HIT) return st.act_t < ACT_BLOW ? P_WIND : P_BLOW;
        if (i == st.tgt && st.act_t >= ACT_HIT && st.act_t < ACT_HIT + HIT_FRAMES && st.hit) return P_HIT;
    }
    if (!un->hp) return P_KO;
    return (u16)(un->hp * 5) <= un->max_hp ? P_KNEEL : 0;
}

// a unit's frame (units.h) seen from orientation rot: its facing turned into the view, front
// frames looking down-left (+v), back ones up-left (-u), mirrored for +u and -v as in FFT (the
// mirrored frames are stored after the others)
static const u8 *unit_frame(u8 rot, u8 i)
{
    static const s8 fdx[4] = { 1, -1, 0, 0 }, fdz[4] = { 0, 0, 1, -1 };
    const Unit *un = &st.unit[i];
    s16 u0, v0, du, dv;
    u8 t = (u8)(st.tick + 11 * i), k;                  // the units out of step
    view_of(rot, 0, 0, &u0, &v0);
    view_of(rot, fdx[un->face], fdz[un->face], &du, &dv);
    du -= u0; dv -= v0;
    k = unit_pose(i);
    if (k) k += du < 0 || dv < 0;                      // a pose: front, back
    else {
        k = st.mode == M_WALK && i == st.sel ? walk_seq[t & 15] : idle_seq[t & 31];
        if (du < 0 || dv < 0) k += 5;
    }
    if (du > 0 || dv < 0) k += UNIT_FRAMES;
    return ugfx + (u16)(un->gfx * (2 * UNIT_FRAMES) + k) * (6 * UNIT_SH);
}

// the opaque pixels of row r (1 .. UNIT_SH - 2) of a sprite mask whose four neighbours are opaque
static u16 erode(const u16 *mk, u8 r)
{
    u16 o = (u16)~mk[r];
    return o & (u16)(o << 1) & (o >> 1) & (u16)~mk[r - 1] & (u16)~mk[r + 1];
}

// a unit and its shadow, both through its cover mask (ExtGraph: dest = dest & mask | data,
// mask 1 = transparent, so the data is cleared under the cover too), the unit's contour drawn
// over the cover, an enemy's diamond above it; c RT_NULL: the unit alone, uncovered (the
// rotation frames). The rows above the covered ones are drawn as they are (on the TI straight
// from the data file), the others copied and eroded.
static void draw_unit_covered(s16 sx, s16 sy, u8 rot, u8 i, const Cover *c)
{
    RtSprite s;
    u16 mk[COVER_H], ml[COVER_H], md[COVER_H];
    u8 r, top = UNIT_SH, cov = 0;              // rows [0, top) are drawn as they are
    const u8 *g = unit_frame(rot, i);
    s.w = 16;
    if (c) {
        const u16 *cm = c->cm;
        for (r = 0; r < 3; r++) {
            u16 k = cm[UNIT_FOOT - 1 + r];
            mk[r] = shadow_gfx[2][r] | k; ml[r] = shadow_gfx[0][r] & ~k; md[r] = shadow_gfx[1][r] & ~k;
        }
        s.h = 3;
        s.light = ml; s.dark = md; s.mask = mk;
        draw_sprite(sx - 8, sy - 1, &s);
        if (c->r0 <= c->r1) { cov = 1; top = c->r0 - 2; }   // the erosion reads two rows above
    }
#ifndef __m68k__
    top = 0;                                   // the PC copies every row (byte order)
#endif
    if (top) {                                 // native big-endian rows: drawn in place
        s.h = top;
        s.light = g; s.dark = g + 2 * UNIT_SH; s.mask = g + 4 * UNIT_SH;
        draw_sprite(sx - 8, sy - UNIT_FOOT, &s);
    }
    if (top < UNIT_SH) {
        for (r = top; r < UNIT_SH; r++) {
            mk[r] = RD16(g, 2 * UNIT_SH + r); ml[r] = RD16(g, r); md[r] = RD16(g, UNIT_SH + r);
        }
        if (cov) {              // covered: only the contour stays (the white outline and the
            const u16 *cm = c->cm;      // black line inside it), the sprite eroded twice and
            u8 r1 = c->r1;              // taken away, on the covered rows only (rows 0, 1 and
            u16 a, b, d;                // the last two: outline)
            r = c->r0;
            a = erode(mk, r - 1); b = erode(mk, r);
            for (; r <= r1; r++, a = b, b = d) {
                u16 h;
                d = erode(mk, r + 1);   // before mk[r] changes (it reads rows r .. r + 2)
                h = cm[r] & b & (u16)(b << 1) & (b >> 1) & a & d;
                mk[r] |= h; ml[r] &= ~h; md[r] &= ~h;
            }
        }
        s.h = UNIT_SH - top;
        s.light = ml + top; s.dark = md + top; s.mask = mk + top;
        draw_sprite(sx - 8, sy - UNIT_FOOT + top, &s);
    }
    if (st.unit[i].team == TEAM_ENEMY) {        // the team at a glance: a diamond above enemies,
        s.h = FOE_H;                            // never covered (it shows a hidden one too)
        s.light = foe_gfx[0]; s.dark = foe_gfx[1]; s.mask = foe_gfx[2];
        draw_sprite(sx - 8, sy - UNIT_FOOT - FOE_H + 1, &s);
    }
}

// ---------------------------------------------------------------- the rotation animation
// 32 angles per turn: S = round(6 sqrt2 sin a * 256), view r is at 45 + 90 r degrees (index
// 4 + 8 r), where the projection equals the cardinal one exactly (6 * 256)
static const s16 sin32[32] = { 0, 424, 831, 1207, 1536, 1806, 2007, 2130, 2172, 2130, 2007,
    1806, 1536, 1207, 831, 424, 0, -424, -831, -1207, -1536, -1806, -2007, -2130, -2172, -2130,
    -2007, -1806, -1536, -1207, -831, -424 };

// Drawn at half resolution (80 x 50, every size halved, a level = 3 px) then doubled onto the
// screen: the rows are what costs (ti-cycles: ~9k per polygon at full size)

static void draw_turn(void)
{
    s16 a = (s16)(4 + 8 * st.rot + st.turn * (s16)(st.turn_t + 1) * (8 / (TURN_FRAMES + 1))) & 31;
    s16 S = sin32[a], C = sin32[(a + 8) & 31];
    s16 pvx, pvy, cu, cv, i, j, x, z;
    u8 n = 0;
    // the pivot (cursor tile centre) stays where it is on screen
    view_of(st.rot, st.cx, st.cz, &cu, &cv);
    pvx = scene_x(st.rot, cu, cv) - st.camx;
    pvy = scene_y(cu, cv) + HH - st.camy;
    memset(half, 0, sizeof half);
    D.l = half[0]; D.d = half[1]; D.stride = TW / 8; D.ox = D.oy = 0;
    D.x0 = 0; D.x1 = TW; D.y0 = 0; D.y1 = TH;
    for (z = 0; z <= MAP_H; z++)                                   // grid corners at height 0
        for (x = 0; x <= MAP_W; x++) {
            s16 x2 = 2 * (x - st.cx) - 1, z2 = 2 * (z - st.cz) - 1;
            gx[x][z] = (pvx >> 1) + (s16)((muls16(x2, C) - muls16(z2, S)) >> 9);
            gy[x][z] = (pvy >> 1) + (s16)((muls16(x2, S) + muls16(z2, C)) >> 10);
        }
    {                                           // counting sort by the centre's depth
        u8 *cnt = W->count;
        memset(cnt, 0, sizeof W->count);
        for (z = 0, i = 0; z < MAP_H; z++)
            for (x = 0; x < MAP_W; x++, i++) {
                s16 k = (gy[x][z] + gy[x + 1][z + 1] + 512) >> 2;
                key[i] = k = k < 0 ? 0 : k > 255 ? 255 : k;
                cnt[k]++;
            }
        for (i = 0, j = 0; i < 256; i++) { u8 c = cnt[i]; cnt[i] = (u8)j; j += c; }
        for (i = 0; i < MAP_W * MAP_H; i++) order[cnt[key[i]]++] = (u8)i;
        n = MAP_W * MAP_H;
    }
    for (i = 0; i < n; i++) {
        u8 o = order[i], f;
        const Tile *t = &map_tiles[o];
        s16 tx = o % MAP_W, tz = o / MAP_W, cx[4], cy[4], px[4], py[4];
        cx[0] = gx[tx][tz]; cx[1] = gx[tx + 1][tz]; cx[2] = gx[tx + 1][tz + 1]; cx[3] = gx[tx][tz + 1];
        cy[0] = gy[tx][tz]; cy[1] = gy[tx + 1][tz]; cy[2] = gy[tx + 1][tz + 1]; cy[3] = gy[tx][tz + 1];
        {                                       // off screen: box of the column down to h 0
            s16 xa = cx[0], xb = cx[0], ya = cy[0], yb = cy[0];
            u8 c;
            for (c = 1; c < 4; c++) {
                if (cx[c] < xa) xa = cx[c];
                if (cx[c] > xb) xb = cx[c];
                if (cy[c] < ya) ya = cy[c];
                if (cy[c] > yb) yb = cy[c];
            }
            if (xb < 0 || xa >= TW || yb < 0 || ya - MAXH * (HU / 2) >= TH) continue;
        }
        cy[0] -= t->c[0] * (HU / 2); cy[1] -= t->c[1] * (HU / 2);
        cy[2] -= t->c[2] * (HU / 2); cy[3] -= t->c[3] * (HU / 2);
        // faces towards the viewer: +x (corners 1, 2) if S > 0, -x (3, 0) if S < 0,
        // +z (2, 3) if C > 0, -z (0, 1) if C < 0; lit when facing left
        for (f = 0; f < 4; f++) {
            static const u8 fa[4] = { 1, 3, 2, 0 }, fb[4] = { 2, 0, 3, 1 };
            static const s8 fdx[4] = { 1, -1, 0, 0 }, fdz[4] = { 0, 0, 1, -1 };
            static const u8 mir[4][4] = { { 1, 0, 3, 2 }, { 1, 0, 3, 2 }, { 3, 2, 1, 0 }, { 3, 2, 1, 0 } };
            s16 facing = f == 0 ? S : f == 1 ? -S : f == 2 ? C : -C, lit;
            const Tile *nt;
            u8 ha, hb, na = 0, nb = 0, k;
            if (facing <= 0) continue;
            nt = tile_at(tx + fdx[f], tz + fdz[f]);
            ha = t->c[fa[f]]; hb = t->c[fb[f]];
            if (nt) { na = nt->c[mir[f][fa[f]]]; nb = nt->c[mir[f][fb[f]]]; }
            if (na >= ha && nb >= hb) continue;
            px[0] = cx[fa[f]]; py[0] = cy[fa[f]];
            px[1] = cx[fb[f]]; py[1] = cy[fb[f]];
            px[2] = cx[fb[f]]; py[2] = cy[fb[f]] + (hb > nb ? (hb - nb) * (HU / 2) : 0);
            px[3] = cx[fa[f]]; py[3] = cy[fa[f]] + (ha > na ? (ha - na) * (HU / 2) : 0);
            lit = f == 0 ? C : f == 1 ? -C : f == 2 ? -S : S;      // normal's screen x < 0
            k = t->look[1 + f] + (lit >= 0);                       // the views' grey, one
            if (k >= SHADES) k = SHADES - 1;                       // darker facing right
            fill_poly(px, py, 4, turn_pat[k][0], turn_pat[k][1]);
        }
        fill_poly(cx, cy, 4, turn_pat[t->look[0]][0], turn_pat[t->look[0]][1]);
    }
    {                                                             // doubled onto the screen
        const u8 *hl = half[0], *hd = half[1];
        u8 *dl = rt_light, *dd = rt_dark;
        for (i = 0; i < TH; i++, dl += 2 * RT_PBYTES, dd += 2 * RT_PBYTES)
            for (j = 0; j < TW / 8; j++) {
                u16 l = dbl[*hl++], d = dbl[*hd++];
                WR16(dl, j, l); WR16(dl + RT_PBYTES, j, l);
                WR16(dd, j, d); WR16(dd + RT_PBYTES, j, d);
            }
    }
   
    // units on top, at their projected feet (no occlusion while turning)
    for (i = 0; i < NUNIT; i++) {
        const Unit *un = &st.unit[i];
        s16 x2 = 2 * (un->x - st.cx), z2 = 2 * (un->z - st.cz), sx, sy;
        if (un->x >= MAP_W) continue;
        sx = pvx + (s16)((muls16(x2, C) - muls16(z2, S)) >> 8);
        sy = pvy + (s16)((muls16(x2, S) + muls16(z2, C)) >> 9) - map_tiles[un->z * MAP_W + un->x].stand * (HU / 2);
        draw_unit_covered(sx, sy, st.rot, (u8)i, RT_NULL);
    }
}

// ---------------------------------------------------------------- camera
static void cam_clamp(s16 *x, s16 *y)
{
    if (*x < 0) *x = 0;
    if (*x > SC_W - RT_W) *x = SC_W - RT_W;
    if (*y < 0) *y = 0;
    if (*y > SC_H - RT_H) *y = SC_H - RT_H;
}

static void cam_target(s16 *tx, s16 *ty)
{
    s16 u, v;
    view_of(st.rot, st.cx, st.cz, &u, &v);
    *tx = scene_x(st.rot, u, v) - RT_W / 2;
    *ty = scene_y(u, v) + HH - map_tiles[st.cz * MAP_W + st.cx].stand * (HU / 2) - RT_H / 2 - 6;
    cam_clamp(tx, ty);
}

static s16 ease(s16 c, s16 t)
{
    s16 d = (t - c) >> 2;
    if (!d) d = t > c ? 1 : t < c ? -1 : 0;
    return c + d;
}

// ---------------------------------------------------------------- game
static u8 *sbuf;
static void hud_labels(void);

static void
#ifdef __m68k__
__attribute__((__stkparm__))
#endif
release(void)
{
    if (sbuf) free(sbuf);
    if (vbuf) free(vbuf);
    sbuf = vbuf = RT_NULL;
    scene_l = scene_d = RT_NULL;
}

void game_init(void)
{
    u16 i;
    rt_state = &st;
    rt_state_size = sizeof(st);
#ifdef __m68k__
    sbuf = vbuf = RT_NULL;
#endif
    for (i = 0; i < 4; i++) {                  // read in place (archived on the TI): fetched
        char name[6] = "fftv0";                // again each run, the archive may have moved
        name[4] = (char)('0' + i);
        views[i] = rt_file(name, RT_NULL);
    }
    ugfx = rt_file("fftu", RT_NULL);
    if (!sbuf) {                               // two blocks: AMS allocates at most ~64 KB
        vbuf = malloc(VIEW_BYTES);
        sbuf = vbuf ? malloc(2 * SC_PLANE + sizeof(Work)) : RT_NULL;
        if (!sbuf) {                           // not enough memory: a message, ESC quits
            if (vbuf) free(vbuf);
            vbuf = RT_NULL; W = RT_NULL;
            return;
        }
        W = (Work *)(sbuf + 2 * SC_PLANE);
        memset(W, 0, sizeof(Work));
        for (i = 0; i < NUNIT; i++) W->cover[i].rot = 0xFF;
        W->hud_t[0] = 0xFE;
        hud_labels();
        scene_l = sbuf; scene_d = sbuf + SC_PLANE;
#ifdef __m68k__
        atexit(release);
#else
        (void)release;
#endif
    }
    scene_rot = vbuf_rot = 0xFF;
    for (i = 0; i < 256; i++) {
        u16 d = 0;
        u8 b;
        for (b = 0; b < 8; b++) if (i & (0x80 >> b)) d |= 0xC000 >> (2 * b);
        dbl[i] = d;
    }
}

void game_scenario(u16 n)
{
    memset(&st, 0, sizeof st);
    if (!W) return;
    rt_seed = n ? n : rt_ticks();                 // a new draw each battle; the tests: fixed
    rand_seed(rt_seed);
    battle_units();                               // Gariland's eleven units at their start tiles
    turn_start();
    while (n && st.act != U_RAMZA) turn_end(0, 0);   // tests: Ramza's first turn
    if (n >= 1 && n <= 4) {                       // an enemy behind a building, each view
        st.unit[U_HIDE].x = 1; st.unit[U_HIDE].z = 11;    // (fft_test --find)
        st.rot = (u8)(n - 1);
        st.cx = st.unit[U_HIDE].x; st.cz = st.unit[U_HIDE].z;
    } else if (n == 5) {                          // Ramza selected, his range shown
        st.mode = M_TARGET; st.sel = U_RAMZA;
        compute_reach(U_RAMZA);
    } else if (n == 6) {                          // in the middle of a rotation
        st.turn = 1; st.turn_t = TURN_FRAMES / 2;
    } else if (n == 7) {                          // Ramza walking
        st.sel = U_RAMZA;
        if (make_path(U_RAMZA, 4, 8)) { st.mode = M_WALK; st.path_i = 1; st.walk_t = 2; }    } else if (n == 8 || n == 9) {                // an enemy in front of Ramza: aimed at, hit
        st.unit[1].x = 4; st.unit[1].z = 10; st.unit[1].face = 2;
        st.mode = M_AIM; st.cx = 4; st.cz = 10;
        attack_range(U_RAMZA);
        if (n == 9) {                             // (a hit: drawn again after a miss)
            st.acted = 1;
            do attack(U_RAMZA, 1); while (!st.hit);
            while (st.act_t < ACT_HIT + 3) attack_step();
        }
    }
    cam_target(&st.camx, &st.camy);
    hl_seq++;
}

static void move_cursor(s16 du, s16 dv)
{
    s16 u, v, x, z;
    view_of(st.rot, st.cx, st.cz, &u, &v);
    u += du; v += dv;
    if ((u16)u >= view_w(st.rot) || (u16)v >= view_h(st.rot)) return;
    world_of(st.rot, u, v, &x, &z);
    st.cx = (u8)x; st.cz = (u8)z;
}

// a step (du, dv) in the view as a facing (world +x, -x, +z, -z), and back
static u8 face_of(s16 du, s16 dv)
{
    s16 x0, z0, x, z;
    world_of(st.rot, 1, 1, &x0, &z0);
    world_of(st.rot, 1 + du, 1 + dv, &x, &z);
    return x > x0 ? 0 : x < x0 ? 1 : z > z0 ? 2 : 3;
}

static void to_menu(void)
{
    st.mode = M_MENU;
    st.cx = st.unit[st.act].x; st.cz = st.unit[st.act].z;
}

// the turn menu: Move and Act once each (a move undone with [shift] before acting), Wait
// (then the facing), Status
static void menu_keys(void)
{
    Unit *u = &st.unit[st.act];
    if (input_pressed(K_UP)) st.menu = st.menu ? st.menu - 1 : MN_N - 1;
    if (input_pressed(K_DOWN)) st.menu = st.menu < MN_N - 1 ? st.menu + 1 : 0;
    if (input_pressed(K_A | K_ENTER)) {
        if (st.menu == MN_MOVE && !st.moved) {
            st.mode = M_TARGET; st.sel = st.act;
            compute_reach(st.act);
        } else if (st.menu == MN_ACT && !st.acted) {
            st.mode = M_AIM;
            attack_range(st.act);
        } else if (st.menu == MN_WAIT) {
            st.mode = M_FACE; st.wface = u->face;
        } else if (st.menu == MN_STATUS) {
            st.mode = M_STATUS; st.sel = st.act; st.back = M_MENU;
        }
    } else if (input_pressed(K_ESC | K_B)) {
        if (st.moved && !st.acted) {               // the move undone
            u->x = st.mx; u->z = st.mz; u->face = st.mface;
            st.moved = 0; st.menu = MN_MOVE;
            to_menu();
        } else st.mode = M_BROWSE;                 // looking around
    }
}

u8 game_update(void)
{
    s16 tx, ty;
    u8 mode0 = st.mode;
    if (!W || !ugfx || !views[0] || !views[1] || !views[2] || !views[3]) return !input_pressed(K_ESC);
    st.tick++;
    if (st.turn) {                                 // rotating: nothing else moves the view
        if (++st.turn_t >= TURN_FRAMES) {          // done: the cursor stays where it was
            s16 u, v, px, py;
            view_of(st.rot, st.cx, st.cz, &u, &v);
            px = scene_x(st.rot, u, v) - st.camx; py = scene_y(u, v) - st.camy;
            st.rot = (st.rot + st.turn) & 3;
            st.turn = 0; st.turn_t = 0;
            view_of(st.rot, st.cx, st.cz, &u, &v);
            st.camx = scene_x(st.rot, u, v) - px; st.camy = scene_y(u, v) - py;
            cam_clamp(&st.camx, &st.camy);
        }
    } else if (input_pressed(K_F5)) { st.turn = 1; st.turn_t = 0; }
    else if (input_pressed(K_F1)) { st.turn = -1; st.turn_t = 0; }

    if (st.mode == M_WALK) {
        const u8 *a = st.path[st.path_i], *b = st.path[st.path_i + 1];
        st.unit[st.sel].face = b[0] > a[0] ? 0 : b[0] < a[0] ? 1 : b[1] > a[1] ? 2 : 3;
        if (++st.walk_t >= WALK_FRAMES) {
            st.walk_t = 0;
            st.path_i++;
            st.unit[st.sel].x = st.path[st.path_i][0];
            st.unit[st.sel].z = st.path[st.path_i][1];
            st.cx = st.unit[st.sel].x; st.cz = st.unit[st.sel].z;
            if (st.path_i + 1 >= st.path_n) {      // back to the menu, on Wait (no Act yet)
                st.mode = M_MENU; st.moved = 1; st.menu = MN_WAIT;
            }
        }
    } else if (st.mode == M_ACT) {
        attack_step();
    } else if (st.wait_t) {                        // an enemy's or the guest's turn: it waits
        if (!st.turn && !--st.wait_t) turn_end(0, 0);
    } else if (!st.turn) {
        if (st.mode == M_MENU) {
            menu_keys();
        } else if (st.mode == M_FACE) {            // the arrows turn the unit, as the cursor
            Unit *u = &st.unit[st.act];
            if (input_pressed(K_UP)) u->face = face_of(0, -1);
            if (input_pressed(K_DOWN)) u->face = face_of(0, 1);
            if (input_pressed(K_LEFT)) u->face = face_of(-1, 0);
            if (input_pressed(K_RIGHT)) u->face = face_of(1, 0);
            if (input_pressed(K_A | K_ENTER)) turn_end(st.moved, st.acted);
            else if (input_pressed(K_ESC | K_B)) { u->face = st.wface; st.mode = M_MENU; }
        } else if (st.mode == M_STATUS) {
            if (input_pressed(K_A | K_ENTER | K_ESC | K_B)) st.mode = st.back;
        } else {                                   // the cursor: looking around, or a tile
            u32 arrows = rt_keys & (K_UP | K_DOWN | K_LEFT | K_RIGHT);
            if (input_pressed(K_UP | K_DOWN | K_LEFT | K_RIGHT)) st.rep = 0;
            if (arrows && (st.rep == 0 || (st.rep >= 8 && !((st.rep - 8) % 3)))) {
                if (arrows & K_UP) move_cursor(0, -1);
                if (arrows & K_DOWN) move_cursor(0, 1);
                if (arrows & K_LEFT) move_cursor(-1, 0);
                if (arrows & K_RIGHT) move_cursor(1, 0);
            }
            st.rep = arrows ? (u8)(st.rep < 255 ? st.rep + 1 : st.rep) : 0;
            if (input_pressed(K_A | K_ENTER)) {
                u8 o = unit_at(st.cx, st.cz), c = (u8)(st.cz * MAP_W + st.cx);
                if (st.mode == M_TARGET) {
                    Unit *u = &st.unit[st.sel];
                    if (st.reach[c] != 0xFF && st.reach[c] > 0 && make_path(st.sel, st.cx, st.cz)) {
                        st.mx = u->x; st.mz = u->z; st.mface = u->face;
                        st.mode = M_WALK; st.path_i = 0; st.walk_t = 0;
                    }
                } else if (st.mode == M_AIM) {     // a unit in the weapon's reach
                    if (st.reach[c] == 1 && o && st.unit[o - 1].hp) { st.acted = 1; attack(st.act, o - 1); }
                } else if (o) {                    // a unit's numbers
                    st.mode = M_STATUS; st.sel = o - 1; st.back = M_BROWSE;
                } else to_menu();
            } else if (input_pressed(K_ESC | K_B)) {
                if (st.mode == M_BROWSE && input_pressed(K_ESC)) return 0;
                to_menu();
            }
        }
    }
    if (!st.turn) {
        cam_target(&tx, &ty);
        st.camx = ease(st.camx, tx);
        st.camy = ease(st.camy, ty);
    }
    if (RINGS(st.mode) != RINGS(mode0)) hl_seq++;   // the reach rings
    return 1;
}

// the camera's window of the scene: 100 rows of 160 px from any pixel position, 32 bits at a
// time (a 32-bit word and the next 16 bits shifted)
static void copy_view(void)
{
    s16 y, i, sh = st.camx & 15, rs = 16 - sh;
    const u8 *sl = scene_l + st.camy * SC_BYTES + ((st.camx >> 4) << 1);
    const u8 *sd = scene_d + st.camy * SC_BYTES + ((st.camx >> 4) << 1);
    u8 *dl = rt_light, *dd = rt_dark;
    for (y = 0; y < RT_H; y++, sl += SC_BYTES, sd += SC_BYTES, dl += RT_PBYTES, dd += RT_PBYTES) {
        if (!sh) {
            for (i = 0; i < RT_W / 16; i += 2) { WR32(dl, i, RD32(sl, i)); WR32(dd, i, RD32(sd, i)); }
        } else {
            for (i = 0; i < RT_W / 16; i += 2) {
                WR32(dl, i, RD32(sl, i) << sh | RD16(sl, i + 2) >> rs);
                WR32(dd, i, RD32(sd, i) << sh | RD16(sd, i + 2) >> rs);
            }
        }
    }
}

static const char *const unit_names[UNIT_GFX_N] = { "Ramza", "Delita", "Squire", "Squire", "Chemist", "Chemist" };

// the turns' labels: AMS text drawn once into the HUD buffer and kept as 16-pixel rows; the
// names turned a quarter for the panels (reading upwards: glyph row y in bit 4 - y); the
// facing arrows: a right triangle pointing up-right, mirrored for the other three, outlined
static void hud_labels(void)
{
    static const char ab[UNIT_GFX_N][3] = { "Ra", "De", "Sq", "Sq", "Ch", "Ch" };
    static const u8 tri[8] = { 0, 0x7E, 0x3E, 0x1E, 0x0E, 0x06, 0x02, 0 };
    void *sl = rt_light, *sd = rt_dark;
    u8 g, r, b;
    rt_light = HUD_L; rt_dark = HUD_D;
    for (g = 0; g < UNIT_GFX_N; g++) {
        u8 x;
        memset(W->hud, 0, sizeof W->hud);
        draw_text(0, 0, ab[g], F_SMALL, C_BLACK);
        for (r = 0; r < 5; r++) W->label[g][r] = (u16)(HUD_L[r * RT_PBYTES] << 8 | HUD_L[r * RT_PBYTES + 1]);
        memset(W->hud, 0, sizeof W->hud);
        draw_text(0, 0, unit_names[g], F_SMALL, C_BLACK);
        for (r = 0; r < 5; r++)
            for (x = 0; x < PANEL_H - 3; x++)
                if (HUD_L[r * RT_PBYTES + (x >> 3)] & (0x80 >> (x & 7))) W->pname[g][PANEL_H - 2 - x] |= (u8)(0x10 >> r);
    }
    rt_light = sl; rt_dark = sd;
    for (g = 0; g < 4; g++)
        for (r = 0; r < 8; r++) {
            u8 f = tri[g == 1 || g == 2 ? 7 - r : r], m = 0;            // +u, +v: upside down
            if (g >= 2) { for (b = 0; b < 8; b++) if (f & (1 << b)) m |= (u8)(0x80 >> b); f = m; }
            W->arrow[g][0][r] = f;                                     // +v, -u: mirrored
        }
    for (g = 0; g < 4; g++)
        for (r = 0; r < 8; r++) {
            u8 o = (u8)((r ? W->arrow[g][0][r - 1] : 0) | W->arrow[g][0][r] | (r < 7 ? W->arrow[g][0][r + 1] : 0));
            W->arrow[g][1][r] = (u8)(o | o << 1 | o >> 1);
        }
}

static u8 put_s(char *s, u8 n, const char *t) { while (*t) s[n++] = *t++; return n; }
static u8 put_n(char *s, u8 n, u16 v)
{
    char b[5];
    u8 k = 0;
    do { b[k++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (k) s[n++] = b[--k];
    return n;
}


// AMS text drawn into a HUD buffer (rows of RT_PBYTES), the screen planes put back
static void text_into(u16 *l, u16 *d, s16 x, s16 y, const char *s, u8 color)
{
    void *sl = rt_light, *sd = rt_dark;
    rt_light = l; rt_dark = d;
    draw_text(x, y, s, F_SMALL, color);
    rt_light = sl; rt_dark = sd;
}

// 3 x 5 digits and '/' for the panels' numbers, turned a quarter: per glyph column (left to
// right), its 5 pixels (bit 4: the top row)
static const u8 digits[11][3] = { { 0x1F, 0x11, 0x1F }, { 0x09, 0x1F, 0x01 }, { 0x17, 0x15, 0x1D },
    { 0x15, 0x15, 0x1F }, { 0x1C, 0x04, 0x1F }, { 0x1D, 0x15, 0x17 }, { 0x1F, 0x15, 0x17 },
    { 0x10, 0x10, 0x1F }, { 0x1F, 0x15, 0x1F }, { 0x1D, 0x15, 0x1F }, { 0x03, 0x04, 0x18 } };

// A unit's panel against a side of the screen, as Metal Gear Solid: Portable Ops' HUD, its
// text turned a quarter (reading upwards): its name (AMS text), then its HP and MP as
// "52/52" and "16/16" in 3 x 5 digits, each beside its 2-pixel gauge filling upwards (HP
// black, MP dark grey); white (light grey: an enemy), a black edge toward the map. 16 x
// PANEL_H; columns: name 0-4, numbers 6-10, gauges 12-13, edge 15 on the left side; edge 0,
// name 2-6, numbers 8-12, gauges 14-15 on the right one (read in the same order).
#define SEG 19                                     // a number and its gauge: 5 characters
static void make_panel(u8 side, u8 i)
{
    u16 *pl = W->pan[side][0], *pd = W->pan[side][1];
    const Unit *u = &st.unit[i];
    const u8 *nm = W->pname[u->gfx];
    u16 bg = u->team == TEAM_ENEMY ? 0xFFFF : 0, edge = side ? 0x8000 : 1, gb = side ? 0x0003 : 0x000C;
    u8 ns = side ? 9 : 11, ds = side ? 3 : 5;      // the name's, the numbers' shift (5 bits)
    u8 r, x, k, n, g;
    char s[12];
    for (r = 0; r < PANEL_H; r++) {                // the edge, the name
        u16 v = (u16)nm[r] << ns;
        pl[r] = bg | edge | v; pd[r] = edge | v;
    }
    pl[0] = pd[0] = 0xFFFF;                        // and the top row
    for (k = 0; k < 2; k++) {                      // HP at the bottom, MP above it
        u16 v = k ? u->mp : u->hp, m = k ? u->max_mp : u->hp > u->max_hp ? u->hp : u->max_hp;
        u8 base = (u8)(PANEL_H - 2 - k * (SEG + 3)), f = v && m ? (u8)((SEG * v + m - 1) / m) : 0;
        n = put_n(s, 0, v); s[n++] = '/'; n = put_n(s, n, m);
        for (x = 0; x < n && 4 * x + 3 <= SEG; x++) {
            const u8 *gl = digits[s[x] == '/' ? 10 : s[x] - '0'];
            for (g = 0; g < 3; g++) {
                u16 b = (u16)gl[g] << ds;
                pl[base - 4 * x - g] |= b; pd[base - 4 * x - g] |= b;
            }
        }
        for (x = 0; x < SEG; x++) {
            r = (u8)(base - x);
            if (x >= f) { if (bg) pl[r] &= ~gb; else pl[r] |= gb; }
            else if (k) { pl[r] &= ~gb; pd[r] |= gb; }
            else { pl[r] |= gb; pd[r] |= gb; }
        }
    }
}

// The menu, a unit's numbers or an attack's preview (its hit chance and damage, as FFT shows
// them on the target) in a box at the top right (white, light grey for an enemy, a black
// edge toward the map), drawn into W->win one part a frame: the box, then each line (AMS
// text costs ~70k a status line). The menu's pointer is added when it is copied.
static void make_window(void)
{
    static const char *const entries[MN_N] = { "Move", "Act", "Wait", "Status" };
    u16 *l = W->win[0], *d = W->win[1];
    const Unit *u = &st.unit[W->win_k[2]];
    u8 r, i, n, k = W->win_n++;
    char s[28];
    if (!k) {
        u8 h = (W->win_k[0] == M_AIM ? 2 : 4) * 6 + 3, w = W->win_k[0] == M_MENU ? 2 : W->win_k[0] == M_AIM ? 3 : 6;
        u8 enemy = W->win_k[0] != M_MENU && u->team == TEAM_ENEMY;
        W->win_w = w; W->win_h = h;
        for (r = 0; r < WIN_ROWS; r++)
            for (i = 0; i < w; i++) {
                u16 e = r == h - 1 ? 0xFFFF : r >= h || i ? 0 : 0x8000;     // the edge: black
                WR16((u8 *)l, r * (RT_PBYTES / 2) + i, (r < h && enemy ? 0xFFFF : 0) | e);
                WR16((u8 *)d, r * (RT_PBYTES / 2) + i, e);
            }
        return;
    }
    k--;
    if (W->win_k[0] == M_MENU) {
        text_into(l, d, 7, 2 + 6 * k, entries[k], k <= MN_ACT && W->win_k[1] & (1 << k) ? C_LGRAY : C_BLACK);
        return;
    }
    if (W->win_k[0] == M_AIM) {                    // the attacker is win_k[1]
        if (k >= 2) return;
        if (k == 0) { n = put_s(s, 0, "Hit "); n = put_n(s, n, hit_chance(W->win_k[1], W->win_k[2])); s[n++] = '%'; }
        else { n = put_s(s, 0, "Dmg "); n = put_n(s, n, attack_damage(W->win_k[1], W->win_k[2], 0, 0)); }
        s[n] = 0;
        text_into(l, d, 3, 2 + 6 * k, s, C_BLACK);
        return;
    }
    if (k == 0) {
        n = put_s(s, 0, unit_names[u->gfx]);
        if (u->gfx <= UG_DELITA) n = put_s(s, n, " Squire");
        n = put_s(s, n, "  Lv "); n = put_n(s, n, u->level);
    } else if (k == 1) {
        n = put_s(s, 0, "HP "); n = put_n(s, n, u->hp); s[n++] = '/'; n = put_n(s, n, u->max_hp);
        n = put_s(s, n, "  MP "); n = put_n(s, n, u->mp); s[n++] = '/'; n = put_n(s, n, u->max_mp);
    } else if (k == 2) {
        n = put_s(s, 0, "Move "); n = put_n(s, n, u->move); n = put_s(s, n, "  Jump "); n = put_n(s, n, u->jump);
        n = put_s(s, n, "  Speed "); n = put_n(s, n, u->sp);
    } else {
        n = put_s(s, 0, "PA "); n = put_n(s, n, u->pa); n = put_s(s, n, "  MA "); n = put_n(s, n, u->ma);
    }
    s[n] = 0;
    text_into(l, d, 3, 2 + 6 * k, s, C_BLACK);
}

// The HUD: the top strip (the turn order, the view and the height under the cursor), the menu
// or a status window at the top right, the units' panels at the bottom corners. Each is drawn
// into a buffer when what it shows changes, at most one per frame and none on a turn's first
// frame (the camera starts moving), and copied every frame.
static void draw_hud(void)
{
    char s[8];
    u8 o = unit_at(st.cx, st.cz), h = map_tiles[st.cz * MAP_W + st.cx].stand, n = 0, i, r, w, k[3];
    static const char dirs[4] = { 'S', 'W', 'N', 'E' };
    u32 *a, *b;
    u16 *c, *d;
    u16 pk[2][3];
    s[n++] = dirs[st.rot]; s[n++] = ' '; s[n++] = 'h';
    if (h >= 20) s[n++] = (char)('0' + h / 20);
    s[n++] = (char)('0' + (h / 2) % 10);
    if (h & 1) { s[n++] = '.'; s[n++] = '5'; }
    s[n] = 0;
    k[0] = st.mode == M_MENU || st.mode == M_STATUS ? st.mode : 0;
    k[1] = k[0] == M_MENU ? (u8)(st.moved | st.acted << 1) : 0;
    k[2] = k[0] == M_STATUS ? st.sel : 0;
    if (st.mode == M_AIM && o && st.reach[st.cz * MAP_W + st.cx] == 1 && st.unit[o - 1].hp) {
        k[0] = M_AIM; k[1] = st.act; k[2] = o - 1;  // the preview
    }
    pk[0][0] = (u16)(st.act + 1); pk[0][1] = st.unit[st.act].hp; pk[0][2] = st.unit[st.act].mp;
    pk[1][0] = o && o - 1 != st.act ? o : 0;
    pk[1][1] = pk[1][0] ? st.unit[o - 1].hp : 0; pk[1][2] = pk[1][0] ? st.unit[o - 1].mp : 0;
    for (i = 0, w = 0; i < ORDER_N; i++) w |= st.turns[i] != W->hud_t[i];
    if (!w) W->hud_late = 0;
    else if (!W->hud_late++) w = 0;                // not on the turn's first frame
    if (w) {
        for (r = 0; r < 8; r++) {                  // white, 64 px
            a = (u32 *)(HUD_L + r * RT_PBYTES); b = (u32 *)(HUD_D + r * RT_PBYTES);
            a[0] = a[1] = b[0] = b[1] = 0;
        }
        // the turn order: the active unit, then the next turns; enemies on light grey,
        // underlined in black
        for (i = 0; i < ORDER_N && st.turns[i] != 0xFF; i++) {
            const Unit *u = &st.unit[st.turns[i]];
            s16 x = i ? 3 + 10 * i : 0, bw = i ? (i == ORDER_N - 1 ? 11 : 10) : 12, tx = x + 1 + !i;
            u8 *pl = HUD_L + (x >> 3), *pd = HUD_D + (x >> 3);
            if (u->team == TEAM_ENEMY) {           // light grey: the light plane; the line black
                u32 m = ((1UL << bw) - 1) << (32 - bw - (x & 7));
                u8 m0 = (u8)(m >> 24), m1 = (u8)(m >> 16), m2 = (u8)(m >> 8);
                for (r = 0; r < 8; r++, pl += RT_PBYTES, pd += RT_PBYTES) {
                    pl[0] |= m0; pl[1] |= m1; pl[2] |= m2;
                    if (r == 7) { pd[0] |= m0; pd[1] |= m1; pd[2] |= m2; }
                }
            }
            pl = HUD_L + RT_PBYTES + (tx >> 3); pd = HUD_D + RT_PBYTES + (tx >> 3);
            for (r = 0; r < 5; r++, pl += RT_PBYTES, pd += RT_PBYTES) {   // black: both planes
                u32 v = (u32)W->label[u->gfx][r] << (8 - (tx & 7));
                u8 b0 = (u8)(v >> 16), b1 = (u8)(v >> 8), b2 = (u8)v;
                pl[0] |= b0; pl[1] |= b1; pl[2] |= b2;
                pd[0] |= b0; pd[1] |= b1; pd[2] |= b2;
            }
        }
        for (i = 0; i < ORDER_N; i++) W->hud_t[i] = st.turns[i];
        W->hud_late = 0;
    } else if (!W->hud_late) {
        for (i = 0; i <= n && s[i] == W->hud_s[i]; i++) ;
        if (i <= n) {                              // the view and the height: 32 px after them
            for (r = 0; r < 8; r++) {
                a = (u32 *)(HUD_L + r * RT_PBYTES); b = (u32 *)(HUD_D + r * RT_PBYTES);
                a[2] = b[2] = 0;
            }
            text_into(W->hud[0], W->hud[1], 67, 1, s, C_BLACK);
            for (i = 0; i <= n; i++) W->hud_s[i] = s[i];
        } else if (k[0] && (k[0] != W->win_k[0] || k[1] != W->win_k[1] || k[2] != W->win_k[2])) {
            for (i = 0; i < 3; i++) W->win_k[i] = k[i];
            W->win_n = 0;
            make_window();
        } else if (k[0] && W->win_n < WIN_DONE) {
            make_window();
        } else {
            for (i = 0; i < 2; i++)
                if (pk[i][0] && (pk[i][0] != W->pan_k[i][0] || pk[i][1] != W->pan_k[i][1] || pk[i][2] != W->pan_k[i][2])) {
                    W->pan_k[i][0] = pk[i][0]; W->pan_k[i][1] = pk[i][1]; W->pan_k[i][2] = pk[i][2];
                    make_panel(i, (u8)(pk[i][0] - 1));
                    break;
                }
        }
    }
    c = W->hud[0]; d = W->hud[1];                  // copied: the strip, 96 px
    for (r = 0; r < 8; r++, c += RT_PBYTES / 2, d += RT_PBYTES / 2) {
        a = (u32 *)((u8 *)rt_light + r * RT_PBYTES); b = (u32 *)((u8 *)rt_dark + r * RT_PBYTES);
        a[0] = ((u32 *)c)[0]; a[1] = ((u32 *)c)[1]; a[2] = ((u32 *)c)[2];
        b[0] = ((u32 *)d)[0]; b[1] = ((u32 *)d)[1]; b[2] = ((u32 *)d)[2];
    }
    if (k[0] && W->win_k[0] == k[0] && W->win_n >= WIN_DONE) {   // the window, against the
        u8 ww = W->win_w;                                          // right edge, once drawn
        c = W->win[0]; d = W->win[1];
        for (r = 0; r < W->win_h; r++, c += RT_PBYTES / 2, d += RT_PBYTES / 2) {
            u16 *pa = (u16 *)((u8 *)rt_light + r * RT_PBYTES) + RT_W / 16 - ww;
            u16 *pb = (u16 *)((u8 *)rt_dark + r * RT_PBYTES) + RT_W / 16 - ww;
            for (i = 0; i < ww; i++) { pa[i] = c[i]; pb[i] = d[i]; }
        }
        if (k[0] == M_MENU)                        // the pointer: a small black triangle
            for (r = 0; r < 5; r++) {
                u16 o2 = (u16)((3 + 6 * st.menu + r) * RT_PBYTES + RT_W / 8 - 2 * ww);
                u8 b2 = (u8)(r == 2 ? 0x38 : r == 1 || r == 3 ? 0x30 : 0x20);
                ((u8 *)rt_light)[o2] |= b2; ((u8 *)rt_dark)[o2] |= b2;
            }
    }
    for (i = 0; i < 2; i++) {                      // the panels, at the bottom corners
        const u16 *pl = W->pan[i][0], *pd = W->pan[i][1];
        u8 *ql = (u8 *)rt_light + (RT_H - PANEL_H) * RT_PBYTES + i * (RT_W / 8 - 2);
        u8 *qd = (u8 *)rt_dark + (RT_H - PANEL_H) * RT_PBYTES + i * (RT_W / 8 - 2);
        if (!pk[i][0] || pk[i][0] != W->pan_k[i][0]) continue;
        for (r = 0; r < PANEL_H; r++, ql += RT_PBYTES, qd += RT_PBYTES) {
            WR16(ql, 0, pl[r]); WR16(qd, 0, pd[r]);
        }
    }
}

// An attack's damage (or Miss) above its target from ACT_HIT, rising NUM_RISE pixels: AMS
// text written once into the strip's spare rows, then a 32-pixel sprite with a white outline
static void draw_number(void)
{
    Feet f = unit_feet(st.rot, st.tgt);
    u8 r, t = (u8)(st.act_t - ACT_HIT);
    RtSprite sp;
    if (!W->num_ok) {
        char s[6];
        u32 g[7], o = 0;
        u8 n = st.hit ? put_n(s, 0, st.dmg) : put_s(s, 0, "Miss");
        s[n] = 0;
        for (r = 8; r < HUD_ROWS; r++) memset(HUD_L + r * RT_PBYTES, 0, 4);
        text_into(W->hud[0], W->hud[1], 1, 8, s, C_BLACK);
        for (r = 0; r < 7; r++) {
            const u8 *b = HUD_L + (7 + r) * RT_PBYTES;
            g[r] = r && r < 6 ? (u32)b[0] << 24 | (u32)b[1] << 16 | (u32)b[2] << 8 | b[3] : 0;
            o |= g[r];
        }
        for (W->num_w = 32; W->num_w && !(o & (1UL << (32 - W->num_w))); W->num_w--) ;
        for (r = 0; r < 7; r++) {
            u32 m = g[r] | (r ? g[r - 1] : 0) | (r < 6 ? g[r + 1] : 0);
            W->num[0][r] = g[r];
            W->num[1][r] = ~(m | m << 1 | m >> 1);
        }
        W->num_ok = 1;
    }
    sp.w = 32; sp.h = 7; sp.light = sp.dark = W->num[0]; sp.mask = W->num[1];
    draw_sprite(f.x - st.camx - ((W->num_w + 2) >> 1), f.y - st.camy - UNIT_FOOT - 6 - (t < NUM_RISE ? t : NUM_RISE), &sp);
}

// the facing arrows around the active unit at Wait, the one it faces black
static void draw_arrows(void)
{
    static const s8 ax[4] = { 13, 13, -13, -13 }, ay[4] = { -6, 7, 7, -6 };
    static const s8 fdx[4] = { 1, -1, 0, 0 }, fdz[4] = { 0, 0, 1, -1 };
    const Unit *un = &st.unit[st.act];
    Feet f = unit_feet(st.rot, st.act);
    s16 u0, v0, du, dv;
    u8 g, r, sel, lm[8], mk[8];
    RtSprite sp;
    view_of(st.rot, 1, 1, &u0, &v0);
    view_of(st.rot, 1 + fdx[un->face], 1 + fdz[un->face], &du, &dv);
    du -= u0; dv -= v0;
    sel = dv < 0 ? 0 : du > 0 ? 1 : dv > 0 ? 2 : 3;
    sp.w = 8; sp.h = 8; sp.light = sp.dark = lm; sp.mask = mk;
    for (g = 0; g < 4; g++) {
        for (r = 0; r < 8; r++) {
            u8 fi = W->arrow[g][0][r], o = W->arrow[g][1][r];
            lm[r] = g == sel ? fi : (u8)(o & ~fi);
            mk[r] = (u8)~o;
        }
        draw_sprite(f.x - st.camx + ax[g] - 4, f.y - st.camy + ay[g] - 4, &sp);
    }
}

void game_render(void)
{
    u8 ord[NUNIT], i, j;
    Feet f[NUNIT];
    s16 u, v;
    if (!W || !ugfx || !views[0] || !views[1] || !views[2] || !views[3]) {
        draw_rect(0, 0, RT_W, RT_H, C_WHITE);
        draw_text(4, 40, W ? "missing fftu, fftv0-fftv3" : "not enough memory (75 KB)", F_SMALL, C_BLACK);
        return;
    }
    if (st.turn) {                                 // the view it turns to, unpacked meanwhile
        ZB(7); draw_turn(); ZE(7);
        ZB(8); unpack_view((st.rot + st.turn) & 3); ZE(8);
        return;
    }
    ZB(6);
    ensure_scene(st.rot);
    ZE(6);
    ZB(3); copy_view(); draw_glints(); ZE(3);
    ZB(4);
    for (i = 0; i < NUNIT; i++) {                  // units back to front
        f[i] = unit_feet(st.rot, i);
        for (j = i; j > 0 && f[ord[j - 1]].depth > f[i].depth; j--) ord[j] = ord[j - 1];
        ord[j] = i;
    }
    for (i = 0; i < NUNIT; i++) {
        u8 k = ord[i];
        s16 bx = f[k].x - st.camx - 8, by = f[k].y - st.camy - UNIT_FOOT;
        Cover *c = &W->cover[k];
        if (st.unit[k].x >= MAP_W) continue;
        if (bx <= -16 || bx >= RT_W || by <= -UNIT_SH || by - FOE_H >= RT_H) continue;   // off screen
        if (c->x != f[k].x || c->y != f[k].y || c->depth != f[k].depth || c->rot != st.rot) {
            c->x = f[k].x; c->y = f[k].y; c->depth = f[k].depth; c->rot = st.rot;
            cover(c->cm, f[k].x, f[k].y, f[k].depth);
            for (c->r0 = 2; c->r0 <= UNIT_SH - 3 && !c->cm[c->r0]; c->r0++) ;
            for (c->r1 = UNIT_SH - 3; c->r1 >= c->r0 && !c->cm[c->r1]; c->r1--) ;
        }
        draw_unit_covered(f[k].x - st.camx, f[k].y - st.camy, st.rot, k, c);
    }
    ZE(4); ZB(5);
    // the cursor, above everything (blinking)
    dst_screen(st.camx, st.camy);
    view_of(st.rot, st.cx, st.cz, &u, &v);
    if (st.mode == M_ACT && st.act_t >= ACT_HIT) draw_number();
    if (st.mode == M_FACE) draw_arrows();
    else if (st.mode != M_ACT && (rt_frame & 8 || RINGS(st.mode)))
        blit24(scene_x(st.rot, u, v) - HW, scene_y(u, v) - (map_tiles[st.cz * MAP_W + st.cx].stand >> 1) * HU,
               12, cur_l, cur_d, cur_m);
    draw_hud();
    ZE(5);
}
