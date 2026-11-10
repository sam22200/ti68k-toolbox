// Width choice, paused real inputs, colour/depth lifetime and overflow restoration.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main edge_runtime_main
#define game_update edge_runtime_update
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_update
#undef _main
#define _main __main
#undef off
#endif
#include "gariland.c"
#include "x/edge-width/expected.h"
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
static const char *output;
#else
u8 edge_runtime_update(void){return 0;}
#endif
static u16 checks,failures;
static u32 digest=17;
static u32 hash_bytes(const u8 *p,u16 n)
{
    u32 h=17;
    while(n--)h=(h<<1|h>>31)^*p++;
    return h;
}
static void check(u8 ok,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^ok;
    if(!ok){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("edges failure line %u\n",line);
#endif
    }
}
#define CHECK(v) check(!!(v),__LINE__)
static void edge_key(u32 keys)
{
#ifdef __m68k__
    rt_prev=rt_keys;rt_keys=keys;config_update();
#else
    if(!sw_step(keys)){failures++;puts("edge input unexpectedly exited");}
#endif
}
static void press(u32 keys){edge_key(0);edge_key(keys);edge_key(0);}
static void settle(void)
{
    u8 i;
    for(i=0;i<24;i++){W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;game_render();}
}
static void shot(u8 rot,u8 wide)
{
    u8 row;
    settle();
    for(row=0;row<RT_H;row++) {
        digest=(digest<<1|digest>>31)^hash_bytes((u8 *)rt_light+row*RT_PBYTES,RT_W/8);
        digest=(digest<<1|digest>>31)^hash_bytes((u8 *)rt_dark+row*RT_PBYTES,RT_W/8);
    }
#ifdef __m68k__
    BENCH_SHOT(rt_dark);
#else
    printf("screen %u %u %04X\n",rot,wide,sw_checksum());
    if(output){char p[512];snprintf(p,sizeof p,"%s/edge-%u-%u.png",output,rot,wide);sw_write_png(p,1);}
#endif
}
static void trials(void)
{
    u8 rot,wide,i;
    CHECK(W&&views[0]&&views[0][1]=='3');if(!W)return;
    // The same real handler/input edge as game_update. Holding a key cannot toggle repeatedly.
    game_scenario(61);st.ai=0;st.banner=0;st.turn=0;st.want=255;st.mode=M_MENU;
    settle();edge_key(K_MODE);CHECK(st.config&&st.config_row==CONFIG_MOVE);
    {
        Unit saved[NUNIT];u16 tick=st.tick,seed=rt_seed;u32 random=rng32;
        memcpy(saved,st.unit,sizeof saved);
        for(i=0;i<6;i++)edge_key(K_MODE);
        CHECK(st.config);edge_key(0);
        press(K_DOWN);CHECK(st.config_row==CONFIG_EDGE);
        edge_key(K_ENTER);CHECK(st.scenery==3);
        for(i=0;i<6;i++)edge_key(K_ENTER);
        CHECK(st.scenery==3&&st.config);
        CHECK(!memcmp(saved,st.unit,sizeof saved)&&tick==st.tick&&seed==rt_seed&&random==rng32);
        press(K_DOWN);CHECK(st.config_row==CONFIG_OVER);
        press(K_ENTER);CHECK(st.over==1);
        edge_key(0);press(K_MODE);CHECK(!st.config&&(st.scenery&1));
    }
    for(rot=0;rot<4;rot++)for(wide=0;wide<2;wide++) {
        u32 depth;static u8 full[4*SC_BYTES*SC_HMAX];
        game_scenario(61);st.ai=0;st.banner=0;st.rot=rot;st.mode=M_MENU;st.tick=0;
        st.scenery=wide;rt_frame=0;scene_rot=vbuf_rot=255;cam_target(&st.camx,&st.camy);
        unpack_view(rot);
        ZB(wide?19:18);unpack_colours(rot);ZE(wide?19:18);
        CHECK(hash_bytes(scene_l,2*SC_PLANE)==edge_colours[rot][wide]);
        depth_planes(full);depth=hash_bytes(full,4*SC_PLANE);
        CHECK(depth==edge_depth[rot]);
        // A warmed scene and a complete row-backup overflow must retain the same width.
        W->left=30000;ensure_scene(rot);
        W->drop_y0=0;W->drop_y1=1;W->bk_over=1;W->left=30000;
        ensure_scene(rot);
        CHECK(!W->bk_over&&!W->bk_n);
        CHECK(hash_bytes(scene_l,2*SC_PLANE)==edge_colours[rot][wide]);
        depth_planes(full);CHECK(hash_bytes(full,4*SC_PLANE)==depth);
        shot(rot,wide);
        // Toggle with Move rings already cached. Closing/reopening and reloading
        // the other width restores exact depth and clean unhighlighted colours.
        st.mode=M_TARGET;compute_reach(U_RAMZA);settle();
        press(K_MODE);press(K_DOWN);CHECK(st.config&&st.config_row==CONFIG_EDGE);
        press(K_A);CHECK((st.scenery&1)==!wide);press(K_MODE);
        st.mode=M_MENU;hl_seq++;settle();
        W->left=30000;ensure_scene(rot);
        CHECK(hash_bytes(scene_l,2*SC_PLANE)==edge_colours[rot][!wide]);
        depth_planes(full);CHECK(hash_bytes(full,4*SC_PLANE)==depth);
    }
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    output=argc>1?argv[1]:RT_NULL;sw_init(61);trials();
    printf("edges %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);
    if(output) {
        u8 wide;char path[512];game_scenario(61);st.ai=st.banner=0;st.mode=M_MENU;
        settle();press(K_MODE);press(K_DOWN);
        for(wide=0;wide<2;wide++) {
            if((st.scenery&1)!=wide)press(K_ENTER);
            game_render();snprintf(path,sizeof path,"%s/settings-%upx.png",output,wide+1);sw_write_png(path,1);
        }
    }
    return failures!=0;
}
#else
void _main(void)
{
    void *planes;edge_runtime_main();planes=malloc(2*RT_PSIZE);
    memset(planes,0,2*RT_PSIZE);rt_dark=planes;rt_light=(u8 *)planes+RT_PSIZE;
    trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);free(planes);
}
#endif
