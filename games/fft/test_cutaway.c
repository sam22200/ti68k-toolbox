// Focused PC tests of the actual private classifier and edge renderer. The normal
// portable game tests/replays cover their integration with actors and tactical state.
#include <stdio.h>
#include "fft.c"
#include "../../runtime/platform-sw/rt_sw.h"
static unsigned failures;
#define CHECK(v) do { if (!(v)) { printf("FAIL %d: %s\n", __LINE__, #v); failures++; } } while (0)

static void sample(u16 x, u16 y, u8 group, u8 depth)
{
    POWNER[y*160+x] = group; PDEPTH[y*160+x] = depth;
}
static void clear_samples(void)
{
    memset(PHIDDEN, 0, 128); memset(POWNER, 0, 16000); memset(PDEPTH, 0, 16000);
}

int main(void)
{
    u8 bank[2140] = {0}, groups[1] = {1};
    unsigned style, x, y, solid = 0, dashed = 0;
    sw_init(62); pgeometry();
    CHECK(PHIDDEN[3] == 0);                     // user's left house stays solid
    st.pdist = 80; st.ppitch = 6; pgeometry();
    CHECK(PHIDDEN[1] == 1 && PHIDDEN[3] == 0);  // genuine foreground structure, not background house
    st.pyaw ^= 32; pgeometry();
    CHECK(PHIDDEN[3] == 0);                     // user's reverse/right structure does not obstruct
    st.psolid = 1; pgeometry();
    for (x = 0; x < 128; x++) CHECK(PHIDDEN[x] == 0);
    st.psolid = 2; pgeometry();
    CHECK(PHIDDEN[0] == 0);
    for (x = 1; x < 7; x++) CHECK(PHIDDEN[x] == 1);

    // Actual visible ownership + depth, with a minimum same-component footprint.
    clear_samples();
    sample(76, 42, 1, 120); sample(80, 42, 1, 120);
    fixed_probe(80, 48, 4, 6, 100); CHECK(PHIDDEN[1] == 0); // grazing two pixels
    sample(84, 42, 1, 120);
    fixed_probe(80, 48, 4, 6, 100); CHECK(PHIDDEN[1] == 1);
    clear_samples();
    for (x = 76; x <= 84; x += 4) sample(x, 42, 1, 80);
    fixed_probe(80, 48, 4, 6, 100); CHECK(PHIDDEN[1] == 0); // behind subject
    clear_samples();
    for (y = 42; y <= 54; y += 6) sample(130, y, 1, 200);
    fixed_probe(80, 48, 4, 6, 100); CHECK(PHIDDEN[1] == 0); // peripheral
    clear_samples();
    for (x = 76; x <= 84; x += 4) sample(x, 42, 0, 200);
    fixed_probe(80, 48, 4, 6, 100); CHECK(PHIDDEN[0] == 0); // terrain excluded

    // A single horizontal architectural edge, no hidden face diagonals. Test the
    // real clipping/projection/line path and its interaction with opaque depth.
    pbank = bank; WR16(bank, 0, 2); WR16(bank, 2, 4);
    WR16(bank+72, 96, 65536/96);
    WR16(bank+2132, 0, 1); WR16(bank+2134, 0, 0); WR16(bank+2134, 1, 1);
    PVERT[0].x = -24; PVERT[0].y = 0; PVERT[0].z = 96;
    PVERT[1].x = 24; PVERT[1].y = 0; PVERT[1].z = 96;
    pegroup = groups;
    for (style = 0; style < 2; style++) {
        unsigned count = 0, run = 0, gaps = 0;
        PHIDDEN[1] = (u8)style; memset(PDEPTH, 0, 16000); draw_clear(); fixed_edges();
        for (x = 0; x < 160; x++) {
            if (sw_level((s16)x, 48)) { count++; run++; if (style == 1) CHECK(run <= 2); }
            else { if (run) gaps++; run = 0; }
        }
        if (!style) { solid = count; CHECK(solid >= 46); }
        if (style == 1) { dashed = count; CHECK(dashed > 10 && dashed < solid); CHECK(gaps > 5); }
        for (y = 0; y < 100; y++) for (x = 0; x < 160; x++) CHECK(PDEPTH[y*160+x] == 0);
    }
    PHIDDEN[1] = 1; draw_clear();
    memset(PDEPTH+48*160+72, 255, 24); fixed_edges();
    for (x = 72; x < 96; x++) { CHECK(sw_level((s16)x, 48) == 0); CHECK(PDEPTH[48*160+x] == 255); }

    // Clip only fragments at the eye; never remove an entire component for proximity.
    WR16(bank+72, 20, 65536/20);
    PVERT[0].x = -4; PVERT[1].x = 4; PVERT[0].z = PVERT[1].z = 20;
    clear_samples(); PHIDDEN[1] = 1; draw_clear(); fixed_edges();
    for (x = 0; x < 160; x++) CHECK(sw_level((s16)x, 48) == 0);
    PHIDDEN[1] = 0; draw_clear(); fixed_edges();
    CHECK(sw_level(80, 48) == C_BLACK);
    printf("pixel obstruction policy and edge visibility: %s (%u solid / %u dashed samples)\n",
           failures ? "FAILED" : "passed", solid, dashed);
    return failures != 0;
}
