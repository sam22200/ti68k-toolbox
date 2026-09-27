// Flappy Bird for the Portable Game Runtime, rules from sdlbird (README.md: spec and scaling).
// 2nd / up / ENTER flaps, ESC quits. Scenarios (injection door): see README.md.
// PC: make pc && ./flappy_pc [--scenario N]    tests: make test    TI: make ti → flappy(N)
#include "flappy.h"
#include "gfx.h"

Flappy st;
static u16 body_l[GROUND_Y], body_d[GROUND_Y];  // pipe body column, filled by game_init

static u8 gap_top(void)                        // GAP_MIN + [0, GAP_RANGE): one mulu, no division
{
    return GAP_MIN + (u8)(((u32)rt_rand() * GAP_RANGE) >> 16);
}

void flappy_reset(void)
{
    u16 i;
    st.mode = S_READY;
    st.t = 0;
    st.y = START_Y << 8;
    st.vy = 0;
    st.angle = 0;
    st.score = 0;
    st.newbest = 0;
    st.night = rt_rand() & 1;
    for (i = 0; i < NPIPE; i++) {
        st.pipe[i].x = PIPE_X0 + i * PIPE_DIST;
        st.pipe[i].top = gap_top();
        st.pipe[i].scored = 0;
    }
}

void game_init(void)
{
    u16 i;
    for (i = 0; i < GROUND_Y; i++) { body_l[i] = PIPE_BODY_L; body_d[i] = PIPE_BODY_D; }
    rt_state = &st;
    rt_state_size = sizeof(st);
}

void game_scenario(u16 n)
{
    u16 i;
    st.best = 0;
    st.clock = 0;
    st.sub = 0;
    st.land = 0;
    st.auto_pilot = 0;
    flappy_reset();
    st.mode = S_TITLE;
    if (n == 0) return;
    st.mode = S_READY;
    if (n == 1) return;
    st.mode = S_PLAY;                          // 2..: playing, pipe[0] arriving at the bird
    for (i = 0; i < NPIPE; i++) { st.pipe[i].x = BIRD_X + 24 + i * PIPE_DIST; st.pipe[i].top = 34; }
    st.y = (34 + (GAP - BIRD_H) / 2) << 8;     // centred in the opening
    if (n == 3) st.y = 10 << 8;                // far above the opening: hits pipe[0]
    if (n == 4) { st.score = 39; st.best = 39; }   // one pipe from the platinum medal
    if (n == 5) { st.mode = S_OVER; st.t = OVER_INPUT; st.score = 25; st.best = 30; st.y = (GROUND_Y - BIRD_H) << 8; }
    if (n == 6) st.y = (GROUND_Y - BIRD_H - 2) << 8;   // about to touch the ground
    if (n == 7) st.auto_pilot = 1;             // endless demo, also the bench scenario
}

u8 flappy_scroll(void)
{
    st.sub ^= 1;
    return 1 + st.sub;
}

u8 flappy_hit(void)
{
    const Pipe *p = &st.pipe[0];
    s16 y = st.y >> 8;
    if (y + BIRD_H >= GROUND_Y) return 1;
    return p->x < BIRD_X + BIRD_W && p->x + PIPE_W > BIRD_X && (y < p->top || y + BIRD_H > p->top + GAP);
}

static void flap(void)
{
    st.vy = FLAP_VY;
    st.angle = ANG_FLAP;
}

void flappy_play_step(u8 press)
{
    u16 i;
    u8 dx = flappy_scroll();
    Pipe *p = st.pipe;
    st.y += st.vy;                             // upstream order: move, then accelerate
    st.vy += GRAVITY;
    st.angle = st.angle > ANG_MAX - ROT_STEP ? ANG_MAX : st.angle + ROT_STEP;
    if (st.y < CEIL_FP) st.y = CEIL_FP;
    if ((st.y >> 8) + BIRD_H > GROUND_Y) st.y = (GROUND_Y - BIRD_H) << 8;
    st.land += dx;
    for (i = 0; i < NPIPE; i++) p[i].x -= dx;
    if (p[0].x < -PIPE_W) {                    // recycle: shift left, new pipe at the end
        for (i = 0; i < NPIPE - 1; i++) p[i] = p[i + 1];
        p[NPIPE - 1].x = p[NPIPE - 2].x + PIPE_DIST;
        p[NPIPE - 1].top = gap_top();
        p[NPIPE - 1].scored = 0;
    }
    if (!p[0].scored && p[0].x + PIPE_W / 2 < BIRD_X && p[0].x + PIPE_W > BIRD_X) {
        p[0].scored = 1;
        st.score++;
    }
    if (flappy_hit()) { st.mode = S_DEAD; st.t = 0; return; }
    if (press) flap();
}

static u8 pilot(void)                          // flap when the bird sinks to the opening bottom
{
    const Pipe *p = st.pipe;
    while (p->x + PIPE_W <= BIRD_X) p++;       // next pipe the bird has not cleared
    return st.vy >= 0 && (st.y >> 8) + BIRD_H > p->top + GAP - 4;
}

u8 flappy_medal(u16 score)
{
    return score >= 40 ? 4 : score >= 30 ? 3 : score >= 20 ? 2 : score >= 10 ? 1 : 0;
}

u8 game_update(void)
{
    u8 press = input_pressed(FLAP_KEYS) != 0;
    if (input_pressed(K_ESC)) return 0;
    st.clock++;
    st.t++;
    switch (st.mode) {
    case S_TITLE:
        st.land += flappy_scroll();
        if (press || st.auto_pilot) flappy_reset();
        break;
    case S_READY:
        st.land += flappy_scroll();
        if (press || st.auto_pilot) { st.mode = S_PLAY; st.t = 0; flap(); }
        break;
    case S_PLAY:
        flappy_play_step(press || (st.auto_pilot && pilot()));
        break;
    case S_DEAD:                               // flash, then drop to the ground at DROP_VY
        if (st.t <= FLASH_T) break;
        st.angle = ANG_MAX;
        st.y += DROP_VY;
        if ((st.y >> 8) + BIRD_H >= GROUND_Y) {
            st.y = (GROUND_Y - BIRD_H) << 8;
            st.mode = S_OVER;
            st.t = 0;
            if (st.score > st.best) { st.best = st.score; st.newbest = 1; }
        }
        break;
    case S_OVER:
        if (st.t >= OVER_INPUT && (press || st.auto_pilot)) flappy_reset();
        break;
    }
    return 1;
}

// ---------------------------------------------------------------- rendering
// Pipes and land are sprites, not stripes of draw_rect: one opaque 16-px sprite per pipe half
// (a column of identical rows), one masked 32-px cap, six 32x4 land tiles (bench: README).
#define CAP_H 6

static void draw_pipe(const Pipe *p)
{
    static const RtSprite cap = { 32, CAP_H, cap_light[0], cap_dark[0], cap_mask[0] };
    RtSprite body = { 16, 0, body_l, body_d, RT_NULL };
    s16 lo = p->top + GAP;
    if (p->x >= RT_W || p->x + PIPE_W + 2 <= 0) return;
    body.h = p->top - CAP_H;
    draw_sprite(p->x, 0, &body);
    draw_sprite(p->x - 2, p->top - CAP_H, &cap);
    draw_sprite(p->x - 2, lo, &cap);
    body.h = GROUND_Y - lo - CAP_H;
    draw_sprite(p->x, lo + CAP_H, &body);
}

static void draw_sky(void)                     // day: white sky, light city; night: one grey darker
{
    static const u8 city[][3] = { {0, 72, 10}, {10, 66, 8}, {18, 74, 12}, {30, 62, 9}, {39, 70, 14},
                                  {53, 76, 10}, {63, 64, 11}, {74, 71, 9}, {83, 67, 13}, {96, 74, 8},
                                  {104, 61, 10}, {114, 69, 12}, {126, 75, 9}, {135, 65, 11}, {146, 72, 14} };
    u16 i;
    u8 fg = st.night ? C_DGRAY : C_LGRAY;
    draw_rect(0, 0, RT_W, GROUND_Y, st.night ? C_LGRAY : C_WHITE);
    for (i = 0; i < 15; i++) draw_rect(city[i][0], city[i][1], city[i][2], GROUND_Y - city[i][1], fg);
}

static void draw_land(void)
{
    static const RtSprite band = { 32, 4, land_light[0], land_dark[0], RT_NULL };
    s16 x;
    draw_rect(0, GROUND_Y, RT_W, 1, C_BLACK);
    for (x = -(s16)(st.land & 7); x < RT_W; x += 32) draw_sprite(x, GROUND_Y + 1, &band);
    draw_rect(0, GROUND_Y + 5, RT_W, 1, C_DGRAY);
    draw_rect(0, GROUND_Y + 6, RT_W, RT_H - GROUND_Y - 6, C_LGRAY);
}

static void draw_number(s16 cx, s16 y, u16 v)  // outlined white digits centred on cx
{
    u8 d[5], n = 0;
    RtSprite s = { 8, 11, 0, 0, 0 };
    do d[n++] = v % 10; while ((v /= 10) && n < 5);
    cx -= (n * 7 + 1) >> 1;
    while (n--) {
        s.light = digit_light[d[n]]; s.dark = digit_dark[d[n]]; s.mask = digit_mask[d[n]];
        draw_sprite(cx, y, &s);
        cx += 7;                               // outlines overlap by one pixel
    }
}

static void plate(s16 x, s16 y, s16 w, s16 h)  // white box, black border
{
    draw_rect(x, y, w, h, C_BLACK);
    draw_rect(x + 1, y + 1, w - 2, h - 2, C_WHITE);
}

static void banner(s16 y, const char *s, u8 n)  // centred F_MEDIUM text on a plate
{
    s16 w = n * 6 + 6;
    plate((RT_W - w) >> 1, y, w, 12);
    draw_text(((RT_W - w) >> 1) + 3, y + 2, s, F_MEDIUM, C_BLACK);
}

static void draw_bird(s16 y)
{
    static const s8 bob[32] = { 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 0,
                                0, 0, -1, -1, -1, -2, -2, -2, -2, -2, -2, -2, -1, -1, -1, 0 };
    static const u8 wing_seq[8] = { 0, 0, 1, 1, 2, 2, 1, 1 };
    RtSprite s = { 16, 16, 0, 0, 0 };
    u8 tilt = 1, wing = wing_seq[(st.clock >> 1) & 7], f;
    if (st.mode == S_PLAY) tilt = st.angle < -20 ? 0 : st.angle < 20 ? 1 : st.angle < 65 ? 2 : 3;
    else if (st.mode >= S_DEAD) { tilt = 3; wing = 1; }
    else y += bob[st.clock & 31];
    f = tilt * 3 + wing;
    s.light = bird_light[f]; s.dark = bird_dark[f]; s.mask = bird_mask[f];
    draw_sprite(BIRD_X - BIRD_SX, y - BIRD_SY, &s);
}

static void draw_panel(void)
{
    RtSprite m = { 8, 8, 0, 0, 0 };
    s16 y = 100 - (s16)(st.t - OVER_PANEL) * 8;
    u16 shown = st.t > OVER_INPUT ? st.t - OVER_INPUT : 0;
    u8 medal = flappy_medal(st.score);
    if (y < 40) y = 40;
    plate(36, y, 88, 34);
    draw_rect(38, y + 2, 84, 30, C_LGRAY);
    draw_rect(39, y + 3, 82, 28, C_WHITE);
    draw_text(42, y + 5, "MEDAL", F_SMALL, C_DGRAY);
    draw_text(76, y + 8, "SCORE", F_SMALL, C_DGRAY);
    draw_text(76, y + 21, "BEST", F_SMALL, C_DGRAY);
    if (st.newbest) draw_text(96, y + 21, "NEW", F_SMALL, C_BLACK);
    if (st.t >= OVER_INPUT) {
        draw_number(112, y + 6, shown < st.score ? shown : st.score);
        draw_number(112, y + 19, st.best);
        if (medal) {
            m.light = medal_light[medal - 1]; m.dark = medal_dark[medal - 1]; m.mask = medal_mask[medal - 1];
            draw_sprite(46, y + 15, &m);
        } else draw_rect(46, y + 15, 8, 8, C_LGRAY);
    }
}

void game_render(void)
{
    u16 i;
    if (st.mode == S_DEAD && st.t <= FLASH_T) { draw_clear(); return; }
    draw_sky();
    if (st.mode >= S_PLAY)
        for (i = 0; i < NPIPE; i++) draw_pipe(&st.pipe[i]);
    draw_land();
    draw_bird(st.y >> 8);
    switch (st.mode) {
    case S_TITLE:
        banner(20, "FLAPPY BIRD", 11);
        draw_text(58, 70, "PRESS 2ND", F_SMALL, C_BLACK);
        return;
    case S_READY:
        banner(20, "GET READY", 9);
        draw_text(46, 70, "2ND / UP = FLAP", F_SMALL, C_BLACK);
        break;
    case S_OVER:
        if (st.t >= OVER_TEXT) {
            s16 y = 6 + (s16)(st.t - OVER_TEXT);
            banner(y > 22 ? 22 : y, "GAME OVER", 9);
        }
        if (st.t >= OVER_PANEL) draw_panel();
        return;
    default: break;
    }
    draw_number(RT_W / 2, 4, st.score);
}
