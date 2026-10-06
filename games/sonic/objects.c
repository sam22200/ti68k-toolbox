#include <string.h>
#include "sonic.h"
#include "art.h"
#include "generated/trig.h"

const u8 *son_objects;
u8 son_object_count;

static u16 word(const u8 *p) { return ((u16)p[0] << 8) | p[1]; }
static s16 abs16(s16 x) { return x < 0 ? -x : x; }
static void move(s16 *x, u8 *frac, s16 speed)
{
    s16 v = *frac + speed;
    *x += v >> 8; *frac = (u8)v;
}
static u8 visible(s16 x, s16 y, s16 w, s16 h)
{
    x = (x >> 1) - st.camx; y = (y >> 1) - st.camy;
    return x >= -w && x < RT_W + w && y >= -h && y < RT_H + h;
}
static u8 touches(s16 x, s16 y, s16 w, s16 h)
{
    return abs16(st.x - x) <= w + 8 &&
        abs16(st.y - y) <= h + ((st.flags & S_ROLL) ? 11 : 16);
}

void sonic_objects_reset(void)
{
    const u8 *p = son_objects;
    u8 i, n = 0;
    if (!p || !st.ready) return;
    for (i = 0; i < son_object_count; i++, p += 6) {
        if (p[4] != 0x25 && n < SON_ENEMIES) {
            SonEnemy *e = st.enemies + n++;
            e->x = word(p); e->y = e->origin_y = word(p + 2);
            e->kind = p[4]; e->direction = p[5];
        }
    }
    st.objects = 1;
}

void sonic_hurt(s16 source_x)
{
    u8 i, n = 0, count;
    if (st.hurt || st.invuln || st.finished) return;
    if (!st.rings) { st.resets++; sonic_start(80, 944); return; }
    count = st.rings < SON_LOST ? st.rings : SON_LOST;
    for (i = 0; i < SON_LOST; i++) {
        SonParticle *p = st.lost + i;
        u8 angle;
        s16 boost;
        /* REV00 refreshes the shared timer of existing spilled rings. */
        if (p->life) { p->life = 255; continue; }
        if (n >= count) continue;
        angle = (u8)(136 + ((u16)(n >> 1) << 4));
        boost = n < 16 ? 4 : 2;
        memset(p, 0, sizeof *p);
        p->x = st.x; p->y = st.y; p->life = 255;
        p->vx = sin_tab[angle] * boost;
        if (n & 1) p->vx = -p->vx;
        p->vy = sin_tab[(u8)(angle + 64)] * boost;
        n++;
    }
    if (st.flags & S_ROLL) st.y -= 5;
    st.flags = (st.flags & S_LEFT) | S_AIR;
    st.rings = 0; st.hurt = 1; st.invuln = 120; st.notice = 180;
    st.jumping = 0; st.speed = 0;
    st.vx = st.x < source_x ? -512 : 512; st.vy = -1024;
}

static void missile(SonEnemy *e, u8 parent)
{
    u8 i;
    for (i = 0; i < SON_SHOTS; i++) {
        SonParticle *p = st.shots + i;
        if (p->life) continue;
        memset(p, 0, sizeof *p);
        p->x = e->x + (e->direction ? 24 : -24); p->y = e->y + 28;
        p->vx = e->direction ? 512 : -512; p->vy = 512;
        p->life = 255; p->delay = 31; p->parent = parent;
        return;
    }
}

static void enemy_step(SonEnemy *e, u8 index)
{
    u8 angle = 0;
    s16 floor;
    if (e->phase == 5) { if (e->flash) e->flash--; return; }
    if (!e->phase) {
        /* Bounded original loader band; no whole-level object scan. */
        s16 left = ((u16)(st.camx << 1) & ~127) - 128;
        if (e->x < left || e->x >= left + 768) return;
        e->phase = 1;
        if (e->kind == E_CHOP) { e->vy = -1792; e->phase = 2; }
    }
    if (e->kind == E_MOTO) {
        if (e->phase == 1) {
            move(&e->y, &e->fy, e->vy); e->vy += 56;
            floor = sonic_floor(e->x, e->y + 14 - 16, e->y + 14 + 16, &angle);
            if (floor != 32767 && e->y + 14 >= floor) {
                e->y = floor - 15; e->vy = 0; e->phase = 2;
                e->direction ^= 1;
            }
            return;
        }
        if (e->phase == 2) {
            if (e->timer) { e->timer--; return; }
            e->phase = 3; e->direction ^= 1;
            e->vx = e->direction ? 256 : -256;
        } else {
            move(&e->x, &e->fx, e->vx);
            floor = sonic_floor(e->x, e->y + 14 - 8, e->y + 14 + 11, &angle);
            if (floor == 32767) { e->phase = 2; e->timer = 59; e->vx = 0; }
            else e->y = floor - 15;
        }
    } else if (e->kind == E_CHOP) {
        move(&e->y, &e->fy, e->vy); e->vy += 24;
        if (e->y > e->origin_y) { e->y = e->origin_y; e->vy = -1792; }
    } else {
        if (e->phase == 1) e->phase = 2;
        if (e->phase == 2) {
            if (e->timer) { e->timer--; return; }
            if (e->fired == 2) {
                missile(e, index); e->fired = 1; e->timer = 59;
            } else {
                e->phase = 3; e->timer = 127;
                e->vx = e->direction ? 1024 : -1024;
            }
        } else {
            if (!e->timer) {
                e->fired = 0; e->direction ^= 1; e->timer = 59;
                e->phase = 2; e->vx = 0; return;
            }
            e->timer--;
            move(&e->x, &e->fx, e->vx);
            if (!e->fired && abs16(st.x - e->x) < 96 && visible(e->x, e->y, 24, 12)) {
                e->fired = 2; e->timer = 29; e->phase = 2; e->vx = 0;
            }
        }
    }
}

void sonic_objects_step(void)
{
    const u8 *record = son_objects;
    u8 i, ring = 0;
    if (!st.objects) return;
    if (st.notice) st.notice--;
    for (i = 0; i < son_object_count; i++, record += 6) {
        s16 x, y;
        u8 *state;
        if (record[4] != 0x25) continue;
        state = st.ring_state + ring++;
        if (*state) { if (*state < 17) (*state)++; continue; }
        x = word(record); y = word(record + 2);
        if (!st.hurt && st.invuln < 90 && visible(x, y, 6, 6) && touches(x, y, 6, 6)) {
            *state = 1; st.rings++; st.notice = 180;
        }
    }
    for (i = 0; i < SON_ENEMIES; i++) {
        SonEnemy *e = st.enemies + i;
        s16 w = e->kind == E_MOTO ? 20 : e->kind == E_BUZZ ? 24 : 12;
        s16 h = e->kind == E_BUZZ ? 12 : 16;
        enemy_step(e, i);
        if (!e->phase || e->phase == 1 || e->phase == 5 || st.hurt ||
            !visible(e->x, e->y, w, h) || !touches(e->x, e->y, w, h)) continue;
        if (st.flags & S_ROLL) {
            e->phase = 5; e->flash = 16; st.kills++;
            if (st.vy < 0) st.vy += 256;
            else if (st.y < e->y) st.vy = -st.vy;
            else st.vy -= 256;
        } else {
            u16 resets = st.resets;
            sonic_hurt(e->x);
            if (st.resets != resets) return;
        }
    }
    for (i = 0; i < SON_SHOTS; i++) {
        SonParticle *p = st.shots + i;
        if (!p->life) continue;
        if (p->delay) {
            if (st.enemies[p->parent].phase == 5) { p->life = 0; continue; }
            p->delay--; continue;
        }
        move(&p->x, &p->fx, p->vx); move(&p->y, &p->fy, p->vy); p->life--;
        if (!visible(p->x, p->y, 16, 16)) { p->life = 0; continue; }
        if (touches(p->x, p->y, 6, 6)) {
            u16 resets = st.resets;
            sonic_hurt(p->x);
            if (st.resets != resets) return;
        }
    }
    for (i = 0; i < SON_LOST; i++) {
        SonParticle *p = st.lost + i;
        u8 angle;
        s16 top, feet;
        if (!p->life) continue;
        feet = p->y + 8;
        move(&p->x, &p->fx, p->vx); move(&p->y, &p->fy, p->vy);
        p->vy += 24; p->life--;
        /* Stagger the original expensive floor probes across four frames. */
        if (p->vy >= 0 && !((st.logic + i) & 3)) {
            top = sonic_floor(p->x, feet - 16, p->y + 8, &angle);
            if (top != 32767) { p->y = top - 9; p->vy = -(p->vy - (p->vy >> 2)); }
        }
        if ((u16)p->x >= SON_WORLD_W || p->y > 1200) { p->life = 0; continue; }
        if (!st.hurt && st.invuln < 90 && visible(p->x, p->y, 6, 6) && touches(p->x, p->y, 6, 6)) {
            p->life = 0; st.rings++; st.notice = 180;
        }
    }
}

static void ring_draw(s16 x, s16 y, u8 sparkle)
{
    u8 frame = (st.logic >> 3) & 3;
    if (sparkle) frame = 4 + (((sparkle - 1) >> 2) & 3);
    sonic_art_ring(frame, x, y);
}

void sonic_objects_render(void)
{
    const u8 *record = son_objects;
    u8 i, ring = 0;
#ifndef SON_NO_PARTICLE_CULL
    u8 last[256];
#endif
    if (!st.objects) return;
    for (i = 0; i < son_object_count; i++, record += 6) {
        s16 x = word(record), y = word(record + 2);
        if (record[4] != 0x25) continue;
        if (st.ring_state[ring] < 17 && visible(x, y, 8, 8)) ring_draw(x, y, st.ring_state[ring]);
        ring++;
    }
    for (i = 0; i < SON_ENEMIES; i++) {
        const SonEnemy *e = st.enemies + i;
        s16 w = e->kind == E_MOTO ? 20 : e->kind == E_BUZZ ? 24 : 12;
        s16 h = e->kind == E_BUZZ ? 12 : 16;
        if (e->phase < 2 || !visible(e->x, e->y, w, h)) continue;
        if (e->phase == 5) {
            if (e->flash) ring_draw(e->x, e->y, 1);
            continue;
        }
        { u16 id;
          if (e->kind == E_MOTO) id = ART_MOTO_0_RIGHT + ((st.logic >> 3) & 1) * 2U;
          else if (e->kind == E_BUZZ) id = (e->fired ? ART_BUZZ_4_RIGHT : ART_BUZZ_0_RIGHT) + ((st.logic >> 2) & 1) * 2U;
          else id = ART_CHOP_0_RIGHT + ((st.logic >> 3) & 1) * 2U;
          /* Native mappings face left; mirrored bank entries face right. */
          id += e->direction != 0;
          sonic_art_draw(id, e->x, e->y);
        }
    }
    for (i = 0; i < SON_SHOTS; i++) {
        const SonParticle *p = st.shots + i;
        if (p->life && p->delay <= 17 && visible(p->x, p->y, 8, 8)) {
            u16 id = (p->delay ? ART_MISSILE_0_RIGHT : ART_MISSILE_2_RIGHT) + ((st.logic >> 2) & 1) * 2U;
            id += p->vx > 0;
            sonic_art_draw(id, p->x, p->y);
        }
    }
#ifndef SON_NO_PARTICLE_CULL
    if (!sonic_art_burst()) {
        memset(last, 0, sizeof last);
        for (i = 0; i < SON_LOST; i++) {
            const SonParticle *p = st.lost + i;
            u16 x = (p->x >> 1) - (st.x >> 1) + 8;
            u16 y = (p->y >> 1) - (st.y >> 1) + 8;
            if (p->life && x < 16 && y < 16) last[(y << 4) + x] = i + 1;
        }
#endif
        for (i = 0; i < SON_LOST; i++) {
            const SonParticle *p = st.lost + i;
            if (p->life && visible(p->x, p->y, 8, 8)) {
#ifndef SON_NO_PARTICLE_CULL
                u16 x = (p->x >> 1) - (st.x >> 1) + 8;
                u16 y = (p->y >> 1) - (st.y >> 1) + 8;
                /* Keep the LAST occurrence: intervening overlapping rings may
                 * overwrite earlier ones. This preserves the full draw order. */
                if (x < 16 && y < 16) {
                    if (last[(y << 4) + x] != i + 1) continue;
                }
#endif
                ring_draw(p->x, p->y, 0);
            }
        }
#ifndef SON_NO_PARTICLE_CULL
    }
#endif
    if (st.notice || st.debug) {
        char text[] = "RINGS 00";
        u8 value = st.rings, tens = 0;
        while (value >= 10) { value -= 10; tens++; }
        text[6] = '0' + tens; text[7] = '0' + value;
        draw_rect(110, 90, 50, 10, C_WHITE);
        draw_text(112, 92, text, F_SMALL, C_BLACK);
    }
}

static u32 mix(u32 h, u16 v) { return ((h << 5) | (h >> 27)) ^ v; }
u32 sonic_objects_hash(u32 h)
{
    u8 i;
    h = mix(h, st.rings); h = mix(h, st.hurt); h = mix(h, st.invuln);
    h = mix(h, st.notice); h = mix(h, st.objects); h = mix(h, st.kills);
    for (i = 0; i < SON_RINGS; i++) h = mix(h, st.ring_state[i]);
    for (i = 0; i < SON_ENEMIES; i++) {
        const SonEnemy *e = st.enemies + i;
        h = mix(h, e->x); h = mix(h, e->y); h = mix(h, e->vx); h = mix(h, e->vy);
        h = mix(h, e->origin_y); h = mix(h, e->fx); h = mix(h, e->fy);
        h = mix(h, e->kind); h = mix(h, e->phase); h = mix(h, e->timer);
        h = mix(h, e->direction); h = mix(h, e->fired); h = mix(h, e->flash);
    }
    for (i = 0; i < SON_SHOTS + SON_LOST; i++) {
        const SonParticle *p = i < SON_SHOTS ? st.shots + i : st.lost + i - SON_SHOTS;
        h = mix(h, p->x); h = mix(h, p->y); h = mix(h, p->vx); h = mix(h, p->vy);
        h = mix(h, p->fx); h = mix(h, p->fy); h = mix(h, p->life);
        h = mix(h, p->delay); h = mix(h, p->parent);
    }
    return h;
}
