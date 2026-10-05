// Alundra-style traversal: a player who walks in 8 directions and jumps, with constant gravity,
// over a world of tile heights (a test room, the village of Inoa) with stairs, a camera that
// follows, and a debug overlay (RE_NOTES.md § Behaviour, § Numbers).
#include "alundra.h"
#ifndef BAKE
#include "world.h"                      // the worlds drawn into 16 x 16 tiles (tools/bake.c, local)
#endif

#ifdef RT_CYCLES                        // render zones under ti-cycles (3 view copy, 4 shadow,
#include "../../tools/m68kbench/bench.h" // 5 hero, 6 tiles in front, 7 overlay)
#define ZB(n) BENCH_BEGIN(n)
#define ZE(n) BENCH_END(n)
#else
#define ZB(n)
#define ZE(n)
#endif

State st;
static u8 pop8[256];

// The test room (RE_NOTES.md § Decisions): open ground (0), a platform at 1 and one at 2,
// reachable ledges 0->1 and 1->2, an unreachable one 0->2 (row 3 -> row 2, columns 7-8), drops,
// a one-tile corridor (column 1, rows 3-4), a pillar at 2, walls and corners.
#define W_ WALL
static const u8 room_test[6][10] = {
    { W_, W_, W_, W_, W_, W_, W_, W_, W_, W_ },
    { W_, 0, 0, 0, 1, 1, 2, 2, 2, W_ },
    { W_, 0, 0, 0, 1, 1, 2, 2, 2, W_ },
    { W_, 0, W_, 0, 0, 1, 1, 0, 0, W_ },
    { W_, 0, W_, 0, 0, 0, 0, 0, 0, W_ },
    { W_, 0, 0, 0, W_, 0, 0, 2, 0, W_ },
};

// The village of Inoa (milestone 9, gfx.h): the game's height map resampled (a level = 16 px
// of the game), stairs, the exits and the doors walled; its image is the game's (the data
// files alvil0, alvil1: tools/extract.py)
#ifdef BAKE
#define TEST_IWB 20
#define TEST_IH 100
static const u8 test_img[2][2];
#endif
World worlds[] = {
    { room_test[0], 10, 6, ROOM_Y, { test_img[0], test_img[1] }, TEST_IWB, TEST_IH },
    { village, VILLAGE_W, VILLAGE_H, VILLAGE_TOP, { 0, 0 }, VILLAGE_IWB, VILLAGE_IH },
};
const World *world = &worlds[W_TEST];
s16 al_camx, al_camy;

u8 al_tile(s16 px, s16 py)
{
    if (px < 0 || py < 0) return WALL;
    px >>= 4;
    py >>= 4;
    if (px >= world->w || py >= world->h) return WALL;
    return world->cell[(u16)py * world->w + (u16)px];
}

// Alundra samples the floor at the four corners of the box and keeps the highest
s16 al_floor(u16 x, u16 y)
{
    s16 l = x / SUB - FOOT_W / 2, r = l + FOOT_W - 1, t = y / SUB - FOOT_D / 2, b = t + FOOT_D - 1;
    u8 h = LV(al_tile(l, t)), k;
    if ((k = LV(al_tile(r, t))) > h) h = k;
    if ((k = LV(al_tile(l, b))) > h) h = k;
    if ((k = LV(al_tile(r, b))) > h) h = k;
    return h * LEVEL_Z;
}

// Alundra blocks a corner on a wall or a floor above the feet (a ledge is a wall until the feet
// are higher than its top); a floor at the feet' level is walkable; on foot, with a stair under
// the box, one level up is walkable too (stairs). Bits: 1 left-back, 2 right-back, 4 left-front,
// 8 right-front (back = smaller y).
u8 al_block(u16 x, u16 y, s16 z)
{
    s16 l = x / SUB - FOOT_W / 2, r = l + FOOT_W - 1, t = y / SUB - FOOT_D / 2, b = t + FOOT_D - 1;
    u8 c[4], k, m = 0;
    c[0] = al_tile(l, t); c[1] = al_tile(r, t); c[2] = al_tile(l, b); c[3] = al_tile(r, b);
    if (st.grounded && ((c[0] | c[1] | c[2] | c[3]) & T_STAIR)) z += 2 * LEVEL_Z;
    for (k = 0; k < 4; k++)
        if ((c[k] & T_WALL) || LV(c[k]) * LEVEL_Z > z) m |= 1 << k;
    return m;
}

static u8 on_stair(void)                // a stair under the foot box or a step away (just left)
{
    s16 l = st.x / SUB - FOOT_W / 2 - SC(4), r = l + FOOT_W - 1 + 2 * SC(4);
    s16 t = st.y / SUB - FOOT_D / 2 - SC(4), b = t + FOOT_D - 1 + 2 * SC(4);
    return (al_tile(l, t) | al_tile(r, t) | al_tile(l, b) | al_tile(r, b)) & T_STAIR;
}

void al_world(u8 n)
{
    if (n == W_VILLAGE && !worlds[n].img[0]) {     // the image read in place (archived on the TI)
        worlds[n].img[0] = rt_file("alvil0", RT_NULL);
        worlds[n].img[1] = rt_file("alvil1", RT_NULL);
        if (!worlds[n].img[0] || !worlds[n].img[1]) {
            worlds[n].img[0] = RT_NULL;
            n = W_TEST;
        }
    }
    world = &worlds[n];
    if (n == W_TEST) {
        st.x = 24 * SUB;                // open ground, column 1 row 2 (nothing tall in front)
        st.y = 40 * SUB;
    } else {
        st.x = VILLAGE_SX * SUB;        // in front of the house Alundra leaves (inoa.state)
        st.y = VILLAGE_SY * SUB;
    }
    st.z = al_floor(st.x, st.y);
    st.vz = 0;
    st.grounded = 1;
    st.a_held = 0;
    st.steps = 0;
    st.dir = 0;
    st.anim = 0;
    st.idle = 0;
}

void al_start(void) { al_world(W_TEST); }

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
// is the highest tile under the foot box; walking off a ledge starts a fall). On stairs the
// feet follow the floor one level up or down, no fall (Alundra's ramps hold the player)
static void vertical(void)
{
    s16 floor = al_floor(st.x, st.y);
    if (st.grounded) {
        if (floor == st.z) return;
        if (on_stair() && (floor > st.z || st.z - floor <= 2 * LEVEL_Z)) { st.z = floor; return; }
        if (floor > st.z) return;
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
static void make_textures(void);

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
    al_start();
}

// 0 the room's start (open ground); 1 in the air at the top of a jump; 2 in front of the
// 0->1 ledge; 3 on the level-2 platform at its front edge (the drop); 4 in the corridor;
// 5 below the unreachable 0->2 ledge; 6 above the corridor, misaligned (a corner to round);
// the village of Inoa (gfx.h): 7 in front of the house Alundra leaves
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
    if (n == 7) al_world(W_VILLAGE);
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

// ---------------------------------------------------------------- drawing
// Textures from the disc's village (gfx.h: 1 = line, 0 = base) made into opaque 16-wide
// sprites with two greys each, once: tops per level (three, by level % 3: neighbouring levels
// always differ), the front face (32 rows, cut to the band height), the walls' top and front
// (the test room; the village is the game's own image)
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
    make_tex(T_TOP2, tex_floor, C_WHITE, C_LGRAY, 1);   // framed
    make_tex(T_FACE, tex_face, C_DGRAY, C_BLACK, 0);
    make_tex(T_WALLTOP, tex_walltop, C_DGRAY, C_BLACK, 0);
    make_tex(T_WALLFACE, tex_wallface, C_BLACK, C_DGRAY, 0);

}

static void band(u8 t, s16 x, s16 y, s16 h)   // h rows of a texture, 32 at a time
{
    RtSprite s = tspr[t];
    for (; h > 0; h -= 32, y += 32) {
        s.h = h > 32 ? 32 : h;
        draw_sprite(x, y, &s);
    }
}

// A tile: its top face raised by its height, the front face down to the tile in front.
// (ox, oy): the screen position of tile (0, 0)'s ground

static void draw_tile(u8 tx, u8 ty, s16 ox, s16 oy)
{
    const u8 *c = world->cell + (u16)ty * world->w + tx;
    u8 t = *c, h = LV(t);
    u8 f = ty + 1 < world->h ? c[world->w] : 0, hf = LV(f);
    s16 x = ox + tx * TILE, top = oy + ty * TILE - LEVEL_PX(h), face = LEVEL_PX(h) - LEVEL_PX(hf);
    if (t & T_WALL) {
        draw_sprite(x, top, &tspr[T_WALLTOP]);
        if (hf < h) band(T_WALLFACE, x, top + TILE, face);
        return;
    }
    draw_sprite(x, top, &tspr[T_TOP0 + h % 3]);
    if (hf < h && !(f & T_WALL)) {
        band(T_FACE, x, top + TILE, face);
        draw_rect(x, top + TILE + face - 1, TILE, 1, C_BLACK);
    }
}

void al_draw_world(s16 ox, s16 oy)      // every tile, back to front (tools/bake.c)
{
    u8 tx, ty;
    for (ty = 0; ty < world->h; ty++)
        for (tx = 0; tx < world->w; tx++) draw_tile(tx, ty, ox, oy);
}

#ifndef BAKE
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
    if (x < 0 || y < 0) return;                     // (never in a room: walls all round)
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

static s16 shadow_y(s16 oy)             // screen row of the shadow's centre: the floor under the feet
{
    return oy + st.y / SUB - al_floor(st.x, st.y) / SUB - SHADOW_DY;
}

static void draw_shadow(s16 ox, s16 oy)
{
    s16 px = ox + st.x / SUB, sy = shadow_y(oy);
    if (st.z - al_floor(st.x, st.y) < LEVEL_Z)
        darken(px - SHADOW0_W / 2, sy - SHADOW0_H / 2, shadow0, SHADOW0_H, SHADOW0_N);
    else
        darken(px - SHADOW1_W / 2, sy - SHADOW1_H / 2, shadow1, SHADOW1_H, SHADOW1_N);
}

static void draw_hero(s16 ox, s16 oy)
{
    s16 px = ox + st.x / SUB, py = oy + st.y / SUB, pz = st.z / SUB;
    const hero_row (*g)[HERO_SH] = hero_gfx[st.dir][al_frame()];
    RtSprite s;
    s.w = HERO_SW;
    s.h = HERO_SH;
    s.light = g[0];
    s.dark = g[1];
    s.mask = g[2];
    draw_sprite(px + hero_ox[st.dir][al_frame()], py - pz - HERO_AY, &s);
}

static void num(char *p, u16 v)        // 3 digits, no sprintf on the TI
{
    p[0] = '0' + v / 100 % 10;
    p[1] = '0' + v / 10 % 10;
    p[2] = '0' + v % 10;
}

static void digit(u8 t, s16 x, s16 top)  // the overlay's level on a top face
{
    static char d[2] = "0";
    u8 h = LV(t);
    d[0] = h < 10 ? '0' + h : 'A' - 10 + h;
    draw_rect(x + 5, top + 4, 5, 7, C_WHITE);
    draw_text(x + 6, top + 5, d, F_SMALL, C_BLACK);
}

// The view: 100 rows of 160 px from the world's image at (cx, cy), shifted to any pixel: per
// row, ten words, each from the two image words under it (even addresses: iwb is even), by the
// shorter shift (left by sh then the high word, or right by 16 - sh: 8 bits at most)
#ifdef __m68k__
#define W10(op) op op op op op op op op op op
static void blit_plane(u8 *dst, const u8 *src, u16 iwb, s16 cx, s16 cy)
{
    const u8 *row = src + (u16)cy * iwb + (((u16)cx >> 4) << 1);
    u16 sh = cx & 15, rs = 16 - sh;
    u8 r;
    for (r = 0; r < RT_H; r++, row += iwb, dst += RT_PBYTES) {
        const u16 *s = (const u16 *)row;
        u16 *d = (u16 *)dst;
        u32 acc;
        if (!sh) { W10(*d++ = *s++;) }
        else if (sh <= 8) { acc = *s++; W10(acc = (acc << 16) | *s++; *d++ = (u16)((acc << sh) >> 16);) }
        else { acc = *s++; W10(acc = (acc << 16) | *s++; *d++ = (u16)(acc >> rs);) }
    }
}
#else
static void blit_plane(u8 *dst, const u8 *src, u16 iwb, s16 cx, s16 cy)
{
    const u8 *row = src + (u16)cy * iwb + (((u16)cx >> 4) << 1);
    u8 sh = cx & 15, r, k;
    for (r = 0; r < RT_H; r++, row += iwb, dst += RT_PBYTES) {
        const u8 *s = row;
        u8 *d = dst;
        u32 acc = (u16)(s[0] << 8 | s[1]);
        for (k = 0; k < 10; k++, d += 2) {
            s += 2;
            acc = (acc << 16) | (u16)(s[0] << 8 | s[1]);
            d[0] = (u8)(acc >> (24 - sh));
            d[1] = (u8)(acc >> (16 - sh));
        }
    }
}
#endif

// The image's pixels back over a screen rectangle [x0, x1) x [y0, y1) (clipped): what stands in
// front of the player hides him
static void restore(s16 x0, s16 y0, s16 x1, s16 y1)
{
    u8 p;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > RT_W) x1 = RT_W;
    if (y1 > RT_H) y1 = RT_H;
    if (x0 >= x1 || y0 >= y1) return;
    for (p = 0; p < 2; p++) {
        u8 *plane = p ? (u8 *)rt_dark : (u8 *)rt_light;
        const u8 *img = world->img[p];
        s16 y, b;
        for (y = y0; y < y1; y++) {
            const u8 *row = img + (u16)(y + al_camy) * world->iwb;
            u8 *d = plane + y * RT_PBYTES;
            for (b = x0 >> 3; b <= (x1 - 1) >> 3; b++) {
                s16 lo = b << 3 < x0 ? x0 - (b << 3) : 0, hi = (b << 3) + 8 > x1 ? x1 - (b << 3) : 8;
                u8 m = (u8)(0xff >> lo) & (u8)(0xff << (8 - hi));
                u16 X = (u16)((b << 3) + al_camx);
                const u8 *s = row + (X >> 3);
                u8 sh = X & 7, v = sh ? (u8)(s[0] << sh | s[1] >> (8 - sh)) : s[0];
                d[b] = (d[b] & ~m) | (v & m);
            }
        }
    }
}

// The world's image is the background (the test room's drawn from its tiles, the village's the
// game's own): the view copied from it, then the overlay's levels, the shadow, the player, and
// the image again over the player where a tile of a row in front of his covers him (rows drawn
// back to front: tiles in front hide him, Alundra). The camera keeps his feet at (80, 62),
// inside the image.
void game_render(void)
{
    s16 px = st.x / SUB, py = st.y / SUB, pz = st.z / SUB;
    s16 cx = px - RT_W / 2, cy = world->top + py - pz - 62;
    s16 mx = (s16)(world->iwb << 3) - RT_W, my = (s16)world->ih - RT_H;
    s16 ox, oy, top, bot, sb, left, right;
    u8 tx, ty, t0, t1;
    if (cx > mx) cx = mx;
    if (cx < 0) cx = 0;
    if (cy > my) cy = my;
    if (cy < 0) cy = 0;
    al_camx = cx;
    al_camy = cy;
    ox = -cx;
    oy = world->top - cy;
    top = oy + py - pz - HERO_AY;                   // the player's sprite and shadow rectangle
    bot = top + HERO_SH;
    sb = shadow_y(oy) + SHADOW0_H / 2;
    if (sb > bot) bot = sb;
    left = ox + px + hero_ox[st.dir][al_frame()];
    right = left + HERO_SW - 1;
    ZB(3);
    blit_plane(rt_light, world->img[0], world->iwb, cx, cy);
    blit_plane(rt_dark, world->img[1], world->iwb, cx, cy);
    if (st.debug) {                                 // the overlay's levels, visible floor tiles
        s16 r0 = (cy - world->top) >> 4, r1 = r0 + 7 + LEVEL_PX(15) / 16;
        if (r0 < 0) r0 = 0;
        if (r1 > world->h - 1) r1 = world->h - 1;
        for (ty = (u8)r0; ty <= (u8)r1; ty++)
            for (tx = (u8)(cx >> 4); tx < world->w && tx <= (u8)((cx + RT_W - 1) >> 4); tx++) {
                const u8 *c = world->cell + (u16)ty * world->w + tx;
                s16 y = oy + ty * TILE - LEVEL_PX(LV(*c));
                u8 k, hidden = 0;
                for (k = 1; k <= 2 && ty + k < world->h; k++)   // a tile in front over the digit
                    hidden |= oy + (ty + k) * TILE - LEVEL_PX(LV(c[k * world->w])) < y + 11;
                if (!(*c & T_WALL) && !hidden) digit(*c, ox + tx * TILE, y);
            }
    }
    ZE(3);
    ZB(4); draw_shadow(ox, oy); ZE(4);
    ZB(5); draw_hero(ox, oy); ZE(5);
    ZB(6);
    t0 = left - ox < 0 ? 0 : (u8)((left - ox) >> 4);   // the sprite's columns
    t1 = (u8)((right - ox) >> 4);
    for (tx = t0; tx <= t1 && tx < world->w; tx++) {   // per column: the tiles of the rows in
        s16 x = ox + tx * TILE, y0 = RT_H, y1 = 0;      // front cover one span (each row's top
        for (ty = (u8)(py >> 4) + 1; ty < world->h; ty++) {   // reaches the row before)
            s16 g = oy + ty * TILE, ttop = g - LEVEL_PX(LV(world->cell[(u16)ty * world->w + tx]));
            if (ttop < bot && g + TILE > top) {
                if (ttop < y0) y0 = ttop;
                y1 = g + TILE;
            }
        }
        if (y0 < y1)
            restore(x < left ? left : x, y0 < top ? top : y0,
                    x + TILE > right + 1 ? right + 1 : x + TILE, y1 < bot ? y1 : bot);
    }
    ZE(6);
    ZB(7);
    if (st.debug) {
        static char line[] = "x000 y000 z000 g";
        num(line + 1, px);
        num(line + 6, py);
        num(line + 11, pz);
        line[15] = st.grounded ? 'G' : 'A';
        draw_rect(0, 0, RT_W, 7, C_WHITE);
        draw_text(1, 1, line, F_SMALL, C_BLACK);
    }
    ZE(7);
}
#else
void game_render(void) { }
#endif
