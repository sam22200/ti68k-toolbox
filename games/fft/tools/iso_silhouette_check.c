// Hidden bodies must read as monochrome silhouettes over textured/mid-grey scenery.
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
static u16 cases,failures;
static u32 hidden_pixels;
static u8 level(u16 x,u16 y)
{
    u16 off=y*RT_PBYTES+(x>>3);u8 bit=0x80>>(x&7);
    return (!!(((u8 *)rt_light)[off]&bit))|((!!(((u8 *)rt_dark)[off]&bit))<<1);
}
static void checks(void)
{
    u8 rot,t,i,bg,r,x;u16 cm[COVER_H];
    game_scenario(65);
    for(rot=0;rot<4;rot++) {
        st.rot=rot;unpack_view(rot);
        for(t=0;t<32;t++) for(i=0;i<2;i++) {
            u8 who=i?1:U_RAMZA;Feet f=unit_feet(rot,who);Cover c;
            const u8 *g;
            st.tick=t;g=unit_frame(rot,who);
            cover(cm,f.x,f.y,f.depth);memset(&c,0,sizeof c);
            memcpy(c.cm,cm,sizeof cm);c.r0=2;c.r1=UNIT_SH-3;
            for(bg=1;bg<4;bg++) {
                draw_rect(0,0,160,100,bg);draw_unit_covered(80,70,rot,who,&c);
                for(r=2;r<UNIT_FOOT;r++) for(x=0;x<16;x++) {
                    u16 bit=0x8000U>>x,inside=RD16(g,3*UNIT_SH+r);
                    if(cm[r]&inside&bit) {
                        hidden_pixels++;
                        if(level(72+x,70-UNIT_FOOT+r)!=C_WHITE) failures++;
                    } else if(!(RD16(g,2*UNIT_SH+r)&bit)) {
                        u8 expected=((RD16(g,r)&bit)!=0)|(((RD16(g,UNIT_SH+r)&bit)!=0)<<1);
                        if(level(72+x,70-UNIT_FOOT+r)!=expected) failures++;
                    }
                }
                for(r=UNIT_FOOT;r<UNIT_SH-2;r++) for(x=0;x<16;x++) {
                    u16 bit=0x8000U>>x;
                    if(cm[r]&RD16(g,3*UNIT_SH+r)&bit)
                        if(level(72+x,70-UNIT_FOOT+r)!=bg) failures++;
                }
                cases++;
            }
        }
    }
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    sw_init(65);checks();
    printf("silhouette: %u cases, %lu hidden pixels, %u failures\n",cases,(unsigned long)hidden_pixels,failures);
    if(argc>1) {game_scenario(65);game_render();sw_write_png(argv[1],4);}
    return !hidden_pixels || failures;
}
#else
void _main(void)
{
    void *planes=malloc(2*RT_PSIZE);
    rt_keys=rt_prev=0;rt_frame=0;rt_seed=1;rt_state=0;rt_state_size=0;
    if(!planes||!get_fonts())return;
    rt_dark=planes;rt_light=(u8 *)planes+RT_PSIZE;BENCH_DARK_FIRST(1);
    game_init();checks();
    BENCH_VALUE(0x77000000UL|cases);BENCH_VALUE(hidden_pixels);BENCH_VALUE(failures);
    free(planes);
}
#endif
