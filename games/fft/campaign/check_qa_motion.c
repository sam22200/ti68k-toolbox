// Every recoil phase: fresh terrain cover versus the live cache, all source views.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main motion_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "battle.c"
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
#endif
static u16 checks,failures;
static u32 hash=17;
#define CHECK(c) do{checks++;if(!(c))failures++;}while(0)
static void trial(void)
{
    u8 rot,kind,t,i;
    for(rot=0;rot<4;rot++)for(kind=0;kind<3;kind++) {
        u8 a,b,pushed=0;
        game_scenario(101);st.rot=rot;st.turn=0;st.want=255;st.ai=st.banner=0;
        st.mode=M_BROWSE;cam_target(&st.camx,&st.camy);
        // Prime old-position feet, terrain covers and all four scenery quarters
        // before applying the push; cold caches cannot reveal an invalidation bug.
        for(i=0;i<12;i++){W->actor_budget=255;W->ac_now++;game_render();}
        st.mode=M_ACT;st.hit=1;st.dmg=1;
        st.abil=kind==0?NO_ABIL:kind==1?0x93:0x94; // weapon / Dash / Throw Stone
        for(a=0;a<NUNIT&&!pushed;a++)for(b=0;b<NUNIT&&!pushed;b++)if(a!=b) {
            st.push=0;knockback(a,b);
            if(st.push){st.atk=a;st.tgt=b;pushed=1;}
        }
        CHECK(pushed);if(!pushed)continue;
        core_action_begin(&st.action,st.atk,NUNIT);
        st.act_t=ACT_HIT;attack_step();
        cursor_place(UNIT_CELL(&st.unit[st.tgt]));
        for(t=ACT_HIT;t<ACT_END;t++) {
            Feet f;
            st.act_t=t;st.tick=t;W->actor_budget=1;W->ac_now++;W->hold=W->frz=0;
            cam_target(&st.camx,&st.camy);game_render();
            for(i=0;i<NUNIT;i++) {
                Cover *c=&W->cover[i];u16 cm[COVER_H];
                s16 bx,by;
                f=unit_feet(st.rot,i);
                if(i==st.tgt&&hit_flash(i)) {
                    static const s8 dx[4]={-1,-1,1,1},dy[4]={1,-1,1,-1};
                    u8 dir=unit_direction(st.rot,st.atk),age=t-ACT_HIT,step=age<6?2:age<10?1:0;
                    f.x+=dx[dir]*step;f.y+=dy[dir]*(step!=0);
                }
                bx=f.x-st.camx-8;by=f.y-st.camy-UNIT_FOOT;
                if(bx<=-16||bx>=160||by<=-UNIT_SH||by-FOE_H>=100)continue;
                cover(cm,f.x,f.y,f.depth);
                CHECK(c->x==f.x&&c->y==f.y&&c->depth==f.depth&&c->rot==st.rot);
                CHECK(!memcmp(cm,c->cm,sizeof cm));
            }
            for(i=0;i<2;i++) {
                const u8 *p=i?rt_dark:rt_light;u16 y,x;
                for(y=0;y<100;y++)for(x=0;x<20;x++)hash=(hash<<1|hash>>31)^p[y*RT_PBYTES+x];
            }
            // Actual damage/EXP/JP headroom, including visual recoil.
            f=unit_feet(st.rot,st.tgt);CHECK(f.y-st.camy>=CAM_MT);
        }
    }
}
#ifndef __m68k__
static void camera_preview(const char *folder)
{
    u8 rot=3,i,top=0,bottom=0;
    Feet a,t;
    char path[512];
    game_scenario(101);st.ai=st.banner=st.turn=0;st.rot=rot;st.want=255;
    for(i=1;i<NUNIT;i++) {
        Feet f=unit_feet(rot,i);
        if(f.y<unit_feet(rot,top).y)top=i;
        if(f.y>unit_feet(rot,bottom).y)bottom=i;
    }
    st.mode=M_ACT;st.atk=bottom;st.tgt=top;st.abil=0x94;st.act_t=ACT_HIT+8;
    st.hit=1;st.dmg=12;st.gain=0;st.tick=0;cursor_place(UNIT_CELL(&st.unit[top]));
    W->num_ok=0;
    a=unit_feet(rot,bottom);t=unit_feet(rot,top);
    st.camx=((a.x+t.x)>>1)-80;st.camy=((a.y+t.y)>>1)-56;cam_clamp(&st.camx,&st.camy);
    for(i=0;i<24;i++){W->actor_budget=255;W->ac_now++;game_render();}
    snprintf(path,sizeof path,"%s/action-headroom-before.png",folder);sw_write_png(path,1);
    cam_target(&st.camx,&st.camy);
    for(i=0;i<24;i++){W->actor_budget=255;W->ac_now++;game_render();}
    CHECK(t.y-st.camy>=CAM_MT);
    snprintf(path,sizeof path,"%s/action-headroom-after.png",folder);sw_write_png(path,1);
}
#endif
#ifndef __m68k__
int main(int argc,char **argv){sw_init(101);trial();if(argc>1)camera_preview(argv[1]);printf("motion %08lX failures %u checks %u\n",(unsigned long)hash,failures,checks);return failures!=0;}
#else
void _main(void){motion_runtime_main();trial();BENCH_VALUE(hash);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
