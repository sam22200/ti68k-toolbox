// Bounded calculator review: original camera/combat, compact isometric presentation.
// The complete tactical UI exceeds one AMS variable with the perspective renderer.
#ifdef FFT_CONFIG
#define FFT_COMPACT_UI
#endif
#define game_render fft_full_render
#define game_update fft_full_update
#define game_scenario fft_full_scenario
#include "../fft.c"
#undef game_render
#undef game_update
#undef game_scenario

static u16 demo_time;
static u32 demo_keys;
static const u8 *demo_look;
#ifndef __m68k__
static u8 demo_look_pc[12008];
#endif

void game_scenario(u16 n)
{
    (void)n;
    fft_full_scenario(65);
    demo_time = 0; demo_keys = 0; demo_look = RT_NULL;
#ifdef __m68k__
    { u16 size; const u8 *p = rt_file("fftlook", &size);
      if (p && size >= 12) demo_look = p; }
#else
    { FILE *f = fopen("fftlook.bin", "rb");
      if (f) {
          if (fread(demo_look_pc, 1, sizeof demo_look_pc, f) >= 12)
              demo_look = demo_look_pc;
          fclose(f);
      } }
#endif
    if (demo_look && (RD16(demo_look, 0) != 0x464C
        || (RD16(demo_look, 1) != 1 && RD16(demo_look, 1) != 2))) demo_look = RT_NULL;
}

u8 game_update(void)
{
#ifdef FFT_CONFIG_INTERACTIVE
    return fft_full_update();
#else
    u32 keys = 0;
    if (input_pressed(K_ESC)) return 0;
    // Advance the script only between complete camera moves/attacks. All transitions
    // and logical combat steps are rendered; recording never changes their timing.
    if (!st.ptrans && !st.cphase) {
#ifdef FFT_CAMERA_CUT_DEMO
        switch (demo_time) {
        case 32: case 96: keys = K_F5; break;
        case 160: case 224: keys = K_F1; break;
        case 288: keys = K_C | K_MODE; break;
        case 352: keys = K_MODE; break;
        case 416: keys = K_B; break;
        }
        if (demo_time < 480) demo_time++;
#else
        switch (demo_time) {
        case 32: keys = K_C | K_MODE; break;
        case 96: keys = K_MODE; break;
        case 160: keys = K_B; break;
        case 208: case 224: case 264: keys = K_A; break;
        case 240: keys = K_RIGHT; break;
        }
        if (demo_time < 352) demo_time++;
#endif
    }
    rt_prev = demo_keys; rt_keys = keys; demo_keys = keys;
    return fft_full_update();
#endif
}

static void demo_iso(void)
{
    Feet feet[NUNIT]; u8 sorted[NUNIT], i, j;
    W->left = 80;
    ensure_scene(st.rot); screen_scene();
    if (W->bg_drawn) draw_glints();
    for (i = 0; i < NUNIT; i++) {
        feet[i] = unit_feet(st.rot, i);
        for (j = i; j && feet[sorted[j-1]].depth > feet[i].depth; j--) sorted[j] = sorted[j-1];
        sorted[j] = i;
    }
    for (j = 0; j < NUNIT; j++) {
        Cover *c; Feet f; s16 x, y;
        i = sorted[j]; f = feet[i]; x = f.x-st.camx; y = f.y-st.camy;
        if (x < -16 || x >= 176 || y < 0 || y >= 130) continue;
        c = &W->cover[i];
        if (c->x != f.x || c->y != f.y || c->depth != f.depth || c->rot != st.rot) {
            c->x = f.x; c->y = f.y; c->depth = f.depth; c->rot = st.rot;
            cover(c->cm, f.x, f.y, f.depth); c->wet = unit_in_water(i);
            for (c->r0 = 2; c->r0 <= UNIT_SH-3 && !c->cm[c->r0]; c->r0++) ;
            for (c->r1 = UNIT_SH-3; c->r1 >= c->r0 && !c->cm[c->r1]; c->r1--) ;
        }
        draw_unit_covered(x, y, st.rot, i, c);
    }
    draw_rect(0, 0, 160, 8, C_WHITE);
#ifdef FFT_CAMERA_CUT_DEMO
    draw_text(2, 0, "RAMZA / DIRECT VIEWS", F_SMALL, C_BLACK);
#else
    draw_text(2, 0, "RAMZA / CINEMA ON", F_SMALL, C_BLACK);
#endif
    draw_rect(0, 93, 160, 7, C_WHITE);
#ifdef FFT_CAMERA_CUT_DEMO
#ifdef FFT_CONFIG_INTERACTIVE
    draw_text(2,93,"CAT:setup  DIAMOND:view",F_SMALL,C_BLACK);
#else
    draw_text(2, 93, demo_time < 160 ? "F5: right turn x2" : demo_time < 288 ?
        "F1: left turn x2" : "Wide - close - map", F_SMALL, C_BLACK);
#endif
#else
    draw_text(2, 93, demo_time < 208 ? "Wide - close - attack demo" :
        st.mode == M_ACTS ? "Act: Attack" : st.mode == M_AIM ? "Attack: select target" :
        demo_time >= 265 ? "Attack complete" : "Ramza: Act", F_SMALL, C_BLACK);
#endif
}

void game_render(void)
{
#ifdef FFT_CONFIG
    pidle_interrupt();
    if (st.config) { config_draw(); pidle_tick(PIDLE_BUDGET); FRAME_END(); return; }
#endif
    if (!W || !pbank || !demo_look) {
        draw_clear(); draw_text(2, 40, "Missing demo data / RAM", F_SMALL, C_BLACK); return;
    }
    if (st.perspective) {
        if (!st.cphase && !st.ptrans && (!pvalid || !FW->view[0].valid)) {
            if (!pvalid) {
#ifdef FFT_CONFIG
                pidle_complete();
#endif
                memset(FW, 0, sizeof *FW); pvalid = 1;
            }
            fprepare(0);
            // Exactly two reviewed fixed cameras, at this fixture's unchanged position.
            if (st.unit[U_RAMZA].x == demo_look[4] && st.unit[U_RAMZA].z == demo_look[5]
                && st.pyaw == demo_look[6] && st.psolid == demo_look[7]
#ifdef FFT_CONFIG
                && !st.pwire
#endif
                )
            {
                if (RD16(demo_look,1)==2)
                    zx0_unpack(demo_look+RD16(demo_look+8,st.pdist==80),scene_l);
                else memcpy(scene_l, demo_look+8+(st.pdist==80 ? 6000 : 0),6000);
            }
        }
        perspective_render();
    } else if (st.turn) {
        // Perspective cuts and isometric rotation have independent presentation policies.
#ifdef FFT_TURN_HOLD
        copy_view();
#else
        ZB(7); render_turn(); ZE(7);
#endif
        unpack_view((st.rot+st.turn) & 3);
    } else demo_iso();
#ifdef FFT_CONFIG
    if(!st.perspective && !st.turn) pidle_tick(PIDLE_BUDGET);
#endif
    W->hud_ok = 1; W->frz = W->hold = 0;
#ifdef FFT_PREFETCH_DEBUG
    { static const char digits[]="0123456789ABCDEF";
      char label[]="P- D00 K00000000"; u8 k;
      if(pidle) { label[1]=digits[pidle->stage]; label[4]=digits[pidle->delay>>4]; label[5]=digits[pidle->delay&15]; }
      for(k=0;k<8;k++) label[8+k]=digits[(rt_keys>>((7-k)*4))&15];
      draw_rect(0,93,160,7,C_WHITE); draw_text(2,93,label,F_SMALL,C_BLACK);
#ifdef __m68k__
      { char mem[]="A0000 X0000"; u16 a=(u16)HeapAvail(),b=(u16)HeapMax();
        for(k=0;k<4;k++) { mem[1+k]=digits[(a>>((3-k)*4))&15];mem[7+k]=digits[(b>>((3-k)*4))&15]; }
        draw_rect(0,85,160,7,C_WHITE);draw_text(2,85,mem,F_SMALL,C_BLACK); }
#endif
    }
#endif
    FRAME_END();
#if defined(RT_CYCLES) && !defined(FFT_CAMERA_AUDIT)
    BENCH_SHOT(rt_dark);
#endif
}

#if !defined(__m68k__) && !defined(FFT_CAMERA_AUDIT)
#include "../../../runtime/platform-sw/rt_sw.h"
int main(int argc, char **argv)
{
    unsigned n, count = argc > 1 ? (unsigned)atoi(argv[1]) : 460;
    u8 seen = 0;
#ifdef FFT_CAMERA_CUT_DEMO
    Unit original[NUNIT]; u32 seed;
#endif
    sw_init(65);
    if (!W || !pbank || !demo_look) return 2;
#ifdef FFT_CAMERA_CUT_DEMO
    memcpy(original,st.unit,sizeof original); seed=rng32;
#endif
    for (n = 0; n < count; n++) {
        if (!sw_step(0)) return 3;
#ifdef FFT_CAMERA_CUT_DEMO
        if (st.ptrans || st.cphase || st.cenabled) return 5;
        if (memcmp(original,st.unit,sizeof original) || rng32 != seed) return 6;
#endif
        if (st.perspective && !st.ptrans && !st.cphase) seen |= st.pdist == 80 ? 1 : 2;
        if (st.cphase) seen |= 4;
        printf("%u %04X\n", n, sw_checksum());
    }
#ifdef FFT_CAMERA_CUT_DEMO
    if (seen != 3 || st.perspective || st.rot || st.unit[1].hp != 43 || demo_time != 480) return 4;
#else
    if (seen != 7 || st.cphase || st.perspective || st.unit[1].hp != 31 || demo_time != 352) return 4;
#endif
    return 0;
}
#endif
