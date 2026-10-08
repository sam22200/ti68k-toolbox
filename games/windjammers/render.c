/* ROM-derived outlined actors, with native animation playback. */
#include "windjam.h"
#include <string.h>

typedef struct { RtSprite sprite; s8 x, y; } WjArt;
typedef struct { u8 interval, count, period, frames[16]; } WjAnimation;
#include "generated/art_bank.h"
#include "generated/layout.h"
#include "generated/court.h"
/* Eight-pixel ring (formerly six), two-pixel rim and a white outline. */
static const u16 disc[10] = {0,0x03c0,0x07e0,0x0c30,0x0810,0x0810,0x0c30,0x07e0,0x03c0,0};
static const u16 disc_mask[10] = {0xfc3f,0xf81f,0xf00f,0xe007,0xe007,0xe007,0xe007,0xf00f,0xf81f,0xfc3f};
static const u8 a_cue[8] = {0x00,0x18,0x3c,0x66,0x66,0x7e,0x66,0x00};
static const u8 star[8] = {0x18,0x18,0x7e,0x3c,0x3c,0x7e,0x18,0x18};
static const u8 *art_bank;
static u16 art_word(const u8 *p) { return ((u16)p[0] << 8) | p[1]; }
static u16 coordinate(s32 v, u16 limit)
{
    s16 n = v >> 16;
    if (n < 0) return 0;
    return (u16)n > limit ? limit : (u16)n;
}

static s16 disc_height(void)
{
    s16 height = st.disc.z > 0 ? st.disc.z >> 18 : 0;
    return height > 24 ? 24 : height;
}

void wj_effect_tick(void)
{
    u16 i, kind = wj_disc_effect();
    s16 y;
    if (!kind) { st.effect_count = st.effect_kind = 0; return; }
    if (kind != st.effect_kind || st.effect_owner != st.disc.owner) st.effect_count = 0;
    st.effect_kind = kind; st.effect_owner = st.disc.owner;
    i = st.effect_count < 4 ? st.effect_count : 3;
    while (i) {
        st.effect_x[i] = st.effect_x[i - 1]; st.effect_y[i] = st.effect_y[i - 1];
        i--;
    }
    st.effect_x[0] = court_x[coordinate(st.disc.x, 319)];
    y = court_y[coordinate(st.disc.y, 224)];
    if (st.disc.mode == WJ_SUPERLOB) {
        y -= disc_height();
        if (y < 5) y = 5;
    }
    st.effect_y[0] = y;
    if (st.effect_count < 4) st.effect_count++;
}

static void disc_trail(void)
{
    static const u8 blank[8] = {0,0,0,0,0,0,0,0};
    static const u8 ring[8] = {0,0x18,0x3c,0x24,0x24,0x3c,0x18,0};
    static const u8 ring_mask[8] = {0xe7,0xc3,0x81,0x81,0x81,0x81,0xc3,0xe7};
    static const u8 energy[8] = {0x24,0x18,0x7e,0x66,0x66,0x7e,0x18,0x24};
    static const RtSprite afterimage = {8,8,blank,ring,ring_mask};
    static const RtSprite charged = {8,8,ring,energy,ring_mask};
    u16 i = st.effect_count;
    if (st.effect_kind != wj_disc_effect() || st.effect_owner != st.disc.owner) return;
    while (i > 1) {
        s16 x, y;
        i--; x = st.effect_x[i]; y = st.effect_y[i];
        if (i == 1) draw_sprite(x - 4, y - 4, st.effect_kind == 2 ? &charged : &afterimage);
        else if (i == 2) draw_rect(x - 2, y - 2, 4, 4, C_LGRAY);
        else if (st.effect_kind == 2) draw_rect(x - 1, y - 1, 2, 2, C_LGRAY);
    }
    if (st.effect_kind == 2 && st.effect_count > 1) {
        s16 x = st.effect_x[1], y = st.effect_y[1];
        s16 dx = (s16)st.effect_x[0] - x, dy = (s16)st.effect_y[0] - y;
        u16 ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
        s16 spread = st.draw_frame & 2 ? 6 : 8;
        /* Perpendicular sparks stay behind the visible disc. Skip duplicates
         * at clipped airborne peaks instead of decorating a stationary point. */
        if (ax || ay) {
            if (ax >= ay) {
                draw_rect(x - 1, y - spread, 2, 2, C_DGRAY);
                draw_rect(x - 1, y + spread - 2, 2, 2, C_DGRAY);
            } else {
                draw_rect(x - spread, y - 1, 2, 2, C_DGRAY);
                draw_rect(x + spread - 2, y - 1, 2, 2, C_DGRAY);
            }
        }
    }
}

u8 wj_art_init(void)
{
    u16 size, i, length;
    art_bank = rt_file("wjart", &size);
    if (!art_bank || size < 8 || memcmp(art_bank, "WJA1", 4) ||
        art_word(art_bank + 4) != WJ_ART_COUNT) goto bad;
    length = art_word(art_bank + 6);
    if (length > size || length < 8 + WJ_ART_COUNT * 8) goto bad;
    for (i = 0; i < WJ_ART_COUNT; i++) {
        const u8 *p = art_bank + 8 + (i << 3);
        u16 offset = art_word(p + 4);
        if ((p[0] != 16 && p[0] != 32) || !p[1] || p[1] > 48 ||
            offset < 8 + WJ_ART_COUNT * 8 || (offset & 3) ||
            (u32)offset + ((u16)(p[0] >> 3) * p[1]) * 3U > length) goto bad;
    }
    return 1;
bad:
    art_bank = RT_NULL;
    return 0;
}

static const WjAnimation *animation_for(u16 port, u16 *which, u16 *facing)
{
    WjPlayer *p = port ? &st.player[1] : &st.player[0];
    u16 action = 0, direction = port ? 192 : 64;
    if (p->ready || p->charging) {
        action = 2;
    } else if (p->throwing) {
        action = 3;
        if (p->aim && !p->throw_catch) direction += p->aim == 1 ? (port ? 32 : -32) : (port ? -32 : 32);
    } else if (p->catching || p->strong || p->dragged) {
        action = 4;
        if (p->recoil_aim == 1) direction += port ? -32 : 32;
        if (p->recoil_aim == 2) direction += port ? 32 : -32;
    } else if (st.disc.mode == WJ_HELD && st.disc.owner == port) {
        action = 2;
    } else if (p->vx || p->vy) {
        action = 1;
        if (!p->vx) direction = p->vy < 0 ? 0 : 128;
        else if (!p->vy) direction = p->vx < 0 ? 192 : 64;
        else direction = p->vx < 0 ? (p->vy < 0 ? 224 : 160) : (p->vy < 0 ? 32 : 96);
    }
    *which = action; *facing = direction;
    return &actor_animation[port][action][direction >> 5];
}

void wj_art_tick(void)
{
    u16 port;
    for (port = 0; port < 2; port++) {
        WjPlayer *p = port ? &st.player[1] : &st.player[0];
        u16 action, direction;
        const WjAnimation *animation = animation_for(port, &action, &direction);
        if (p->art_action != action || p->art_direction != direction) {
            p->art_action = action; p->art_direction = direction; p->art_tick = 0;
        } else if (++p->art_tick >= animation->period) p->art_tick = 0;
    }
}

static const WjArt *actor(u16 port)
{
    WjPlayer *p = port ? &st.player[1] : &st.player[0];
    u16 action, direction, time = p->art_tick;
    const WjAnimation *animation = animation_for(port, &action, &direction);
    if (action == 3) time = p->age;
    else if (action == 4) time = p->catch_age - 1;
    time = animation->interval == 4 ? time >> 2 : animation->interval == 8 ? time >> 3 :
           animation->interval == 5 ? art_div5[time] : art_div6[time];
    if (action == 3 && time >= animation->count) time = animation->count - 1;
    else while (time >= animation->count) time -= animation->count;
    {
        static WjArt cached;
        const u8 *r = art_bank + 8 + ((u16)animation->frames[time] << 3);
        u16 bytes = (r[0] >> 3) * r[1];
        cached.sprite.w = r[0]; cached.sprite.h = r[1];
        cached.x = (s8)r[2]; cached.y = (s8)r[3];
        cached.sprite.light = art_bank + art_word(r + 4);
        cached.sprite.dark = (const u8 *)cached.sprite.light + bytes;
        cached.sprite.mask = (const u8 *)cached.sprite.dark + bytes;
        return &cached;
    }
}

static void digit(s16 x, s16 y, u16 value, u8 inverse)
{
    RtSprite glyph;
    glyph.w = 8; glyph.h = 10;
    glyph.light = glyph.dark = inverse ? digit_mask[value] : score_pixels[value];
    glyph.mask = RT_NULL;
    draw_sprite(x, y, &glyph);
}

static void number(s16 x, u16 value, u8 inverse)
{
    u16 tens = value / 10;
    digit(x, 1, tens, inverse);
    digit(x + 7, 1, value - tens * 10, inverse);
}

static void landing_target(s16 x, s16 y)
{
    /* Fixed destination, distinct from the small moving ground shadow. */
    draw_rect(x - 6, y - 5, 4, 2, C_BLACK);
    draw_rect(x + 2, y - 5, 4, 2, C_BLACK);
    draw_rect(x - 6, y + 3, 4, 2, C_BLACK);
    draw_rect(x + 2, y + 3, 4, 2, C_BLACK);
    draw_rect(x - 6, y - 3, 2, 2, C_BLACK);
    draw_rect(x + 4, y - 3, 2, 2, C_BLACK);
    draw_rect(x - 6, y + 1, 2, 2, C_BLACK);
    draw_rect(x + 4, y + 1, 2, 2, C_BLACK);
    draw_rect(x - 1, y - 1, 2, 2, C_BLACK);
}

void game_render(void)
{
    u16 port, strip;
    s16 screen_x = 0;
    static const RtSprite disk = {16,10,disc,disc,disc_mask};
    static const RtSprite cue = {8,8,a_cue,a_cue,RT_NULL};
    static const RtSprite charged = {8,8,star,star,RT_NULL};
    if (!art_bank) {
        draw_clear();
        draw_text(4, 38, "Send wjart.89y", F_SMALL, C_BLACK);
        return;
    }
    /* Five opaque native court strips reset the entire screen without a clear. */
    for (strip = 0; strip < 5; strip++) {
        draw_sprite(screen_x, 0, &court_strips[strip]);
        screen_x += 32;
    }
    disc_trail();
    if (st.disc.mode == WJ_LOB || st.disc.mode == WJ_SUPERLOB) {
        landing_target(court_x[st.disc.target_x], court_y[st.disc.target_y]);
    }
    for (port = 0; port < 2; port++) {
        WjPlayer *p = port ? &st.player[1] : &st.player[0];
        const WjArt *art = actor(port);
        s16 x = court_x[coordinate(p->x, 319)] + art->x;
        s16 y = court_y[coordinate(p->y, 224)] + art->y;
        /* Keep the whole outlined actor visible at the LCD boundaries. */
        if (x < 3) x = 3;
        else if (x > 157 - art->sprite.w) x = 157 - art->sprite.w;
        if (y < 1) y = 1;
        else if (y > 99 - art->sprite.h) y = 99 - art->sprite.h;
        if (p->dash_age && p->dash_age < (port ? 13 : 11)) {
            s16 tx = court_x[coordinate(p->x, 319)], ty = court_y[coordinate(p->y, 224)];
            s16 dx = p->vx < 0 ? 1 : p->vx ? -1 : 0;
            s16 dy = p->vy < 0 ? 1 : p->vy ? -1 : 0;
            draw_rect(tx + dx * 13 - 2, ty + dy * 10 - 2, 4, 2, C_DGRAY);
            draw_rect(tx + dx * 18 - 2, ty + dy * 14 - 2, 4, 2, C_LGRAY);
        }
        draw_sprite(x, y, &art->sprite);
        if (p->charging || p->charged) {
            u16 fill = p->charged ? 12 : p->charge >> 2;
            s16 bx = port ? x - 8 : x + art->sprite.w + 1;
            if (bx < 1) bx = 1;
            else if (bx > 153) bx = 153;
            if (fill > 12) fill = 12;
            draw_rect(bx - 1, y - 1, 8, 18, C_WHITE);
            draw_rect(bx, y, 6, 16, C_BLACK);
            draw_rect(bx + 1, y + 1, 4, 14, C_WHITE);
            if (fill) draw_rect(bx + 2, y + 14 - fill, 2, fill, C_BLACK);
            if (p->charged) {
                s16 cy = y > 9 ? y - 9 : y + art->sprite.h + 1;
                draw_rect(x + 3, cy - 1, 10, 10, C_WHITE);
                draw_sprite(x + 4, cy, &charged);
            }
        } else if (p->ready) {
            /* Two-pixel ready cue; held ROM pose is cosmetic until the full
             * action bank is extracted in the charged/lob art slice. */
            s16 cy = y > 9 ? y - 9 : y + art->sprite.h + 1;
            draw_rect(x + 3, cy - 1, 10, 10, C_WHITE);
            draw_sprite(x + 4, cy, &cue);
        } else if (port == st.human_port && wj_prepare_hint(port)) {
            s16 cy = y > 9 ? y - 9 : y + art->sprite.h + 1;
            draw_rect(x + 3, cy - 1, 10, 10, C_WHITE);
            draw_sprite(x + 4, cy, &cue);
        }
    }
    /* Compact overlay: the court continues underneath, with no HUD bands. */
    draw_rect(49, 0, 62, 13, C_BLACK);
    draw_rect(50, 1, 20, 11, C_WHITE); draw_rect(90, 1, 20, 11, C_WHITE);
    number(53, st.points[0], 0); number(73, st.seconds, 1);
    number(93, st.points[1], 0);
    if (st.disc.mode != WJ_GOAL) {
        s16 x = court_x[coordinate(st.disc.x, 319)], y = court_y[coordinate(st.disc.y, 224)];
        if (st.disc.mode == WJ_LIFT || st.disc.mode == WJ_BLOCK || st.disc.mode == WJ_LOB || st.disc.mode == WJ_SUPERLOB) {
            /* Ground position distinguishes airborne height from aim/court Y. */
            s16 height = disc_height();
            draw_rect(x - 3, y, 6, 2, C_DGRAY);
            y -= height;
            if (y < 5) y = 5;
        }
        draw_sprite(x - 8, y - 5, &disk);
    } else {
        draw_rect(58, 45, 44, 11, C_BLACK);
        draw_rect(59, 46, 42, 9, C_WHITE);
        draw_text(62, 48, st.last_award == 5 ? "+5 POINTS" : st.last_award == 2 ? "+2 POINTS" : "+3 POINTS", F_SMALL, C_BLACK);
    }
}
