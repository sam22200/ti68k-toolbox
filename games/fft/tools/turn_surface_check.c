// Exercise legal pivots at map corners, street and roof, plus subpixel anchor phases.
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
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#endif

static u16 surface_checks(void)
{
    static const u8 pivots[6][2]={{0,0},{MAP_W-1,0},{0,MAP_H-1},{MAP_W-1,MAP_H-1},{4,5},{6,9}};
    u8 pivot,phase,c,angle; u16 n=0;
    game_scenario(65);
#ifdef FFT_TURN_TEXTURE
    if(!tt_ready) return 0;
#endif
#ifdef FFT_TURN_TEXTURE_QUARTER_X
    // Independent full-screen oracle for the two-stage horizontal expansion LUT.
    {
        u16 i,x,y; u8 plane;
        for(i=0;i<500;i++) {half[0][i]=(u8)(i*37U+(i>>3));half[1][i]=(u8)~half[0][i];}
        tt_expand_horizontal();
        for(plane=0;plane<2;plane++) for(y=0;y<100;y++) for(x=0;x<160;x++) {
            const u8 *dest=plane ? rt_dark:rt_light;
            u8 expected=(half[plane][y*5+(x>>5)]>>(7-((x>>2)&7)))&1;
            if(((dest[y*RT_PBYTES+(x>>3)]>>(7-(x&7)))&1)!=expected) return 0;
        }
    }
#endif
    for(pivot=0;pivot<6;pivot++) for(phase=0;phase<4;phase++) for(c=0;c<8;c++) {
        st.rot=c>>1; st.cx=pivots[pivot][0]; st.cz=pivots[pivot][1];
        cam_target(&st.camx,&st.camy);
#ifdef FFT_TURN_TEXTURE_QUARTER_X
        st.camx+=phase; // All four horizontal rounding phases; native vertical pixels.
#else
        st.camx+=phase&1; st.camy+=phase>>1;
#endif
        st.turn=c&1 ? -1:1;
        for(angle=0;angle<3;angle++) {
            st.turn_t=angle; game_render();
#ifdef __m68k__
            BENCH_SHOT(rt_dark);
#else
            printf("%u,%04X\n",n,sw_checksum());
#endif
            n++;
        }
        st.turn=0;
    }
    return n;
}

#ifndef __m68k__
int main(void)
{
    sw_init(65); return surface_checks()==576 ? 0:1;
}
#else
void _main(void)
{
    void *planes=malloc(2*RT_PSIZE); u16 n;
    rt_keys=rt_prev=0; rt_seed=1; rt_frame=0; rt_state=0; rt_state_size=0;
    if(!planes || !get_fonts()) return;
    rt_dark=planes; rt_light=(u8 *)planes+RT_PSIZE; BENCH_DARK_FIRST(1);
    game_init(); n=surface_checks(); BENCH_VALUE(0x74000000UL|n);
    free(planes);
}
#endif
