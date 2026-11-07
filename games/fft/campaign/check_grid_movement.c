// Historical search as a frozen regression/cycle control for the shared kernel.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main grid_unused_main
#define game_render grid_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void grid_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "battle.c"
typedef struct {
    u8 queue[4][MAP_NMAX],size[4],distance,index,limit,out,dist[256],from[256];
} Legacy;
static Legacy *legacy;
static u32 checks,failures,digest=17;
static void equal(u16 a,u16 b)
{
    checks++;digest=(digest<<1|digest>>31)^a;if(a!=b)failures++;
}
static u8 __attribute__((noinline)) legacy_go(u16 budget)
{
    u8 d=legacy->distance,i=legacy->index,limit=legacy->limit,out=legacy->out,b;
    u8 *queues[4];for(b=0;b<4;b++)queues[b]=legacy->queue[b];
    for(;d<limit;d++,i=0) {
        u8 bucket=d&3;
        if(!(legacy->size[0]|legacy->size[1]|legacy->size[2]|legacy->size[3]))break;
        for(;i<legacy->size[bucket];i++) {
            u8 t=queues[bucket][i],k,m,wet;const u8 *e;
            if(legacy->dist[t]!=d)continue;
            if(!budget--){legacy->distance=d;legacy->index=i;return 0;}
            wet=W->hw[t]&2;e=EDGE_ROW(t);m=W->en[t];
            for(k=0;k<m;k+=2) {
                u8 next=e[k],kind=e[k+1],nd;
                if(W->blk[next]||((kind&1)&&W->blk[((u16)t+next)>>1]))continue;
                nd=(u8)(d+1+(kind&1)+(wet&&(out||kind)));
                if(nd>limit||nd>=legacy->dist[next]||((kind&2)&&!wet&&nd==limit))continue;
                legacy->dist[next]=nd;legacy->from[next]=t;
                queues[nd&3][legacy->size[nd&3]++]=next;
            }
        }
        legacy->size[bucket]=0;
    }
    return 1;
}
static void trials(void)
{
    u8 who,jump,limit_index;u16 i;
    const u8 limits[3]={4,12,124};
    equal(W!=0,1);if(!W)return;
    legacy=malloc(sizeof(*legacy));equal(legacy!=0,1);if(!legacy)return;
    for(who=0;who<NUNIT;who++)for(jump=1;jump<=4;jump++)for(limit_index=0;limit_index<3;limit_index++) {
        u8 origin=(u8)(st.unit[who].z*MAP_W+st.unit[who].x),limit=limits[limit_index],out=limit_index&1;
        u16 budget=jump&1?1:65535;
#ifdef __m68k__
        u32 before,old_cycles,new_cycles;
#endif
        st.unit[who].jump=jump;
        // Build shared edges/blockers first; benchmark only the two searches.
        spread_start(who,origin,limit,st.reach,bfs_from,out,0,0);
        memset(legacy,0,sizeof(*legacy));memset(legacy->dist,255,MAP_N);
        legacy->dist[origin]=0;legacy->queue[0][0]=origin;legacy->size[0]=1;
        legacy->limit=limit;legacy->out=out;
#ifdef __m68k__
        BENCH_NAME(4,"Legacy map search");before=BENCH_CYCLES;BENCH_BEGIN(4);
#endif
        while(!legacy_go(budget)){}
#ifdef __m68k__
        BENCH_END(4);old_cycles=BENCH_CYCLES-before;
        BENCH_NAME(5,"Compact map search");before=BENCH_CYCLES;BENCH_BEGIN(5);
#endif
        while(!spread_go(budget)){}
#ifdef __m68k__
        BENCH_END(5);
        new_cycles=BENCH_CYCLES-before;
        BENCH_VALUE(((u32)who<<16)|((u16)jump<<8)|limit);
        BENCH_VALUE(old_cycles);BENCH_VALUE(new_cycles);
#endif
        equal(W->nav.fault,0);
        for(i=0;i<MAP_N;i++) {
            equal(st.reach[i],legacy->dist[i]);
            if(i!=origin&&st.reach[i]!=255)equal(bfs_from[i],legacy->from[i]);
        }
    }
    free(legacy);
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    sw_init(argc>1?(u16)atoi(argv[1]):201);trials();
    printf("movement %08lX failures %lu checks %lu\n",(unsigned long)digest,(unsigned long)failures,(unsigned long)checks);
    return failures!=0;
}
#else
void _main(void)
{
    game_init();game_scenario(BENCH_ARG);trials();
    BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);
}
#endif
