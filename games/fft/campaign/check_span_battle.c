#define FFT_NAV_SPANS
// Frozen source graph versus scratch-cache graph; query restart owns its queues.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main step_cache_unused_main
#define game_render cache_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void cache_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "battle.c"
// Queue integration only: abstract weighted links below are not a movement
// geometry/actor-span certificate. The shipping graph still has one-gap links.
static u32 digest=17;static u16 checks,failures;
static void equal(u32 a,u32 b){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b)failures++;}
static void append(u8 a,u8 b,u8 kind)
{
    u8 n=W->en[a];EDGE_ROW(a)[n]=b;EDGE_ROW(a)[n+1]=kind;W->en[a]=n+2;
}
static void finish(u8 wide)
{
    u8 done;
    do {
        if(wide){ZB(8);done=spread_go(1);ZE(8);}
        else {ZB(7);done=spread_go(1);ZE(7);}
    }while(!done);
}
static void trials(void)
{
    u8 out;u16 i;u8 dist[256],from[256];
    equal(W!=0,1);if(!W)return;
    for(out=0;out<2;out++) {
        spread_start(U_RAMZA,UNIT_CELL(&st.unit[U_RAMZA]),12,st.reach,bfs_from,out,0,0);
        ZB(5);finish(0);ZE(5);
        memcpy(dist,st.reach,MAP_N);memcpy(from,bfs_from,MAP_N);
        spread_start(U_RAMZA,UNIT_CELL(&st.unit[U_RAMZA]),12,st.reach,bfs_from,out|CORE_NAV_WIDE,0,0);
        ZB(6);finish(1);ZE(6);
        equal(W->nav.fault,0);
        for(i=0;i<MAP_N;i++){equal(st.reach[i],dist[i]);if(dist[i]!=255)equal(bfs_from[i],from[i]);}
        // Switch back from linked queues to edge scratch and the ordinary FIFO.
        W->ej=255;spread_start(U_RAMZA,UNIT_CELL(&st.unit[U_RAMZA]),12,st.reach,bfs_from,out,0,0);
        while(!spread_go(65535)){}
        for(i=0;i<MAP_N;i++){equal(st.reach[i],dist[i]);if(dist[i]!=255)equal(bfs_from[i],from[i]);}
    }
    // Abstract weighted graph through the ACTUAL Work-bound search adapter.
    for(i=0;i<NUNIT;i++)st.unit[i].dc=DC_GONE;
    equal(unit_place(&st.unit[U_RAMZA],0),1);st.unit[U_RAMZA].dc=3;
    st.unit[U_RAMZA].jump=W->ej;memset(W->en,0,MAP_N);memset(W->hw,1,MAP_N);W->hw[0]=3;
    append(0,1,17);append(0,3,17);append(0,4,17);append(0,2,0);
    append(2,3,0);append(2,4,0);append(2,1,0);
    append(3,5,0);append(4,5,0);append(1,5,0);
    spread_start(U_RAMZA,0,9,st.reach,bfs_from,CORE_NAV_WIDE,0,0);
    while(!spread_go(1)){}
    equal(st.reach[1],2);equal(st.reach[5],3);equal(bfs_from[5],3);
    W->from_who=U_RAMZA+1;equal(make_path_cell(U_RAMZA,5),4);
    equal(st.path[0],0);equal(st.path[1],2);equal(st.path[2],3);equal(st.path[3],5);
    // A whole search with missing predecessors must preserve the same costs.
    spread_start(U_RAMZA,0,9,dist,0,CORE_NAV_WIDE,0,0);while(!spread_go(65535)){}
    for(i=0;i<MAP_N;i++)equal(dist[i],st.reach[i]);equal(W->nav.fault,0);
}
#ifndef __m68k__
int main(int argc,char **argv){sw_init(argc>1?(u16)atoi(argv[1]):201);trials();printf("spansearch %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){BENCH_NAME(5,"Fast search chunks");BENCH_NAME(6,"Wide search chunks");BENCH_NAME(7,"Fast single chunk");BENCH_NAME(8,"Wide single chunk");game_init();game_scenario(BENCH_ARG);trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
