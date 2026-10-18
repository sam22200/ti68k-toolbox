// Shared case driver: no platform timing, hardware access or substitute rendering here.
#include "camera_tiemu_demo.c"

#define AUDIT_N 13
static const char *const audit_name[AUDIT_N] = {
    "F5", "F1", "ISO>C", "ISO>W", "W>C", "C>W", "W>ISO",
    "CINE M", "CINE R", "COLD C", "COLD W", "WARM C", "WARM W"
};
static u16 audit_frame;
static u8 audit_case, audit_phase, audit_ok;

static u8 audit_step(u32 keys)
{
    rt_prev = rt_keys; rt_keys = keys;
    if (!fft_full_update()) return 0;
    audit_phase = st.cphase;
    game_render(); rt_frame++;
    return 1;
}

static void audit_setup(u8 c)
{
    u8 warm;
    audit_case = c; audit_frame = 0; audit_ok = 1;
    game_scenario(65);
    if (c == 8) fft_full_scenario(66);
    if (!W || !pbank || !demo_look || !cimages) { audit_ok = 0; return; }
#ifdef FFT_ULTRA_TRANSITION
    if (!ultra_bank) { audit_ok = 0; return; }
#endif
    scene_rot = vbuf_rot = 0xFF; W->bg_ok = 0; pvalid = 0;
    rt_prev = rt_keys = 0;
    game_render();
    if ((c >= 4 && c <= 6) || c >= 9) {
        st.pzoom = c == 4 || c == 6 || c == 10 || c == 12;
        perspective_enter(1);
        // For cold cases the first render belongs inside the timed interval.
        if (c < 9 || c >= 11) game_render();
        if (c >= 11) for (warm = 0; warm < 32; warm++) audit_step(0);
    }
    if (c == 7 || c == 8) {
        aim(c == 8 ? 1 : NO_ABIL);
        st.cx = st.unit[1].x; st.cz = st.unit[1].z;
        if (!st.reach[st.cz*MAP_W+st.cx]) audit_ok = 0;
    }
    rt_prev = rt_keys = 0;
}

static u8 audit_next(void)
{
    u32 buttons = 0;
    if (!audit_frame) {
        switch (audit_case) {
        case 0: buttons = K_F5; break;
        case 1: buttons = K_F1; break;
        case 2: buttons = K_C; break;
        case 3: buttons = K_C | K_MODE; break;
        case 4: case 5: buttons = K_MODE; break;
        case 6: buttons = K_B; break;
        case 7: case 8: buttons = K_A; break;
        }
    }
    audit_frame++;
    return audit_step(buttons);
}

static u8 audit_done(void)
{
    if (!audit_frame) return 0;
    if (audit_case < 2) return !st.turn;
    if (audit_case < 7) return !st.ptrans;
    if (audit_case < 9) return !st.cphase;
    return audit_frame >= (audit_case < 11 ? 1 : 128);
}

static u8 audit_verify(void)
{
    if (!audit_ok || audit_frame > 128) return 0;
    if (audit_case < 2) return st.rot == (audit_case ? 3 : 1) && audit_frame == TURN_FRAMES+1;
    if (audit_case == 6) return !st.perspective;
    if (audit_case < 7) return st.perspective && st.pdist == (audit_case == 3 || audit_case == 5 ? 80 : 40);
    if (audit_case < 9) return !st.perspective && st.mode == M_MENU && st.unit[1].hp == (audit_case == 7 ? 31 : 37);
    return st.perspective && !st.ptrans;
}
