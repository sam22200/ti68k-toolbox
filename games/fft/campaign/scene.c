// The scene player (fftscn, ENGINE.md): what a chapter shows between battles, read from a scene
// set (tools/scenes.py): a title, scripted beats over a fixed picture with depth (Orbonne's
// chapel: actors walking behind its pillars, dialogue), a chapter card. fftscn(k) plays ffs<k>
// and returns 1 once it is over (0: ESC). The beats, dialogue, text and actor bank are the
// battle engine's own code (../beats.inc, ../text.inc, ../actors.inc).
#include "../../../runtime/core/rt.h"
#include "../units.h"
#include "../ui_common.h"
#include <stdlib.h>
#include <string.h>
#include "../base.inc"

#ifdef RT_CYCLES
#include "../../../tools/m68kbench/bench.h"
#define ZB(n) BENCH_BEGIN(n)
#define ZE(n) BENCH_END(n)
#else
#define ZB(n)
#define ZE(n)
#endif

enum { STEP_TITLE = 1, STEP_STORY, STEP_CARD };
enum { OPEN_PLAY, OPEN_TITLE, OPEN_CHAPEL, OPEN_GATE, OPEN_AFTER, OPEN_DONE, OPEN_BANTER };
#define NUNIT 11                        // scripted actor slots (beats: slot << 4 | gfx)
#define COVER_H UNIT_SH
typedef struct { s16 x, y; u8 depth, rot, r0, r1, wet; u16 cm[COVER_H]; } Cover;
typedef struct { s16 x, y; u8 depth; } Feet;

typedef struct {
    u8 step, open_stage, open_beat, open_clock, open_white;
    u16 open_reveal, open_offset, open_seen, tick;
    u8 open_pan, open_fade, open_next, done;
    s16 camx, camy;                     // (beats.inc: map scenes only)
} State;
State st;

#define ACTOR_SLOTS (NUNIT + 2)
#define ACTOR_UNITS NUNIT
#define ACTOR_KEY(logical) RD16(amap, logical)   // the set's pose table (ffoa: logical -> frame)
#include "../actors.h"

typedef struct {
    ACTOR_FIELDS
    char dialog[6][48];
    u8 dialog_n, dialog_w, dialog_h, dialog_next;
    u16 dialog_total, dialog_key, dialog_drawn;
    u8 dialog_white, dialog_part;
    Cover cover[NUNIT];
    Feet feet[NUNIT];
    u16 feet_k[NUNIT];
    u16 status[2][RT_H * RT_PBYTES / 2];   // the dialogue's own planes, copied box by box
} Work;
static Work *W;
static u8 *vbuf;                        // the picture: colour, colour, depth/mask x 2
static const u8 *set, *amap, *cgfx;
static const u8 *opening_story, *opening_title;
#define PIC_STRIDE_MAX 40                // a picture is 320 pixels wide at most
static u16 pic_stride, pic_plane;
#define CHAPEL_STRIDE pic_stride
#define CHAPEL_PLANE pic_plane
#define OPENING_BEATS RD16(opening_story, 0)
#define UI_SHARED(s) (cgfx + UIC_##s)
static const u8 walk_seq[16] = { 2, 1, 1, 0, 0, 0, 1, 1, 2, 3, 3, 4, 4, 4, 3, 3 };

#include "../actors.inc"
#include "../text.inc"
static void opening_actor(u8 gfx,u8 slot,s16 x,s16 y,u8 face,u16 logical);
#define BEATS_PICTURE
#include "../beats.inc"

static const u8 *step_at(u8 i) { return set + 12 + 8 * i; }

// A step begins (the battle engine's opening_stage for the chapel).
static void step_start(void)
{
    const u8 *s;
    for (;;) {
        if (st.step >= RD16(set, 5)) { st.done = 1; return; }
        s = step_at(st.step);
        st.open_clock = 0; st.open_seen = 0;
        memset(W->actor_key, 255, sizeof W->actor_key);
        W->dialog_key = 0xFFFF;
        if (s[0] == STEP_TITLE) { st.open_stage = OPEN_TITLE; opening_title = set + RD16(s, 1); return; }
        if (s[0] == STEP_STORY) {
            opening_story = set + RD16(s, 3);
            pic_stride = RD16(s, 2); pic_plane = (u16)(pic_stride * 100);
            zx0_unpack(set + RD16(s, 1), vbuf);
            st.open_stage = OPEN_CHAPEL;
            opening_find(opening_story[RD16(opening_story, 1)]);   // its first beat's event
            st.open_pan = s[1];
            return;
        }
        if (s[0] == STEP_CARD) { st.open_stage = OPEN_DONE; return; }
        st.step++;
    }
}

static void next_step(void) { st.step++; step_start(); }
static void transition(void) { st.open_fade = 1; }

static void advance(void)
{
    u8 event = opening_beat()[0];
    if (st.open_beat + 1 < OPENING_BEATS) {
        st.open_beat++;
        if (opening_beat()[0] == event) { opening_dialog_reset(); return; }
    }
    transition();
}

u8 chapter_completed(void) { return st.done == 1; }

static void release(void)
{
    if (acache) free(acache);
    if (vbuf) free(vbuf);
    if (W) free(W);
    acache = RT_NULL; vbuf = RT_NULL; W = RT_NULL;
}

void game_init(void)
{
    W = RT_NULL; vbuf = RT_NULL; acache = RT_NULL;   // statics survive between runs
    cgfx = rt_file("ffcu", RT_NULL);
    abank[0] = rt_file("ffca", RT_NULL); abank[1] = rt_file("ffcb", RT_NULL);
    if (abank[0]) abank_n0 = RD16(abank[0], 0);
    W = malloc(sizeof(Work));
    vbuf = W ? malloc(6U * PIC_STRIDE_MAX * 100) : RT_NULL;
    acache = vbuf ? malloc(AC_N * (UNIT_FB + 4)) : RT_NULL;
    if (!acache) { release(); return; }
    ac_key = (u16 *)acache[AC_N]; ac_age = ac_key + AC_N;
#ifdef __m68k__
    atexit(release);
#endif
}

void game_scenario(u16 n)
{
    char name[6] = "ffs0";
    memset(&st, 0, sizeof st);
    name[3] = (char)('0' + n);
    set = rt_file(name, RT_NULL);
    if (!W || !set || !cgfx || !abank[0] || !abank[1] || RD16(set, 0) != 0x4653) { set = RT_NULL; return; }
    {
        char map[7]; memcpy(map, set + 4, 6); map[6] = 0;
        amap = *map ? rt_file(map, RT_NULL) : RT_NULL;   // (no actors: no pose table)
        if (*map && !amap) { set = RT_NULL; return; }
    }
    memset(W, 0, sizeof *W);
    actor_reset(); W->dialog_key = 0xFFFF;
    step_start();
    if (!st.done && step_at(st.step)[0] == STEP_CARD && step_at(st.step)[1] & 1)
        st.open_fade = 9;               // the previous program faded out: this card fades in
}

u8 game_update(void)
{
    if (!set) return !input_pressed(K_ESC);
    W->actor_budget = 1; W->actor_misses = 0; W->ac_now++;
    st.tick++; if (st.open_clock < 250) st.open_clock++;
    if (st.open_fade) {                 // out over 8 frames, the next step in over 8 more
        if (++st.open_fade == 9) { next_step(); if (st.done) return 0; st.open_fade = 9; }
        else if (st.open_fade >= 17) st.open_fade = 0;
        return !input_pressed(K_ESC);
    }
    if (st.open_stage == OPEN_TITLE) {
        // Do not consume the ENTER which launched the program. Finish the source-letter
        // reveal and leave a readable hold before skipping.
        if (st.open_clock >= 150 || (st.open_clock >= 110 && input_pressed(K_A | K_ENTER))) next_step();
        return !input_pressed(K_ESC);
    }
    if (st.open_stage == OPEN_DONE) {
        if (input_pressed(K_A | K_ENTER)) { next_step(); return !st.done; }
        return !input_pressed(K_ESC);
    }
    opening_positions();
    if (input_pressed(K_CATALOG)) { st.open_white ^= 1; W->dialog_key = 0xFFFF; }
    if (input_pressed(K_B)) { transition(); return 1; }
    if (opening_beat()[2]) {
        if (st.open_clock >= opening_beat()[2]) advance();
        return !input_pressed(K_ESC);
    }
    if (W->dialog_key != st.open_beat) opening_layout();
    if (st.open_reveal < W->dialog_total) {
        st.open_reveal += 3;
        if (st.open_reveal > W->dialog_total) st.open_reveal = W->dialog_total;
        if (input_pressed(K_A | K_ENTER)) st.open_reveal = W->dialog_total;
    } else if (input_pressed(K_A | K_ENTER)) {
        const char *s = opening_text();
        if (W->dialog_drawn != 0xFFFF && W->dialog_drawn < W->dialog_total) return 1;
        if (s[W->dialog_next]) { st.open_offset += W->dialog_next; st.open_reveal = 0; W->dialog_key = 0xFFFF; return 1; }
        advance();
    }
    return !input_pressed(K_ESC);
}

static void draw_title(void)
{
    u8 col, y; u16 sweep = (u16)st.open_clock * 2;
    draw_rect(0, 0, 160, 100, C_BLACK);
    for (col = 0; col < 18; col++) {
        const u8 *p = opening_title + (u16)col * 42; u8 l[14], d[14], m[14], white = 0, mid = 0, dim = 0, x;
        for (x = 0; x < 8; x++) {
            s16 delta = (s16)sweep - col * 8 - x; u8 bit = 0x80 >> x;
            if (delta >= 16) white |= bit; else if (delta >= 8) mid |= bit; else if (delta >= 0) dim |= bit;
        }
        for (y = 0; y < 14; y++) {
            u8 ink = (u8)~p[28 + y]; m[y] = (u8)~(ink & (white | mid | dim));
            l[y] = (p[y] & white) | (ink & mid); d[y] = (p[14 + y] & white) | (ink & dim);
        }
        { RtSprite sp = { 8, 14, l, d, m }; draw_sprite(8 + col * 8, 42, &sp); }
    }
}

static void draw_card(void)
{
    const u8 *p = set + RD16(step_at(st.step), 1);
    u8 n = *p++;
    draw_rect(0, 0, 160, 100, C_BLACK);
    while (n--) {
        ui_text(p[0], p[1], (const char *)p + 3, p[2], 158);
        p += 3; while (*p++) ;
    }
}

void game_render(void)
{
    u8 i;
    if (!set) { draw_rect(0, 0, 160, 100, C_WHITE); ui_text(3, 40, "Missing scene set", C_BLACK, 160); return; }
    if (st.done) return;
    if (st.open_stage == OPEN_TITLE) draw_title();
    else if (st.open_stage == OPEN_DONE) draw_card();
    else {
        const u8 *b = opening_beat(), *a = b + 4;
        ZB(21);
        for (i = 0; i < 100; i++) {
            u16 at = (u16)i * CHAPEL_STRIDE + (st.open_pan >> 3);
            memcpy((u8 *)rt_light + i * RT_PBYTES, vbuf + at, 20);
            memcpy((u8 *)rt_dark + i * RT_PBYTES, vbuf + CHAPEL_PLANE + at, 20);
        }
        for (i = 0; i < b[1]; i++, a += 8) {
            u8 slot = a[0] >> 4;
            opening_actor(a[0] & 15, slot, W->feet[slot].x, W->feet[slot].y, a[1], RD16(a + 6, 0));
        }
        ZE(21);
        if (!b[2]) {
            if (W->dialog_key != st.open_beat) opening_layout();
            ZB(22); opening_dialog_draw(); ZE(22);
        }
    }
    if (st.open_fade) {
        u8 level = st.open_fade < 9 ? st.open_fade : 17 - st.open_fade;
        u8 phase = (level + 1) >> 1, r; u16 pat = phase >= 4 ? 0xFFFF : phase == 3 ? 0xEEEE : phase == 2 ? 0xAAAA : 0x8888;
        for (r = 0; r < 100; r++) {
            u8 *l = (u8 *)rt_light + (u16)r * RT_PBYTES, *d = (u8 *)rt_dark + (u16)r * RT_PBYTES; u8 w;
            u16 bits = r & 1 ? (u16)((pat << 1) | (pat >> 15)) : pat;
            for (w = 0; w < 10; w++) { RMW16(l + 2 * w, bits, bits); RMW16(d + 2 * w, bits, bits); }
        }
    }
}
