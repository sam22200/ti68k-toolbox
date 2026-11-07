// Actual campaign geometry at all three moving angles, both keys and four views.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main qa_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#define FFT_BATTLE
#define FFT_SOURCE_CAMERA
#include "../fft.h"
#ifdef QA_HALF_TURN                    // historical80x50 geometry control, never shipping
#undef FFT_TURN_LOWPOLY_NATIVE
#endif
#include "../fft.c"
#ifndef __m68k__
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "../../../runtime/platform-sw/rt_sw.h"
static const char *output;
#endif
static u32 digest=17;
static u16 turn_scenario=101;
static u16 failures,checks;
static void check(u8 yes){checks++;if(!yes)failures++;}
static void shot(u8 c,u8 f)
{
    u8 p;u16 y,x;
    for(p=0;p<2;p++)for(y=0;y<100;y++)for(x=0;x<20;x++)
        digest=(digest<<1|digest>>31)^((const u8 *)(p?rt_dark:rt_light))[y*RT_PBYTES+x];
#ifndef __m68k__
    if(output) {char path[512];snprintf(path,sizeof path,"%s/turn-%u-%u.png",output,c,f);sw_write_png(path,1);}
#else
    (void)c;(void)f;BENCH_SHOT(rt_dark);
#endif
}
static void trials(void)
{
    u8 c,f;
    for(c=0;c<8;c++) {
        Unit before[NUNIT];u32 random;
        u8 i,source=(c&7)>>1;
        game_scenario(turn_scenario);st.rot=source;st.ai=0;st.mode=M_BROWSE;st.tick=0;
        st.banner=0;st.turn=0;st.want=255;cam_target(&st.camx,&st.camy);
        W->frz=W->hold=0;W->actor_budget=255;game_render();shot(c,0);
        memcpy(before,st.unit,sizeof before);random=rng32;
        for(f=0;f<3;f++) {
            rt_prev=rt_keys;rt_keys=f?0:c&1?K_F1:K_F5;
            check(game_update());check(st.turn==(c&1?-1:1)&&st.turn_t==f&&st.rot==source);
            W->actor_budget=255;game_render();shot(c,f+1);
        }
        rt_prev=rt_keys;rt_keys=0;check(game_update());
        check(!st.turn&&!st.turn_t&&st.rot==((source+(c&1?3:1))&3));
        for(i=0;i<24;i++){W->actor_budget=255;W->hold=W->frz=0;game_render();}
        shot(c,4);
        check(!memcmp(before,st.unit,sizeof before)&&random==rng32);
    }
}
#if !defined(__m68k__) && !defined(QA_HALF_TURN)
// Independent angular/cardinal oracle for the ACTIVE mirrored source camera.
// Diagnostic endpoints use the same mesh; no historical perspective banks are loaded.
static unsigned direction_oracle(void)
{
    unsigned pivot,c,step,x,z,n=0;
    {
        game_scenario(turn_scenario);
        for(pivot=0;pivot<6;pivot++)for(c=0;c<8;c++) {
            s16 oldx[MAP_W+1][MAP_H+1],oldy[MAP_W+1][MAP_H+1],cu,cv,px,py;
            int direction=c&1?-1:1;
            st.rot=c>>1;
            st.cx=pivot==0||pivot==2?0:pivot==1||pivot==3?MAP_W-1:pivot==4?4:6;
            st.cz=pivot<2?0:pivot<4?MAP_H-1:pivot==4?5:9;
            cursor_place((u8)node_at(st.cx,st.cz,0));
            cam_target(&st.camx,&st.camy);view_of(st.rot,st.cx,st.cz,&cu,&cv);
            px=scene_x(st.rot,cu,cv)-st.camx;py=scene_y(cu,cv)+HH-st.camy;
            for(step=0;step<=4;step++) {
                double angle=(45.0+90.0*st.rot+direction*22.5*step)*acos(-1.0)/180.0;
                st.turn=step?direction:0;st.turn_t=step?step-1:0;draw_turn();
                for(z=0;z<=MAP_H;z++)for(x=0;x<=MAP_W;x++) {
                    int dx=2*((int)x-st.cx)-1,dz=2*((int)z-st.cz)-1;
                    double ex=px-6.0*sqrt(2.0)*(dx*cos(angle)-dz*sin(angle));
                    double ey=py+3.0*sqrt(2.0)*(dx*sin(angle)+dz*cos(angle));
                    assert(fabs(gx[x][z]-ex)<2.0&&fabs(gy[x][z]-ey)<2.0);
                    if(step==0||step==4) {
                        int rot=(st.rot+(step?direction:0))&3,rx=dx,rz=dz;
                        if(rot==1){rx=-dz;rz=dx;}if(rot==2){rx=-dx;rz=-dz;}if(rot==3){rx=dz;rz=-dx;}
                        assert(gx[x][z]==px-(rx-rz)*HW/2&&gy[x][z]==py+(rx+rz)*HH/2);
                    }
                    if(step) {
                        long cross=(long)(oldx[x][z]-px)*(gy[x][z]-py)-(long)(oldy[x][z]-py)*(gx[x][z]-px);
                        assert(direction*cross<0); // x reflection reverses screen handedness
                    }
                    oldx[x][z]=gx[x][z];oldy[x][z]=gy[x][z];n++;
                }
            }
        }
    }
    return n;
}
#endif
#ifndef __m68k__
int main(int argc,char **argv)
{
    if(argc>1)output=argv[1];if(argc>2)turn_scenario=(u16)atoi(argv[2]);sw_init(turn_scenario);trials();
    printf("turns %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);
#ifndef QA_HALF_TURN
    printf("direction %u active-source-camera corners\n",direction_oracle());
#endif
    return failures!=0;
}
#else
void _main(void) {turn_scenario=BENCH_ARG;qa_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
