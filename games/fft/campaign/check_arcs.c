// Retail arc selection, independent ballistic controls, actual weapon owners.
#define COMBAT_UI_CHECK
#include "check_combat.c"
#include "../arc_rules.h"
#include "x/core-arcs/oracle/source.h"
static void arc_source(void)
{
 u16 size,i;const u8 *r,*data=rt_file("farc",&size);s16 dx,dz,rise,curve,height;u8 high,k,step;const u8 steps[5]={1,16,32,48,63};CoreArcStep path;
 CHECK(data!=RT_NULL,1);CHECK(size>=ARC_CHOICES*5+ARC_MODELS*20,1);
 for(i=0;i<ARC_CHOICES;i++){
  r=data+i*5;CHECK(r[0]==1||r[1]==1?1:core_arc_recipient(r[0],r[1],r[2],r[3]),r[4]);
 }
 for(i=0;i<ARC_MODELS;i++){
  r=data+ARC_CHOICES*5+i*20;dx=(s16)cj_word(r);dz=(s16)cj_word(r+2);rise=(s16)cj_word(r+4);high=cj_word(r+6);
  curve=core_arc_curve(dx,dz,rise,high);CHECK(curve,cj_word(r+8));
  if(curve>=0){core_arc_begin(&path,80,rise,curve);CHECK(path.y>>12,80);k=0;
   for(step=1;step<=64;step++){height=core_arc_next(&path);if(k<5&&step==steps[k]){CHECK(height,cj_word(r+10+k*2));k++;}}
   CHECK(height,80+rise);}
 }
}
static u8 arc_surface[2048];static const u8 *saved_surface;static u8 saved_flags;static u8 bow;
static void arc_arena(void)
{
 u16 c,i;combat_arena();saved_surface=W->surface;saved_flags=W->surface_flags;
 memcpy(arc_surface,W->surface,(u16)MAP_W*MAP_H*8);W->surface=arc_surface;W->surface_flags|=SURFACE_HIT_UPPER;
 for(c=0;c+4<MAP_N;c++)if(TILE_Z[c]==TILE_Z[c+4]){
  for(i=0;i<5;i++)if(!map_tiles[c+i].walk)break;if(i==5)break;
 }
 CHECK(c+4<MAP_N,1);first=c;second=c+1;third=c+4;
 for(i=0;i<MAP_N;i++)W->hs[i]=8;
 for(i=0;i<(u16)MAP_W*MAP_H;i++){arc_surface[i*8]=4;arc_surface[i*8+1]=0;arc_surface[i*8+3]=0;arc_surface[i*8+4]=0;arc_surface[i*8+7]=128;}
 unit_place(&st.unit[U_RAMZA],first);unit_place(&st.unit[10],third);
 for(bow=0;bow<128;bow++)if(cj_weapon(common,bow)[1]&16)break;
 CHECK(bow<128,1);st.unit[U_RAMZA].eq[EQ_WEAPON]=bow;
}
static void arc_restore(void){W->surface=saved_surface;W->surface_flags=saved_flags;}
static void arc_ceiling(void)
{
 u16 i;for(i=0;i<(u16)MAP_W*MAP_H;i++){arc_surface[i*8+4]=12;arc_surface[i*8+7]=1;}
}
static u16 arc_column(u8 cell){return (u16)(W->grid_row[TILE_Z[cell]]+TILE_X[cell])*8;}
static void arc_finish(void)
{
 u8 n;for(n=0;n<80&&st.action.phase!=CTX_IDLE;n++){if(st.act_t<ACT_HIT)impact();else end_anim();}CHECK(st.action.phase,CTX_IDLE);
}
static void arc_executions(void)
{
 u16 hp,at;u32 seed;u16 n;u8 home,ready;
 // A low wall blocks the straight ray; the upper ballistic arc still reaches.
 arc_arena();at=arc_column(first+2);arc_surface[at]=10;rng32=1;
 CHECK(battle_line_clear(U_RAMZA,10),1);attack(U_RAMZA,10);CHECK(st.tgt,10);CHECK(st.hit,1);arc_finish();arc_restore();
 // A taller column blocks both arcs before any evade/critical/proc RNG.
 arc_arena();at=arc_column(first+2);arc_surface[at]=20;seed=rng32=1;hp=st.unit[10].hp;
 CHECK(battle_line_clear(U_RAMZA,10),0);CHECK(rng32==seed,1);attack(U_RAMZA,10);CHECK(st.hit,0);CHECK(st.dmg,0);CHECK(rng32==seed,1);impact();CHECK(st.unit[10].hp,hp);arc_finish();arc_restore();
 // Lower arc interception plus upper ceiling: actual recipient, original panel.
 arc_arena();arc_ceiling();st.unit[8]=st.unit[10];unit_place(&st.unit[8],second);hp=st.unit[10].hp;rng32=1;
 attack(U_RAMZA,10);CHECK(st.tgt,8);CHECK(st.action.cell,third);CHECK(st.hit,1);st.crit=st.push=0;impact();CHECK(st.unit[10].hp,hp);CHECK(st.unit[8].hp<999,1);arc_finish();arc_restore();
 // With no ceiling, a high arc clears that same intervening body.
 arc_arena();st.unit[8]=st.unit[10];unit_place(&st.unit[8],second);attack(U_RAMZA,10);CHECK(st.tgt,10);CHECK(st.hit,1);arc_finish();arc_restore();
 // Pure origin cache uses ballistic paths and restores the actor and RNG.
 arc_arena();at=arc_column(first+2);arc_surface[at]=10;home=UNIT_CELL(&st.unit[U_RAMZA]);
 memset(W->ai_m,0,sizeof W->ai_m);W->ai_m[home]=1;W->ai_weapon_left=512;W->ai_melee_at=0;st.act=U_RAMZA;seed=rng32;
 for(n=0;n<128&&W->ai_weapon_left;n++){
  ZB(61);
#ifdef FFT_MODULES
  mod_call(MOD_PLANNER,0,0);
#else
  melee_cache();
#endif
  ZE(61);
 }
 CHECK(W->ai_weapon_left,0);CHECK(W->ai_melee_mask[home]&((u16)1<<10),(u16)1<<10);CHECK(UNIT_CELL(&st.unit[U_RAMZA]),home);CHECK(rng32==seed,1);arc_restore();
 // Hamedo's preflight now accepts the defender's bow above that low wall.
 arc_arena();at=arc_column(first+2);arc_surface[at]=10;st.unit[10].eq[EQ_WEAPON]=bow;st.reaction[10]=REACTION_HAMEDO;rng32=1;
 attack(U_RAMZA,10);CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.tgt,U_RAMZA);arc_finish();arc_restore();
 // The full stationary AI chooses the target above a wall; no straight-ray
 // veto may undo the ballistic origin mask. Final ranking may draw RNG.
 arc_arena();at=arc_column(first+2);arc_surface[at]=10;
 st.unit[10].eq[EQ_WEAPON]=bow;st.unit[10].move=0;st.unit[10].learned=0;
 st.act=10;st.ai=AI_THINK;st.acted=st.moved=0;W->ai_k=0;seed=rng32;
 for(n=0,ready=0;n<1000&&!ready;n++)ready=ai_think();
 CHECK(ready,1);CHECK(st.ai_ab,NO_ABIL);CHECK(st.ai_tgt,UNIT_CELL(&st.unit[U_RAMZA]));arc_restore();
 // Direct weapons share the same cache contract but retain a straight path.
 arc_arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=71;at=arc_column(first+2);arc_surface[at]=10;
 memset(W->ai_m,0,sizeof W->ai_m);home=UNIT_CELL(&st.unit[U_RAMZA]);W->ai_m[home]=1;
 W->ai_weapon_left=512;W->ai_melee_at=0;st.act=U_RAMZA;seed=rng32;
 for(n=0;n<128&&W->ai_weapon_left;n++){
#ifdef FFT_MODULES
  mod_call(MOD_PLANNER,0,0);
#else
  melee_cache();
#endif
 }
 CHECK(W->ai_weapon_left,0);CHECK(W->ai_melee_mask[home]&((u16)1<<10),0);CHECK(rng32==seed,1);CHECK(UNIT_CELL(&st.unit[U_RAMZA]),home);arc_restore();
}
static void arc_trials(void){arc_source();arc_executions();}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);arc_trials();printf("arcs %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();arc_trials();BENCH_VALUE(sizeof(Unit));BENCH_VALUE(sizeof(State));BENCH_VALUE(sizeof(Work));BENCH_VALUE(sizeof(Save));BENCH_VALUE(sizeof(ModCtx));BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
