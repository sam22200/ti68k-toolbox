// Alundra-style traversal: a rectangle that walks in 8 directions and jumps, with constant
// gravity, over a room of tile heights, and a debug overlay (RE_NOTES.md § Behaviour, § Numbers).
#include "alundra.h"
#include "gfx.h"                       // the disc's art (tools/extract.py, local)
#ifdef RT_CYCLES                        // render zones under ti-cycles (3 copy, 4 shadow, 5 hero,
#include "../../tools/m68kbench/bench.h" // 6 tiles in front, 7 overlay)
#define ZB(n) BENCH_BEGIN(n)
#define ZE(n) BENCH_END(n)
#else
#define ZB(n)
#define ZE(n)
#endif

State st;
static void make_textures(void);
static u8 pop8[256];
extern u8 bg_debug;

// The test room (RE_NOTES.md § Decisions): open ground (0), a platform at 1 and one at 2,
// reachable ledges 0->1 and 1->2, an unreachable one 0->2 (row 3 -> row 2, columns 7-8), drops,
// a one-tile corridor (column 1, rows 3-4), a pillar at 2, walls and corners.
const u8 room[MAP_H][MAP_W] = {
    { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 3, 0, 0, 0, 1, 1, 2, 2, 2, 3 },
    { 3, 0, 0, 0, 1, 1, 2, 2, 2, 3 },
    { 3, 0, 3, 0, 0, 1, 1, 0, 0, 3 },
    { 3, 0, 3, 0, 0, 0, 0, 0, 0, 3 },
    { 3, 0, 0, 0, 3, 0, 0, 2, 0, 3 },
};

u8 al_tile(s16 px, s16 py)
{
    if (px < 0 || py < 0 || px >= ROOM_W || py >= ROOM_H) return WALL;
    return room[(u16)py >> 4][(u16)px >> 4];
}

// Alundra samples the floor at the four corners of the box and keeps the highest
s16 al_floor(u16 x, u16 y)
{
    s16 l = x / SUB - FOOT_W / 2, r = l + FOOT_W - 1, t = y / SUB - FOOT_D / 2, b = t + FOOT_D - 1;
    u8 h = al_tile(l, t), k;
    if ((k = al_tile(r, t)) > h) h = k;
    if ((k = al_tile(l, b)) > h) h = k;
    if ((k = al_tile(r, b)) > h) h = k;
    return h * LEVEL * SUB;
}

// Alundra blocks a corner on a wall or a floor above the feet (a ledge is a wall until the feet
// are higher than its top); a floor at the feet' level is walkable. Bits: 1 left-back,
// 2 right-back, 4 left-front, 8 right-front (back = smaller y).
u8 al_block(u16 x, u16 y, s16 z)
{
    s16 l = x / SUB - FOOT_W / 2, r = l + FOOT_W - 1, t = y / SUB - FOOT_D / 2, b = t + FOOT_D - 1;
    u8 c[4], k, m = 0;
    c[0] = al_tile(l, t); c[1] = al_tile(r, t); c[2] = al_tile(l, b); c[3] = al_tile(r, b);
    for (k = 0; k < 4; k++)
        if (c[k] == WALL || c[k] * LEVEL * SUB > z) m |= 1 << k;
    return m;
}

void al_start(void)
{
    st.x = 24 * SUB;                    // open ground, column 1 row 2 (nothing tall in front)
    st.y = 40 * SUB;
    st.z = 0;
    st.vz = 0;
    st.grounded = 1;
    st.a_held = 0;
    st.steps = 0;
    st.dir = 0;
    st.anim = 0;
    st.idle = 0;
}

// One axis of a move, pixel by pixel when blocked, so the box stops flush against the edge
// (Alundra halves the step); returns 0 when it could not move at all
static u8 axis(u16 *v, s16 d)
{
    u16 old = *v;
    *v = old + d;
    if (al_free(st.x, st.y, st.z)) return 1;
    *v = old;
    while (d) {                         // the remaining whole pixels towards the wall
        s16 s = d > 0 ? SUB : -SUB;
        if ((d > 0 ? d : -d) < SUB) s = d;
        *v += s;
        if (!al_free(st.x, st.y, st.z)) { *v -= s; break; }
        d -= s;
    }
    return *v != old;
}

// A straight move that cannot progress, with one front corner blocked and the other free, is
// turned into a sideways nudge away from the blocked corner (Alundra rounds corners this way,
// at ~1/3 of the walk speed): the player slips past a corner or into a narrow passage.
// f: the front corners' bits, lo: the bit of the corner on the low side of the other axis
static void nudge(u16 *v, s16 d, u16 *w, s16 n, u8 f, u8 lo)
{
    u8 m;
    if (axis(v, d)) return;
    *v += d;
    m = al_block(st.x, st.y, st.z) & f;
    *v -= d;
    if (m == lo) axis(w, n);
    else if (m == (f ^ lo)) axis(w, -n);
}

// Planar move (Alundra: constant speed, full control in the air, no inertia); a diagonal goes
// x then y, so a blocked one slides along the wall (Alundra drops the blocked axis)
static void move(u32 keys)
{
    s16 dx = 0, dy = 0;
    if (keys & K_LEFT) dx = -1;
    if (keys & K_RIGHT) dx += 1;
    if (keys & K_UP) dy = -1;
    if (keys & K_DOWN) dy += 1;
    if (dx && dy) {
        axis(&st.x, dx * DIAG_X);
        axis(&st.y, dy * DIAG_Y);
    } else if (dx)
        nudge(&st.x, dx * SPEED_X, &st.y, NUDGE_Y, dx > 0 ? 10 : 5, dx > 0 ? 2 : 1);
    else if (dy)
        nudge(&st.y, dy * SPEED_Y, &st.x, NUDGE_X, dy > 0 ? 12 : 3, dy > 0 ? 4 : 1);
}

// Vertical step (Alundra: z += vz, vz -= gravity; land when the arc reaches the floor, which
// is the highest tile under the foot box; walking off a ledge starts a fall)
static void vertical(void)
{
    s16 floor = al_floor(st.x, st.y);
    if (st.grounded) {
        if (floor >= st.z) return;
        st.grounded = 0;
        st.vz = 0;
    }
    st.z += st.vz;
    st.vz -= GRAVITY;
    if (st.vz < -FALL_MAX) st.vz = -FALL_MAX;
    if (st.z <= floor) {
        st.z = floor;
        st.vz = 0;
        st.grounded = 1;
    }
}

void al_step(u32 keys)
{
    if (st.grounded && (keys & K_A) && !st.a_held) {
        st.grounded = 0;
        st.vz = JUMP_VZ;
    }
    st.a_held = (keys & K_A) != 0;
    // facing (Alundra: up or down wins on a diagonal) and the walk cycle
    if (keys & (K_UP | K_DOWN | K_LEFT | K_RIGHT)) {
        st.dir = keys & K_UP ? 1 : keys & K_DOWN ? 0 : keys & K_LEFT ? 2 : 3;
        st.anim++;
        st.idle = 0;
    } else {
        st.anim = 0;
        if (++st.idle == IDLE_STAND + IDLE_IN + IDLE_OUT) st.idle = 0;
    }
    move(keys);
    vertical();
    st.steps++;
}

// Alundra: a walk image every 10 frames at 60 Hz (5 steps here), six images; the take-off
// image while rising fast, then the jump image until landing (a fall uses it too); standing,
// Alundra breathes: the stand image 40 frames, then in 10 and out 4 (21, 5, 2 steps here)
u8 al_frame(void)
{
    if (!st.grounded) return st.vz > JUMP_VZ - 2 * GRAVITY ? 7 : 8;
    if (!st.anim)
        return st.idle < IDLE_STAND ? 0 : st.idle < IDLE_STAND + IDLE_IN ? 9 : 10;
    return 1 + (u8)((st.anim - 1) / 5) % 6;
}

u32 al_hash(void)
{
    const u16 v[] = { st.x, st.y, (u16)st.z, (u16)st.vz, st.grounded, st.a_held, st.steps, st.dir, st.anim, st.idle };
    u32 h = 0;
    u8 k;
    for (k = 0; k < sizeof v / sizeof v[0]; k++) h = ((h << 5) | (h >> 27)) ^ v[k];
    return h;
}

// ---------------------------------------------------------------- runtime hooks
void game_init(void)
{
    st.debug = 1;
    rt_state = &st;
    rt_state_size = sizeof(st);
    make_textures();
    {
        u16 i;
        for (i = 1; i < 256; i++) pop8[i] = (u8)((i & 1) + pop8[i >> 1]);
    }
    bg_debug = 0xff;                    // compose again, every plane copied in full once
    al_start();
}

// 0 the room's start (open ground); 1 in the air at the top of a jump; 2 in front of the
// 0->1 ledge; 3 on the level-2 platform at its front edge (the drop); 4 in the corridor;
// 5 below the unreachable 0->2 ledge; 6 above the corridor, misaligned (a corner to round)
static void place(u8 px, u8 py)
{
    st.x = px * SUB;
    st.y = py * SUB;
    st.z = al_floor(st.x, st.y);
}

void game_scenario(u16 n)
{
    al_start();
    if (n == 1) { st.grounded = 0; st.z = 13 * SUB; st.vz = 0; }
    if (n == 2) place(56, 24);
    if (n == 3) place(120, 40);
    if (n == 4) place(24, 60);
    if (n == 5) place(128, 56);
    if (n == 6) place(30, 40);
}

u8 game_update(void)
{
    if (input_held(K_ESC)) return 0;
    if (input_pressed(K_C)) st.debug ^= 1;
    al_step(rt_keys);
#ifdef STATE_HASH
    BENCH_VALUE(al_hash());
#endif
    return 1;
}

// ---------------------------------------------------------------- drawing (debug rendering)
static void num(char *p, u16 v)        // 3 digits, no sprintf on the TI
{
    p[0] = '0' + v / 100 % 10;
    p[1] = '0' + v / 10 % 10;
    p[2] = '0' + v % 10;
}

// Textures from the disc's deck (gfx.h: 1 = line, 0 = base) made into opaque 16-wide sprites
// with two greys each, once: tops per level, the front face (32 rows, cut to the band height),
// the walls' top and front
enum { T_TOP0, T_TOP1, T_TOP2, T_FACE, T_WALLTOP, T_WALLFACE, T_N };
static u16 tex[T_N][2][32];             // light, dark rows
static RtSprite tspr[T_N];

static void make_tex(u8 t, const u8 (*src)[2], u8 base, u8 line, u8 frame)
{
    u8 r, c;
    for (r = 0; r < 32; r++) {
        u16 bits = (u16)src[r & 15][0] << 8 | src[r & 15][1], l = 0, d = 0;
        for (c = 0; c < 16; c++) {
            u16 b = 0x8000 >> c;
            u8 g = bits & b ? line : base;
            if (frame && (r == 0 || r == 15 || c == 0 || c == 15)) g = line;
            if (g & 1) l |= b;
            if (g & 2) d |= b;
        }
        tex[t][0][r] = l;
        tex[t][1][r] = d;
    }
    tspr[t].w = 16;
    tspr[t].h = 16;
    tspr[t].light = tex[t][0];
    tspr[t].dark = tex[t][1];
    tspr[t].mask = RT_NULL;
}

static void make_textures(void)
{
    make_tex(T_TOP0, tex_floor, C_LGRAY, C_DGRAY, 0);
    make_tex(T_TOP1, tex_floor, C_WHITE, C_LGRAY, 0);
    make_tex(T_TOP2, tex_floor, C_WHITE, C_LGRAY, 1);   // framed: the highest level
    make_tex(T_FACE, tex_face, C_DGRAY, C_BLACK, 0);
    make_tex(T_WALLTOP, tex_walltop, C_DGRAY, C_BLACK, 0);
    make_tex(T_WALLFACE, tex_wallface, C_BLACK, C_DGRAY, 0);
}

static void band(u8 t, s16 x, s16 y, u8 h)    // the first h rows of a texture
{
    RtSprite s = tspr[t];
    s.h = h;
    draw_sprite(x, y, &s);
}

// A tile: its top face raised by its height, the front face down to the tile in front
static void draw_tile(u8 tx, u8 ty)
{
    u8 h = room[ty][tx];
    u8 hf = ty + 1 < MAP_H ? room[ty + 1][tx] : 0;
    s16 x = tx * TILE, top = ROOM_Y + ty * TILE - h * LEVEL;
    if (h == WALL) {
        draw_sprite(x, top, &tspr[T_WALLTOP]);
        if (hf < h) band(T_WALLFACE, x, top + TILE, (h - hf) * LEVEL);
        return;
    }
    draw_sprite(x, top, &tspr[T_TOP0 + h]);
    if (hf < h && hf != WALL) {
        band(T_FACE, x, top + TILE, (h - hf) * LEVEL);
        draw_rect(x, top + TILE + (h - hf) * LEVEL - 1, TILE, 1, C_BLACK);
    }
    if (st.debug) {
        static char d[2] = "0";
        d[0] = '0' + h;
        draw_rect(x + 5, top + 4, 5, 7, C_WHITE);
        draw_text(x + 6, top + 5, d, F_SMALL, C_BLACK);
    }
}

// The shadow (Alundra: a semi-transparent ellipse, always under the player, on the floor during
// a jump, a little smaller high up), made to stand out on any floor: the greys under its mask
// are measured first; on a light floor (mean under 1.5) every pixel goes two greys darker (white
// -> dark grey, light grey -> black: the floor's texture still shows), on a dark floor it is a
// black ellipse ringed with light grey. Both planes in one pass.
// pop8: bits set per byte (filled at start)
#define POP8(v) pop8[(u8)(v)]

static void darken(s16 x, s16 y, const u16 *rows, u8 h, u8 n)   // n: pixels in the mask
{
    u8 sh = x & 7, r;
    u16 sum = 0, off = (u16)y * RT_PBYTES + ((u16)x >> 3);
    u8 *l, *d;
    for (r = 0, l = (u8 *)rt_light + off, d = (u8 *)rt_dark + off; r < h;
         r++, l += RT_PBYTES, d += RT_PBYTES) {      // measure: the greys under the mask
        u16 v = rows[r];
        u8 a = (u8)((v >> 8) >> sh), b = (u8)(v >> sh), c = (u8)(v << (8 - sh));
        sum += POP8(l[0] & a) + POP8(l[1] & b) + POP8(l[2] & c);
        sum += 2 * (POP8(d[0] & a) + POP8(d[1] & b) + POP8(d[2] & c));
    }
    if (sum + sum >= (u16)(n * 3)) {                // dark floor: black inside, light grey ring
        for (r = 0, l = (u8 *)rt_light + off, d = (u8 *)rt_dark + off; r < h;
             r++, l += RT_PBYTES, d += RT_PBYTES) {
            u16 v = rows[r], up = r ? rows[r - 1] : 0, dn = r + 1 < h ? rows[r + 1] : 0;
            u16 e = v & ~(up & dn & (v << 1) & (v >> 1));
            u8 a = (u8)((v >> 8) >> sh), b = (u8)(v >> sh), c = (u8)(v << (8 - sh));
            u8 ea = (u8)((e >> 8) >> sh), eb = (u8)(e >> sh), ec = (u8)(e << (8 - sh));
            l[0] |= a; l[1] |= b; l[2] |= c;
            d[0] = (d[0] & ~a) | (a & ~ea);
            d[1] = (d[1] & ~b) | (b & ~eb);
            d[2] = (d[2] & ~c) | (c & ~ec);
        }
    } else                                          // light floor: two greys darker
        for (r = 0, l = (u8 *)rt_light + off, d = (u8 *)rt_dark + off; r < h;
             r++, l += RT_PBYTES, d += RT_PBYTES) {
            u16 v = rows[r];
            u8 a = (u8)((v >> 8) >> sh), b = (u8)(v >> sh), c = (u8)(v << (8 - sh));
            l[0] |= d[0] & a; l[1] |= d[1] & b; l[2] |= d[2] & c;
            d[0] |= a; d[1] |= b; d[2] |= c;
        }
}

static s16 shadow_y(void)               // screen row of the shadow's centre: the floor under the feet
{
    return ROOM_Y + st.y / SUB - al_floor(st.x, st.y) / SUB - SHADOW_DY;
}

static void draw_shadow(void)
{
    s16 px = st.x / SUB, sy = shadow_y();
    if (st.z - al_floor(st.x, st.y) < LEVEL * SUB)
        darken(px - SHADOW0_W / 2, sy - SHADOW0_H / 2, shadow0, SHADOW0_H, SHADOW0_N);
    else
        darken(px - SHADOW1_W / 2, sy - SHADOW1_H / 2, shadow1, SHADOW1_H, SHADOW1_N);
}

static void draw_hero(void)
{
    s16 px = st.x / SUB, py = ROOM_Y + st.y / SUB, pz = st.z / SUB;
    const u16 (*g)[HERO_SH] = hero_gfx[st.dir][al_frame()];
    RtSprite s;
    s.w = HERO_SW;
    s.h = HERO_SH;
    s.light = g[0];
    s.dark = g[1];
    s.mask = g[2];
    draw_sprite(px + hero_ox[st.dir][al_frame()], py - pz - HERO_AY, &s);
}

static u32 bgw[2][RT_PSIZE / 4];        // light, dark: the room drawn once (u32: even
#define bg ((u8 (*)[RT_PSIZE])bgw)     // addresses for the long copies, as games/desolate)
u8 bg_debug = 0xff;                     // the overlay state bg was composed with

static void compose(void)
{
    void *l = rt_light, *d = rt_dark;
    u8 tx, ty;
    rt_light = bg[0]; rt_dark = bg[1];
    draw_clear();
    for (ty = 0; ty < MAP_H; ty++)
        for (tx = 0; tx < MAP_W; tx++) draw_tile(tx, ty);
    rt_light = l; rt_dark = d;
    bg_debug = st.debug;
}

static void copy_bg(void)               // the 100 visible rows of both planes
{
    u8 i;
    for (i = 0; i < 2; i++) {
        const u32 *s = bgw[i];
        u32 *t = (u32 *)(i ? rt_dark : rt_light);
        u8 n;
        for (n = 0; n < 100 * RT_PBYTES / 4 / 25; n++) {
            t[0] = s[0]; t[1] = s[1]; t[2] = s[2]; t[3] = s[3]; t[4] = s[4];
            t[5] = s[5]; t[6] = s[6]; t[7] = s[7]; t[8] = s[8]; t[9] = s[9];
            t[10] = s[10]; t[11] = s[11]; t[12] = s[12]; t[13] = s[13]; t[14] = s[14];
            t[15] = s[15]; t[16] = s[16]; t[17] = s[17]; t[18] = s[18]; t[19] = s[19];
            t[20] = s[20]; t[21] = s[21]; t[22] = s[22]; t[23] = s[23]; t[24] = s[24];
            s += 25; t += 25;
        }
    }
}

// The rectangle the player and the shadow covered in each hidden plane (two alternate on the TI):
// the next frame drawn into that plane restores only it from the background, not the whole room
typedef struct { const void *plane; u8 bx0, bx1, y0, y1; } Dirty;
static Dirty dirty[2];
static u8 dirty_next;
u8 al_full_copy;                        // tests: copy the whole background every frame

static void restore(const Dirty *k)
{
    u16 o = (u16)k->y0 * RT_PBYTES + k->bx0;
    const u8 *s0 = bg[0] + o, *s1 = bg[1] + o;
    u8 *l = (u8 *)rt_light + o, *d = (u8 *)rt_dark + o;
    u8 y = k->y1 - k->y0, n = k->bx1 - k->bx0 + 1, b;
    for (; y; y--, s0 += RT_PBYTES, s1 += RT_PBYTES, l += RT_PBYTES, d += RT_PBYTES)
        for (b = 0; b < n; b++) {
            l[b] = s0[b];
            d[b] = s1[b];
        }
}

// The room is static: copy it, draw the player, then redraw only the tiles in front of the
// player's row that overlap it (rows drawn back to front: tiles in front hide it, Alundra)
void game_render(void)
{
    s16 px = st.x / SUB, py = st.y / SUB, pz = st.z / SUB;
    s16 top = ROOM_Y + py - pz - HERO_AY;           // the player's sprite and shadow rectangle
    s16 bot = top + HERO_SH, sb = shadow_y() + SHADOW0_H / 2;
    u8 tx, ty, t0, t1;
    if (sb > bot) bot = sb;
    s16 left = px + hero_ox[st.dir][al_frame()], right = left + HERO_SW - 1;
    Dirty *k = 0;
    if (bg_debug != st.debug) {
        compose();
        dirty[0].plane = dirty[1].plane = 0;
    }
    ZB(3);
    if (!al_full_copy) {
        if (dirty[0].plane == rt_light) k = &dirty[0];
        else if (dirty[1].plane == rt_light) k = &dirty[1];
    }
    if (k) restore(k);
    else {
        copy_bg();
        k = &dirty[dirty_next];
        dirty_next ^= 1;
        k->plane = rt_light;
    }
    if (px - 7 < left) left = px - 7;               // the shadow's columns
    if (px + 7 > right) right = px + 7;
    k->bx0 = (u8)(left >> 3);
    k->bx1 = (u8)(right >> 3);
    k->y0 = (u8)(top < 0 ? 0 : top);
    k->y1 = (u8)(bot > 100 ? 100 : bot);
    ZE(3);
    ZB(4); draw_shadow(); ZE(4);
    ZB(5); draw_hero(); ZE(5);
    ZB(6);
    t0 = (u8)((px + hero_ox[st.dir][al_frame()]) >> 4);
    t1 = (u8)((px + hero_ox[st.dir][al_frame()] + HERO_SW - 1) >> 4);
    for (ty = (u8)(py >> 4) + 1; ty < MAP_H; ty++)
        for (tx = t0; tx <= t1 && tx < MAP_W; tx++) {
            u8 h = room[ty][tx];
            s16 ttop = ROOM_Y + ty * TILE - h * LEVEL;
            if (ttop < bot && ttop + TILE + h * LEVEL > top) draw_tile(tx, ty);
        }
    ZE(6);
    ZB(7);
    if (st.debug) {
        static char line[] = "x000 y000 z000 g";
        num(line + 1, px);
        num(line + 6, py);
        num(line + 11, pz);
        line[15] = st.grounded ? 'G' : 'A';
        draw_rect(0, 0, ROOM_W, 7, C_WHITE);
        draw_text(1, 1, line, F_SMALL, C_BLACK);
    }
    ZE(7);
}
