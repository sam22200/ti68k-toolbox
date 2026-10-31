// Full-node tactical adapter boundaries. Synthetic geometry, no scene renderer.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main full_unused_main
#define game_render full_no_frame_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void full_no_frame_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include <stdlib.h>
static unsigned char fail_allocation;
static void *fixture_malloc(unsigned long bytes)
{
    if(fail_allocation){fail_allocation=0;return 0;}
    return malloc(bytes);
}
#define malloc fixture_malloc
#include "battle.c"
#undef malloc
typedef struct {
    CoreGrid grid;CoreTilePosition position[256];Tile tiles[256];
    u8 row[18],lookup[512],present[64],x[256],z[256],expected[256];
} Fixture;
static Fixture *f;static u16 checks,failures;static u32 digest=17;
static void check(u16 actual,u16 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^actual;
    if(actual!=expected){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,actual,expected);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(v,e) check((u16)(v),(u16)(e),__LINE__)
static void reset(u8 floors)
{
    u16 i;chapter_now=0;game_scenario(261);CHECK(W!=0,1);
    memset(f,0,sizeof(*f));f->grid.count=256;f->grid.width=floors?8:16;f->grid.height=16;
    f->grid.row=f->row;f->grid.position=f->position;f->grid.lookup=f->lookup;f->grid.present=f->present;
    for(i=0;i<16;i++)f->row[i]=(u8)(i*f->grid.width);
    for(i=0;i<256;i++) {
        u16 source=floors&&i<128?i:255-i,address;u8 layer=floors&&i>=128,k;
        u8 x=source%f->grid.width,z=source/f->grid.width;
        address=f->row[z]+x+((u16)layer<<8);f->x[i]=x;f->z[i]=z;
        f->position[i].x=x;f->position[i].z_layer=z|(layer<<5);
        f->lookup[address]=(u8)i;f->present[address>>3]|=(u8)(1U<<(address&7));
        f->tiles[i].walk=1;f->tiles[i].stand=layer?2:0;
        for(k=0;k<4;k++)f->tiles[i].c[k]=layer?1:0;
    }
    if(nav_capacity<(floors?NAV_BYTES(256):NAV_SINGLE_BYTES(256))) {
        u8 *old=navbuf;u16 count=MAP_N;
        fail_allocation=1;CHECK(battle_bind_grid(&f->grid,f->tiles,f->x,f->z),0);
        CHECK(navbuf==old,1);CHECK(MAP_N,count);CHECK(W!=0,1);
    }
    CHECK(battle_bind_grid(&f->grid,f->tiles,f->x,f->z),1);CHECK(MAP_N,256);CHECK(NUNIT,13);
    CHECK(nav_capacity>=(floors?NAV_BYTES(256):NAV_SINGLE_BYTES(256)),1);CHECK(navbuf!=0,1);
    for(i=0;i<NUNIT;i++){st.unit[i].hp=0;st.unit[i].dc=DC_GONE;st.unit[i].x=255;}
    st.unit[U_RAMZA].hp=st.unit[U_RAMZA].max_hp=100;st.unit[U_RAMZA].dc=3;
    st.unit[U_RAMZA].team=TEAM_PLAYER;st.unit[U_RAMZA].jump=3;st.unit[U_RAMZA].move=4;
    unit_place(&st.unit[U_RAMZA],254);cursor_place(255);
    st.act=st.sel=U_RAMZA;st.mode=M_MENU;st.ai=st.banner=st.turn=0;st.want=255;
    st.open_stage=st.open_fade=st.end=st.acted=st.moved=0;rt_prev=rt_keys=0;
}
static void origin_order(void)
{
    u16 pattern,i,j;
    for(pattern=0;pattern<4;pattern++) {
        memset(W->ai_m,3,256);memset(W->ai_dist,0,256);st.acted=1;
        W->ai_focus=st.act;W->ai_retreat=0;W->ai_er=W->ai_ar=0;
        for(i=0;i<256;i++)W->ai_key[i]=pattern==0?17:pattern==1?(s16)i:
            pattern==2?-(s16)i:(s16)(((i*41)^(i>>3))&255)-128;
        // Frozen insertion rule is the reference, including its reversed ties.
        for(i=0;i<256;i++) {
            for(j=i;j&&W->ai_key[f->expected[j-1]]>=W->ai_key[i];j--)
                f->expected[j]=f->expected[j-1];
            f->expected[j]=(u8)i;
        }
#ifdef FFT_MODULES
        mod_call(MOD_AI_SETUP,1,0);
#else
        ai_setup2(st.act,1);
#endif
        CHECK(W->ai_no,256);
        for(i=0;i<256;i++)CHECK(W->ai_ord[i],f->expected[i]);
    }
}
static void cells(u8 floors)
{
    u16 i,steps=0;u8 wanted=255;
    reset(floors);CHECK(node_at(0,0,floors),255);CHECK(CURSOR_CELL,255);
    CHECK(unit_place(&st.unit[U_RAMZA],255),1);CHECK(unit_at_cell(255),U_RAMZA+1);
    memset(W->ai_m,1,256);
    ZB(4);spread_start(U_RAMZA,255,124,W->ai_dist,bfs_from,0,W->ai_m,256);
    CHECK(W->nav.remaining,256);while(!spread_go(1)&&steps++<1024){}ZE(4);
    CHECK(steps<1024,1);CHECK(W->nav.remaining,0);CHECK(W->ai_dist[255],0);
    for(i=0;i<256;i++)CHECK(W->ai_dist[i]!=255,1);
    unit_place(&st.unit[U_RAMZA],254);compute_reach(U_RAMZA);
    CHECK(st.reach[255],1);CHECK(make_path_cell(U_RAMZA,255),2);
    CHECK(st.path[0],254);CHECK(st.path[1],255);
    st.mode=M_TARGET;CHECK(pickable(255,0),1);
    memset(st.reach,1,256);memset(W->ring_bits,255,sizeof W->ring_bits);
    W->ring_style=ring_style();W->ring_i=256;
    CHECK(rings_unchanged(),1);st.reach[255]=255;CHECK(rings_unchanged(),0);
    CHECK(W->ring_i>=MAP_N,1);CHECK(reach_edges(255,0)!=0,1);
    if(floors){CHECK(node_at(0,0,0)!=255,1);CHECK(node_at(7,15,1),128);}
    for(i=0;i<256;i++)key[i]=17;
    ZB(6);tile_order();ZE(6);CHECK(W->count[17],256);for(i=0;i<256;i++)CHECK(order[i],i);
    if(!floors) {
        // Two nodes project onto the same point: ID255 must return256 in the +1 API.
        f->tiles[238].stand=4;memset(st.reach,255,256);st.reach[255]=1;
        CHECK(twin(238,0),256);f->tiles[238].stand=0;
    }
    // All256 nodes are AI origins;256 must survive both modules' contexts.
    memset(W->ai_m,3,256);memset(W->ai_dist,0,256);memset(W->ai_gain,0,sizeof W->ai_gain);
    memset(W->ai_key,0,sizeof W->ai_key);W->ai_focus=st.act;W->ai_er=W->ai_ar=0;
    W->ai_retreat=0;st.acted=1;
#ifdef FFT_MODULES
    ZB(5);mod_call(MOD_AI_SETUP,1,0);ZE(5);
#else
    ZB(5);ai_setup2(st.act,1);ZE(5);
#endif
    CHECK(W->ai_no,256);CHECK(W->ai_ord[0],255);CHECK(W->ai_ord[255],0);
    // A valid node255 must not trigger the "no candidate" nearest fallback.
    memset(W->ai_dist,2,256);W->ai_dist[0]=0;W->ai_dist[255]=1;W->ai_key[255]=-100;
    st.acted=0;
#ifdef FFT_MODULES
    ZB(5);mod_call(MOD_AI_SETUP,1,0);ZE(5);
#else
    ZB(5);ai_setup2(st.act,0);ZE(5);
#endif
    CHECK(W->ai_c0,255);CHECK(W->ai_c1,255);CHECK(W->ai_no,256);

    W->ab=NO_ABIL;W->ab_k=W->ab_n=W->ab_done=W->ai_use=W->ai_n=0;
    W->ab_best.pri=AI_BAD;W->ab_best.f=0;W->ai_nab=1;W->ai_a=W->ai_self=0;
    steps=0;ZB(7);
    for(;;) {
        u8 done;
#ifdef FFT_MODULES
        done=(u8)mod_call(MOD_AI,0,0);
#else
        done=ai_candidates();
#endif
        CHECK(UNIT_CELL(&st.unit[st.act]),254);
        if(done||++steps>=100)break;
    }
    ZE(7);CHECK(steps<100,1);CHECK(W->ab_k,256);
    origin_order();
    st.unit[1].hp=st.unit[1].max_hp=100;st.unit[1].dc=3;st.unit[1].team=TEAM_ENEMY;
    unit_place(&st.unit[1],wanted);attack_range(st.act);CHECK(st.reach[255],1);
    CHECK(unit_at_cell(255),2);skill(st.act,1,0x196);
    CHECK(st.pending[st.act].tile,255);CHECK(st.pending[st.act].origin,254);
    st.pending[st.act].ticks=0;charge_resolve();CHECK(st.open_resolving,1);CHECK(st.tgt,1);
    for(i=0;i<NUNIT;i++) {
        unit_place(&st.unit[i],(u8)(255-i));st.unit[i].hp=100;st.unit[i].dc=3;
        st.unit[i].ct=0;st.unit[i].sp=1;
    }
    CHECK(unit_at_cell((u8)(256-NUNIT)),NUNIT);
    st.unit[NUNIT-1].ct=100;CHECK(ct_next(),NUNIT-1);

}
static void trials(void){cells(0);cells(1);release();CHECK(navbuf==0,1);CHECK(nav_capacity,0);}
#ifndef __m68k__
int main(void)
{
    f=fixture_malloc(sizeof(*f));if(!f)return 2;sw_init(261);trials();free(f);
    printf("full %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;
}
#else
void _main(void)
{
    f=fixture_malloc(sizeof(*f));if(!f){BENCH_VALUE(0xBAD);return;}
    BENCH_NAME(4,"Full256 graph search");BENCH_NAME(5,"Full256 AI origins");
    BENCH_NAME(6,"Full256 tile sort");BENCH_NAME(7,"Full256 AI iteration");
    game_init();game_scenario(261);trials();free(f);
    BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);
}
#endif
