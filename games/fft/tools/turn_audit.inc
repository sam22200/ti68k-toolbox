// Actual native F1/F5 path: three low-poly frames, then the textured target view.
#include "camera_tiemu_demo.c"
static u8 turn_case, turn_frame, turn_ok;
static Unit turn_units[NUNIT];
static u32 turn_seed;

static void turn_setup(u8 n)
{
    turn_case=n; turn_frame=0; turn_ok=1;
    game_scenario(65); st.rot=n>>1; cam_target(&st.camx,&st.camy);
    rt_keys=rt_prev=0; scene_rot=vbuf_rot=0xff; W->bg_ok=0;
    game_render();
    memcpy(turn_units,st.unit,sizeof turn_units); turn_seed=rng32;
#ifdef FFT_TURN_TEXTURE
    if(!tt_ready) turn_ok=0;
#endif
}

static u8 turn_next(void)
{
    rt_prev=rt_keys; rt_keys=turn_frame ? 0 : turn_case&1 ? K_F1:K_F5;
    if (!fft_full_update()) return 0;
    game_render(); rt_frame++; turn_frame++;
#ifdef FFT_TURN_ORDER_LABELS
    { char label[]="TURN 0"; label[5]+=(char)turn_frame;
      draw_rect(0,93,160,7,C_WHITE); draw_text(2,93,label,F_SMALL,C_BLACK); }
#endif
    if (memcmp(turn_units,st.unit,sizeof turn_units) || rng32!=turn_seed
        || st.ptrans || st.perspective || st.cphase || st.mode!=M_MENU) turn_ok=0;
    return 1;
}

static u8 turn_verify(void)
{
    return turn_ok && !st.turn && turn_frame==TURN_FRAMES+1
        && st.rot==(((turn_case>>1)+(turn_case&1 ? 3:1))&3);
}
