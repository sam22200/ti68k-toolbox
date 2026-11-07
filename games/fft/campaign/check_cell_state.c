// Real battle-state transitions on an asymmetric two-floor tactical fixture.
// Scenery is intentionally not drawn: raster/depth integration is a later gate.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main cells_unused_main
#define game_render cells_no_frame_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void cells_no_frame_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "battle.c"
enum { LOW_A=2,LOW_B=4,LOW_C=7,UP_A=1,UP_B=5,UP_C=8 };
typedef struct {
    CoreGrid grid;CoreTilePosition position[9];Tile tiles[9];
    u8 row[2],lookup[512],present[64],x[9],z[9];
} Fixture;
static Fixture *f;
static u16 checks,failures;
static u32 digest=17;
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
static void reset(u8 upper_c)
{
    static const u8 positions[9][3]={{0,1,0},{0,0,1},{0,0,0},{1,1,0},{1,0,0},
                                    {1,0,1},{2,1,0},{2,0,0},{2,0,1}};
    u16 i;
    chapter_now=0;game_scenario(261);CHECK(W!=0,1);
    memset(f,0,sizeof(*f));f->row[1]=3;
    f->grid.count=upper_c?9:8;f->grid.width=3;f->grid.height=2;f->grid.position=f->position;
    f->grid.row=f->row;f->grid.lookup=f->lookup;f->grid.present=f->present;
    for(i=0;i<f->grid.count;i++) {
        u8 x=positions[i][0],z=positions[i][1],layer=positions[i][2],k;
        u16 address=f->row[z]+x+((u16)layer<<8);
        f->x[i]=x;f->z[i]=z;f->position[i].x=x;f->position[i].z_layer=z|(layer<<5);
        f->lookup[address]=(u8)i;f->present[address>>3]|=(u8)(1U<<(address&7));
        f->tiles[i].walk=1;f->tiles[i].stand=layer?6:0;
        for(k=0;k<4;k++)f->tiles[i].c[k]=layer?3:0;
    }
    f->tiles[LOW_A].water=1;
    CHECK(battle_bind_grid(&f->grid,f->tiles,f->x,f->z),1);
    for(i=0;i<NUNIT;i++) {st.unit[i].hp=0;st.unit[i].dc=DC_GONE;st.unit[i].x=255;}
    unit_place(&st.unit[U_RAMZA],UP_A);unit_place(&st.unit[1],LOW_B);
    unit_place(&st.unit[2],UP_B);unit_place(&st.unit[8],0);
    for(i=0;i<NUNIT;i++)if(i==U_RAMZA||i==1||i==2||i==8) {
        st.unit[i].hp=st.unit[i].max_hp=100;st.unit[i].dc=3;st.unit[i].ct=0;st.unit[i].sp=1;
        st.unit[i].jump=3;st.unit[i].move=4;
    }
    st.act=st.sel=U_RAMZA;st.ai=st.banner=st.turn=0;st.want=255;
    st.open_stage=st.open_fade=st.end=0;st.mode=M_MENU;cursor_place(UP_A);
    rt_prev=rt_keys=0;
}
static void identity(void)
{
    u8 old;u16 cached_key;CoreGrid bad;Feet above,below;
    reset(1);CHECK(node_at(1,0,0),LOW_B);CHECK(node_at(1,0,1),UP_B);
    CHECK(unit_at_cell(LOW_B),2);CHECK(unit_at_cell(UP_B),3);CHECK(unit_at(1,0),2);
    CHECK(unit_at_cell(UP_A),U_RAMZA+1);CHECK(node_at(2,1,1),256);
    old=st.unit[2].cell;CHECK(unit_place(&st.unit[2],255),0);CHECK(st.unit[2].cell,old);
    cursor_place(UP_B);CHECK(CURSOR_CELL,UP_B);move_cursor(0,-1);CHECK(CURSOR_CELL,UP_A);
    cursor_place(LOW_B);CHECK(CURSOR_CELL,LOW_B);
    bad=f->grid;bad.count=256;
    CHECK(battle_bind_grid(&bad,f->tiles,f->x,f->z),0);CHECK(MAP_N,9);
    st.unit[2].hp=0;st.unit[2].dc=3;CHECK(unit_at_cell(UP_B),3);
    st.unit[2].dc=DC_CHEST;CHECK(unit_at_cell(UP_B),0);CHECK(unit_at_cell(LOW_B),2);
    above=unit_feet(0,U_RAMZA);cached_key=unit_feet_key(U_RAMZA,0);
    unit_place(&st.unit[U_RAMZA],LOW_A);below=unit_feet(0,U_RAMZA);
    CHECK(above.x,below.x);CHECK(below.y-above.y,3*HU);
    CHECK(unit_feet_key(U_RAMZA,0)!=cached_key,1);
}
static void movement(void)
{
    u16 frames=0;u8 i;
    reset(1);unit_place(&st.unit[1],LOW_C);unit_place(&st.unit[2],LOW_A);
    st.unit[2].hp=0;st.unit[2].dc=DC_GONE;
    st.unit[2].x=255;
    compute_reach(U_RAMZA);CHECK(st.reach[LOW_A]!=255,1);CHECK(st.reach[UP_B]!=255,1);
    CHECK(st.reach[LOW_C],255);
    st.mode=M_TARGET;cursor_place(LOW_A);
    rt_keys=K_ENTER;CHECK(game_update(),1);CHECK(st.mode,M_WALK);CHECK(st.move_cell,UP_A);
    CHECK(st.path[0],UP_A);CHECK(st.path[st.path_n-1],LOW_A);
    rt_prev=rt_keys;rt_keys=0;
    while(st.mode==M_WALK&&frames++<100)CHECK(game_update(),1);
    CHECK(frames<100,1);CHECK(st.unit[U_RAMZA].cell,LOW_A);
    CHECK(st.mode,M_MENU);CHECK(st.pick_t,0);
    CHECK(st.unit[U_RAMZA].x,0);CHECK(st.unit[U_RAMZA].z,0);CHECK(unit_in_water(U_RAMZA),1);
    rt_prev=0;rt_keys=K_B;CHECK(game_update(),1);
    CHECK(st.unit[U_RAMZA].cell,UP_A);CHECK(CURSOR_CELL,UP_A);CHECK(st.moved,0);
    CHECK(unit_in_water(U_RAMZA),0);
    for(i=0;i<st.path_n;i++)CHECK(st.path[i]<MAP_N,1);
}
static void gap(void)
{
    u8 i,found=0;
    reset(1);
    f->present[257>>3]&=(u8)~(1U<<(257&7));
    f->present[259>>3]|=(u8)(1U<<(259&7));f->lookup[259]=UP_B;
    f->position[UP_B].x=0;f->position[UP_B].z_layer=33;f->x[UP_B]=0;f->z[UP_B]=1;
    CHECK(battle_bind_grid(&f->grid,f->tiles,f->x,f->z),1);
    unit_place(&st.unit[2],UP_B);compute_reach(U_RAMZA);
    CHECK(node_at(1,0,1),256);CHECK(unit_at_cell(LOW_B),2);CHECK(st.reach[UP_C],2);
    for(i=0;i<W->en[UP_A];i+=2)if(EDGE_ROW(UP_A)[i]==UP_C)found=EDGE_ROW(UP_A)[i+1]&63;
    CHECK((found&5)==5,1);CHECK(make_path_cell(U_RAMZA,UP_C),2);
    CHECK(st.path[0],UP_A);CHECK(st.path[1],UP_C);
}
static void charge(void)
{
    u16 lower_hp;
    reset(1);lower_hp=st.unit[1].hp;
    skill(U_RAMZA,2,0x196);CHECK(st.pending[U_RAMZA].tile,UP_B);
    CHECK(st.pending[U_RAMZA].origin,UP_A);
    unit_place(&st.unit[2],LOW_A);st.pending[U_RAMZA].ticks=0;
    charge_resolve();CHECK(st.open_resolving,0);CHECK(st.unit[1].hp,lower_hp);
    reset(1);skill(U_RAMZA,2,0x196);unit_place(&st.unit[U_RAMZA],LOW_A);
    CHECK(st.unit[U_RAMZA].x,0);CHECK(st.unit[U_RAMZA].z,0);
    CHECK(pending_next(),255);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
    reset(1);skill(U_RAMZA,2,0x196);unit_place(&st.unit[2],LOW_A);
    unit_place(&st.unit[8],UP_B);st.pending[U_RAMZA].ticks=0;
    charge_resolve();CHECK(st.open_resolving,1);CHECK(st.tgt,8);CHECK(CURSOR_CELL,UP_B);
}
static void push(void)
{
    u8 present;
    for(present=0;present<2;present++) {
        Feet before,after;
        reset(present);before=unit_feet(0,2);st.unit[2].jump=1;
        act_start(U_RAMZA,2);st.hit=1;st.dmg=1;st.abil=NO_ABIL;
        knockback(U_RAMZA,2);CHECK(st.push,1);CHECK(st.push_cell,present?UP_C:LOW_C);
        CHECK(st.dmg,present?1:21);
        st.act_t=ACT_HIT;attack_step();
        CHECK(st.unit[2].cell,present?UP_C:LOW_C);CHECK(st.unit[2].x,2);CHECK(st.unit[2].z,0);
        CHECK(unit_at_cell(UP_B),0);CHECK(unit_at_cell(present?UP_C:LOW_C),3);
        CHECK(st.unit[2].hp,present?99:79);
        after=unit_feet(0,2);CHECK(before.x!=after.x,1);
        CHECK(after.y,scene_y(2,0)+HH-map_tiles[st.unit[2].cell].stand*(HU/2));
    }
}
static void ai(void)
{
    u16 steps=0;u8 home;
    reset(1);st.unit[1].hp=st.unit[8].hp=0;st.unit[1].dc=st.unit[8].dc=DC_GONE;
    st.unit[1].x=st.unit[8].x=255;st.unit[U_RAMZA].learned=0;
    home=st.unit[U_RAMZA].cell;W->ai_k=0;
    while(!ai_think()&&steps++<1000){}
    CHECK(steps<1000,1);CHECK(st.unit[U_RAMZA].cell,home);CHECK(st.unit[U_RAMZA].x,TILE_X[home]);
    CHECK(st.unit[U_RAMZA].z,TILE_Z[home]);CHECK(st.ai_tgt,UP_B);CHECK(st.ai_o<MAP_N,1);
    st.ai=AI_AIM;st.ai_t=0;st.ai_tgt=UP_B;W->hud_ok=1;cursor_place(LOW_B);
    steps=0;while(CURSOR_CELL!=UP_B&&steps++<100)(void)ai_shown();
    CHECK(steps<100,1);CHECK(CURSOR_CELL,UP_B);CHECK(unit_at_cell(CURSOR_CELL),3);
}
static void trials(void){identity();movement();gap();charge();push();ai();}
#ifndef __m68k__
int main(void)
{
    f=malloc(sizeof(*f));if(!f)return 2;sw_init(261);trials();free(f);
    printf("cells %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;
}
#else
void _main(void)
{
    f=malloc(sizeof(*f));if(!f){BENCH_VALUE(0xBAD);return;}
    game_init();game_scenario(261);trials();free(f);
    BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);
}
#endif
