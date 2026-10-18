// Banner completeness, timing and queued loot transformations using the actual renderer.
#include <assert.h>
#include <stdio.h>
#include "fft.c"
#include "../../runtime/platform-sw/rt_sw.h"

static u8 pixel(u16 x, u16 y)
{
    u16 o = y * RT_PBYTES + (x >> 3); u8 b = 0x80 >> (x & 7);
    return (!!(((u8 *)rt_light)[o] & b)) | ((!!(((u8 *)rt_dark)[o] & b)) << 1);
}

static void banners(void)
{
    u8 kind, line;
    for (kind = BN_START; kind <= BN_DEFEAT; kind++) {
        sw_init(59); bn_start(kind);
        assert(DK_SPEED == 8 && BN_SPEED == 12);
        for (line = 0; line < (kind == BN_START ? 2 : 1); line++) {
            u8 k = bn_line(kind, line), r, x, hold;
            const u8 *bank = igfx + UI_BANNERS, *g = bank + RD16(bank, k), *p = g + 4;
            u16 size = g[2] * g[3], opaque = 0, right = 0;
            // The entire generated strip, including the final character, must be visible
            // for every hold frame, even when no rasterization budget is available.
            for (hold = 0; hold < BN_HOLD; hold++) {
                draw_rect(0, 0, RT_W, RT_H, C_DGRAY);
                st.banner_t = W->bn_end - DK_N - BN_HOLD + hold;
                W->left = -200; bn_draw();
                for (r = 0; r < g[2]; r++) for (x = 0; x < g[3] * 8; x++) {
                    u16 off = r * g[3] + (x >> 3); u8 bit = 0x80 >> (x & 7);
                    if (p[2 * size + off] & bit) {
                        u8 expected = (!!(p[off] & bit)) | ((!!(p[size + off] & bit)) << 1);
                        assert(pixel(g[0] + x, BN_Y + g[1] + r) == expected);
                        opaque++; if (g[0] + x > RT_W / 2 + bn_width[k] / 4) right++;
                    }
                }
            }
            assert(opaque && right);
        }
        // ENTER still skips either to the shade leaving or to the results.
        st.banner_t = 0; assert(sw_step(K_ENTER)); assert(st.banner_t >= W->bn_end - DK_N);
        assert(sw_step(0)); assert(sw_step(K_ENTER)); assert(!st.banner);
    }
}

static void loot(void)
{
    unsigned n, frame, phases = 0;
    u16 hashes[CRYSTAL_FRAMES];
    sw_init(69); st.morph_mask = 0;
    for (frame = 0; frame < CRYSTAL_FRAMES; frame++) {
        const u8 *g; u16 hash = 0; unsigned k;
        st.tick = frame * 4; g = unit_frame(0, 1);
        for (k = 0; k < UNIT_FB; k++) hash = (hash << 1 | hash >> 15) ^ g[k];
        hashes[frame] = hash;
        for (k = 0; k < frame && hashes[k] != hash; k++) ;
        phases += k == frame;
    }
    assert(phases >= 6);
    printf("crystal: %u distinct source phases\n", phases);
    for (n = 69; n <= 72; n++) {
        State before; u32 rng; u8 previous = 255, seen = 0; u16 sum[32] = {0};
        sw_init(n); before = st; rng = rng32;
        for (frame = 0; frame < 300 && st.morph_mask; frame++) {
            u8 who = morph_first(), t = st.morph_t;
            if (who != previous) { seen++; previous = who; }
            // Held acceleration and navigation cannot skip the visible effect or run AI.
            assert(sw_step(K_A | K_RIGHT));
            assert(!memcmp(before.unit, st.unit, sizeof st.unit));
            assert(rng32 == rng && st.mode == before.mode && st.act == before.act);
            if (who == 1 && t < 32) sum[t] = sw_checksum();
        }
        assert(frame < 300 && frame >= MORPH_FRAMES && !st.morph_mask);
        assert(seen == (n == 71 ? 2 : 1));
        assert(sum[4] != sum[16] && sum[16] != sum[28]);
        printf("loot scenario %u: %u frames, %u transformation(s)\n", n, frame, seen);
    }
    // A natural CT conversion registers an event while preserving the clock's selection.
    sw_init(10);
    st.unit[1].hp = 0; st.unit[1].dc = 0; st.unit[1].ct = 254;
    ct_next(); assert(st.morph_mask & 2); assert(DC_OBJ(st.unit[1].dc));
}

static void covered_morph(void)
{
    u8 rot, t, r, x, bg; unsigned hidden = 0;
    Cover c;
    sw_init(69); memset(&c, 0, sizeof c); c.r0 = 2; c.r1 = UNIT_SH - 3;
    for (r = c.r0; r <= c.r1; r++) c.cm[r] = 0xFFFF;
    for (rot = 0; rot < 4; rot++) for (t = 0; t < MORPH_FRAMES; t += 4) {
        const u8 *g;
        st.morph_t = t; g = unit_frame(rot, 1);
        for (bg = C_LGRAY; bg <= C_BLACK; bg++) {
            draw_rect(0, 0, RT_W, RT_H, bg); draw_unit_covered(80, 70, rot, 1, &c);
            for (r = 2; r < UNIT_FOOT; r++) for (x = 0; x < 16; x++) {
                u16 bit = 0x8000U >> x;
                if (c.cm[r] & RD16(g, 3 * UNIT_SH + r) & bit) {
                    assert(pixel(72 + x, 70 - UNIT_FOOT + r) == C_WHITE); hidden++;
                }
            }
        }
    }
    assert(hidden); printf("transform occlusion: %u covered interior pixels checked\n", hidden);
}

static void floating_chest(void)
{
    u8 t, r, x, lo = UNIT_SH, hi = 0; unsigned hidden = 0;
    const u8 *base;
    Cover c;
    sw_init(70); st.morph_mask = 0; st.tick = 0; base = unit_frame(0, 1);
    memset(&c, 0, sizeof c); c.r0 = 2; c.r1 = UNIT_SH - 3;
    for (r = c.r0; r <= c.r1; r++) c.cm[r] = 0xFFFF;
    for (t = 0; t < 64; t++) {
        const u8 *g; u8 top = 0;
        st.tick = t; g = unit_frame(t & 3, 1);
        while (top < UNIT_SH && RD16(g, 2 * UNIT_SH + top) == 0xFFFF) top++;
        if (top < lo) lo = top;
        if (top > hi) hi = top;
        // The ground shadow must not bob along with the object.
        for (r = UNIT_FOOT + 1; r < UNIT_SH; r++) for (x = 0; x < 4; x++)
            assert(RD16(g, x * UNIT_SH + r) == RD16(base, x * UNIT_SH + r));
        draw_rect(0, 0, RT_W, RT_H, C_BLACK);
        draw_unit_covered(80, 70, t & 3, 1, &c);
        for (r = c.r0; r < UNIT_FOOT; r++) for (x = 0; x < 16; x++)
            if (RD16(g, 3 * UNIT_SH + r) & (0x8000U >> x)) {
                assert(pixel(72 + x, 70 - UNIT_FOOT + r) == C_WHITE); hidden++;
            }
    }
    assert(hi - lo == 2 && hidden);
    st.tick = 64; assert(unit_frame(0, 1) == base);
    printf("chest float: two-pixel range, fixed shadow, %u covered pixels checked\n", hidden);
}

static void capture(const char *directory)
{
    static const u8 cases[] = { 0, 59, 60, 69, 70, 71, 72 };
    unsigned i, frame;
    for (i = 0; i < sizeof cases; i++) {
        sw_init(cases[i]);
        for (frame = 1; frame <= 160; frame++) {
            char name[256];
            assert(sw_step(0));
            if (!(frame & 1)) {
                snprintf(name, sizeof name, "%s/%u-%03u.png", directory, cases[i], frame);
                assert(!sw_write_png(name, 1));
            }
        }
    }
}

int main(int argc, char **argv)
{
    banners(); loot(); covered_morph(); floating_chest(); puts("banner and loot presentation tests passed");
    if (argc > 1) capture(argv[1]);
    return 0;
}
