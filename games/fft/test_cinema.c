// Actual validation doors plus exact combat/RNG conservation through noninteractive shots.
#include <stdio.h>
#include "fft.c"
#include "../../runtime/platform-sw/rt_sw.h"
static unsigned failures, checked;
#define CHECK(v) do { if (!(v)) { printf("FAIL line %d: %s\n", __LINE__, #v); failures++; } } while (0)
static void press(u32 buttons) { CHECK(sw_step(buttons)); CHECK(sw_step(0)); }
static void idle(unsigned n) { while (n--) CHECK(sw_step(0)); }
static void reset(u16 n) { game_scenario(n); rt_keys = rt_prev = 0; }

static void canonical(State *s)
{
    s->tick = s->perspective = 0;
    s->cphase = s->cframe = s->cenabled = 0;
    s->cax = s->caz = s->cah = s->cyaw = s->cdist = s->ceye = s->cpitch = 0;
    s->cside = s->ciso_x = s->ciso_y = s->ciso_rot = 0;
}

static void check_shot(void)
{
    State start = st, expected, actual;
    Work *saved = malloc(sizeof *W);
    u32 battle_seed = rng32, expected_seed;
    u16 seed = rt_seed;
    unsigned n = 0, entry = 0, action = 0, leave = 0, logical, impacts = 0;
    CHECK(st.cphase == 1 && st.act_t == 0 && st.mode == M_ACT);
    { s16 delta = ((st.cyaw-(56-16*st.ciso_rot)+32) & 63)-32;
      CHECK(abs(delta) <= 8); }
    memcpy(saved, W, sizeof *W);
    // Resolve once with the existing combat routine, with no cinematic wrapper.
    while (st.mode == M_ACT && n++ < 200) { st.tick++; attack_step(); }
    CHECK(n < 200); logical = n; expected = st; expected_seed = rng32;
    st = start; rng32 = battle_seed; rt_seed = seed; memcpy(W, saved, sizeof *W); free(saved);
    n = 0;
    while (st.cphase && n++ < 160) {
        u8 phase = st.cphase, frame = st.cframe, timer = st.act_t;
        u16 hp = st.unit[st.tgt].hp;
        if (phase == 1) entry++;
        else if (phase == 2) action++;
        else leave++;
        // Game, inspection and camera commands must all be ignored during the shot.
        CHECK(sw_step(n & 1 ? K_A | K_UP | K_C | K_F5 | K_MODE | K_DIGIT(7) : 0));
        if (st.unit[st.tgt].hp != hp) {
            impacts++; CHECK(phase == 2 && timer <= ACT_HIT && st.act_t > ACT_HIT);
        }
        if (phase != 2) CHECK(frame <= 9);
        if (phase == 1 && st.cphase != 2) {
            CHECK(st.act_t == 0 && !memcmp(st.unit, start.unit, sizeof st.unit));
            CHECK(rng32 == battle_seed && rt_seed == seed);
        }
        if (st.cphase == 2) {
            CHECK(FW->view[0].rect[st.tgt].rows && FW->view[0].rect[st.atk].rows);
            CHECK(FW->used <= FPOOL_BYTES && FW->masks <= FMASK_BYTES);
            if (st.gain && st.act_t >= ACT_HIT+GAIN_T) {
                CRewardBox before = creward_box(8), after;
                FRect saved_target = FW->view[0].rect[st.tgt];
                CHECK(before.x >= 1 && before.x+before.width <= 159 && before.y >= 10 && before.y <= 82);
                // Target recoil/knockback must never move the attacker's reward notice.
                FW->view[0].rect[st.tgt].x += 37; FW->view[0].rect[st.tgt].y -= 9;
                after = creward_box(8);
                CHECK(before.x == after.x && before.y == after.y);
                FW->view[0].rect[st.tgt] = saved_target;
                { FRect saved_attacker = FW->view[0].rect[st.atk];
                  FW->view[0].rect[st.atk].x = 20;
                  before = creward_box(8);
                  FW->view[0].rect[st.atk].x = 100;
                  after = creward_box(8);
                  CHECK(after.x > before.x);
                  FW->view[0].rect[st.atk] = saved_attacker; }
            }
        }
    }
    CHECK(n < 160 && !st.perspective && !st.cphase);
    CHECK(entry == 10 && leave == 10 && action == (logical+1)/2);
    CHECK(entry+action+leave <= 55);
    CHECK(impacts == (start.unit[start.tgt].hp != expected.unit[start.tgt].hp));
    CHECK(rng32 == expected_seed && rt_seed == seed);
    actual = st; canonical(&actual); canonical(&expected);
    CHECK(!memcmp(&actual, &expected, sizeof actual));
    CHECK(st.camx == start.ciso_x && st.camy == start.ciso_y && st.rot == start.ciso_rot);
    checked++;
}

static void select_attack(u8 ranged)
{
    // All keys before confirmation remain the real isometric menu/cursor route.
    press(K_A); CHECK(st.mode == M_ACTS);
    if (ranged) { press(K_DOWN); press(K_A); CHECK(st.mode == M_SKILLS); press(K_A); }
    else press(K_A);
    CHECK(st.mode == M_AIM && !st.cphase && !st.perspective);
    st.cx = st.unit[1].x; st.cz = st.unit[1].z;
    CHECK(st.reach[st.cz*MAP_W+st.cx] == 1);
    CHECK(sw_step(K_A)); CHECK(st.cphase == 1 && st.mode == M_ACT);
}

// At the first travelling sample, the mesh must face the same way as the ISO map.
// In particular rotations 1/3 must not start with an accidental half-turn.
static void check_entry_alignment(void)
{
    unsigned rot, axis;
    for (rot = 0; rot < 4; rot++) {
        s16 u, v, x0, y0;
        PScreen origin;
        reset(65); st.rot = (u8)rot; st.acted = 1; attack(U_RAMZA, 1); cinema_begin();
        pgeometry();
        origin = pproject(ptransform(st.cax*28+14, 0, st.caz*28+14));
        view_of(st.rot, st.cax, st.caz, &u, &v);
        x0 = scene_x(st.rot,u,v); y0 = scene_y(u,v);
        for (axis = 0; axis < 2; axis++) {
            u8 x = st.cax+(axis == 0), z = st.caz+(axis == 1);
            PScreen p = pproject(ptransform(x*28+14, 0, z*28+14));
            view_of(st.rot,x,z,&u,&v);
            CHECK((p.x > origin.x) == (scene_x(st.rot,u,v) > x0));
            CHECK((p.y > origin.y) == (scene_y(u,v) > y0));
        }
    }
}

int main(void)
{
    unsigned rot, ranged;
    sw_init(61); game_render();
    check_entry_alignment(); reset(61); game_render();
    { Unit before[NUNIT]; u32 seed = rng32;
      memcpy(before, st.unit, sizeof before); press(K_C | K_MODE); idle(18);
      CHECK(st.perspective && st.pdist == 80 && !st.cphase);
      press(K_A); press(K_UP); press(K_DOWN); CHECK(st.mode == M_MENU);
      CHECK(!memcmp(before, st.unit, sizeof before) && rng32 == seed);
      press(K_B); idle(18); CHECK(!st.perspective); }
#ifdef FFT_TEXTURE_PREVIEW
    { Unit before[NUNIT]; u32 seed; u16 runtime_seed;
      reset(61); game_render(); memcpy(before,st.unit,sizeof before);
      seed = rng32; runtime_seed = rt_seed;
      for (rot = 0; rot < 8; rot++) {
          u8 initial = st.rot;
          press(rot < 4 ? K_F5 : K_F1);
          CHECK(st.turn && preview_pixels > 1000);
          idle(TURN_FRAMES);
          CHECK(!st.turn && st.rot == ((initial+(rot < 4 ? 1 : 3)) & 3));
          CHECK(!memcmp(before,st.unit,sizeof before) && rng32 == seed && rt_seed == runtime_seed);
      }
      CHECK(st.rot == 0); }
#endif
    for (ranged = 0; ranged < 2; ranged++) for (rot = 0; rot < 4; rot++) {
        reset((u16)(65+ranged)); st.rot = (u8)rot; cam_target(&st.camx, &st.camy); game_render();
        select_attack((u8)ranged); check_shot();
    }
    // Invalid target, disabled camera, miss, critical knockback, KO, and final victory.
    reset(65); game_render(); press(K_A); press(K_A);
    st.cx = 0; st.cz = 0; press(K_A); CHECK(st.mode == M_AIM && !st.cphase && !st.acted);
    reset(65); game_render(); press(K_DIGIT(7)); CHECK(!st.cenabled);
    press(K_A); press(K_A); st.cx = 6; st.cz = 5; press(K_A);
    CHECK(st.mode == M_ACT && !st.cphase && !st.perspective);
    idle(110); CHECK(st.mode == M_MENU);
    for (rot = 0; rot < 4; rot++) {
        reset(65); game_render();
        if (rot >= 2) st.unit[1].hp = 1;
        if (rot == 3) { unsigned j; for (j = 2; j <= 5; j++) st.unit[j].hp = 0; }
        st.acted = 1; attack(U_RAMZA, 1);
        if (!rot) { st.hit = 0; st.crit = st.push = 0; st.dmg = 0; }
        if (rot == 1) { st.hit = st.crit = st.push = 1; st.kx = 7; st.kz = 5; st.dmg = 1; }
        CHECK(cinema_eligible()); cinema_begin(); check_shot();
    }
    printf("attack camera: %s; %u shots, melee/ranged x four isometric angles, miss/knockback/KO/victory, exact combat + RNG, <=55 display frames, single impact, locked input, inspection frozen\n",
           failures ? "FAILED" : "passed", checked);
    return failures != 0;
}
