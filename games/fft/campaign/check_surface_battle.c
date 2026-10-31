// Real shared battle graph/ray queries beneath an asymmetric three-column bridge.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main bridge_unused_main
#define game_render bridge_no_frame_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void bridge_no_frame_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "battle.c"
static u16 checks,failures;static u32 digest=17;
static void check(u16 v,u16 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^v;
    if(v!=expected){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,v,expected);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(v,e) check((u16)(v),(u16)(e),__LINE__)
static u32 graph_hash(void)
{
    u16 i;u8 k;u32 hash=17;
    for(i=0;i<MAP_N;i++)for(k=0;k<W->en[i];k+=2) {
        hash=(hash<<1|hash>>31)^EDGE_ROW(i)[k];
        hash=(hash<<1|hash>>31)^(EDGE_ROW(i)[k+1]&63);
        {u8 kind=EDGE_ROW(i)[k+1];u16 via=kind&1?edge_via((u8)i,kind):i;
         hash=(hash<<1|hash>>31)^(via<MAP_N?via:0);}
    }
    return hash;
}
static void cold_edges(void)
{
    const u8 *surface;u8 needed;u32 control;
    CHECK(W!=0,1);if(!W)return;
    surface=W->surface;needed=W->surface_flags;CHECK(surface!=0,1);
    // Explicit historical rule control, never a retail loading fallback.
    W->surface=0;W->surface_flags=0;ZB(5);edges(3);ZE(5);control=graph_hash();
    W->surface=surface;W->surface_flags=needed|SURFACE_ROOM|SURFACE_MOVE_UPPER|SURFACE_HIT_UPPER;ZB(6);edges(3);ZE(6);
    // Source intermediate jump spans can remove historical one-floor shortcuts.
    // The ceiling classifier must preserve the complete source-filtered graph.
    CHECK(graph_hash()!=control,1);control=graph_hash();
    W->surface_flags=needed;ZB(7);edges(3);ZE(7);CHECK(graph_hash()==control,1);
    W->surface_flags=needed|SURFACE_MOVE_UPPER|SURFACE_HIT_UPPER;
    ZB(8);edges(3);ZE(8);CHECK(graph_hash()==control,1);W->surface_flags=needed;
}
static void trials(void)
{
    static const u8 xs[6]={1,2,0,1,2,0},zs[6]={0,0,0,0,0,0},row[1]={0};
    static const CoreTilePosition positions[6]={{1,0},{2,0},{0,0},{1,32},{2,32},{0,32}};
    u8 lookup[512],present[64],bank[38],bad[38],i,variant;
    Tile tiles[6];CoreGrid grid;const u8 *old;
    cold_edges();
    memset(lookup,0,sizeof lookup);memset(present,0,sizeof present);memset(tiles,0,sizeof tiles);
    for(i=0;i<6;i++) {
        u16 address=xs[i]+(i>=3?256:0);lookup[address]=i;present[address>>3]|=(u8)(1U<<(address&7));
        tiles[i].walk=i<3?1:0;
    }
    memset(&grid,0,sizeof grid);grid.count=6;grid.width=3;grid.height=1;
    grid.row=row;grid.position=positions;grid.lookup=lookup;grid.present=present;
    CHECK(battle_bind_grid(&grid,tiles,xs,zs),1);CHECK(W->surface==0,1);
    for(i=0;i<NUNIT;i++){st.unit[i].hp=0;st.unit[i].dc=DC_GONE;st.unit[i].x=255;}
    unit_place(&st.unit[U_RAMZA],2);unit_place(&st.unit[1],1);
    st.unit[U_RAMZA].hp=st.unit[1].hp=100;st.unit[U_RAMZA].dc=st.unit[1].dc=3;
    st.unit[U_RAMZA].jump=1;st.unit[U_RAMZA].move=4;
    st.act=st.sel=U_RAMZA;st.mode=M_MENU;st.open_stage=0;
    memset(bank,0,sizeof bank);memcpy(bank,"FGS1",4);bank[4]=3;bank[5]=1;bank[7]=3;
    // All upper cells are initially blocked; only the middle column carries a bridge.
    for(i=0;i<3;i++)bank[8+8*i+7]=128;
    for(variant=0;variant<5;variant++) {
        u8 height=variant==0?8:variant==1?4:variant==2?3:2;
        u8 depth=variant==1?1:0,blocked=variant==4?128:0;
        tiles[3].stand=2*height;for(i=0;i<4;i++)tiles[3].c[i]=height;
        tiles[3].walk=blocked?0:1;
        bank[20]=height;bank[21]=0;bank[22]=0;bank[23]=depth|blocked;
        CHECK(battle_bind_grid(&grid,tiles,xs,zs),1);CHECK(W->surface==0,1);
        CHECK(battle_bind_surface(bank,32),1);
        old=W->surface;memcpy(bad,bank,sizeof bad);bad[4]=4;
        CHECK(battle_bind_surface(bad,32),0);CHECK(W->surface==old,1);
        CHECK(battle_line_clear(U_RAMZA,1),height>2);
        CHECK(battle_line_clear(1,U_RAMZA),height>2);
        compute_reach(U_RAMZA);CHECK(st.reach[0]!=255,variant!=3);
        CHECK(W->surface==old,1);
    }
    CHECK(battle_line_clear(NUNIT,0),0);CHECK(battle_line_clear(0,NUNIT),0);
    CHECK(battle_bind_grid(&grid,tiles,xs,zs),1);CHECK(W->surface==0,1);
}
#ifndef __m68k__
int main(void)
{
    sw_init(261);trials();printf("bridge %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;
}
#else
void _main(void){BENCH_NAME(5,"Legacy terrain edges");BENCH_NAME(6,"Source terrain edges");BENCH_NAME(7,"Classified terrain edges");BENCH_NAME(8,"Two-layer terrain edges");game_init();game_scenario(261);trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
