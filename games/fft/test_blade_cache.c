// Bounded blade cache: original bytes, warm reuse, eviction and native call costs.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main rt_unused_main
#include "../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "fft.c"
#ifndef __m68k__
#include <stdio.h>
#include "../../runtime/platform-sw/rt_sw.h"
#endif
static u8 success = 1;
#define REQUIRE(v) do { if (!(v)) success = 0; } while (0)

static void suite(void)
{
    const u8 *raw = rt_file("bladeraw", RT_NULL);
    State before = st;
    u8 pose;
    REQUIRE(raw && W && wgfx);
    if (!success) return;
    REQUIRE(!W->blade_k[0] && !W->blade_k[1] && !W->blade_k[2]);
#ifdef __m68k__
    BENCH_NAME(19,"Blade decode miss");
    BENCH_NAME(20,"Blade cold lookup");
    BENCH_NAME(21,"Blade warm lookup");
    BENCH_VALUE(0xb1000000UL | sizeof(Work));
#endif
    for (pose = 0; pose < 48; pose++) {
        const u8 *cold, *warm;
        u8 id = wgfx[WEAPON_INDEX + pose], slot = pose % 3;
#ifdef __m68k__
        u32 start, cost, hit;
        BENCH_VALUE(0xb2000000UL | pose);
#endif
        W->blade_k[slot] = 0;
#ifdef __m68k__
        BENCH_BEGIN(20); start = BENCH_CYCLES;
#endif
        cold = weapon_frame(pose, slot);
#ifdef __m68k__
        cost = BENCH_CYCLES - start; BENCH_END(20);
        BENCH_BEGIN(21); start = BENCH_CYCLES;
#endif
        warm = weapon_frame(pose, slot);
#ifdef __m68k__
        hit = BENCH_CYCLES - start; BENCH_END(21);
        BENCH_VALUE(cost); BENCH_VALUE(hit);
#endif
        REQUIRE(cold == warm && cold == (const u8 *)W->blade[slot] && W->blade_k[slot] == id + 1);
        REQUIRE(!memcmp(cold, raw + (u16)id * WEAPON_FB, WEAPON_FB));
        // Replace the entry with another family/facing/weapon, then request it again.
        weapon_frame((pose + 3) % 48, slot);
        REQUIRE(!memcmp(weapon_frame(pose, slot), raw + (u16)id * WEAPON_FB, WEAPON_FB));
    }
    REQUIRE(!memcmp(&before, &st, sizeof st));
#ifndef __m68k__
    weapon_frame(0,0); REQUIRE(W->blade_k[0]);
    game_init(); REQUIRE(!W->blade_k[0] && !W->blade_k[1] && !W->blade_k[2]);
#endif
}

#ifndef __m68k__
int main(void)
{
    sw_init(17); suite();
    printf("blade cache: 48 original frames, warm reuse, replacement and reload %s\n", success ? "passed" : "FAILED");
    return !success;
}
#else
void _main(void)
{
    void *planes = malloc(2 * RT_PSIZE);
    rt_keys = rt_prev = 0; rt_frame = 0; rt_seed = 1; rt_state = RT_NULL; rt_state_size = 0;
    if (!planes || !get_fonts()) { BENCH_VALUE(0xb3000000UL); return; }
    rt_dark = planes; rt_light = (u8 *)planes + RT_PSIZE;
    game_init(); game_scenario(17); suite();
    BENCH_VALUE(0xb3000000UL | success);
    free(planes);
}
#endif
