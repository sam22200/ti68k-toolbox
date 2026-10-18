// Automatic 256 Hz wall-clock audit with the config review's fixed-plane presenter.
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
#include "camera_tiemu_demo.c"
typedef struct { u16 cold,prepared,reopen,preparation,frames,worst; } IdleTime;
static IdleTime measured[2];
static u16 last;
static void present(void)
{
    GrayWaitNSwitches(1);
    FastCopyScreen_R(rt_light,GrayGetPlane(LIGHT_PLANE));
    FastCopyScreen_R(rt_dark,GrayGetPlane(DARK_PLANE));
    while((u16)(ticks-last)<8) pokeIO(0x600005,0x1D);
    last+=8; if((u16)(ticks-last)>16) last=ticks;
    rt_frame++;
}
void _main(void)
{
    INT_HANDLER old1=GetIntVec(AUTO_INT_1); short font=FontGetSys();
    void *dbuf=RT_NULL; u8 n,ok=0; u16 start,frame;
    memset(measured,0,sizeof measured); tm_big=0; tm_cur.map=0;
    rt_frame=0; rt_keys=rt_prev=0; rt_seed=1; rt_state=0; rt_state_size=0;
    if(!get_fonts()) return;
    dbuf=malloc(GRAYDBUFFER_SIZE); if(!dbuf) return;
    SetIntVec(AUTO_INT_1,DUMMY_HANDLER); if(!GrayOn()) goto done;
    GraySetInt1Handler(tick_handler); GrayDBufInit(dbuf);
    rt_light=GrayDBufGetHiddenPlane(LIGHT_PLANE); rt_dark=GrayDBufGetHiddenPlane(DARK_PLANE);
    game_init(); game_scenario(65);
    if(!W || !pbank || !pidle || !demo_look) goto grey_done;
    for(n=0;n<2;n++) {
        IdleTime *r=&measured[n]; game_scenario(65); st.pzoom=n;
        pidle->stage=pidle->delay=0;
        last=start=ticks; perspective_enter(1); game_render(); present(); r->cold=ticks-start;
        st.perspective=0; st.config=1; pvalid=0; pidle->stage=pidle->delay=0;
        last=start=ticks;
        do {
            frame=ticks; game_render(); present();
            if((u16)(ticks-frame)>r->worst) r->worst=ticks-frame;
            r->frames++;
        } while(pidle->stage!=PIDLE_READY && r->frames<1500);
        r->preparation=ticks-start; if(pidle->stage!=PIDLE_READY) goto grey_done;
        st.config=0; last=start=ticks;
        perspective_enter(1); game_render(); present(); r->prepared=ticks-start;
        st.perspective=0; pvalid=0; last=start=ticks;
        perspective_enter(1); game_render(); present(); r->reopen=ticks-start;
    }
    ok=1;
grey_done:
    GraySetInt1Handler(DUMMY_HANDLER); GrayOff();
done:
    SetIntVec(AUTO_INT_1,old1); free(dbuf); ClrScr(); FontSetSys(F_4x6);
    if(!ok) {
        printf_xy(0,0,"PREFETCH FAILED / RAM / DATA");
        printf_xy(0,10,"W%u mesh%u cache%u",!!W,!!pbank,!!pidle);
        printf_xy(0,20,"look%u",!!demo_look);
        printf_xy(0,30,"free %lu max %lu",(u32)HeapAvail(),(u32)HeapMax());
    }
    else {
        printf_xy(0,0,"TIEMU HW%u 256Hz preparation geometry",(u16)HW_VERSION);
        printf_xy(0,10,"view cold ready reopen (ms)");
        for(n=0;n<2;n++) printf_xy(0,20+n*10,"%c %lu %lu %lu",n ? 'W' : 'C',
            (u32)measured[n].cold*1000/256,(u32)measured[n].prepared*1000/256,
            (u32)measured[n].reopen*1000/256);
        printf_xy(0,44,"view prep ms frames max frame ms");
        for(n=0;n<2;n++) printf_xy(0,54+n*10,"%c %lu %u %lu",n ? 'W' : 'C',
            (u32)measured[n].preparation*1000/256,measured[n].frames,(u32)measured[n].worst*1000/256);
        printf_xy(0,80,"Extra heap: %u bytes",(u16)PIDLE_BYTES);
        printf_xy(0,90,"Fixed copies, grey, 32 FPS cap");
    }
    GKeyFlush(); ngetchx(); FontSetSys(font);
}
