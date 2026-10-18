// Actual native cooperative work and cold/prepared picture equality. No UI/timing guesses.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main rt_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "camera_tiemu_demo.c"
#include <stddef.h>
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
static u8 success=1;
#define CHECK(v) do { if(!(v)) { success=0; REPORT_FAILURE(__LINE__); } } while(0)
#ifdef __m68k__
#define REPORT_FAILURE(n) BENCH_VALUE(0x7f000000UL | (n))
#else
#define REPORT_FAILURE(n) fprintf(stderr,"prefetch check failed at %u\n",(unsigned)(n))
#endif
static u16 memory_hash(const void *mem,u16 bytes)
{
    const u8 *p=mem; u16 s=0;
    while(bytes--) s=(u16)((s<<1)|(s>>15))^*p++;
    return s;
}
static u16 work_hash(void)
{
    u16 first=offsetof(Work,status),last=first+sizeof W->status;
    return memory_hash(W,first)^memory_hash((u8 *)W+last,sizeof *W-last);
}
static void shot(u8 n)
{
#ifdef __m68k__
    BENCH_VALUE(0x79000000UL|n); BENCH_SHOT(rt_dark);
#else
    printf("shot,%u,%04X\n",n,sw_checksum());
#endif
}
static void suite(void)
{
    u8 n;
    game_scenario(65);
    CHECK(pidle && pbank && W && demo_look);
#ifdef __m68k__
    BENCH_VALUE(0x7a000000UL|PIDLE_BYTES);
    BENCH_VALUE(0x7c000000UL|sizeof *W);
#else
    printf("memory,%u\n",(unsigned)PIDLE_BYTES);
#endif
    for(n=0;n<16 && success;n++) {
        State before; u16 frames=0,w_hash,v_hash,s_hash,screen_hash;
#ifdef __m68k__
        u32 total=0,worst=0,t;
#endif
        game_scenario(65); st.unit[U_RAMZA].face=n>>2; st.pzoom=(n>>1)&1; st.pwire=n&1;
        st.config=1; game_render();
        // A partial job yields to input without touching its payload.
        for(frames=0;frames<20;frames++) pidle_tick(PIDLE_BUDGET);
        { u8 stage=pidle->stage; u16 index=pidle->index;
          rt_keys=K_DOWN; pidle_tick(PIDLE_BUDGET); rt_keys=0;
          CHECK(pidle->stage==stage && pidle->index==index && !pidle->delay); }
        before=st;
        w_hash=work_hash(); v_hash=memory_hash(vbuf,(u16)VIEW_BYTES);
        s_hash=memory_hash(scene_l,2*SC_PLANE);
        screen_hash=memory_hash(rt_light,RT_PSIZE)^memory_hash(rt_dark,RT_PSIZE);
        frames=0;
        while(pidle->stage!=PIDLE_READY && frames<3000) {
#ifdef __m68k__
            t=BENCH_CYCLES;
#endif
            pidle_tick(PIDLE_BUDGET);
#ifdef __m68k__
            t=BENCH_CYCLES-t; total+=t; if(t>worst) worst=t;
#endif
            frames++;
        }
        CHECK(frames<3000 && pidle->stage==PIDLE_READY);
        CHECK(!memcmp(&before,&st,sizeof st));
        CHECK(work_hash()==w_hash && memory_hash(vbuf,(u16)VIEW_BYTES)==v_hash);
        CHECK(memory_hash(scene_l,2*SC_PLANE)==s_hash);
        CHECK((memory_hash(rt_light,RT_PSIZE)^memory_hash(rt_dark,RT_PSIZE))==screen_hash);
        st.config=0; perspective_enter(1); st.tick=0;
#ifdef __m68k__
        BENCH_VALUE(0x78000000UL|n); BENCH_VALUE(frames); BENCH_VALUE(total); BENCH_VALUE(worst);
        t=BENCH_CYCLES;
#endif
        game_render();
#ifdef __m68k__
        BENCH_VALUE(BENCH_CYCLES-t);
#endif
        shot(n*2);
        // Same camera, same current actors, ordinary synchronous rendering as the oracle.
        pidle->stage=0; pvalid=0;
#ifdef __m68k__
        t=BENCH_CYCLES;
#endif
        game_render();
#ifdef __m68k__
        BENCH_VALUE(BENCH_CYCLES-t);
#endif
        shot(n*2+1);
        if(n==14) {
            // Geometry can survive another unit moving; its actor/mask must be rebuilt.
            st.unit[1].x++; st.tick=7; pidle->stage=PIDLE_READY; pvalid=0;
            game_render(); shot(32);
            pidle->stage=0; pvalid=0; game_render(); shot(33);
        }
        // An opposite/moved/changed preset must never consume stale front geometry.
        pidle->stage=PIDLE_READY;
        st.pyaw^=32; CHECK(!pidle_take()); st.pyaw^=32;
        st.unit[U_RAMZA].x++; CHECK(!pidle_take()); st.unit[U_RAMZA].x--;
        st.pzoom^=1; CHECK(!pidle_take()); st.pzoom^=1;
        st.pwire^=1; CHECK(!pidle_take()); st.pwire^=1;
        st.psolid=0; CHECK(!pidle_take()); st.psolid=1;
        // Allocation failure is a supported synchronous fallback.
        { PIdle *saved=pidle; pidle=RT_NULL; CHECK(!pidle_take()); pidle_tick(PIDLE_BUDGET); pidle=saved; }
        pidle->stage=0;
    }
#ifdef __m68k__
    BENCH_VALUE(0x7b000000UL|success);
#else
    printf("ok,%u\n",success);
#endif
}
#ifndef __m68k__
int main(void) { sw_init(65); suite(); return !success; }
#else
void _main(void)
{
    void *planes=malloc(2*RT_PSIZE);
    rt_keys=rt_prev=0; rt_frame=0; rt_seed=1; rt_state=0; rt_state_size=0;
    if(!planes || !get_fonts()) return;
    rt_dark=planes; rt_light=(u8 *)planes+RT_PSIZE; BENCH_DARK_FIRST(1);
    game_init(); suite(); free(planes);
}
#endif
