#include "yoshi.h"
#include "terrain.h"
#include "art.h"
#include <string.h>

#define BABY_TIME (10 * RT_HZ)
/* Wall/virtual clock anchor is not serialized with raw PC game snapshots.
   Loading a previous reserve must not charge elapsed time since that save. */
static u16 last_tick;

void damage_reset(void)
{
    YoshiBaby *b = &yoshi.baby;
    memset(b, 0, sizeof(*b));
    b->remaining = BABY_TIME;
    last_tick = b->tick = rt_ticks();
    b->enabled = 1;
}

void damage_scenario(u16 n)
{
    YoshiBaby *b = &yoshi.baby;
    /* Earlier doors remain controlled measurements of isolated mechanics. */
    if (n && (n < 54 || n > 58)) b->enabled = 0;
    if (n >= 54 && n <= 58) {
        YoshiActor *a = yoshi.action.actors;
        yoshi.x = 490; yoshi.y = 1888; yoshi.x_sub = 0;
        memset(a, 0, sizeof(yoshi.action.actors));
        yoshi.action.enabled = 1;
        if (n == 54) {
            a->x = a->home = 496; a->y = 1904;
            a->state = 16; a->awake = 1; a->timer = 20;
        } else {
            b->mode = YB_LOST; b->phase = 11; b->age = 80;
            b->x = 390; b->y = 1816; b->vx = -187; b->vy = -319;
            if (n == 56) { b->x = 498; b->y = 1888; }
            if (n == 57) b->remaining = 5;
            if (n == 58) {
                u16 k;
                yoshi.x = 256; yoshi.y = 1904;
                b->x = 300; b->y = 1836;
                yoshi.action.eggs = 6;
                for (k = 0; k < YA_COUNT; ++k, ++a) {
                    a->x = a->home = 144 + (k << 4) + (k << 3);
                    a->y = 1904; a->state = 16; a->awake = 1;
                }
            }
        }
        yoshi.camx = (yoshi.x - 144) >> 1;
        yoshi.camy = (yoshi.y - 144 - YT_TOP) >> 1;
    }
}

u8 damage_clock(void)
{
    YoshiBaby *b = &yoshi.baby;
    u16 now, elapsed;
    if (!b->enabled) return 1;
    now = rt_ticks(); elapsed = now - last_tick;
    last_tick = now;
    b->tick = now;
    if (b->mode != YB_RETURN) {
        if (b->invincible) --b->invincible;
        if (b->recoil) --b->recoil;
    }
    if (b->mode == YB_LOST) {
        if (elapsed >= b->remaining) {
            b->remaining = 0; b->mode = YB_FAILED;
            yoshi.vx = yoshi.vy = 0;
            return 0;
        }
        b->remaining -= elapsed;
    } else if (b->mode == YB_ATTACHED && b->remaining < BABY_TIME) {
        /* PAL recharges one tenth every twelve updates. Our reserve is in
           real clock ticks, so one tenth becomes 26 ticks (no division). */
        if (++b->recharge == 12) {
            b->recharge = 0; b->remaining += 26;
            if (b->remaining > BABY_TIME) b->remaining = BABY_TIME;
        }
    }
    return b->mode != YB_FAILED && b->mode != YB_RETURN;
}

static void hit(const YoshiActor *a)
{
    YoshiBaby *b = &yoshi.baby;
    YoshiInteraction *i = &yoshi.action;
    if (i->holding) i->actors[i->holding - 1].state = 0;
    i->mouth = i->length = i->timer = i->up = i->slot = i->holding = i->blocked = 0;
    i->swallow = 0;
    yoshi.items.aim = yoshi.items.throwing = yoshi.items.locked = 0;
    b->invincible = b->mode == YB_ATTACHED ? 160 : 128;
    b->recoil = 8; b->recharge = 0;
    ++b->hits;
    yoshi.vx = yoshi.x < a->x ? -640 : 640;
    yoshi.vy = -1162;
    yoshi.grounded = 0; yoshi.jump = 8; yoshi.flutter = 0;
    yoshi.phase_timer = yoshi.cooldown = yoshi.head_timer = 0;
    if (b->mode == YB_ATTACHED) b->mode = YB_PENDING;
}

static void bubble(void)
{
    YoshiBaby *b = &yoshi.baby;
    s16 sum, left = yoshi.camx << 1, top = YT_TOP + (yoshi.camy << 1) + 16;
    if (b->age < 255) ++b->age;
    if (!b->phase) b->vy += 80;
    else if (b->phase == 10) {
        b->vx += b->vx < 0 ? 10 : -10;
        b->vy -= 40;
    } else {
        if (b->leftward) { if (b->vx > -161) b->vx -= 2; }
        else if (b->vx < 161) b->vx += 2;
        if (b->vy > -319) b->vy -= 4;
        if (b->vy < -319) b->vy = -319;
    }
    sum = b->xsub + b->vx; b->xsub = (u8)sum; b->x += sum >> 8;
    sum = b->ysub + b->vy; b->ysub = (u8)sum; b->y += sum >> 8;
    if (!b->phase && b->age == 23) b->phase = 10;
    else if (b->phase == 10 && b->vy <= 0) { b->phase = 11; b->vy = -290; }
    /* Camera-edge reflection is a native adaptation to the smaller LCD.
       The bubble stays reachable, and never needs original PPU effects. */
    if ((s16)b->x < left + 8) { b->x = left + 8; b->vx = 161; b->leftward = 0; }
    else if (b->x > (u16)(left + 296)) { b->x = left + 296; b->vx = -161; b->leftward = 1; }
    if (b->y < (u16)top) { b->y = top; b->vy = 320; }
    else if (b->y > (u16)(top + 156)) { b->y = top + 156; b->vy = -319; }
}

static u8 near(s16 x, s16 y, u16 bx, u16 by, u16 width, u16 height)
{
    /* Unsigned biased ranges generate comparisons rather than abs helpers. */
    return (u16)(x - (s16)bx + width) <= (width << 1) &&
           (u16)(y - (s16)by + height) <= (height << 1);
}

void damage_step(u16 old_feet)
{
    YoshiBaby *b = &yoshi.baby;
    YoshiInteraction *i = &yoshi.action;
    YoshiActor *a;
    u16 k;
    s16 tx, ty;
    if (!b->enabled) return;
    if (b->mode == YB_RETURN) {
        b->x += ((s16)yoshi.x + 4 - (s16)b->x) >> 2;
        b->y += ((s16)yoshi.y - (s16)b->y) >> 2;
        if (!--b->age) { b->mode = YB_ATTACHED; ++b->rescues; }
        return;
    }
    if (b->mode == YB_PENDING) {
        b->mode = YB_LOST; b->phase = b->age = b->xsub = b->ysub = 0;
        b->x = yoshi.x; b->y = yoshi.y;
        b->vx = i->facing ? 435 : -435; b->vy = -1162;
        b->leftward = i->facing;
        last_tick = b->tick = rt_ticks();
    } else if (b->mode == YB_LOST) {
        bubble();
        if (b->age >= 32) {
            tx = yoshi.x; ty = yoshi.y + 8;
            if (!near(tx, ty, b->x, b->y, 14, 22) && i->length) {
                tx = yoshi.x + (i->up ? (i->facing ? -1 : 17) :
                                     (i->facing ? 4 - (s16)i->length : 12 + i->length));
                ty = yoshi.y + (i->up ? 15 - (s16)i->length : 20);
                tx -= 8; ty -= 8;
            }
            if (near(tx,ty,b->x,b->y,14,22)) {
                b->mode = YB_RETURN; b->age = 32; b->recharge = 0;
                i->mouth = i->length = i->timer = i->up = i->blocked = 0;
                yoshi.vx = 0;
                return;
            }
        }
    }
    if (b->invincible || !i->enabled) return;
    for (k = 0, a = i->actors; k < YA_COUNT; ++k, ++a) {
        if (a->state != 16 || !a->awake || a->shot) continue;
        if ((u16)(yoshi.x - a->x + 13) > 26 ||
            yoshi.y + 30 < a->y || yoshi.y + 4 >= a->y + 16) continue;
        if (yoshi.vy > 0 && old_feet <= a->y + 8) {
            /* Native stomp response: Shy Guy disappears, no baby loss. */
            a->state = 0; a->defeated = 8; yoshi.vy = -664; yoshi.grounded = 0;
            yoshi.flutter = 1; yoshi.jump = 6;
        } else hit(a);
        break;
    }
}

void damage_render(void)
{
    const YoshiBaby *b = &yoshi.baby;
    u16 seconds;
    if (!b->enabled) return;
    if (b->mode == YB_LOST || b->mode == YB_RETURN)
        art_baby((b->x >> 1) - yoshi.camx,
                 (((s16)b->y - YT_TOP) >> 1) - yoshi.camy, b->mode == YB_LOST);
    seconds = (b->remaining + 255) >> 8;
    art_counter(seconds, b->mode == YB_LOST);
    if (b->mode == YB_FAILED) {
        draw_rect(15, 30, 130, 33, C_WHITE);
        draw_text(31, 36, "BABY MARIO LOST", F_SMALL, C_BLACK);
        draw_text(42, 50, "ENTER: RETRY", F_SMALL, C_BLACK);
    }
}
