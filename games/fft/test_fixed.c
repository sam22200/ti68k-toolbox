// Native close shoulder: inspection conservation, both directions, lazy poses vs rebuild,
// different actual terrain anchors and return to tactical Move.
#include <stdio.h>
#include <string.h>
#include "fft.h"
#include "../../runtime/platform-sw/rt_sw.h"
static unsigned failures;
#define CHECK(v) do { if (!(v)) { printf("FAIL %d: %s\n", __LINE__, #v); failures++; } } while (0)
static void steps(u32 keys, unsigned n) { while (n--) sw_step(keys); }
static void press(u32 keys)
{
    unsigned frames = 0;
    sw_step(keys); sw_step(0);
    while (st.ptrans && frames++ < 20) sw_step(0);
    CHECK(!st.ptrans);
}
static void rebuild_equals(void)
{
    State saved = st;
    u16 hash = sw_checksum();
    game_scenario(62); st = saved; game_render();
    CHECK(sw_checksum() == hash);
}
int main(void)
{
    State before, after;
    u32 rng;
    unsigned face, side, frame, zoom;
    // Default inspection cuts directly; old camera motion is an explicit research build.
#ifdef FFT_CAMERA_MOTION
    sw_init(61); before = st; rng = rt_seed;
    sw_step(K_C); CHECK(st.ptrans == 1 && st.ptrans_t == 0);
    for (frame = 1; frame <= 16; frame++) {
        sw_step(0); CHECK(st.ptrans == 1 && st.ptrans_t == frame);
        CHECK(!memcmp(st.unit, before.unit, sizeof st.unit) && rt_seed == rng);
    }
    sw_step(0); CHECK(st.perspective == 1 && !st.ptrans);
    sw_step(K_B); CHECK(st.ptrans == 2 && st.ptrans_t == 16);
    for (frame = 16; frame; frame--) {
        sw_step(0); CHECK(st.ptrans == 2 && st.ptrans_t == frame-1);
    }
    sw_step(0); CHECK(!st.perspective && !st.ptrans && !st.ptrans_t);
    CHECK(!memcmp(st.unit, before.unit, sizeof st.unit) && rt_seed == rng);
#else
    sw_init(61); before = st; rng = rt_seed;
    sw_step(K_C); CHECK(st.perspective == 1 && !st.ptrans && !st.ptrans_t);
    CHECK(!memcmp(st.unit, before.unit, sizeof st.unit) && rt_seed == rng);
    sw_step(K_B); CHECK(!st.perspective && !st.ptrans && !st.ptrans_t);
    CHECK(!memcmp(st.unit, before.unit, sizeof st.unit) && rt_seed == rng);
#endif
    // Direct wide selection and remembered re-entry preserve the tactical state.
    sw_init(61); before = st; rng = rt_seed;
    press(K_C | K_MODE); CHECK(st.pdist == 80 && st.ppitch == 6 && st.pzoom == 1);
    press(K_B); CHECK(!st.perspective && st.pzoom == 1);
    press(K_C); CHECK(st.pdist == 80 && st.ppitch == 6);
    press(K_MODE); CHECK(st.pdist == 40 && st.ppitch == 1 && !st.pzoom);
    press(K_B); press(K_C); CHECK(st.pdist == 40 && !st.pzoom);
    CHECK(!memcmp(st.unit, before.unit, sizeof st.unit) && rt_seed == rng);
    // Both zoom endpoints preserve facing and match a direct cache rebuild.
    for (face = 0; face < 4; face++) for (side = 0; side < 2; side++) {
#ifdef FFT_CAMERA_MOTION
        u16 previous; unsigned changes = 0;
#endif
        sw_init(61); st.unit[U_RAMZA].face = (u8)face;
        press(K_C); if (side) press(K_F5);
        before = st; rng = rt_seed;
#ifdef FFT_CAMERA_MOTION
        previous = sw_checksum();
        sw_step(K_MODE); CHECK(st.ptrans == 3 && st.ptrans_t == 0 && st.pdist == 80);
        for (frame = 1; frame <= 16; frame++) {
            sw_step(0); CHECK(st.ptrans == 3 && st.ptrans_t == frame);
            if (sw_checksum() != previous) changes++;
            previous = sw_checksum();
            CHECK(!memcmp(st.unit, before.unit, sizeof st.unit) && rt_seed == rng);
            CHECK(st.pyaw == before.pyaw);
        }
        CHECK(changes >= 8);
        sw_step(0); CHECK(!st.ptrans && st.pdist == 80 && st.pzoom == 1);
#else
        sw_step(K_MODE); CHECK(!st.ptrans && !st.ptrans_t && st.pdist == 80 && st.pzoom == 1);
        CHECK(!memcmp(st.unit, before.unit, sizeof st.unit) && rt_seed == rng);
        CHECK(st.pyaw == before.pyaw); sw_step(0);
#endif
        rebuild_equals(); press(K_MODE);
        CHECK(!st.ptrans && st.pdist == 40 && !st.pzoom); rebuild_equals();
    }
    sw_init(61); before = st; rng = rt_seed;
    press(K_C); CHECK(st.perspective == 1 && st.pdist == 40);
    CHECK(st.ppitch == 1 && st.pside == 6);
    steps(K_LEFT | K_RIGHT | K_UP, 8); CHECK(st.pyaw == st.pbase);
    { u16 hash = sw_checksum(); u8 tick = st.tick;
      press(K_F5); CHECK(st.pyaw == (st.pbase ^ 32));
      CHECK(sw_checksum() != hash);
      press(K_F5); CHECK(st.pyaw == st.pbase);
      st.tick = tick; game_render(); CHECK(sw_checksum() == hash);
    }
    press(K_F5); press(K_ENTER); CHECK(st.pyaw == st.pbase);
    press(K_MODE); CHECK(st.pdist == 80 && st.ppitch == 6 && st.pyaw == st.pbase);
    press(K_F5); CHECK(st.pdist == 80 && st.ppitch == 6 && st.pyaw == (st.pbase ^ 32));
    press(K_MODE); CHECK(st.pdist == 40 && st.ppitch == 1 && st.pyaw == (st.pbase ^ 32));
    press(K_ENTER);
    { State saved = st; u16 hash = sw_checksum();
      press(K_D); CHECK(st.psolid == 1);
      st.tick = saved.tick; game_render(); CHECK(sw_checksum() != hash);
      CHECK(!memcmp(st.unit, saved.unit, sizeof st.unit));
      rebuild_equals();
      press(K_D); CHECK(st.psolid == 2); rebuild_equals();
      press(K_D); CHECK(st.psolid == 0);
      st.tick = saved.tick; game_render(); CHECK(sw_checksum() == hash);
    }
    CHECK(rt_seed == rng);
    press(K_B); after = st;
    after.tick = before.tick; after.perspective = before.perspective;
    after.pyaw = before.pyaw; after.pbase = before.pbase; after.pdist = before.pdist;
    after.ppitch = before.ppitch; after.pside = before.pside;
    after.psolid = before.psolid;
    CHECK(!memcmp(&before, &after, sizeof before));
    press(K_A); CHECK(st.mode == M_TARGET);
    for (face = 0; face < 4; face++) {
        sw_init(61);
        if (face & 1) { st.unit[U_RAMZA].x = 3; st.unit[U_RAMZA].z = 8; }
        st.unit[U_RAMZA].face = (u8)face; press(K_C);
        for (zoom = 0; zoom < 2; zoom++) {
          if (zoom) press(K_MODE);
          for (side = 0; side < 2; side++) {
            if (side) press(K_F5);
            for (frame = 0; frame < 36; frame++) {
                sw_step(0);
                if (frame == 5 || frame == 17 || frame == 35) rebuild_equals();
            }
          }
        }
    }
    // Freeze actor poses so any changing pixel belongs to the animated canal alone.
    for (zoom = 0; zoom < 2; zoom++) {
        u8 pixels[160*85]; unsigned x, y, changed = 0; u16 hash;
        sw_init(61);
        for (face = 0; face < NUNIT; face++) st.unit[face].hp = 0;
        press(K_C); if (zoom) press(K_MODE);
        st.tick = 0; game_render(); hash = sw_checksum();
        for (y = 8; y < 93; y++) for (x = 0; x < 160; x++) pixels[(y-8)*160+x] = sw_level(x, y);
        st.tick = 8; game_render();
        for (y = 8; y < 93; y++) for (x = 0; x < 160; x++) {
            u8 a = pixels[(y-8)*160+x], b = sw_level(x, y);
            if (a != b) { changed++; CHECK((a == 0 && b == 1) || (a == 1 && b == 0)); }
        }
        CHECK(changed > 0);
        st.tick = 32; game_render(); CHECK(sw_checksum() == hash);
        rebuild_equals();
    }
    printf("fixed shoulder: %s; elevated wide angle, auto/solid/dotted, both directions, tactical/RNG conservation, 48 street/roof cache rebuilds, four-phase water, Move\n", failures ? "FAILED" : "passed");
    return failures != 0;
}
