// Actual chapter renderer with a synthetic FMB1 bank (check_monster_body.py): pose
// selection, facing/mirror, strips, cover, water crop, flash, markers, actor cache.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef off
#undef _main
#define _main body_runtime_main
#define game_render body_no_runtime_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void body_no_runtime_render(void){}
#undef _main
#define _main __main
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
static u16 checks,failures;static u32 digest=17;
static void check(u32 a,u32 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("monster-body line%u: %lu expected%lu\n",line,(unsigned long)a,(unsigned long)b);
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
    char name[40];sprintf(name,"body-%02u.png",n);sw_write_png(name,1);
    printf("body-screen %u %08lX\n",n,(unsigned long)value);
#endif
}
static void face(u8 dir)
{
    u8 f;for(f=0;f<4;f++){st.unit[0].face=f;if(unit_direction(0,0)==dir)return;}
    CHECK(0,1);
}
static void background(void){draw_rect(0,0,160,100,C_LGRAY);}
static void draw(void){background();draw_unit_covered(80,68,0,0,RT_NULL);}
static const u8 body_jobs[2]={94,133};
static void trials(void)
{
    Unit *u;u8 i,dir,n=0,misses;u16 size,cell;u8 *copy;const u8 *bank;u32 seed,before;
    u16 keys[AC_N];
    CHECK(W!=RT_NULL,1);CHECK(body_gfx!=RT_NULL,1);if(!W||!body_gfx)return;
    bank=resource_file("ffmb",&size);CHECK(monster_body_valid(bank,size),1);
    copy=malloc(size);CHECK(copy!=RT_NULL,1);if(!copy)return;
    memcpy(copy,bank,size);copy[3]='2';CHECK(monster_body_valid(copy,size),0);
    memcpy(copy,bank,size);WR16(copy,2,49);CHECK(monster_body_valid(copy,size),0);
    memcpy(copy,bank,size);copy[8]=200;CHECK(monster_body_valid(copy,size),0);
    memcpy(copy,bank,size);copy[MB_HEADER+3]=0;CHECK(monster_body_valid(copy,size),0);
    memcpy(copy,bank,size);WR16(copy+MB_HEADER,0,1000);CHECK(monster_body_valid(copy,size),0);
    {u16 base=(u16)(MB_HEADER+4*MB_ANIMS*RD16(bank,2)),f=RD16(bank+base,0);
     memcpy(copy,bank,size);copy[f+1]=8;CHECK(monster_body_valid(copy,size),0);
     memcpy(copy,bank,size);copy[f+4]|=1;copy[f+4+2*copy[f]*copy[f+1]]|=1;   // opaque under mask
     CHECK(monster_body_valid(copy,size),bank[f+4+2*bank[f]*bank[f+1]]&1?0:1);}
    CHECK(monster_body_valid(bank,size-1),0);free(copy);
    st.mode=M_MENU;st.banner=st.open_stage=st.open_fade=st.end=0;st.morph_mask=0;
    st.rot=0;st.camx=st.camy=0;st.pending[0].state=0;st.tick=0;
    u=&st.unit[0];u->hp=u->max_hp=50;u->dc=3;u->team=TEAM_GUEST;u->gfx=255;
    memset(&u->statuses,0,sizeof u->statuses);
    for(cell=0;cell<MAP_N;cell++){unit_place(u,cell);if(!unit_in_water(0))break;}
    CHECK(cell<MAP_N,1);W->feet[0].depth=255;
    seed=rng32;
    // 0..7: idle of both families in the four view directions (screens checked
    // against the source PNGs by the script, mirrors included).
    for(i=0;i<2;i++)for(dir=0;dir<4;dir++){u->job=body_jobs[i];face(dir);draw();shot(n++);}
    // Human jobs never reach the body bank; monsters never reach the human actor cache.
    CHECK(monster_body_frame(bank,0x4a,MB_IDLE,0,0)==RT_NULL,1);
    CHECK(monster_body_frame(bank,142,MB_IDLE,0,0)==RT_NULL,1);
    CHECK(monster_body_frame(bank,95,MB_IDLE,0,0)==RT_NULL,1);   // job without a family
    memcpy(keys,ac_key,sizeof keys);misses=W->actor_misses;W->actor_budget=255;
    {u8 vis[NUNIT];memset(vis,0,sizeof vis);vis[0]=1;actor_animate(vis);}
    u->job=94;face(0);draw();
    CHECK(memcmp(keys,ac_key,sizeof keys),0);CHECK(W->actor_misses,misses);
    // Idle loop: frame 1 after one wait (synthetic wait 6), back to frame 0 after two.
    face(0);st.tick=0;draw();before=pixels();
    st.tick=6;draw();CHECK(pixels()!=before,1);shot(n++);                 // 8
    st.tick=12;draw();CHECK(pixels(),before);st.tick=0;
    // Walk, attack held on its last frame, hurt, Dead.
    st.mode=M_WALK;st.sel=0;draw();shot(n++);                              // 9
    CHECK(monster_body_pose(0,0)==monster_body_frame(bank,94,2*MB_WALK,0,0),1);
    st.mode=M_ACT;st.atk=0;st.tgt=1;st.abil=NO_ABIL;
    st.act_t=ACT_WIND;draw();shot(n++);                                    // 10
    CHECK(monster_body_pose(0,0)==monster_body_frame(bank,94,2*MB_ATTACK,0,1),1);
    st.act_t=ACT_HIT-1;CHECK(monster_body_pose(0,0)==monster_body_frame(bank,94,2*MB_ATTACK,ACT_HIT-1-ACT_WIND,1),1);
    st.act_t=ACT_HIT-1;draw();shot(n++);                                   // 11
    st.atk=1;st.tgt=0;st.hit=1;st.act_t=ACT_HIT;
    if(unit_pose(0)==P_HIT){CHECK(monster_body_pose(0,0)==monster_body_frame(bank,94,2*MB_HURT,0,0),1);}
    st.mode=M_MENU;u->hp=0;draw();shot(n++);                               // 12
    CHECK(monster_body_pose(0,0)==monster_body_frame(bank,94,2*MB_DEAD,0,0),1);
    u->hp=50;
    // Enemy diamond above the taller family; flash, cover, edge clipping, water.
    u->team=TEAM_ENEMY;u->job=133;face(0);draw();shot(n++);                // 13
    st.banner=BN_START;st.banner_t=0;draw();shot(n++);st.banner=0;         // 14
    {   // 15: the first place where scenery in front changes the body (depth 0)
        s16 x,y;u8 found=0;ensure_scene(st.rot);
        W->cover[0].depth=0;W->cover[0].wet=0;
        for(y=40;y<100&&!found;y+=6)for(x=24;x<140&&!found;x+=16){
            background();draw_unit_covered(x,y,0,0,RT_NULL);before=pixels();
            background();draw_unit_covered(x,y,0,0,&W->cover[0]);
            if(pixels()!=before){found=1;shot(n++);}
        }
        CHECK(found,1);if(!found)n++;
    }
    face(2);background();draw_unit_covered(2,68,0,0,RT_NULL);shot(n++);    // 16
    background();draw_unit_covered(158,68,0,0,RT_NULL);shot(n++);          // 17
    for(cell=0;cell<MAP_N;cell++){unit_place(u,cell);if(unit_in_water(0))break;}
    CHECK(cell<MAP_N,1);u->team=TEAM_GUEST;face(0);draw();shot(n++);       // 18
    CHECK(rng32,seed);CHECK(u->hp,50);
    // A human with the bank present draws exactly as without it.
    for(cell=0;cell<MAP_N;cell++){unit_place(u,cell);if(!unit_in_water(0))break;}
    u->job=0x4a;u->gfx=UG_SQUIRE_M;W->actor_budget=255;draw();before=pixels();
    body_gfx=RT_NULL;W->actor_budget=255;draw();CHECK(pixels(),before);body_gfx=bank;
    // Reentry reacquires the borrowed archive pointer.
    chapter_now=0;game_scenario(261);CHECK(body_gfx==bank,1);
}
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("monster-body %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){body_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
