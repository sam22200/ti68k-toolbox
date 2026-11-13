// Actual chapter renderer, optional archive bank and source poses; no hardware UI.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef off
#undef _main
#define _main ko_runtime_main
#define game_render ko_no_runtime_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void ko_no_runtime_render(void){}
#undef _main
#define _main __main
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
static u16 checks,failures;static u32 digest=17;
static const u8 source_jobs[6]={133,100,97,109,106,94};
static void check(u32 a,u32 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("monster-ko line%u: %lu expected%lu\n",line,(unsigned long)a,(unsigned long)b);
#endif
    }
}
#define CHECK(a,b) check((u32)(a),(u32)(b),__LINE__)
static u32 pixels(void)
{
    u16 r,x;u8 p;u32 hash=17;
    for(p=0;p<2;p++)for(r=0;r<100;r++)for(x=0;x<20;x++)
        hash=(hash<<1|hash>>31)^((u8 *)(p?rt_dark:rt_light))[r*RT_PBYTES+x];
    return hash;
}
static void shot(u8 n)
{
    u32 value=pixels();digest=(digest<<1|digest>>31)^value;
#ifdef __m68k__
    BENCH_VALUE(value);
#else
    char name[40];sprintf(name,"pose-%02u.png",n);sw_write_png(name,1);
    printf("ko-screen %u %08lX\n",n,(unsigned long)value);
#endif
}
static void face(u8 dir)
{
    u8 f;for(f=0;f<4;f++){st.unit[0].face=f;if(unit_direction(0,0)==dir)return;}
    CHECK(0,1);
}
static void background(void){draw_rect(0,0,160,100,C_LGRAY);}
static void trials(void)
{
    Unit *u;u8 i,dir;u16 size,cell;u8 *copy;const u8 *bank;u32 before,seed;
    CHECK(W!=RT_NULL,1);CHECK(ko_gfx!=RT_NULL,1);if(!W||!ko_gfx)return;
    bank=resource_file("ffko",&size);CHECK(monster_ko_valid(bank,size),1);
    copy=malloc(size);CHECK(copy!=RT_NULL,1);if(!copy)return;
    memcpy(copy,bank,size);copy[0]=0;CHECK(monster_ko_valid(copy,size),0);
    memcpy(copy,bank,size);WR16(copy,4,160);CHECK(monster_ko_valid(copy,size),0);
    memcpy(copy,bank,size);WR16(copy,2,65535);CHECK(monster_ko_valid(copy,size),0);
    memcpy(copy,bank,size);WR16(copy,3,65535);CHECK(monster_ko_valid(copy,size),0);
    memcpy(copy,bank,size);WR16(copy,164,32767);CHECK(monster_ko_valid(copy,size),0);
    CHECK(monster_ko_valid(bank,size-1),0);free(copy);
    st.mode=M_MENU;st.banner=st.open_stage=st.open_fade=st.end=0;st.morph_mask=0;
    st.rot=0;st.camx=st.camy=0;st.pending[0].state=0;
    u=&st.unit[0];u->hp=0;u->dc=3;u->team=TEAM_ENEMY;memset(&u->statuses,0,sizeof u->statuses);
    // Source image equality uses a dry cell and no scenery in front of the actor.
    for(cell=0;cell<MAP_N;cell++){unit_place(u,cell);if(!unit_in_water(0))break;}
    CHECK(cell<MAP_N,1);W->feet[0].depth=255;
    seed=rng32;
    for(i=0;i<6;i++)for(dir=0;dir<4;dir++){
        u->job=source_jobs[i];face(dir);background();
        CHECK(draw_monster_ko(80,68,0,0,255),1);shot((u8)(4*i+dir));
        CHECK(u->hp,0);CHECK(u->dc,3);CHECK(rng32,seed);
    }
    // A warm hit must not prepare again. Simulate eviction of either half:
    // remembered indexes are hints, never proof of current cache ownership.
    u->job=133;face(0);background();CHECK(draw_monster_ko(80,68,0,0,255),1);before=pixels();
    i=W->actor_misses;background();CHECK(draw_monster_ko(80,68,0,0,255),1);
    CHECK(pixels(),before);CHECK(W->actor_misses,i);
    dir=(u8)W->cover[0].cm[0];ac_key[dir]=65535U;memset(acache[dir],0,UNIT_FB);
    background();CHECK(draw_monster_ko(80,68,0,0,255),1);CHECK(pixels(),before);
    dir=(u8)W->cover[0].cm[1];ac_key[dir]=65535U;memset(acache[dir],0,UNIT_FB);
    background();CHECK(draw_monster_ko(80,68,0,0,255),1);CHECK(pixels(),before);
    // Unsupported human jobs retain exactly the old renderer and markers.
    u->job=0x4a;face(0);W->actor_budget=255;background();
    CHECK(draw_monster_ko(80,68,0,0,255),0);draw_unit_covered(80,68,0,0,RT_NULL);before=pixels();
    ko_gfx=RT_NULL;background();draw_unit_covered(80,68,0,0,RT_NULL);CHECK(pixels(),before);ko_gfx=bank;
    // Actual marker path, scenery cover, horizontal clipping and water clipping.
    u->job=133;W->feet[0].depth=0;face(2);background();
    W->cover[0].depth=0;draw_unit_covered(80,68,0,0,&W->cover[0]);shot(24);
    W->feet[0].depth=255;background();CHECK(draw_monster_ko(0,68,0,0,255),1);shot(25);
    for(cell=0;cell<MAP_N;cell++){unit_place(u,cell);if(unit_in_water(0))break;}
    CHECK(cell<MAP_N,1);background();CHECK(draw_monster_ko(80,68,0,0,255),1);shot(26);
    // Every dissolve phase uses the same object frame and consumes no game RNG.
    u->dc=DC_CRYSTAL;st.morph_mask=1;st.tick=0;
    for(i=0;i<5;i++){st.morph_t=(u8)(i*8);background();CHECK(draw_monster_ko(80,68,0,0,255),1);shot((u8)(27+i));}
    st.morph_mask=0;background();CHECK(draw_monster_ko(80,68,0,0,255),0);CHECK(rng32,seed);
    CHECK(monster_ko_frame(bank,255,0,&i)==RT_NULL,1);
    // Reentry reacquires the borrowed archive pointer and keeps the same layout.
    chapter_now=0;game_scenario(261);CHECK(ko_gfx==bank,1);
#ifdef __m68k__
    CHECK(sizeof(Work)==29310,1);CHECK(sizeof(State)==2224,1);
#else
    CHECK(sizeof(Work)==29904,1);CHECK(sizeof(State)==2276,1);
#endif
}
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("monster-ko %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){ko_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
