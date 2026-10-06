#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "sonic.h"

typedef struct {
    u16 keys, x, y;
    u8 fx, fy;
    s16 vx, vy, speed;
    u8 flags, radius;
} RefFrame;
typedef struct { const char *name; const RefFrame *frames; u16 count; } RefCase;
#include "generated/physics_ref.h"
typedef struct { s16 x, y; u8 fx, fy; s16 vx, vy; } ObjectRef;
#include "generated/objects_ref.h"

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static void reference_actions(void)
{
    u16 c, i;
    for (c = 0; c < sizeof ref_cases / sizeof ref_cases[0]; c++) {
        const RefCase *test = &ref_cases[c];
        u16 old = 0;
        sw_init(1);
        CHECK(st.ready);
        for (i = 0; i < test->count; i++) {
            const RefFrame *r = test->frames + i;
            s32 x, y, rx, ry;
            sonic_step(r->keys, (r->keys & ~old & K_A) != 0);
            old = r->keys;
            x = (s32)st.x * 256 + st.fx; rx = (s32)r->x * 256 + r->fx;
            y = (s32)st.y * 256 + st.fy; ry = (s32)r->y * 256 + r->fy;
            /* Subpixel collision residue is deliberately not a bit-exact goal.
             * Velocities/flags and X are exact for these measured flat actions;
             * Y tolerance is one original pixel, half a target pixel. */
            if (st.vx != r->vx || st.vy != r->vy || st.speed != r->speed ||
                (st.flags & 7) != (r->flags & 7) || labs(x - rx) > 1 || labs(y - ry) > 256) {
                printf("reference %s frame %u: pos %ld,%ld / %ld,%ld vel %d,%d,%d / %d,%d,%d flags %u/%u\n",
                    test->name, i, (long)x, (long)y, (long)rx, (long)ry,
                    st.vx, st.vy, st.speed, r->vx, r->vy, r->speed, st.flags, r->flags);
                failures++; break;
            }
        }
    }
}

static void traversal(void)
{
    u16 i;
    u8 crossed_bridge = 0;
    sw_init(0);
    st.objects = 0; /* Keep the previous terrain-only regression as well. */
    for (i = 0; i < 260 && !st.finished; i++) {
        sw_step(K_RIGHT);
        if (st.flags & S_BRIDGE) crossed_bridge = 1;
        CHECK(st.y < 1200 && st.resets == 0);
    }
    CHECK(crossed_bridge && st.finished && st.x == SON_END);
    CHECK(st.camx <= (SON_WORLD_W >> 1) - RT_W && st.camy < 1024);
    { Sonic final = st; sw_step(K_A | K_RIGHT); CHECK(!memcmp(&final, &st, sizeof st)); }
    sw_step(K_ENTER);
    CHECK(!st.finished && st.x == 80);
    CHECK(sw_step(K_ESC) == 0);
}

static void collisions(void)
{
    u16 i;
    u8 angle = 0;
    sw_init(1);
    CHECK(sonic_floor(192, 950, 970, &angle) == 960 && angle == 0);
    CHECK(!sonic_solid(192, 959, 0) && sonic_solid(192, 960, 0));
    CHECK(sonic_floor(1184, 880, 910, &angle) == SON_BRIDGE_Y);
    CHECK(!sonic_solid(1184, SON_BRIDGE_Y + 3, 1)); /* bridge is one-way */
    sonic_start(1184, 930); st.flags = S_AIR;
    st.objects = 0;
    st.vy = -SON_JUMP_SPEED;
    for (i = 0; i < 7; i++) sonic_step(K_A, 0);
    CHECK(st.flags & S_AIR); /* passes upward through bridge */
    for (i = 0; i < 80 && (st.flags & S_AIR); i++) sonic_step(0, 0);
    CHECK(!(st.flags & S_AIR) && st.y == SON_BRIDGE_Y - 20);
    sw_init(6);
    for (i = 0; i < 150 && !st.resets; i++) sw_step(0);
    CHECK(st.resets == 1 && st.x == 80);
    /* Slope is part of the actual start terrain, not a flat surrogate. */
    sw_init(0); CHECK(st.angle != 0);
    /* Real solid banks beneath the bridge: horizontal wall and underside. */
    sonic_start(1250, 950); st.flags = S_AIR; st.vx = SON_TOP_SPEED;
    for (i = 0; i < 4; i++) sonic_step(0, 0);
    CHECK(st.x == 1270 && st.vx == 0);
    sonic_start(1078, 960); st.flags = S_AIR; st.vy = -SON_JUMP_SPEED;
    for (i = 0; i < 3; i++) sonic_step(0, 0);
    CHECK(st.vy == 0 && st.y >= 947);
    /* Repeated held jump cannot turn landing into an automatic second jump. */
    sw_init(1); sw_step(K_A);
    for (i = 0; i < 70; i++) sw_step(K_A);
    CHECK(!(st.flags & S_AIR));
}

static void original_actors(void)
{
    const ObjectRef *refs[] = { objref_moto, objref_buzz, objref_chop };
    const u16 counts[] = { 40, 110, 160 };
    const u8 kinds[] = { E_MOTO, E_BUZZ, E_CHOP };
    const s16 xs[] = { 256, 288, 256 }, ys[] = { 940, 864, 1120 };
    u16 n, f;
    for (n = 0; n < 3; n++) {
        SonEnemy *e;
        sw_init(1); st.invuln = 255;
        memset(st.enemies, 0, sizeof st.enemies);
        st.enemies[1].phase = st.enemies[2].phase = 5;
        e = st.enemies;
        e->kind = kinds[n]; e->x = xs[n]; e->y = e->origin_y = ys[n];
        for (f = 0; f < counts[n]; f++) {
            const ObjectRef *r = refs[n] + f;
            sonic_objects_step();
            if (e->x != r->x || e->y != r->y || e->fx != r->fx || e->fy != r->fy ||
                e->vx != r->vx || e->vy != r->vy) {
                printf("actor %u frame %u: %d,%d +%u,%u vel %d,%d / %d,%d +%u,%u vel %d,%d\n",
                    n, f, e->x, e->y, e->fx, e->fy, e->vx, e->vy,
                    r->x, r->y, r->fx, r->fy, r->vx, r->vy);
                failures++; break;
            }
            if (n == 1 && f == 31) CHECK(st.shots[0].life && st.shots[0].delay == 30);
            if (n == 1 && f == 61) CHECK(st.shots[0].x == 260 && !st.shots[0].delay);
            if (n == 1 && f == 62) CHECK(st.shots[0].x == 258 && st.shots[0].y == 894);
        }
    }
}

static void interactions(void)
{
    u16 i;
    sw_init(8); sonic_objects_step();
    CHECK(st.rings == 1 && st.ring_state[0] == 1 && st.notice);
    sonic_objects_step(); CHECK(st.rings == 1); /* no duplicate collection */
    st.x = 348; sonic_objects_step(); CHECK(st.rings == 2);
    st.x = 324; sonic_objects_step(); CHECK(st.rings == 2); /* persisted */

    sw_init(1); st.rings = 10; sonic_hurt(st.x + 10);
    CHECK(st.hurt && st.invuln == 120 && st.rings == 0 && st.vx == -512 && st.vy == -1024);
    for (i = 0; i < 10; i++) CHECK(st.lost[i].life == 255);
    CHECK(!st.lost[10].life && st.lost[0].vx == -st.lost[1].vx);
    sonic_hurt(st.x - 10); CHECK(st.vx == -512 && !st.resets);
    { s16 vx = st.vx; sonic_step(K_RIGHT | K_A, 1); CHECK(st.vx == vx && st.vy == -976); }
    for (i = 0; i < 80 && st.hurt; i++) sonic_step(K_RIGHT, 0);
    CHECK(!st.hurt && !(st.flags & S_AIR) && st.vx == 0 && st.invuln == 120);
    st.x = 192; st.y = 940; st.invuln = 90;
    st.lost[0].x = st.x; st.lost[0].y = st.y;
    st.lost[0].vx = st.lost[0].vy = 0;
    st.hurt = 1; sonic_objects_step(); CHECK(!st.rings); /* cannot immediately reclaim */
    st.hurt = 0; sonic_objects_step(); CHECK(!st.rings); /* 90-frame boundary */
    st.hurt = 0; st.invuln = 89;
    sonic_objects_step(); CHECK(st.rings >= 1 && !st.lost[0].life);
    st.invuln = 0; st.rings = 0; sonic_hurt(st.x - 10);
    CHECK(st.resets == 1 && st.x == 80 && !st.hurt && !st.invuln && !st.rings);

    sw_init(1);
    memset(st.enemies, 0, sizeof st.enemies);
    st.enemies[1].phase = st.enemies[2].phase = 5;
    st.enemies[0].kind = E_MOTO; st.enemies[0].phase = 2;
    st.enemies[0].timer = 60; st.enemies[0].x = st.x; st.enemies[0].y = st.y + 10;
    st.flags = S_ROLL | S_AIR; st.vy = 768;
    sonic_objects_step();
    CHECK(st.kills == 1 && st.enemies[0].phase == 5 && st.vy == -768 && !st.resets);
    st.shots[0].life = 100; st.shots[0].delay = 10; st.shots[0].parent = 0;
    sonic_objects_step(); CHECK(st.kills == 1 && !st.shots[0].life); /* cancel charging missile */
    st.rings = 2; st.shots[0].life = 100; st.shots[0].delay = 0;
    st.shots[0].x = st.x; st.shots[0].y = st.y;
    sonic_objects_step(); CHECK(st.hurt && !st.rings && !(st.flags & S_ROLL)); /* balls do not stop missiles */
    sonic_start(192, 940); st.rings = 255; sonic_hurt(210);
    for (i = 0; i < SON_LOST; i++) CHECK(st.lost[i].life == 255);
    for (i = 0; i < 256; i++) { st.logic++; sonic_objects_step(); }
    for (i = 0; i < SON_LOST; i++) CHECK(!st.lost[i].life);
    game_scenario(0); CHECK(st.objects && !st.kills && !st.ring_state[0]);
}

static void playable_traversal(void)
{
    u16 i;
    static SwScript script;
    sw_init(0);
    for (i = 0; i < 180 && !st.resets; i++) sw_step(K_RIGHT);
    CHECK(st.resets == 1); /* Walking through a badnik with zero rings is lethal. */
    CHECK(sw_load_script(&script, "keys/play.txt") == 0);
    sw_init(0);
    for (i = 0; i < 340 && !st.finished; i++) sw_step(sw_script_keys(&script, i));
    printf("play: x%d rings%u kills%u resets%u finished%u frames%u\n", st.x, st.rings, st.kills, st.resets, st.finished, i);
    CHECK(st.finished && !st.resets && st.kills && st.rings);
}

static void scripts(void)
{
    static const char *names[] = { "keys/jump.txt", "keys/roll.txt" };
    static SwScript script;
    u16 n, i;
    for (n = 0; n < 2; n++) {
        Sonic expected;
        u16 screen;
        CHECK(sw_load_script(&script, names[n]) == 0);
        sw_init(0);
        for (i = 0; i < 240; i++) sw_step(sw_script_keys(&script, i));
        expected = st; screen = sw_checksum();
        CHECK(st.ready && st.x >= 16 && st.x <= SON_END && st.resets < 3);
        sw_load_script(&script, names[n]); sw_init(0);
        for (i = 0; i < 240; i++) sw_step(sw_script_keys(&script, i));
        CHECK(!memcmp(&expected, &st, sizeof st) && screen == sw_checksum());
    }
}

int main(int argc, char **argv)
{
    if ((argc == 4 || argc == 5) && (!strcmp(argv[1], "--hash") || !strcmp(argv[1], "--trace") || !strcmp(argv[1], "--planes"))) {
        static SwScript script;
        u16 i, frames = atoi(argv[3]);
        if (sw_load_script(&script, argv[2])) return 1;
        sw_init(argc == 5 ? atoi(argv[4]) : 0);
        for (i = 0; i < frames; i++) {
            sw_step(sw_script_keys(&script, i));
            if (!strcmp(argv[1], "--trace")) {
                printf("%u x%d y%d vx%d vy%d flags%u rings%u kills%u hurt%u inv%u resets%u moto%d,%d\n",
                    i, st.x, st.y, st.vx, st.vy, st.flags, st.rings, st.kills, st.hurt,
                    st.invuln, st.resets, st.enemies[0].x, st.enemies[0].y);
            } else if (!strcmp(argv[1], "--planes")) printf("%04X\n", sw_checksum());
            else printf("%ld\n", (long)(s32)sonic_hash());
        }
        return 0;
    }
    reference_actions(); traversal(); collisions(); original_actors(); interactions(); scripts(); playable_traversal();
    printf("Sonic: %d failures\n", failures);
    return failures != 0;
}
