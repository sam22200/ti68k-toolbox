#include "yoshi.h"
#include "terrain.h"
#include "art.h"
#include <string.h>
#if defined(YJ_TRACE) || defined(YJ_ZONES)
#include "../../tools/m68kbench/bench.h"
#endif

YoshiMovement yoshi;
#ifdef YJ_ITEM_TRACE
static void emit_item(u32 value) { BENCH_VALUE(value); }
#endif

void yoshi_reset(void)
{
    /* Clear padding too: portable state files must be deterministic. */
    memset(&yoshi, 0, sizeof(yoshi));
    yoshi.x = 119;
    yoshi.x_sub = 248;
    yoshi.y = 1904;
    yoshi.sub = 65535;
    yoshi.vy = 0;
    yoshi.grounded = 1;
    yoshi.flutter = 1;
    yoshi.native = 1;
    yoshi.camy = 110;
    actors_reset();
    damage_reset();
    items_reset();
}

static void land(u16 top)
{
    yoshi.y = top - 32;
    yoshi.sub = 65535;
    yoshi.vy = 0;
    if (!yoshi.grounded) yoshi.phase_timer = 5;
    yoshi.grounded = 1;
    yoshi.jump = 0;
}

static void vertical(u8 held, u8 pressed)
{
    s16 sum, floor, old_feet = yoshi.y + 32;
    u8 old_jump = yoshi.jump;
    u8 active = 0;
    if (yoshi.grounded) {
        yoshi.flutter = 1;
        if (yoshi.native) {
            floor = terrain_floor(yoshi.x, old_feet - 16, old_feet + 16, &yoshi.angle);
            if (floor != YT_NONE) land(floor);
            else {
                yoshi.grounded = 0;
                yoshi.jump = 8;
                yoshi.angle = 0;
            }
        }
    } else {
        if (!yoshi.flutter && pressed && yoshi.cooldown < 8)
            yoshi.flutter = 1;
        if (yoshi.flutter && !yoshi.cooldown) {
            if (yoshi.flutter == 1) {
                if (held && yoshi.vy > 352) yoshi.flutter = 2;
            }
            if (yoshi.flutter >= 2) {
                if (!held || yoshi.vy <= -384) {
                    yoshi.flutter = 0;
                    yoshi.cooldown = 15;
                    yoshi.jump = 6;
                } else {
                    active = 1;
                    if (!yoshi.phase_timer) {
                        yoshi.phase_timer = 2;
                        yoshi.flutter += 2;
                        if (yoshi.flutter > 8) yoshi.flutter = 4;
                    }
                    sum = yoshi.vy - 272;
                    if (sum < 0) sum = 0;
                    yoshi.vy -= 16 + ((u16)sum >> 3);
                }
            }
        }
        if (!active && !yoshi.head_timer) yoshi.vy += held ? 50 : 200;
        if (yoshi.vy > 1280) yoshi.vy = 1280;
        if (yoshi.vy >= 0 && yoshi.flutter < 2) {
            if (old_jump < 8 && !yoshi.phase_timer) {
                ++old_jump;
                yoshi.phase_timer = 8;
            }
            yoshi.jump = old_jump;
        }
        /* The whole signed intermediate is stored, only its low byte reused. */
        sum = (s16)(yoshi.sub & 255) + yoshi.vy;
        yoshi.sub = (u16)sum;
        yoshi.y += sum >> 8;
        if (yoshi.native) {
            yoshi.angle = 0;
            if (yoshi.vy <= 0) {
                u16 head = yoshi.y + 4;
                if (terrain_solid(yoshi.x + 6, head) || terrain_solid(yoshi.x + 10, head)) {
                    ++yoshi.y;
                    if (yoshi.vy < 0) {
                        yoshi.vy = 0;
                        yoshi.flutter = 1;
                        yoshi.head_timer = 8;
                        yoshi.cooldown = 20;
                    }
                }
            } else {
                floor = terrain_floor(yoshi.x, old_feet, yoshi.y + 32, &yoshi.angle);
                if (floor != YT_NONE) land(floor);
            }
        } else if (yoshi.y >= 1904) {
            land(1936);
        }
    }
    /* The original accepts a new press on the landing frame as well. */
    if (yoshi.grounded && pressed) {
        yoshi.vy = -1328;
        yoshi.grounded = 0;
        yoshi.jump = 6;
        yoshi.flutter = 1;
    }
}

void yoshi_step(s16 direction, u8 held, u8 pressed)
{
    s16 sum, target, accel;
    u16 old_x = yoshi.x;
    if (yoshi.phase_timer) --yoshi.phase_timer;
    if (yoshi.cooldown) --yoshi.cooldown;
    if (yoshi.skid) --yoshi.skid;
    if (yoshi.head_timer) --yoshi.head_timer;
    if (yoshi.anchor) yoshi.x = old_x = yoshi.anchor_x;
    /* Position uses the previous horizontal speed, unlike the vertical axis. */
    sum = (s16)(yoshi.x_sub & 255) + yoshi.vx;
    yoshi.x_sub = (u16)sum;
    yoshi.x += sum >> 8;
    if (yoshi.native && yoshi.x != old_x) {
        u16 target_x = yoshi.x;
        s16 dx = yoshi.vx < 0 ? -1 : 1;
        yoshi.x = old_x;
        while (yoshi.x != target_x) {
            u16 next = yoshi.x + dx, front = next + (dx < 0 ? 1 : 15);
            if (terrain_solid(front, yoshi.y + 9) || terrain_solid(front, yoshi.y + 23)) {
                yoshi.x_sub = 0;
                yoshi.vx = 0;
                break;
            }
            yoshi.x = next;
        }
    }
    if (!yoshi.anchor) {
        if (yoshi.x < 16 || yoshi.x > 32767) {
            yoshi.x = 16;
            yoshi.x_sub = 0;
            yoshi.vx = 0;
        } else if (yoshi.x > 1264) {
            yoshi.x = 1264;
            yoshi.x_sub = 0;
            yoshi.vx = 0;
        }
    }
    vertical(held, pressed);
    target = yoshi.flutter >= 2 ? 480 : 960;
    if (direction) {
        if ((direction > 0 && yoshi.vx < 0) ||
            (direction < 0 && yoshi.vx > 0)) yoshi.skid = 4;
        if (direction < 0) target = -target;
        if ((direction > 0 && yoshi.vx < target) ||
            (direction < 0 && yoshi.vx > target)) {
            accel = yoshi.skid ? 70 : 20;
            yoshi.vx += direction > 0 ? accel : -accel;
            return;
        }
    } else if (!yoshi.grounded && yoshi.vx > -target && yoshi.vx < target) {
        return; /* Releasing direction in the air preserves momentum. */
    }
    if (yoshi.vx > 0) {
        yoshi.vx -= 40;
        if (yoshi.vx < 0) yoshi.vx = 0;
    } else if (yoshi.vx < 0) {
        yoshi.vx += 40;
        if (yoshi.vx > 0) yoshi.vx = 0;
    }
}

void game_init(void)
{
#ifdef YJ_ZONES
    BENCH_NAME(3, "terrain"); BENCH_NAME(4, "actors");
    BENCH_NAME(5, "baby"); BENCH_NAME(6, "counter");
#endif
    terrain_init();
    art_init();
    yoshi_reset();
    rt_state = &yoshi;
    rt_state_size = sizeof(yoshi);
}

void game_scenario(u16 n)
{
    yoshi_reset();
    if ((n >= 1 && n <= 3) || n == 10 || n == 11) yoshi.native = 0;
    if (n == 1) {
        yoshi.y = 1838;
        yoshi.sub = 0;
        yoshi.vy = -28;
        yoshi.grounded = 0;
        yoshi.jump = 6;
    } else if (n == 2) {
        yoshi.y = 1898;
        yoshi.sub = 128;
        yoshi.vy = 1280;
        yoshi.grounded = 0;
        yoshi.jump = 8;
    } else if (n == 3) {
        yoshi.y = 1858;
        yoshi.sub = 32;
        yoshi.vy = -57;
        yoshi.grounded = 0;
        yoshi.jump = 7;
        yoshi.flutter = 4;
        yoshi.phase_timer = 2;
    } else if (n == 10) {
        /* Same disclosed X anchor as the original controlled measurements. */
        yoshi.anchor = 1;
        yoshi.anchor_x = 119;
    } else if (n >= 20 && n < 47) {
        static const u16 xs[27] = {119,32,47,160,175,195,208,240,420,480,604,632,
                                  744,790,840,860,895,925,950,980,1008,1040,1118,1152,1184,1240,1264};
        u16 x = xs[n - 20];
        s16 low = x == 744 || x == 1152 ? YT_TOP : 1800;
        s16 floor = terrain_floor(x, low, YT_BOTTOM - 1, RT_NULL);
        yoshi_drop(x, floor - 72);
    } else if (n == 12) {
        yoshi_drop(744, 1708);
        yoshi.vy = -1328;
    } else if (n == 13) {
        yoshi_drop(1008, 1672);
        yoshi.vy = -1328;
    } else if (n == 14) {
        yoshi.x = 1120;
        yoshi.y = 1856;
    } else if (n == 15) {
        yoshi.x = 1220;
        yoshi.y = 1872;
    }
    actors_scenario(n);
    damage_scenario(n);
    items_scenario(n);
}

void yoshi_drop(u16 x, u16 y)
{
    yoshi_reset();
    yoshi.x = yoshi.anchor_x = x;
    yoshi.y = y;
    yoshi.sub = yoshi.x_sub = 0;
    yoshi.grounded = 0;
    yoshi.jump = 6;
    yoshi.anchor = 1;
}

static void camera(void)
{
    s16 target = (s16)yoshi.y - 144;
    yoshi.camx = yoshi.x > 144 ? (yoshi.x - 144) >> 1 : 0;
    if (yoshi.camx > 480) yoshi.camx = 480;
    if (target < YT_TOP) target = YT_TOP;
    if (target > YT_BOTTOM - 200) target = YT_BOTTOM - 200;
    yoshi.camy = ((u16)target - YT_TOP) >> 1;
}

u8 game_update(void)
{
    u16 old_feet = yoshi.y + 32;
    if (input_pressed(K_ESC)) return 0;
    if (input_pressed(K_ENTER)) yoshi_reset();
    if ((yoshi.native && !terrain_ready()) || yoshi.won || yoshi.baby.mode == YB_FAILED) goto trace;
    if (!damage_clock()) { if (yoshi.baby.mode == YB_RETURN) damage_step(old_feet); goto trace; }
    yoshi_step(yoshi.baby.recoil ? 0 : (input_held(K_RIGHT) != 0) - (input_held(K_LEFT) != 0),
               !yoshi.baby.recoil && input_held(K_A) != 0,
               !yoshi.baby.recoil && input_pressed(K_A) != 0);
    actors_step((input_held(K_RIGHT) != 0) - (input_held(K_LEFT) != 0));
    items_step();
    damage_step(old_feet);
    if (yoshi.native) {
        if (yoshi.y >= YT_BOTTOM) yoshi_reset();
        if (!yoshi.anchor && yoshi.x >= YT_END && yoshi.grounded &&
            yoshi.baby.mode == YB_ATTACHED) yoshi.won = 1;
        camera();
    }
trace:
#ifdef YJ_ITEM_TRACE
    items_trace(emit_item);
#endif
#ifdef YJ_TRACE
    BENCH_VALUE(((u32)yoshi.x << 16) | yoshi.x_sub);
    BENCH_VALUE(((u32)(u16)yoshi.vx << 16) | (u16)yoshi.vy);
    BENCH_VALUE(((u32)yoshi.y << 16) | yoshi.sub);
    BENCH_VALUE(((u32)yoshi.flutter << 24) | ((u32)yoshi.phase_timer << 16) |
                ((u16)yoshi.cooldown << 8) | yoshi.jump);
    BENCH_VALUE(((u16)yoshi.skid << 8) | yoshi.grounded);
#ifdef YJ_TERR_TRACE
    BENCH_VALUE(((u32)yoshi.camx << 16) | yoshi.camy);
    BENCH_VALUE(((u32)yoshi.angle << 24) | ((u32)yoshi.won << 16) |
                ((u16)yoshi.head_timer << 8) | yoshi.native);
#endif
#endif
#ifdef YJ_ACTOR_TRACE
    {
    const YoshiActor *a = yoshi.action.actors;
    u16 n;
    BENCH_VALUE(((u32)yoshi.action.mouth << 24) | ((u32)yoshi.action.length << 16) |
                ((u16)yoshi.action.timer << 8) | yoshi.action.up);
    BENCH_VALUE(((u32)yoshi.action.swallow << 16) | ((u16)yoshi.action.slot << 8) |
                yoshi.action.holding);
    BENCH_VALUE(((u32)yoshi.action.enabled << 24) | ((u32)yoshi.action.facing << 16) |
                ((u16)yoshi.action.blocked << 8) | yoshi.action.eggs);
    for (n = 0; n < YA_COUNT; ++n, ++a) {
        BENCH_VALUE(((u32)a->x << 16) | a->y);
        BENCH_VALUE(((u32)(u16)a->vx << 16) | ((u16)a->sub << 8) | a->state);
        BENCH_VALUE(((u32)a->home << 16) | (u16)a->vy);
        BENCH_VALUE(((u32)a->defeated << 24) | ((u32)a->awake << 16) | ((u16)a->timer << 8) | a->shot);
    }
    }
#endif
#ifdef YJ_DAMAGE_TRACE
    {
        const YoshiBaby *b = &yoshi.baby;
        BENCH_VALUE(((u32)b->x << 16) | b->y);
        BENCH_VALUE(((u32)(u16)b->vx << 16) | (u16)b->vy);
        BENCH_VALUE(((u32)b->remaining << 16) | b->tick);
        BENCH_VALUE(((u32)b->mode << 24) | ((u32)b->phase << 16) | ((u16)b->age << 8) | b->invincible);
        BENCH_VALUE(((u32)b->xsub << 24) | ((u32)b->ysub << 16) | ((u16)b->recoil << 8) | b->recharge);
        BENCH_VALUE(((u32)b->leftward << 24) | ((u32)b->enabled << 16) | ((u16)b->hits << 8) | b->rescues);
    }
#endif
    return 1;
}

void game_render(void)
{
    s16 sy = 70 + (((s16)yoshi.y - 1904) >> 1);
    u16 camera = yoshi.x > 144 ? yoshi.x - 144 : 0;
    s16 sx;
    if (camera > 960) camera = 960;
    sx = (yoshi.x - camera) >> 1;
    if (yoshi.native) {
        if (!terrain_ready() || !art_ready()) {
            draw_clear();
            draw_text(8, 42, "yjterr + yjart REQUIRED", F_SMALL, C_BLACK);
            return;
        }
#ifdef YJ_ZONES
        BENCH_BEGIN(3);
#endif
        terrain_render(yoshi.camx, yoshi.camy);
#ifdef YJ_ZONES
        BENCH_END(3); BENCH_BEGIN(4);
#endif
        items_render();
        art_hero();
        actors_render();
        damage_render();
        items_aim_render();
#ifdef YJ_ZONES
        BENCH_END(4);
#endif
        if (yoshi.won) {
            draw_rect(13, 30, 134, 22, C_WHITE);
            draw_text(25, 38, "1-1 SLICE CLEAR", F_SMALL, C_BLACK);
        }
        return;
    }
    draw_clear();
    draw_text(4, 3, "YOSHI: MOVEMENT PROBE", F_SMALL, C_BLACK);
    draw_text(4, 12, "arrows + 2nd; ENTER reset", F_SMALL, C_DGRAY);
    draw_rect(0, 77, 160, 23, C_LGRAY);
    draw_rect(0, 77, 160, 2, C_BLACK);
    /* Diagnostic actor with a white border; ROM art is a later milestone. */
    draw_rect(sx - 7, sy - 14, 14, 22, C_WHITE);
    draw_rect(sx - 5, sy - 12, 10, 18, C_BLACK);
}
