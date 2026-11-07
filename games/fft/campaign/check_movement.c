// Actual movement consumer, synthetic asymmetric terrain and saved capability IDs.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main movement_runtime_main
#define game_render movement_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void movement_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
static u16 checks,failures;static u32 digest=17;
static void equal(u16 a,u16 b,u16 line){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b){failures++;
#ifndef __m68k__
printf("line%u: %u expected%u\n",line,a,b);
#else
BENCH_VALUE(line);
#endif
}}
#define CHECK(a,b) equal(a,b,__LINE__)
typedef struct {CoreGrid grid;Tile tiles[32];u8 x[32],z[32],row[4],bank[328];} Fixture;
static Fixture *f;
static void prepare(u16 ability)
{
 u8 i,k;Unit *u;
 memset(f,0,sizeof *f);memcpy(f->bank,"FGS2",4);f->bank[4]=8;f->bank[5]=4;f->bank[7]=32;
 for(i=0;i<4;i++)f->row[i]=i<<3;
 for(i=0;i<32;i++){
  f->x[i]=i&7;f->z[i]=i>>3;f->tiles[i].stand=4;f->tiles[i].walk=1;
  for(k=0;k<4;k++)f->tiles[i].c[k]=2;
  f->bank[8+8*i]=2;f->bank[15+8*i]=128;
 }
 f->grid.count=32;f->grid.width=8;f->grid.height=4;f->grid.row=f->row;
 CHECK(battle_bind_grid(&f->grid,f->tiles,f->x,f->z),1);CHECK(battle_bind_surface(f->bank,sizeof f->bank),1);
 for(i=0;i<NUNIT;i++){
  st.unit[i].hp=0;st.unit[i].dc=DC_GONE;st.unit[i].x=st.unit[i].z=255;
  st.movement[i]=0;memset(&st.unit[i].statuses,0,sizeof st.unit[i].statuses);
 }
 st.act=st.sel=U_RAMZA;u=&st.unit[U_RAMZA];u->job=0x4a;u->hp=u->max_hp=100;u->mp=u->max_mp=20;u->dc=3;u->move=4;u->jump=3;u->team=TEAM_PLAYER;
 unit_place(u,0);cursor_place(0);st.banner=st.morph_mask=0;st.want=st.prot=255;unit_status_refresh(u);st.movement[U_RAMZA]=ability?(u8)(ability-485):0;
 st.ai=AI_OFF;st.mode=M_MENU;st.acted=st.moved=st.pick_t=0;st.weather=0;W->move_who=0;W->from_who=0;
}
static void terrain(u8 cell,u8 type,u8 water)
{f->bank[264+2*cell]=type;f->bank[9+8*cell]=water<<5;f->tiles[cell].water=water;movement_profile();}
static void range_and_costs(void)
{
 static const u16 ids[]={486,487,488,489,490,491,492,498,500,501,502,503,504,506,507};
 u8 a,i,k;u32 seed;Unit *u=&st.unit[U_RAMZA];u8 dist[32];
 for(a=0;a<sizeof ids/sizeof ids[0];a++){
  prepare(ids[a]);seed=rng32;ZB(61);compute_reach(U_RAMZA);ZE(61);
  CHECK(st.reach[0],0);CHECK(st.reach[1],1);CHECK(rng32==seed,1);CHECK(W->nav.fault,0);
  CHECK(make_path_cell(U_RAMZA,1)>=2,1);CHECK(st.path[0],0);CHECK(st.path[st.path_n-1],1);
 }
 // Source surface costs are archived, status Float and equipped Float stay separate.
 for(a=0;a<3;a++){
  prepare(a==0?500:a==1?501:506);st.weather=4;terrain(1,9,0);compute_reach(U_RAMZA);
  CHECK(st.reach[1],a==0||a==2?1:3);
 }
 prepare(0);st.weather=4;terrain(1,9,0);compute_reach(U_RAMZA);CHECK(st.reach[1],3);
 prepare(0);terrain(1,14,0);compute_reach(U_RAMZA);CHECK(st.reach[1],2);
 prepare(501);terrain(1,14,0);compute_reach(U_RAMZA);CHECK(st.reach[1],1);
 // Any Ground flattens traversable terrain, never a source FF forbidden cell.
 prepare(501);terrain(1,63,0);compute_reach(U_RAMZA);CHECK(st.reach[1],255);
 // Freeze a column barrier: walking cannot cross; Fly and Teleport can land beyond it.
 for(a=0;a<3;a++){
  prepare(a==0?486:a==1?507:498);
  for(i=0;i<4;i++)for(k=(i<<3)+3;k<=(i<<3)+4;k++){f->tiles[k].walk=0;f->bank[11+8*k]=128;}
  u->move=7;compute_reach(U_RAMZA);CHECK(st.reach[6],a==0?255:6);
  if(a){CHECK(make_path_cell(U_RAMZA,6),2);CHECK(st.path[1],6);}
 }
 // Same source floor, asymmetric tall destination; Ignore Height raises only traversal.
 for(a=0;a<2;a++){
  prepare(a?492:486);f->tiles[1].stand=40;f->bank[16]=20;
  for(k=0;k<4;k++)f->tiles[1].c[k]=20;
  compute_reach(U_RAMZA);CHECK(st.reach[1],a?1:255);
 }
 // Special moves obey landing occupancy and move incapacity.
 prepare(498);unit_place(&st.unit[1],5);st.unit[1].hp=100;st.unit[1].dc=3;
 compute_reach(U_RAMZA);CHECK(st.reach[5],255);CHECK(make_path_cell(U_RAMZA,5),0);
 prepare(507);u->statuses.inflicted[3]|=2;unit_status_refresh(u);compute_reach(U_RAMZA);
 CHECK(st.reach[0],0);CHECK(st.reach[1],255);CHECK(make_path_cell(U_RAMZA,1),0);
 // Deep-water and lava landing differ from mere traversal.
 for(a=0;a<3;a++){
  prepare(a==0?502:a==1?503:506);terrain(1,17,3);compute_reach(U_RAMZA);CHECK(st.reach[1]!=255,1);
 }
 prepare(507);terrain(1,17,3);compute_reach(U_RAMZA);CHECK(st.reach[1],255);
 prepare(504);terrain(1,18,0);compute_reach(U_RAMZA);CHECK(st.reach[1],1);
 prepare(498);terrain(1,18,0);compute_reach(U_RAMZA);CHECK(st.reach[1],255);
 // Cached graph changes with equipment code and source weather, without a manual reset.
 prepare(500);st.weather=4;terrain(1,9,0);compute_reach(U_RAMZA);CHECK(st.reach[1],1);
 st.movement[U_RAMZA]=501-485;compute_reach(U_RAMZA);CHECK(st.reach[1],3);
 st.weather=0;compute_reach(U_RAMZA);CHECK(st.reach[1],1);
 // Custom costs have identical narrow/wide outcomes when all edges cost at most3.
 prepare(500);movement_ready(U_RAMZA);memset(W->blk,0,MAP_N);
 core_nav_begin(&W->nav,MAP_N,0,4,dist,0,W->blk,CORE_NAV_CUSTOM,0,0);while(!spread_go(1)){}
 memcpy(st.reach,dist,32);core_nav_begin(&W->nav,MAP_N,0,4,dist,0,W->blk,CORE_NAV_CUSTOM|CORE_NAV_WIDE,0,0);
 while(!spread_go(1)){}CHECK(memcmp(st.reach,dist,32)==0,1);
}
static void bonuses_and_save(void)
{
 u8 n;Unit *u=&st.unit[U_RAMZA];u8 move,jump;
 prepare(0);memset(u->eq,255,sizeof u->eq);u->level=1;unit_stats(u);move=u->move;jump=u->jump;
 for(n=1;n<=6;n++){
  st.movement[U_RAMZA]=n;unit_stats(u);CHECK(u->move,move+(n<=3?n:0));CHECK(u->jump,jump+(n>3?n-3:0)>7?7:jump+(n>3?n-3:0));
  {u8 m=u->move,j=u->jump;unit_stats(u);CHECK(u->move,m);CHECK(u->jump,j);}
 }
 // Actual FSV3 party loader loads movement before deriving stats and retains the source ID.
 CHECK(chapter_load(1),1);game_scenario(61);memset(&battle_save,0,sizeof battle_save);battle_save.n=1;
 battle_save.unit[0].type=2;battle_save.unit[0].job=0x4a;battle_save.unit[0].level=1;battle_save.unit[0].movement[0]=1;battle_save.unit[0].movement[1]=232; //488 Move+3
 battle_save.unit[0].brave=battle_save.unit[0].faith=70;memcpy(battle_save.magic,SAVE_MAGIC,4);
 CHECK(rt_save("ffsave",&battle_save,sizeof battle_save),1);
#ifdef __m68k__
 save_write();
#endif
 party_load();CHECK(st.movement[U_RAMZA],3);
 CHECK(st.unit[U_RAMZA].move,jobs[0x4a].move+3);CHECK(battle_save.unit[0].movement[1],232);
 battle_save.unit[0].movement[1]=250;CHECK(rt_save("ffsave",&battle_save,sizeof battle_save),1);
#ifdef __m68k__
 save_write();
#endif
 party_load();CHECK(st.movement[U_RAMZA],21);CHECK(st.unit[U_RAMZA].statuses.innate[2]&64,64);
 CHECK(skill_kind(U_RAMZA,U_RAMZA,0x40),AB_NULL);
 {CoreStatusDelta d;memset(&d,0,sizeof d);d.remove[2]=64;battle_status_apply(U_RAMZA,&d,0);CHECK(core_status_current(&st.unit[U_RAMZA].statuses,2)&64,64);}
 CHECK(sizeof(Save),SAVE_V3_BYTES+SAVE_DISCOVERY_BYTES);CHECK(sizeof(SaveUnit),176);
}
static void movement_step(u32 keys)
{
#ifdef __m68k__
 rt_prev=rt_keys;rt_keys=keys;game_update();rt_frame++;
#else
 sw_step(keys);
#endif
}
static void teleport_execution(void)
{
 u8 n;u32 seed,after;Unit *u=&st.unit[U_RAMZA];
 // Cost and RNG are committed only after a legal target. A failed attempt spends Move.
 for(n=0;n<2;n++){
  prepare(498);u->move=n?1:4;compute_reach(U_RAMZA);seed=rng32;
  CHECK(make_path_cell(U_RAMZA,0),0);CHECK(rng32==seed,1);
  rng32=1;rand15();after=rng32;rng32=1;CHECK(make_path_cell(U_RAMZA,n?31:1),2);CHECK(rng32==after,1);
  if(n)CHECK(st.path[1],0);else CHECK(st.path[1],1);
  st.mode=M_WALK;st.path_i=st.walk_t=0;st.move_cell=0;
  {u8 frames=0;while(st.mode==M_WALK&&frames++<32)movement_step(0);CHECK(frames<32,1);}
  CHECK(st.moved,1);CHECK(UNIT_CELL(u),n?0:1);CHECK(rng32==after,1);
  st.menu=MN_ACT;movement_step(K_B);CHECK(st.moved,1);CHECK(UNIT_CELL(u),n?0:1);
 }
}
static void special_ai(void)
{
 u8 n;u16 steps;Unit *u=&st.unit[U_RAMZA];
 for(n=0;n<2;n++){
  prepare(n?498:507);unit_place(&st.unit[1],6);st.unit[1].hp=100;st.unit[1].dc=3;st.unit[1].team=TEAM_ENEMY;
  memset(u->eq,255,sizeof u->eq);u->learned=0;u->set=5;st.acted=0;W->ai_k=0;
  for(steps=0;steps<1024&&!ai_think();steps++){}
  CHECK(steps<1024,1);CHECK(st.ai_o<MAP_N,1);
  if(st.ai_o<MAP_N){CHECK(map_tiles[st.ai_o].walk,1);CHECK(st.ai_o!=6,1);}
 }
}
static void trials(void)
{
 CHECK(W!=0,1);if(!W)return;f=malloc(sizeof *f);CHECK(f!=0,1);if(!f)return;
 range_and_costs();teleport_execution();special_ai();bonuses_and_save();free(f);f=0;
}
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("equipped_movement %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){movement_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
