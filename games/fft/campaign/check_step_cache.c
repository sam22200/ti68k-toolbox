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
static u8 (*reference_via)[16];
#include "step_reference_edges.inc"
static u32 digest=17;static u16 checks,failures;
static void equal(u32 a,u32 b){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b)failures++;}
static u32 graph_hash(u8 reference)
{
    u16 i;u8 k;u32 h=17;
    for(i=0;i<MAP_N;i++) {
        h=(h<<1|h>>31)^W->en[i];
        for(k=0;k<W->en[i];k+=2) {
            u8 kind=EDGE_ROW(i)[k+1];u16 via=!reference&&(kind&1)?edge_via((u8)i,kind):i;
            h=(h<<1|h>>31)^EDGE_ROW(i)[k];h=(h<<1|h>>31)^(kind&63);
            h=(h<<1|h>>31)^(reference?reference_via[i][k>>1]:via<MAP_N?via:0);
        }
    }
    return h;
}
static void maximum_grid(void)
{
    typedef struct {CoreGrid grid;Tile tiles[256];u8 x[256],z[256],row[16],bank[2056];} Large;
    Large *m=malloc(sizeof(*m));u16 i;u32 reference;
    equal(m!=0,1);if(!m)return;memset(m,0,sizeof(*m));
    memcpy(m->bank,"FGS1",4);m->bank[4]=m->bank[5]=16;m->bank[6]=1;
    for(i=0;i<16;i++)m->row[i]=(u8)(i<<4);
    for(i=0;i<256;i++) {
        u8 k,h=(u8)(2+!(i&3));m->x[i]=i&15;m->z[i]=i>>4;
        m->bank[8+(i<<3)]=h;m->bank[15+(i<<3)]=128;
        m->tiles[i].stand=2*h;m->tiles[i].walk=1;for(k=0;k<4;k++)m->tiles[i].c[k]=h;
    }
    m->grid.width=m->grid.height=16;m->grid.count=256;m->grid.row=m->row;
    equal(battle_bind_grid(&m->grid,m->tiles,m->x,m->z),1);equal(battle_bind_surface(m->bank,sizeof m->bank),1);
    step_reference_edges(3);reference=graph_hash(1);edges(3);equal(graph_hash(0),reference);
    for(i=0;i<NUNIT;i++){st.unit[i].x=255;st.unit[i].hp=0;st.unit[i].dc=DC_GONE;}
    equal(unit_place(&st.unit[U_RAMZA],255),1);st.unit[U_RAMZA].hp=100;st.unit[U_RAMZA].dc=3;
    st.unit[U_RAMZA].move=2;st.unit[U_RAMZA].jump=3;st.act=st.sel=U_RAMZA;
    compute_reach(U_RAMZA);equal(st.reach[255],0);equal(st.reach[254],1);
    equal(make_path_cell(U_RAMZA,254),2);equal(st.path[0],255);equal(st.path[1],254);equal(W->nav.fault,0);
    st.unit[U_RAMZA].jump=8;W->ej=255;compute_reach(U_RAMZA);equal(W->ej,7);
    reference=graph_hash(0);st.unit[U_RAMZA].jump=255;compute_reach(U_RAMZA);
    equal(W->ej,7);equal(graph_hash(0),reference);equal(W->nav.fault,0);
    free(m); // no further terrain queries after this fixture
}
static void trials(void)
{
    u8 jump;u16 i;u32 reference;u8 expected[256];
    equal(W!=0,1);if(!W)return;
    reference_via=malloc(256*16);equal(reference_via!=0,1);if(!reference_via)return;
    for(jump=1;jump<=4;jump++) {
        ZB(5);step_reference_edges(jump);ZE(5);reference=graph_hash(1);
        ZB(6);edges(jump);ZE(6);equal(graph_hash(0),reference);
        st.unit[U_RAMZA].jump=jump;
        // Starting a new search must reset scratch flags/queue sizes, even
        // after a previous search has yielded in the middle of its frontier.
        spread_start(U_RAMZA,UNIT_CELL(&st.unit[U_RAMZA]),5,st.reach,bfs_from,0,0,0);
        equal(spread_go(1),CORE_NAV_MORE);
        W->ej=255;spread_start(U_RAMZA,UNIT_CELL(&st.unit[U_RAMZA]),5,st.reach,bfs_from,0,0,0);
        while(!spread_go(1)){}
        memcpy(expected,st.reach,MAP_N);
        spread_start(U_RAMZA,UNIT_CELL(&st.unit[U_RAMZA]),5,st.reach,bfs_from,0,0,0);
        while(!spread_go(65535)){}
        for(i=0;i<MAP_N;i++)equal(st.reach[i],expected[i]);
        equal(W->nav.fault,0);
    }
    maximum_grid();free(reference_via);
}
#ifndef __m68k__
int main(int argc,char **argv){sw_init(argc>1?(u16)atoi(argv[1]):201);trials();printf("stepcache %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){BENCH_NAME(5,"Reference step graph");BENCH_NAME(6,"Cached step graph");game_init();game_scenario(BENCH_ARG);trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
