// Metal Gear Solid (GBC), milestone 1: VR Training, Sneaking, Practice Lv.01, on the Portable
// Game Runtime. Our own engine, not a translation: every number below was measured on the ROM
// under PyBoy (README.md § Measured, tools/extract.py). Logic runs per GB frame (59.7 Hz), two
// steps per TI frame (30.1 fps), so the measured timings hold as they are.
#include "mgs.h"
#include "level.h"
#include "gfx.h"
#ifdef STATE_HASH
#include "../../tools/m68kbench/bench.h"
#endif

State st;
const u8 *mgs_data;
static RtTilemap tmap;

// ---------------------------------------------------------------- level
// solid 8 x 8 quadrant at level pixel (x, y); outside the level is solid
static u8 solid_q(u8 qx, u8 qy)
{
    if (qx >= LV_W / 8 || qy >= LV_H / 8) return 1;
    return (lv1_solid[(qy >> 1) * 10 + (qx >> 1)] >> (((qy & 1) << 1) | (qx & 1))) & 1;
}

// Snake's box: [x - 6, x + 5) x [y - 4, y + 7) (measured: walls stop him at x 22 / 91, y 180 /
// 201 from the start, on 16-pixel cell edges)
u8 mgs_free(u8 x, u8 y)
{
    u8 qx, qy, qx1, qy1;
    if (x < 6 || y < 4) return 0;
    qx1 = (u8)(x + 4) >> 3;
    qy1 = (u8)(y + 6) >> 3;
    for (qy = (u8)(y - 4) >> 3; qy <= qy1; qy++)
        for (qx = (u8)(x - 6) >> 3; qx <= qx1; qx++)
            if (solid_q(qx, qy)) return 0;
    return 1;
}

// ---------------------------------------------------------------- Snake
static const s8 DX[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
static const s8 DY[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
// D-pad bits (K_UP 1, K_LEFT 2, K_DOWN 4, K_RIGHT 8) -> direction, 8 = none
static const u8 PAD_DIR[16] = { 8, 0, 6, 7, 4, 8, 5, 6, 2, 1, 8, 0, 3, 2, 4, 8 };

static void snake_step(u8 keys)
{
    u8 want = PAD_DIR[keys & 15];
    st.moving = 0;
    if (want == 8) {                    // released: stand, a new press turns from scratch
        st.turn = st.settle = 0;
        st.anim = st.animcnt = 0;
        return;
    }
    if (want != st.sdir) {              // turn one eighth every 2 steps, clockwise on a tie
        if (++st.turn == 2) {
            st.turn = 0;
            st.sdir = (st.sdir + (((u8)(want - st.sdir) & 7) <= 4 ? 1 : 7)) & 7;
            st.settle = 2;
        }
        return;
    }
    if (st.settle) { st.settle--; return; }
    {
        s8 dx = DX[st.sdir], dy = DY[st.sdir];
        u8 fx = dx && mgs_free(st.sx + dx, st.sy);
        u8 fy = dy && mgs_free(st.sx, st.sy + dy);
        if (fx && fy) {                 // diagonal: both axes on 2 steps of 3
            if (++st.diag == 3) st.diag = 0;
            else if (mgs_free(st.sx + dx, st.sy + dy)) { st.sx += dx; st.sy += dy; }
        } else if (fx) st.sx += dx;     // along a wall: full speed on the free axis
        else if (fy) st.sy += dy;
    }
    st.moving = 1;
    if (++st.animcnt == 3) {            // the walk cycle: 11 steps of 3 frames
        st.animcnt = 0;
        st.anim = st.anim == 10 ? 0 : st.anim + 1;
    }
}

// ---------------------------------------------------------------- the guard
// Vision boxes (x0, x1, y0, y1) relative to the guard, Snake's feet inside = seen. Measured
// with Snake poked on a 2-pixel grid (README § Vision); up mirrors down, left mirrors right.
static const s8 CONE_DOWN[4][4] = { { -4, 4, 0, 8 }, { -12, 12, 8, 24 }, { -20, 20, 24, 48 }, { -12, 12, 48, 56 } };
static const s8 CONE_RIGHT[6][4] = { { -4, 52, -4, 4 }, { 4, 52, -12, -4 }, { 4, 52, 4, 12 },
    { 20, 44, -20, -12 }, { 20, 44, 12, 20 }, { 28, 36, -28, -20 } };

static u8 in_box(s16 dx, s16 dy, const s8 *b) { return dx >= b[0] && dx < b[1] && dy >= b[2] && dy < b[3]; }

u8 mgs_sees(void)
{
    s16 dx = (s16)st.sx - st.gx, dy = (s16)st.sy - st.gy;
    u8 k, d = st.gdir;
    // the ROM's guards see only while on screen (measured): here, while any of the sprite
    // shows in the TI's 100 rows
    if ((s16)st.gy + 4 <= (s16)st.camy || (s16)st.gy - 21 >= (s16)st.camy + VIEW_H) return 0;
    if (d == 0 || d == 4) {
        if (d == 0) dy = -dy - 1;
        for (k = 0; k < 4; k++) if (in_box(dx, dy, CONE_DOWN[k])) return 1;
    } else {
        if (d > 4) dx = -dx;
        for (k = 0; k < 6; k++) if (in_box(dx, dy, CONE_RIGHT[k])) return 1;
    }
    return 0;
}

static void guard_walk(s8 dy)
{
    if (++st.gsub == 3) { st.gsub = 0; st.gy += dy; st.gsteps++; }
}

// The patrol (measured over 1,300 frames): down at 1 px / 3 frames from y TOP to BOTTOM, 288
// frames facing down, turn to the right (2 frames per eighth) and hold 20, walk up turning to
// face up, 288 frames at the top (turning to the right), then down again.
static void guard_step(void)
{
    st.gtimer++;
    switch (st.gstate) {
    case G_WAIT_TOP:
        if (st.gtimer == 2 || st.gtimer == 4) st.gdir = st.gdir == 0 ? 1 : 2;
        if (st.gtimer >= 288) { st.gstate = G_WALK_DOWN; st.gtimer = 0; }
        break;
    case G_WALK_DOWN:
        if (st.gtimer == 2) st.gdir = 3;
        if (st.gtimer == 4) st.gdir = 4;
        guard_walk(1);
        if (st.gy >= LV1_GUARD_BOTTOM) { st.gstate = G_WAIT_BOTTOM; st.gtimer = 0; st.gdir = 4; }
        break;
    case G_WAIT_BOTTOM:
        if (st.gtimer >= 288) { st.gstate = G_TURN_BOTTOM; st.gtimer = 0; }
        break;
    case G_TURN_BOTTOM:
        if (st.gtimer == 1) st.gdir = 3;
        if (st.gtimer == 3) st.gdir = 2;
        if (st.gtimer >= 24) { st.gstate = G_WALK_UP; st.gtimer = 0; }
        break;
    case G_WALK_UP:
        if (st.gtimer == 2) st.gdir = 1;
        if (st.gtimer == 4) st.gdir = 0;
        guard_walk(-1);
        if (st.gy <= LV1_GUARD_TOP) { st.gstate = G_WAIT_TOP; st.gtimer = 0; st.gdir = 0; }
        break;
    }
}

// ---------------------------------------------------------------- the level
static void camera(void)
{
    s16 c = (s16)st.sy - 56;            // Snake a little below the middle (his sprite is tall)
    if (c < 0) c = 0;
    if (c > LV_H - VIEW_H) c = LV_H - VIEW_H;
    st.camy = c;
}

void mgs_start(void)
{
    u8 *b = (u8 *)&st;
    u16 n;
    for (n = 0; n < sizeof(st); n++) b[n] = 0;
    st.sx = LV1_START_X;
    st.sy = LV1_START_Y;
    st.sdir = 4;
    st.gx = LV1_GUARD_X;
    st.gy = LV1_GUARD_TOP;
    st.gdir = 2;
    st.gstate = G_WAIT_TOP;
    st.gtimer = 288 - 160;              // when Snake appears, the guard has 160 frames left
    camera();
}

void mgs_step(u8 keys)
{
    st.timer++;
    if (st.mode != M_PLAY) {
        if (st.mode == M_SPOTTED && st.timer >= 60) { st.mode = M_FAILED; st.timer = 0; }
        return;
    }
    st.steps++;
    snake_step(keys);
    guard_step();
    camera();
    if ((u8)(st.sx - LV1_GOAL_X) < 20 && (u8)(st.sy - LV1_GOAL_Y) < 14) {   // 0C:4627
        st.mode = M_CLEAR;
        st.timer = 0;
    } else if (mgs_sees()) {
        st.mode = M_SPOTTED;
        st.timer = 0;
    }
}

// ---------------------------------------------------------------- runtime hooks
void game_init(void)
{
    u16 size;
    mgs_data = rt_file("mgsdat", &size);
    if (mgs_data && size < GFX_BYTES) mgs_data = RT_NULL;   // the TI counts the OTH tag too
    tmap.map = lv1_map;
    tmap.w = 11;
    tmap.h = 15;
    tmap.tiles = (const u16 *)mgs_data;
    tmap.ntiles = GFX_NTILES;
    rt_state = &st;
    rt_state_size = sizeof(st);
    mgs_start();
}

// 0 the level start; 1 Snake next to the goal; 2 Snake in the corridor below the guard, who
// walks down towards him; 3 Snake hidden in the side room while the guard walks down
void game_scenario(u16 n)
{
    mgs_start();
    if (n == 1) { st.sx = 108; st.sy = 32; st.gy = 120; st.gstate = G_WAIT_BOTTOM; st.gtimer = 0; st.gdir = 4; }
    if (n == 2) { st.sx = 80; st.sy = 150; st.gstate = G_WALK_DOWN; st.gtimer = 10; st.gy = 90; st.gdir = 4; }
    if (n == 3) { st.sx = 56; st.sy = 136; st.gstate = G_WALK_DOWN; st.gtimer = 10; st.gy = 90; st.gdir = 4; }
    camera();
}

// Hash of every state field (not the raw struct: byte order and padding differ on the 68000):
// the TI binary prints it per frame under ti-cycles (make tihash), mgs_test --hash on the PC
u32 mgs_hash(void)
{
    const u16 v[] = { st.mode, st.timer, st.steps, st.sx, st.sy, st.sdir, st.turn, st.settle,
        st.moving, st.anim, st.animcnt, st.diag, st.gx, st.gy, st.gdir, st.gstate, st.gtimer,
        st.gsub, st.gsteps, st.camy };
    u32 h = 0;
    u8 k;
    for (k = 0; k < sizeof v / sizeof v[0]; k++) h = ((h << 5) | (h >> 27)) ^ v[k];
    return h;
}

static void update(void)
{
    u8 keys = (u8)(rt_keys & 15);
    if (st.mode == M_FAILED || st.mode == M_CLEAR) {
        if (input_pressed(K_A | K_ENTER) && st.timer > 30) mgs_start();
        else st.timer++;
        return;
    }
    mgs_step(keys);
    mgs_step(keys);
}

u8 game_update(void)
{
    if (input_held(K_ESC) || !mgs_data) return 0;
    update();
#ifdef STATE_HASH
    BENCH_VALUE(mgs_hash());
#endif
    return 1;
}

// ---------------------------------------------------------------- drawing
static const u8 SNAKE_IMG[11] = { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6 };   // walk step -> image

static void sprite(s16 x, s16 y, u16 off, u16 i)
{
    RtSprite s;
    const u8 *p = mgs_data + off + i * (SPR_H * 6);
    s.w = 16;
    s.h = SPR_H;
    s.light = p;
    s.dark = p + SPR_H * 2;
    s.mask = p + SPR_H * 4;
    draw_sprite(x, y, &s);
}

static void draw_snake(void)
{
    u16 i = st.sdir * SNAKE_STEPS + (st.moving || st.anim ? SNAKE_IMG[st.anim] : 0);
    sprite(st.sx + SNAKE_OX, st.sy - st.camy + SNAKE_OY, GFX_SNAKE, i);
}

static void draw_guard(void)
{
    u16 i;
    if (st.gstate == G_WALK_DOWN && st.gdir == 4) i = (st.gsteps >> 2) % GUARD_WALK_DOWN;
    else if (st.gstate == G_WALK_UP && st.gdir == 0) i = GUARD_WALK_DOWN + (st.gsteps >> 2) % GUARD_WALK_UP;
    else i = GUARD_STILL + (st.gdir > 4 ? 2 : st.gdir);
    sprite(st.gx + GUARD_OX, st.gy - st.camy + GUARD_OY, GFX_GUARD, i);
}

static void box(const char *a, const char *b)
{
    draw_rect(20, 34, 120, 32, C_BLACK);
    draw_rect(22, 36, 116, 28, C_WHITE);
    draw_text(30, 40, a, F_MEDIUM, C_BLACK);
    draw_text(30, 54, b, F_SMALL, C_BLACK);
}

void game_render(void)
{
    if (!mgs_data) {
        draw_clear();
        draw_text(4, 40, "mgsdat missing", F_MEDIUM, C_BLACK);
        return;
    }
    draw_tilemap(&tmap, 0, st.camy);
    if (st.gy < st.sy) { draw_guard(); draw_snake(); }
    else { draw_snake(); draw_guard(); }
    if (st.mode == M_SPOTTED) {         // "!" over the guard
        s16 y = st.gy - st.camy - 30;
        draw_rect(st.gx - 3, y - 1, 7, 12, C_BLACK);
        draw_rect(st.gx - 1, y + 1, 3, 5, C_WHITE);
        draw_rect(st.gx - 1, y + 7, 3, 2, C_WHITE);
    }
    if (st.mode == M_FAILED) box("MISSION FAILED", "[2nd] try again   [ESC] quit");
    if (st.mode == M_CLEAR) box("VR LV.01 CLEAR", "[2nd] again   [ESC] quit");
    if (st.mode == M_PLAY && st.steps < 120) {
        draw_rect(0, 0, 160, 9, C_BLACK);
        draw_text(2, 1, "VR SNEAKING  PRACTICE  LV.01", F_SMALL, C_WHITE);
    }
}
