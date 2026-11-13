// Guard the 100-row draw target and compare visible output with the physical-plane build.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main planes_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"

static u8 *guarded[2];
static u16 failures,checks;
static u32 digest=17;
#define GUARD 32
#define CHECK(c) do{checks++;if(!(c))failures++;}while(0)
static u16 pixels(void)
{
    u16 a=0,b=0,p,y,x;
    for(p=0;p<2;p++)for(y=0;y<RT_H;y++)for(x=0;x<RT_W/8;x++){
        const u8 *s=p?rt_dark:rt_light;
        a+=s[y*RT_PBYTES+x];if(a>=255)a-=255;
        b+=a;if(b>=255)b-=255;
    }
    return b<<8|a;
}
static void guards(void)
{
    u16 p,i;
    for(p=0;p<2;p++)for(i=0;i<GUARD;i++){
        CHECK(guarded[p][i]==0xa5);
        CHECK(guarded[p][GUARD+RT_DRAW_BYTES+i]==0xa5);
    }
}
static u8 setup(void)
{
    u8 p;
    for(p=0;p<2;p++){
        guarded[p]=malloc(RT_DRAW_BYTES+2*GUARD);
        if(!guarded[p])return 0;
        memset(guarded[p],0xa5,RT_DRAW_BYTES+2*GUARD);
    }
    rt_light=guarded[0]+GUARD;rt_dark=guarded[1]+GUARD;
    return 1;
}
static void remember(void)
{
    digest=(digest<<1|digest>>31)^pixels();guards();
}
static void primitives(void)
{
    union {u8 b[64];u16 w[32];u32 l[16];} l,d,m;
    const s16 ys[]={-16,-3,0,90,95,99,100,127};
    const s16 xs[]={-31,-9,0,153,223,239};
    RtSprite sprite;u16 w,r,i,j,masked;
    draw_clear();remember();
    for(w=8;w<=32;w<<=1){
        for(r=0;r<16;r++){
            if(w==8){l.b[r]=0x81;d.b[r]=0x42;m.b[r]=0x24;}
            else if(w==16){l.w[r]=0x8118;d.w[r]=0x4224;m.w[r]=0x2442;}
            else{l.l[r]=0x81181881UL;d.l[r]=0x42242442UL;m.l[r]=0x24424224UL;}
        }
        sprite.w=(u8)w;sprite.h=16;sprite.light=&l;sprite.dark=&d;
        for(masked=0;masked<2;masked++){
            sprite.mask=masked?&m:RT_NULL;
            for(i=0;i<sizeof ys/sizeof ys[0];i++)for(j=0;j<sizeof xs/sizeof xs[0];j++){
                draw_sprite(xs[j],ys[i],&sprite);remember();
            }
        }
    }
    for(i=0;i<sizeof ys/sizeof ys[0];i++){
        draw_text(2,ys[i],"Bottom",F_SMALL,C_BLACK);remember();
        draw_text(80,ys[i],"Edge",F_MEDIUM,C_BLACK);remember();
    }
    rt_fill(0,98,239,127,C_DGRAY);remember();
    rt_fill(0,100,239,127,C_BLACK);remember();
}
static void play(u16 scenario,u16 frames)
{
    u16 i;
    game_init();game_scenario(scenario);
    CHECK(W!=RT_NULL&&scene_l!=RT_NULL);
    for(i=0;i<frames;i++){
        rt_prev=rt_keys;rt_keys=0;
        if(i==12)rt_keys=K_F5;if(i==55)rt_keys=K_F1;
        if(i==100)rt_keys=K_ENTER;if(i==125)rt_keys=K_B;
        if(i==150){st.sel=st.act;st.mode=M_STATUS;st.page=0;W->status_i=0;}
        if(i==175){st.page=1;W->status_i=0;}
        if(i==200){st.mode=M_END;st.end=END_DEFEAT;st.banner=st.turn=st.open_stage=st.open_fade=0;W->end_n=0;}
        if(i==215)st.config=1;
        if(i<200&&!game_update())break;
        game_render();remember();rt_frame++;
    }
    digest^=(u32)st.rot<<24|(u32)st.mode<<16|st.unit[st.act].hp;
    final_release();
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    u16 scenario=argc>1?(u16)atoi(argv[1]):61;
    if(!setup())return 2;
    if(scenario==0)primitives();else play(scenario,225);
    printf("planes %08lX failures %u\n",(unsigned long)digest,failures);
    free(guarded[0]);free(guarded[1]);return failures!=0;
}
#else
void _main(void)
{
    if(!get_fonts()||!setup())return;
    if(!BENCH_ARG)primitives();else play(BENCH_ARG,225);
    BENCH_VALUE(digest);BENCH_VALUE(failures);
    free(guarded[0]);free(guarded[1]);
}
#endif
