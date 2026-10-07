#include "yoshi.h"
#include "terrain.h"
#include "art.h"
#include <string.h>
#include "generated/actors_map.h"

/* Native actor integration. Tongue/ingestion phases are measured PAL rules;
   ground-following, activation and patrol endpoints are our target model. */
void actors_reset(void)
{
    YoshiActor *a = yoshi.action.actors;
    const u16 *x = ya_spawn_x, *y = ya_spawn_y;
    u16 count = YA_COUNT;
    memset(&yoshi.action, 0, sizeof(yoshi.action));
    yoshi.action.enabled = 1;
    while (count--) {
        a->x = a->home = *x++;
        a->y = *y++;
        a->state = 16;
        a->timer = 24;
        ++a;
    }
}

void actors_scenario(u16 n)
{
    if (n && (n < 50 || n > 53)) {
        yoshi.action.enabled = 0;
        return;
    }
    if (n >= 50) {
        memset(yoshi.action.actors, 0, sizeof(yoshi.action.actors));
        if (n == 51) yoshi.action.facing = 1;
        if (n == 52) {
            YoshiActor *a = yoshi.action.actors;
            yoshi.x = yoshi.anchor_x = 464;
            yoshi.y = 1888;
            yoshi.x_sub = 0;
            a->x = a->home = 496; a->y = 1904;
            a->state = 16; a->awake = 1; a->timer = 20;
        } else if (n == 53) {
            YoshiActor *a = yoshi.action.actors;
            u16 count;
            yoshi.x = 256;
            for (count = 0; count < YA_COUNT; ++count, ++a) {
                a->x = a->home = 144 + (count << 4) + (count << 3);
                a->y = 1904; a->state = 16; a->awake = 1;
            }
        }
    }
}

static void walk(YoshiActor *a)
{
    s16 sum, floor, old_feet = a->y + 16;
    u16 old_x = a->x;
    if (a->timer) { --a->timer; return; }
    if (!a->shot) {
        /* Unlike Yoshi's X, the Shy Guy integrates its newly updated speed. */
        if (a->vx >= 0) a->vx += a->vx < 90 ? 5 : -5;
        else a->vx += a->vx > -90 ? -5 : 5;
    }
    sum = a->sub + a->vx;
    a->sub = (u8)sum;
    a->x += sum >> 8;
    if (a->shot) {
        a->vy += 64;
        if (a->vy > 1280) a->vy = 1280;
        a->y += a->vy >> 8;
        floor = terrain_actor_floor(a->x + 8, old_feet, a->y + 16);
        if (floor != YT_NONE) { a->y = floor - 16; a->vy = 0; }
        if (a->x > YT_END || a->x < 16 || a->y >= YT_BOTTOM ||
            terrain_solid(a->x + (a->vx < 0 ? 1 : 15), a->y + 8))
            a->state = 0;
        return;
    }
    if (a->x == old_x) return; /* Ground geometry cannot change within one pixel. */
    floor = terrain_actor_floor(a->x + 8, old_feet - 16, old_feet + 16);
    if (floor == YT_NONE || terrain_solid(a->x + (a->vx < 0 ? 1 : 15), a->y + 8) ||
        (s16)a->x < (s16)a->home - 64 || a->x > a->home + 64) {
        a->x = old_x; a->sub = 0;
        a->vx = a->vx >= 0 ? -5 : 5;
    } else {
        a->y = floor - 16;
    }
}

static void tip(s16 *x, s16 *y)
{
    YoshiInteraction *i = &yoshi.action;
    if (i->up) {
        *x = yoshi.x + (i->facing ? -1 : 17);
        *y = yoshi.y + 15 - i->length;
    } else {
        *x = yoshi.x + (i->facing ? 4 - (s16)i->length : 12 + i->length);
        *y = yoshi.y + 20;
    }
}

static void swallow_next(void)
{
    static const u8 durations[8] = {3,3,7,3,3,2,2,3};
    YoshiInteraction *i = &yoshi.action;
    if (i->mouth == 75) {
        i->mouth = i->holding = 0;
    } else {
        i->mouth += 4;
        i->timer = durations[(i->mouth - 47) >> 2];
        if (i->mouth == 67) {
            if (i->eggs < 6) ++i->eggs;
            if (i->holding) i->actors[i->holding - 1].state = 0;
        }
    }
}

void actors_step(s16 direction)
{
    YoshiInteraction *i = &yoshi.action;
    YoshiActor *a;
    u16 n;
    s16 x, y;
    u8 retract;
    if (!i->enabled) return;
    if (direction && !i->mouth) i->facing = direction < 0;
    if (i->swallow) --i->swallow;
    if (i->timer) --i->timer;
    if (i->mouth >= 47) {
        if (!i->timer) swallow_next();
    } else if (!i->mouth) {
        if (i->holding && (input_held(K_DOWN) || !i->swallow)) {
            i->slot = 0; i->mouth = 47; i->timer = 3;
        } else if (!yoshi.baby.recoil && input_pressed(K_B)) {
            i->up = input_held(K_UP) != 0;
            i->mouth = i->up ? 3 : 1;
            if (i->holding) { i->holding = 0; i->timer = 10; }
        }
    }
    if (i->slot && !i->holding && i->mouth < 5 && i->timer == 2) {
        a = i->actors + i->slot - 1;
        a->state = 16; a->shot = 2; a->timer = 0;
        a->x = yoshi.x + (i->facing ? 0 : 16);
        a->y = yoshi.y + 8;
        a->vx = i->facing ? -640 : 640; a->vy = 128;
        if (i->up) { a->vx = i->facing ? -384 : 384; a->vy = -896; }
        i->mouth = i->up = i->slot = 0;
        /* The original sprite release happens after this frame's motion. */
    } else if (i->mouth && i->mouth < 5 && !i->timer) {
        retract = !(i->mouth & 1);
        if (retract) {
            if (i->length) i->length -= 8;
            if (!i->length) { i->mouth = 0; i->up = 0; i->blocked = 0; }
        } else {
            i->length += 8;
            if (i->length == 56 || (!input_held(K_B) && i->length >= (i->up ? 40 : 32))) {
                ++i->mouth; i->timer = 3;
            }
        }
    }
    if (i->length && !i->slot) {
        tip(&x, &y);
        if (terrain_solid((u16)x, (u16)y)) {
            if (i->mouth & 1) ++i->mouth;
            i->blocked = 1; i->timer = 0;
        } else {
            for (n = 0, a = i->actors; n < YA_COUNT; ++n, ++a) {
                if (a->state != 16 || !a->awake || a->shot) continue;
                if ((s16)(x - (s16)a->x - 8) >= -12 && (s16)(x - (s16)a->x - 8) <= 12 &&
                    (s16)(y - (s16)a->y - 8) >= -8 && (s16)(y - (s16)a->y - 8) <= 8) {
                    i->slot = i->holding = n + 1;
                    i->mouth = i->up ? 4 : 2;
                    i->timer = 0; i->swallow = 1200;
                    a->state = 8;
                    break;
                }
            }
        }
    }
    for (n = 0, a = i->actors; n < YA_COUNT; ++n, ++a) {
        if(a->defeated) --a->defeated;
        if (a->state != 16) continue;
        if (a->shot == 2) { a->shot = 1; continue; }
        if (!a->awake) {
            if (a->x > (yoshi.camx << 1) + 320) continue;
            a->awake = 1;
        }
        /* Our streaming policy pauses AI behind the camera's activation margin. */
        if ((s16)a->x + 16 < (s16)(yoshi.camx << 1) - 32) continue;
        walk(a);
    }
}

void actors_render(void)
{
    YoshiInteraction *i = &yoshi.action;
    const YoshiActor *a = i->actors;
    u16 n;
    s16 x, y, sx, sy;
    if (!i->enabled) return;
    for (n = 0; n < YA_COUNT; ++n, ++a) {
        if(a->defeated) {
            art_hit((a->x>>1)-yoshi.camx,((a->y-YT_TOP)>>1)-yoshi.camy);
            continue;
        }
        if (a->state != 16 || !a->awake) continue;
        x = (a->x >> 1) - yoshi.camx;
        y = ((a->y - YT_TOP) >> 1) - yoshi.camy;
        if (x < -16 || x >= RT_W + 4 || y < -16 || y >= RT_H + 4) continue;
        art_shy(x, y, a->vx < 0, !a->timer);
    }
    if (i->length) {
        RtSprite tongue;
        tip(&x, &y);
        sx = (x >> 1) - yoshi.camx;
        sy = ((y - YT_TOP) >> 1) - yoshi.camy;
        n = i->length >> 1;
        if (i->up) {
            u8 pixels[32],mask[32];
            u16 row;
            for(row=0;row<n+3;++row) { pixels[row]=(row && row<n+2)?0x60:0; mask[row]=0x0f; }
            tongue.w=8; tongue.h=n+3; tongue.light=tongue.dark=pixels; tongue.mask=mask;
            draw_sprite(sx-1,sy-1,&tongue);
        } else {
            u32 black[4],mask[4],line=(u32)(0xffffffffUL<<(31-n))>>1;
            if (!i->facing) sx -= n;
            black[0]=black[3]=0; black[1]=black[2]=line;
            mask[0]=mask[1]=mask[2]=mask[3]=~(0xffffffffUL<<(29-n));
            tongue.w=32; tongue.h=4; tongue.light=tongue.dark=black; tongue.mask=mask;
            draw_sprite(sx-1,sy-1,&tongue);
        }
    }
    /* Captured enemies ride the retracting tip all the way into the mouth. */
    if (i->length && i->holding) {
        tip(&x,&y);
        art_shy((x>>1)-yoshi.camx-4,((y-YT_TOP)>>1)-yoshi.camy-4,i->facing,0);
    }
}
