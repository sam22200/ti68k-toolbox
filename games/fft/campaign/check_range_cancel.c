// Cancelling Move/Attack removes every old range pixel in the very next render.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main cancel_runtime_main
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
static u16 failures,checks;
static u32 digest=17;
#define CHECK(c) do{checks++;if(!(c))failures++;}while(0)
static u32 scene_hash(void)
{
    u32 hash=17;u16 i;
    for(i=0;i<2*SC_PLANE;i++)hash=(hash<<1|hash>>31)^scene_l[i];
    return hash;
}
static void trials(u16 scenario)
{
    u8 rot,aim,overflow;
    for(rot=0;rot<4;rot++)for(aim=0;aim<2;aim++)for(overflow=0;overflow<2;overflow++) {
        u8 frame;u32 plain;
        game_scenario(scenario);st.rot=rot;st.ai=st.banner=st.turn=0;st.want=255;
        st.act=st.sel=U_RAMZA;st.mode=M_MENU;st.menu=MN_MOVE;
        cursor_place(UNIT_CELL(&st.unit[st.act]));cam_target(&st.camx,&st.camy);
        for(frame=0;frame<16;frame++){W->actor_budget=255;game_render();}
        plain=scene_hash();
        rt_prev=0;rt_keys=K_ENTER;CHECK(game_update());CHECK(st.mode==M_TARGET);
        if(aim){st.mode=M_AIM;attack_range(st.act);}
        game_render();CHECK(W->ring_y0<W->ring_y1&&scene_hash()!=plain);
        if(overflow)W->bk_over=1; // Also exercise the original-colour reload path.
        // Exhaust optional work and keep a usable screen cache: neither may delay cleanup.
        W->left=-30000;W->drop_wait=0;W->hold=W->frz=0;
        rt_prev=rt_keys;rt_keys=K_B;CHECK(game_update());CHECK(!RINGS(st.mode));
        ensure_scene(rot); // exercise cleanup before game_render resets its normal budget
        CHECK(W->drop_y0==W->drop_y1&&W->ring_i==MAP_N);CHECK(scene_hash()==plain);
        game_render();CHECK(W->drop_y0==W->drop_y1&&W->ring_i==MAP_N);
        CHECK(scene_hash()==plain);
        digest=(digest<<1|digest>>31)^scene_hash();
    }
}
#ifdef FFT_ARCHIVE_REACH_TEST
#include "../view_format.h"
#ifndef __m68k__
static const char *review_directory;
#endif
static void range_trace(u8 rot,u8 style,u8 pass,u8 wide)
{
    u16 p,y,x;u32 visible=17,scene=scene_hash();
    copy_view();
    for(p=0;p<2;p++)for(y=0;y<RT_H;y++)for(x=0;x<RT_W/8;x++)
        visible=(visible<<1|visible>>31)^((const u8 *)(p?rt_dark:rt_light))[y*RT_PBYTES+x];
#ifndef __m68k__
    printf("range %u %u %u %u %08lX %08lX\n",rot,style,pass,wide,(unsigned long)scene,(unsigned long)visible);
    if(review_directory&&rot==3&&!style&&!pass&&!wide){
        char path[512];u8 frame;
        for(frame=0;frame<16;frame++){W->actor_budget=255;W->hold=W->frz=0;game_render();}
        snprintf(path,sizeof path,"%s/range.png",review_directory);sw_write_png(path,4);
    }
#else
    (void)rot;(void)style;(void)pass;(void)wide;BENCH_VALUE(scene);BENCH_VALUE(visible);
#endif
}
static void format_trials(void)
{
    u8 header[8];u16 i;
    for(i=0;i<8;i++)header[i]=views[0][i];
    CHECK(core_view_rows(header,65512,SC_HMAX)==SC_H);
    CHECK(!core_view_rows(RT_NULL,65512,SC_HMAX));
    for(i=0;i<9;i++)CHECK(!core_view_rows(header,i,SC_HMAX));
    CHECK(!core_view_rows(header,65512,(u16)(SC_H-1)));
    CHECK(!core_view_rows(header,(u16)(8+RD16(header,1)+RD16(header,2)),SC_HMAX));
    header[1]='3';CHECK(!core_view_rows(header,65512,SC_HMAX));header[1]='4';
    header[3]^=1;CHECK(!core_view_rows(header,65512,SC_HMAX));header[3]^=1;
    header[4]=0;header[5]=1;CHECK(!core_view_rows(header,65512,SC_HMAX));
    header[4]=header[5]=0;header[6]=0xff;header[7]=0xfc;
    CHECK(!core_view_rows(header,65512,SC_HMAX));
}
static void sort_trials(void)
{
    u16 saved=map_n,n,i,profile;u8 seen[MAP_NMAX];
    for(n=1;n<=256;n+=(n==1?254:1))for(profile=0;profile<5;profile++){
        map_n=n;memset(seen,0,sizeof seen);
        for(i=0;i<n;i++)key[i]=(s16)(profile==0?17:profile==1?255:profile==2?(i&1):profile==3?(255-i):((i*73+11)&255));
        tile_order();
        for(i=0;i<n;i++){
            u8 id=order[i];CHECK(id<n&&!seen[id]);seen[id]=1;
            if(i)CHECK(key[order[i-1]]<key[id]||(key[order[i-1]]==key[id]&&order[i-1]<id));
        }
    }
    map_n=saved;
}
// Exercise more rows than the old128-row pool, repeated dense/sparse ranges,
// slope/contour choices and the whole pristine scene/depth, not just an LCD crop.
static void dense_trials(u16 scenario,u8 wide)
{
    u8 rot,style,pass;u16 i;u32 plain,depth;
    for(rot=0;rot<4;rot++)for(style=0;style<2;style++){
        game_scenario(scenario);st.rot=rot;st.ai=st.banner=st.turn=0;st.want=255;
        st.act=st.sel=U_RAMZA;st.mode=M_MENU;st.menu=MN_MOVE;st.scenery=wide;
        st.reach_contour=style;W->frz=W->hold=0;
        ensure_scene(rot);plain=scene_hash();
        depth=0;for(i=0;i<RD16(views[rot],3);i++)depth=(depth<<1|depth>>31)^vbuf[i];
        for(pass=0;pass<4;pass++){
            st.mode=(pass&1)?M_AIM:M_TARGET;st.over=(pass>>1)&1;
            memset(st.reach,pass==3?255:0,sizeof st.reach);
            if(pass==3)for(i=0;i<MAP_N;i+=7)st.reach[i]=0;
            hl_seq++;ensure_scene(rot);CHECK(W->ring_i==MAP_N);
#ifdef FFT_ARCHIVE_COLOURS
            CHECK(!W->bk_over);
#endif
            digest=(digest<<1|digest>>31)^scene_hash();
            range_trace(rot,style,pass,wide);
            st.mode=M_MENU;hl_seq++;W->left=-30000;ensure_scene(rot);
            CHECK(W->drop_y0==W->drop_y1);CHECK(scene_hash()==plain);
            {u32 after=0;for(i=0;i<RD16(views[rot],3);i++)after=(after<<1|after>>31)^vbuf[i];CHECK(after==depth);}
        }
    }
}
#endif
#ifndef __m68k__
int main(int argc,char **argv)
{
    u16 scenario=argc>1?(u16)atoi(argv[1]):201;
#ifdef FFT_ARCHIVE_REACH_TEST
    review_directory=argc>2?argv[2]:RT_NULL;
#endif
    sw_init(scenario);trials(scenario);
#ifdef FFT_ARCHIVE_REACH_TEST
#ifndef FFT_LEGACY_REACH_BACKUPS
    format_trials();
#endif
    dense_trials(scenario,0);dense_trials(scenario,1);
    sort_trials();
#endif
    printf("cancel %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;
}
#else
void _main(void){cancel_runtime_main();trials(BENCH_ARG);
#ifdef FFT_ARCHIVE_REACH_TEST
    format_trials();
    dense_trials(BENCH_ARG,0);dense_trials(BENCH_ARG,1);
    sort_trials();
#endif
    BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
