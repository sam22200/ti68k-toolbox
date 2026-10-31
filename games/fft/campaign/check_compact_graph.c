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
// Storage/cost adapter fixture. Long links are supplied explicitly; this does
// not bypass the pending runtime actor-record/all-intermediate policy gate.
typedef struct {
    CoreGrid grid;CoreTilePosition pos[252];Tile tile[252];
    u8 row[14],lookup[512],present[64],x[252],z[252];
} Board;
static u32 digest=17;static u16 checks,failures;
static void equal(u32 a,u32 b){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b)failures++;}
static void append(u8 from,u8 to,u8 kind)
{
    u8 n=W->en[from];equal(n+2<=((u16)1<<W->edge_shift),1);
    EDGE_ROW(from)[n]=to;EDGE_ROW(from)[n+1]=kind;W->en[from]=n+2;
}
static void trials(void)
{
    Board *b=malloc(sizeof(*b));u16 i;u8 origin=58,direction,distance,layer;
    equal(b!=0,1);if(!b)return;memset(b,0,sizeof(*b));
    b->grid.count=252;b->grid.width=9;b->grid.height=14;
    b->grid.row=b->row;b->grid.position=b->pos;b->grid.lookup=b->lookup;b->grid.present=b->present;
    for(i=0;i<14;i++)b->row[i]=(u8)(i*9);
    for(i=0;i<252;i++) {
        u16 at=(i%126)+((i>=126)?256:0);u8 cell=i%126;
        b->x[i]=cell%9;b->z[i]=cell/9;b->pos[i].x=b->x[i];b->pos[i].z_layer=b->z[i]|(i>=126?32:0);
        b->lookup[at]=(u8)i;b->present[at>>3]|=(u8)(1U<<(at&7));b->tile[i].walk=1;
    }
    equal(battle_bind_grid(&b->grid,b->tile,b->x,b->z),1);equal(W->edge_shift,6);
    equal(nav_capacity>=NAV_BYTES(252),1);equal(EDGE_ROW(251)-EDGE_ROW(0),251*64);
    for(i=0;i<NUNIT;i++){st.unit[i].dc=DC_GONE;st.unit[i].hp=0;}
    equal(unit_place(&st.unit[U_RAMZA],origin),1);st.unit[U_RAMZA].dc=3;st.unit[U_RAMZA].hp=100;
    st.unit[U_RAMZA].jump=W->ej=3;memset(W->en,0,MAP_N);memset(W->hw,1,MAP_N);
    for(direction=0;direction<4;direction++)for(distance=1;distance<=4;distance++)for(layer=0;layer<2;layer++) {
        u16 to=node_at(4+dir_x[direction]*distance,6+dir_z[direction]*distance,layer);
        u8 kind=distance>1?(u8)(1+((distance-2)<<3)+(direction<<6)):0;
        equal(to<MAP_N,1);append(origin,(u8)to,kind);
    }
    for(direction=0;direction<4;direction++)W->via[origin][direction]=(u8)node_at(4+dir_x[direction],6+dir_z[direction],0);
    equal(W->en[origin],64);
    spread_start(U_RAMZA,origin,6,st.reach,bfs_from,CORE_NAV_WIDE,0,0);while(!spread_go(1)){}
    equal(W->nav.fault,0);
    for(direction=0;direction<4;direction++)for(distance=1;distance<=4;distance++)for(layer=0;layer<2;layer++) {
        u8 to=(u8)node_at(4+dir_x[direction]*distance,6+dir_z[direction]*distance,layer);
        equal(st.reach[to],distance);equal(bfs_from[to],origin);
        if(distance>1)equal(edge_via(origin,EDGE_ROW(origin)[(direction*8+(distance-1)*2+layer)*2+1]),
                           node_at(4+dir_x[direction],6+dir_z[direction],0));
    }
    // Explicit cost3 plus three intervening cells costs6 on the real adapter.
    memset(W->en,0,MAP_N);append(origin,62,(u8)(49|(0<<6)));
    spread_start(U_RAMZA,origin,6,st.reach,bfs_from,CORE_NAV_WIDE,0,0);while(!spread_go(1)){}
    equal(st.reach[62],6);W->from_who=U_RAMZA+1;equal(make_path_cell(U_RAMZA,62),2);
    equal(st.path[0],origin);equal(st.path[1],62);
    spread_start(U_RAMZA,origin,5,st.reach,bfs_from,CORE_NAV_WIDE,0,0);while(!spread_go(65535)){}
    equal(st.reach[62],255);
    // Rebinding to one layer changes stride without shrinking/reallocating
    // retained capacity; pointer arithmetic must use the current stride.
    b->grid.count=126;memset(b->present+32,0,32);
    equal(battle_bind_grid(&b->grid,b->tile,b->x,b->z),1);equal(W->edge_shift,5);
    equal(EDGE_ROW(125)-EDGE_ROW(0),125*32);equal(nav_capacity>=NAV_BYTES(252),1);
    free(b); // no later grid queries in this fixture
}
#ifndef __m68k__
int main(void){sw_init(201);trials();printf("compact %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){game_init();game_scenario(201);trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
