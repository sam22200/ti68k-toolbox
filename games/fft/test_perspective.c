// Inspection changes only presentation. Prove tactical state/RNG conservation, controls,
// bounded lateral motion, full orbit and the direct, animated Ramza entry.
#include <stdio.h>
#include <string.h>
#include "fft.h"
#include "../../runtime/platform-sw/rt_sw.h"
static unsigned failures;
#define CHECK(v) do { if (!(v)) { printf("FAIL %d: %s\n", __LINE__, #v); failures++; } } while (0)
static void steps(u32 keys, unsigned n) { while (n--) sw_step(keys); }
static void press(u32 keys) { sw_step(keys); sw_step(0); }
int main(void)
{
    State before, after;
    u32 rng;
    unsigned kind;
    for (kind = 1; kind <= 3; kind++) {
        u16 image;
        sw_init(61);
        CHECK(st.act == U_RAMZA && st.mode == M_MENU && !st.ai && !st.banner);
        before = st; rng = rt_seed;
        press(K_C);
        while (st.perspective != kind) press(K_MODE);
        image = sw_checksum(); steps(0, 10);
        CHECK(image != sw_checksum());             // the original idle poses remain animated
        { State visual = st; u16 hash = sw_checksum();
          game_render(); CHECK(sw_checksum() == hash);  // retained native framebuffer
          st.pside++; game_render(); st = visual;
          game_render(); CHECK(sw_checksum() == hash);  // forced geometry/sprite rebuild
        }
        steps(K_LEFT, 24);
        if (kind == 1) CHECK(st.pside == 0 && st.pyaw == st.pbase);
        if (kind == 2) {
            CHECK(st.pside == -24);
            steps(K_D | K_LEFT, 48);
            CHECK(((st.pyaw-st.pbase+64)&63) == 56);
            steps(K_D | K_RIGHT, 64);
            CHECK(((st.pyaw-st.pbase+64)&63) == 8);
        }
        if (kind == 3) {
            u8 start = st.pyaw;
            steps(K_RIGHT, 128); CHECK(st.pyaw == start);  // a complete 360 degree orbit
            steps(K_UP, 40); CHECK(st.ppitch == 12);
            steps(K_DOWN, 48); CHECK(st.ppitch == -5);
            steps(K_F5, 80); CHECK(st.pdist == 112);
            steps(K_F1, 90); CHECK(st.pdist == 40);
        }
        CHECK(rt_seed == rng);
        press(K_B); after = st;
        after.tick = before.tick;
        after.perspective = before.perspective; after.pyaw = before.pyaw;
        after.pbase = before.pbase; after.pdist = before.pdist;
        after.ppitch = before.ppitch; after.pside = before.pside;
        CHECK(!memcmp(&before, &after, sizeof before));
        press(K_A); CHECK(st.mode == M_TARGET);      // normal tactical Move still works
        sw_init((u16)(61+kind));
        CHECK(st.perspective == kind && st.act == U_RAMZA && !st.ai);
    }
    sw_init(0); CHECK(st.act == U_RAMZA && st.mode == M_MENU && !st.ai && !st.banner);
    printf("perspective: %s; tactical state/RNG, idle animation, lateral limits, full orbit, return to Move\n",
           failures ? "FAILED" : "passed");
    return failures != 0;
}
