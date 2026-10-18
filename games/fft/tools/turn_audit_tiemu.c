// Automatic real-time capture plus 256 Hz timings, grayscale and ordinary frame cap.
#define USE_TI89
#define MIN_AMS 200
#define SAVE_SCREEN
#include <tigcclib.h>
#undef _main
#define _main rt_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#include "turn_audit.inc"
typedef struct {u16 total,worst; u8 frames,ok;} TurnResult;
static TurnResult results[8];
void _main(void)
{
    INT_HANDLER old1=GetIntVec(AUTO_INT_1); short font=FontGetSys();
    void *dbuf=RT_NULL; u8 c,ready=0; u16 last,started,elapsed,k;
    memset(results,0,sizeof results); tm_big=0; tm_cur.map=0;
    rt_frame=0; rt_keys=rt_prev=0; rt_seed=1; rt_state=0; rt_state_size=0;
    if(!get_fonts()) return;
    dbuf=malloc(GRAYDBUFFER_SIZE); if(!dbuf) return;
    SetIntVec(AUTO_INT_1,DUMMY_HANDLER); if(!GrayOn()) goto done;
    GraySetInt1Handler(tick_handler); GrayDBufInit(dbuf);
    rt_light=GrayDBufGetHiddenPlane(LIGHT_PLANE); rt_dark=GrayDBufGetHiddenPlane(DARK_PLANE);
    game_init();
    for(c=0;c<8;c++) {
        TurnResult *r=&results[c]; turn_setup(c);
        // Hold the source view for one second, outside the timed interaction.
        draw_rect(0,0,160,8,C_WHITE); draw_text(2,0,c&1 ? "F1 / LEFT":"F5 / RIGHT",F_SMALL,C_BLACK);
        GrayDBufToggleSync(); rt_light=GrayDBufGetHiddenPlane(LIGHT_PLANE); rt_dark=GrayDBufGetHiddenPlane(DARK_PLANE);
        started=ticks; while((u16)(ticks-started)<256) pokeIO(0x600005,0x1D);
        last=ticks;
        do {
            started=ticks;
            if(!turn_next()) goto grey_done;
            GrayDBufToggleSync(); rt_light=GrayDBufGetHiddenPlane(LIGHT_PLANE); rt_dark=GrayDBufGetHiddenPlane(DARK_PLANE);
            while((u16)(ticks-last)<8) pokeIO(0x600005,0x1D);
            last+=8; if((u16)(ticks-last)>16) last=ticks;
            elapsed=ticks-started; r->total+=elapsed;
            if(elapsed>r->worst) r->worst=elapsed;
        } while(st.turn && turn_frame<16);
        r->frames=turn_frame; r->ok=turn_verify();
        // Keep the destination visible before preparing the next independent case.
        for(k=0;k<24;k++) {
            rt_keys=rt_prev=0; if(!fft_full_update()) goto grey_done;
            game_render(); GrayDBufToggleSync(); rt_light=GrayDBufGetHiddenPlane(LIGHT_PLANE); rt_dark=GrayDBufGetHiddenPlane(DARK_PLANE);
            started=ticks; while((u16)(ticks-started)<8) pokeIO(0x600005,0x1D);
        }
    }
    ready=1;
grey_done:
    GraySetInt1Handler(DUMMY_HANDLER); GrayOff();
done:
    SetIntVec(AUTO_INT_1,old1); free(dbuf); ClrScr(); FontSetSys(F_4x6);
    if(!ready) printf_xy(0,0,"TURN AUDIT FAILED");
    else {
#ifdef FFT_TURN_TEXTURE
        printf_xy(0,0,"TIEMU HW%u 256Hz affine tex",(u16)HW_VERSION);
#else
        printf_xy(0,0,"TIEMU HW%u 256Hz low poly",(u16)HW_VERSION);
#endif
        printf_xy(0,7,"case total ms n max ms ok");
        for(c=0;c<8;c++) printf_xy(0,14+c*8,"%u %lu %u %lu %u",(u16)c,
            (u32)results[c].total*1000/256,(u16)results[c].frames,
            (u32)results[c].worst*1000/256,(u16)results[c].ok);
    }
    GKeyFlush(); ngetchx(); FontSetSys(font);
}
