// The combat-effect layer draws and decides nothing (campaign/COMBAT_FX.md).
// The spec's acceptance test is that HP, MP and CT are identical whether the animation
// plays normally, fast or not at all. The engine already applies the result in the combat
// core at ACT_HIT, so what has to be proven here is the other half: that presenting the
// effect never touches battle state, whatever frame of the timeline it is asked for.
#include <stdio.h>
#include <string.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#include "gariland.c"

static u16 checks, failures;
static void check(const char *what, int ok)
{
    checks++;
    if (!ok) { failures++; printf("FAIL %s\n", what); }
}
static void frame(void) { W->frz = W->hold = 0; W->actor_budget = 255; game_render(); }

int main(void)
{
    static Unit before[UNIT_CAPACITY];
    u32 seed;
    u16 t;
    u8 atk = 8, tgt = 10;

    sw_init(201);
    st.ai = 0; st.banner = 0; st.tick = 0;
    st.cx = st.unit[atk].x; st.cz = st.unit[atk].z;
    cam_target(&st.camx, &st.camy);

    for (t = 0; t < ACT_GAIN_END; t++) {
        st.mode = M_ACT; st.atk = atk; st.tgt = tgt; st.act_t = (u8)t;
        st.abil = 0x098;                             // Wish: measured, dims 3.03 s
        memcpy(before, st.unit, sizeof before);
        seed = rng32;
        frame();
        // Battle state: untouched, every frame of the timeline.
        check("units unchanged", !memcmp(before, st.unit, sizeof before));
        check("rng unchanged", seed == rng32);
        // The schedule, read back from the one byte that holds it. The focus only
        // engages for a category the original stages: measured with
        // tools/effect_watch.py, Heal plays in full light and Potion does not.
        if (t < ACT_WIND || t >= ACT_END) check("no focus outside the action", W->fx_sel == 0);
        else if (t < ACT_BLOW)            check("caster in focus", W->fx_sel == (u8)(atk + 1));
        else                              check("target in focus", W->fx_sel == (u8)(tgt + 1));
        // Pixel stability across frames is deliberately NOT asserted here. The engine
        // spreads its HUD and screen copy over several frames and keeps animating the
        // damage number on its own clock, so consecutive frames differ for reasons that
        // have nothing to do with this layer. PC/68000 equality covers that ground, in
        // check_capacity_render.py's battle-fx case.
    }

    // An ability the original leaves in full light never takes the focus, whatever
    // the phase. Dash and Wish carry identical flags and the same outcome category
    // and are staged oppositely, which is why the table holds the fact per ability.
    for (t = 0; t < ACT_END; t++) {
        st.mode = M_ACT; st.atk = atk; st.tgt = tgt; st.act_t = (u8)t;
        st.abil = 0x093;                             // Dash: measured, plays in full light
        frame();
        check("an ability that does not stage never focuses", W->fx_sel == 0);
    }

    // Leaving the action restores by itself: one frame, no repair pass.
    st.mode = M_BROWSE;
    frame();
    check("focus released on leaving the action", W->fx_sel == 0);

    // Skipping the animation must land on the same state as playing it.
    {
        static Unit played[UNIT_CAPACITY];
        st.mode = M_ACT; st.atk = atk; st.tgt = tgt;
        memcpy(before, st.unit, sizeof before);
        for (t = 0; t < ACT_END; t++) { st.act_t = (u8)t; frame(); }
        memcpy(played, st.unit, sizeof played);
        memcpy(st.unit, before, sizeof before);
        st.act_t = ACT_END - 1; frame();                 // straight to the end
        check("skipped animation lands on the played state", !memcmp(played, st.unit, sizeof played));
    }

    // fx_known records which bits were watched on the original. Every bit of
    // fx_staged has to be one of them: a staged bit it does not cover would be a
    // guess spending 55 kilocycles a frame, which is exactly what the second map
    // exists to make impossible to write by accident.
    {
        u8 i, all_measured = 1;
        for (i = 0; i < FX_ABILITIES / 8; i++)
            if (fx_staged[i] & (u8)~fx_known[i]) all_measured = 0;
        check("every staged ability is a measured one", all_measured);
    }

    printf("combat-fx failures %u checks %u\n", failures, checks);
    return failures != 0;
}
