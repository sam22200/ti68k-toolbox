// Bounded playtest reproducers. Screens are candidates for review, never golden updates.
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
#ifndef QA_SOURCE
#define QA_SOURCE "battle.c"
#endif
#include QA_SOURCE
#ifndef __m68k__
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
static const char *output;
#endif
static u16 failures, checks;
static u32 digest = 17;
static void check(u8 ok, u16 line)
{
    checks++; if (!ok) {
        failures++;
#ifndef __m68k__
        printf("failure line %u\n", line);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(ok) check((ok), __LINE__)
static void shot(const char *name)
{
    u16 r, b;
    u8 p;
    for(p=0;p<2;p++)for(r=0;r<RT_H;r++)for(b=0;b<RT_W/8;b++)
        digest=(digest<<1|digest>>31)^((const u8 *)(p?rt_dark:rt_light))[r*RT_PBYTES+b];
#ifndef __m68k__
    if(output) { char path[512];snprintf(path,sizeof path,"%s/%s.png",output,name);sw_write_png(path,1); }
#else
    (void)name;BENCH_SHOT(rt_dark);
#endif
}
static void render(void)
{
    u8 j;
    for(j=0;j<24;j++) { W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;game_render(); }
}
static void battle(u8 rot)
{
    game_scenario(101);st.ai=0;st.banner=0;st.rot=rot;st.mode=M_BROWSE;
    st.tick=0;st.turn=0;st.want=255;rt_prev=rt_keys=0;
    cam_target(&st.camx,&st.camy);
}
static void trials(void)
{
    u8 rot, i, face;
    // Source item names, without changing item stats or IDs.
    battle(0);
    CHECK(sizeof(Item)==30);
    CHECK(!memcmp(items[0x91].name,"Bronze Helmet",14));
    st.sel=4;st.mode=M_STATUS;st.page=1;st.status_info=0;W->status_i=0;
    render();shot("lavian-equipment");
    st.status_info=1;st.status_col=0;st.status_row=EQ_HEAD;W->status_i=0;
    render();shot("lavian-equipment-detail");
    // Every Wait direction is checked against independently projected world displacement.
    for(rot=0;rot<4;rot++)for(face=0;face<4;face++) {
        static const s8 dx[4]={1,-1,0,0},dz[4]={0,0,1,-1};
        Feet f;
        s16 u,v,x0,x1,y0,y1;
        u8 screen_arrow;
        char label[]="wait-0-0";
        battle(rot);st.act=U_RAMZA;st.sel=st.act;st.mode=M_FACE;st.unit[st.act].face=face;
        f=unit_feet(rot,st.act);st.camx=f.x-80;st.camy=f.y-55;cam_clamp(&st.camx,&st.camy);
        view_of(rot,1,1,&u,&v);x0=scene_x(rot,u,v);y0=scene_y(u,v);
        view_of(rot,1+dx[face],1+dz[face],&u,&v);x1=scene_x(rot,u,v);y1=scene_y(u,v);
        screen_arrow=x1>x0?(y1<y0?0:1):(y1>y0?2:3);
        CHECK(wait_arrow()==screen_arrow);
        CHECK(unit_direction(rot,st.act)==(screen_arrow==0?3:screen_arrow==1?2:screen_arrow==2?0:1));
        render();
        label[5]+=(char)rot;label[7]+=(char)face;shot(label);
        digest=(digest<<1|digest>>31)^screen_arrow;
    }
    // Hidden selected tile: mandatory first-frame masking, for player and AI,
    // even with no optional budget. A cached result must equal a cold result.
    for(rot=0;rot<4;rot++) {
        u16 hidden=0;
        battle(rot);render();
        for(i=0;i<MAP_N;i++) {
            Marker mk;const PlaneWord *mask;u8 row,ai;
            marker(rot,i,0,&mk);
            for(ai=0;ai<2;ai++) {
                st.ai=ai?AI_AIM:0;W->left=-300;W->bg_drawn=0;W->cur_k=0;W->cur_wait=0;
                mask=cursor_mask(&mk,(u16)(i|0x1000));
                CHECK(mask!=mk.m);
                for(row=0;row<mk.rows;row++)hidden+=(mask[row]!=mk.m[row]);
                CHECK(mask==cursor_mask(&mk,(u16)(i|0x1000)));
                // Independent pixel oracle: depth arrays are sampled directly;
                // no cache keys, budget decisions or word-shift code is reused.
                for(row=0;row<mk.rows;row++) {
                    PlaneWord expected=mk.m[row];u8 col;
                    for(col=0;col<24;col++) {
                        s16 x=mk.x+col,y=mk.y+row;
                        if((u16)x<SC_W&&(u16)y<SC_H) {
                            u8 bit=(u8)(0x80>>(x&7));
                            u8 covered=((depth_byte(0,y,(u16)(x>>3))>mk.depth&&(depth_byte(1,y,(u16)(x>>3))&bit))
                                ||(depth_byte(2,y,(u16)(x>>3))>mk.depth&&(depth_byte(3,y,(u16)(x>>3))&bit)));
                            if(covered&&((y&1?0xCC:0x33)&bit))expected&=~((PlaneWord)0x80000000UL>>col);
                        }
                    }
                    CHECK(mask[row]==expected);
                }
            }
        }
        CHECK(hidden!=0);
    }
#ifndef QA_NO_VISUALS
    {
        u8 best=0,best_hidden=0;
        battle(0);render();
        for(i=0;i<MAP_N;i++)if(map_tiles[i].walk) {
            Marker mk;const PlaneWord *mask;u8 row,n=0;
            marker(0,i,0,&mk);W->cur_k=0;W->left=30000;mask=cursor_mask(&mk,(u16)(i|0x1000));
            for(row=0;row<mk.rows;row++)n+=(mask[row]!=mk.m[row]);
            if(n>best_hidden){best_hidden=n;best=i;}
        }
        CHECK(best_hidden!=0);
        st.mode=M_TARGET;cursor_place((u8)best);
        for(i=0;i<MAP_N;i++)st.reach[i]=map_tiles[i].walk?0:255;
        st.camx=0;st.camy=100;cam_target(&st.camx,&st.camy);hl_seq++;render();
        shot("cursor-masked");
        // Same image and tile, deliberately replay the prior full-marker fallback.
        {Marker mk;marker(0,best,0,&mk);dst_screen(st.camx,st.camy);
            PlaneWord inverse[24];u8 row;
            for(row=0;row<mk.rows;row++)inverse[row]=mk.m[row]&~mk.l[row];
            blit24(mk.x,mk.y,mk.rows,inverse,inverse,mk.m);}
        shot("cursor-prior-full-fallback");
    }
#endif
    // Position/cover after an actual applied knockback, then after view changes.
    // Independently sampled cover must match the renderer's cache on every frame.
    for(rot=0;rot<4;rot++) {
        u8 a,t,pushed=0;
        battle(rot);
        for(a=0;a<NUNIT&&!pushed;a++)for(t=0;t<NUNIT&&!pushed;t++)if(a!=t) {
            st.dmg=1;st.push=0;knockback(a,t);
            if(st.push) {
                u8 j;
                st.atk=a;st.tgt=t;st.mode=M_ACT;st.abil=NO_ABIL;st.hit=1;st.act_t=ACT_HIT;
                core_action_begin(&st.action,a,NUNIT);
                attack_step();st.act_t=ACT_HIT+HIT_FRAMES;pushed=1;
                for(j=0;j<4;j++) {
                    Feet f;u16 cm[COVER_H];Cover *c=&W->cover[t];
                    st.rot=(u8)((rot+j)&3);f=unit_feet(st.rot,t);
                    st.camx=f.x-80;st.camy=f.y-55;cam_clamp(&st.camx,&st.camy);
                    render();cover(cm,f.x,f.y,f.depth);
                    CHECK(c->x==f.x&&c->y==f.y&&c->depth==f.depth&&c->rot==st.rot);
                    CHECK(!memcmp(cm,c->cm,sizeof cm));
                    if(!j) {char label[]="knockback-0";label[10]+=(char)rot;shot(label);}
                }
            }
        }
        CHECK(pushed);
    }
    // Top of each map view: no negative scene read, padded screen cache remains exact.
    for(rot=0;rot<4;rot++) {
        s16 lowest=32767;u8 top=0;
        char label[]="headroom-0";
        battle(rot);
        for(i=0;i<NUNIT;i++) {Feet f=unit_feet(rot,i);if(f.y<lowest){lowest=f.y;top=i;}}
        st.act=st.sel=top;cursor_place(UNIT_CELL(&st.unit[top]));st.mode=M_FACE;
        st.camx=0;st.camy=100;cam_target(&st.camx,&st.camy);
        render();CHECK(unit_feet(rot,top).y-st.camy>=CAM_MT);
        label[9]+=(char)rot;shot(label);
    }
#ifndef QA_NO_VISUALS
    // A clean renderer-derived scenery/actor layer for the victory layout review.
    // Preview only: reward amounts and source staging are not changed here.
    battle(0);W->left=30000;ensure_scene(st.rot);copy_view();W->actor_budget=255;
    for(i=0;i<NUNIT;i++)if(st.unit[i].team!=TEAM_ENEMY&&st.unit[i].hp) {
        Feet f=unit_feet(st.rot,i);Cover c;
        cover(c.cm,f.x,f.y,f.depth);c.wet=unit_in_water(i);c.r0=2;c.r1=UNIT_SH-3;
        draw_unit_covered(f.x-st.camx,f.y-st.camy,st.rot,i,&c);
    }
    shot("victory-background");
#endif
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    if(argc>1)output=argv[1];sw_init(101);trials();
    printf("qa %08lX failure %u checks %u\n",(unsigned long)digest,failures,checks);
    return failures!=0;
}
#else
void _main(void)
{
    qa_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);
}
#endif
