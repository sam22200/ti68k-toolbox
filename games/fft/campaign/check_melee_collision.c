// Retail interval fixtures and actual weapon/response preparation.
#define COMBAT_UI_CHECK
#include "check_combat.c"
#include "../melee_rules.h"
#include "x/core-melee-collision/oracle/source.h"
static void source_melee(void)
{
 u16 size,i;const u8 *r,*data=rt_file("fmelee",&size);MeleeSpan a,t,m[2];u8 floor[2];
 CHECK(data!=RT_NULL,1);CHECK(size>=MELEE_SOURCE_CASES*MELEE_SOURCE_FIELDS,1);
 for(i=0;i<MELEE_SOURCE_CASES;i++){
  r=data+i*MELEE_SOURCE_FIELDS;a.feet=r[0];a.top=r[1];a.id=0;t.feet=r[2];t.top=r[3];t.id=r[4];
  floor[0]=r[6];floor[1]=r[7];m[0].feet=r[8];m[0].top=r[9];m[0].id=r[10];m[1].feet=r[11];m[1].top=r[12];m[1].id=r[13];
  CHECK(core_melee_lunge(&a,&t,r[5],floor,m),r[14]);
 }
}
static u8 arena_surface[2048];static const u8 *original_surface;
static void melee_arena(void)
{
 u16 i;combat_arena();original_surface=W->surface;memcpy(arena_surface,W->surface,(u16)MAP_W*MAP_H*8);W->surface=arena_surface;
 for(i=0;i<(u16)MAP_W*MAP_H;i++){arena_surface[i*8]=4;arena_surface[i*8+1]=0;arena_surface[i*8+3]=0;arena_surface[i*8+7]=128;}
 st.unit[U_RAMZA].eq[EQ_WEAPON]=99;unit_place(&st.unit[10],third);
}
static void restore_surface(void){W->surface=original_surface;}
static void middle_unit(void)
{
 st.unit[8]=st.unit[U_RAMZA];unit_place(&st.unit[8],second);st.unit[8].team=TEAM_PLAYER;
}
static void melee_finish(void)
{
 u8 n;for(n=0;n<80&&st.action.phase!=CTX_IDLE;n++){
  if(st.act_t<ACT_HIT)impact();else end_anim();
 }
 CHECK(st.action.phase,CTX_IDLE);
}
static void executions(void)
{
 u16 hp;u32 seed;u16 at;u8 i;
 melee_arena();rng32=1;attack(U_RAMZA,10);CHECK(st.tgt,10);CHECK(st.hit,1);melee_finish();restore_surface();
 // A living middle body intercepts the lunge; the chosen panel remains fixed.
 melee_arena();middle_unit();seed=rng32=1;hp=st.unit[10].hp;attack(U_RAMZA,10);
 CHECK(st.tgt,8);CHECK(st.action.cell,third);CHECK(st.hit,1);st.crit=st.push=0;impact();CHECK(st.unit[10].hp,hp);CHECK(st.unit[8].hp<999,1);melee_finish();restore_surface();
 // Empty solid terrain blocks before evasion/critical/proc draws.
 melee_arena();at=(u16)(W->grid_row[TILE_Z[second]]+TILE_X[second])*8;arena_surface[at]=8;
 seed=rng32=1;hp=st.unit[10].hp;attack(U_RAMZA,10);CHECK(st.hit,0);CHECK(st.dmg,0);CHECK(rng32==seed,1);impact();CHECK(st.unit[10].hp,hp);melee_finish();restore_surface();
 // A KO middle body supplies no blocking span.
 melee_arena();middle_unit();st.unit[8].hp=0;attack(U_RAMZA,10);CHECK(st.tgt,10);CHECK(st.hit,1);melee_finish();restore_surface();
 // Hamedo uses the same geometry, then independently prepares the returned strike.
 melee_arena();middle_unit();st.unit[10].eq[EQ_WEAPON]=99;st.reaction[10]=REACTION_HAMEDO;rng32=1;
 attack(U_RAMZA,10);CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.tgt,8);melee_finish();restore_surface();
 // Pure AI cache rejects the farther victim behind a middle ally at this origin.
 melee_arena();middle_unit();memset(W->ai_m,0,sizeof W->ai_m);W->ai_m[first]=1;W->ai_weapon_left=512;W->ai_melee_at=0;st.act=U_RAMZA;seed=rng32;
 for(i=0;i<20&&W->ai_weapon_left;i++){
  ZB(61);
#ifdef FFT_MODULES
  mod_call(MOD_PLANNER,0,0);
#else
  melee_cache();
#endif
  ZE(61);
 }
 CHECK(W->ai_weapon_left,0);CHECK(W->ai_melee_mask[first]&((u16)1<<10),0);CHECK(W->ai_melee_mask[first]&((u16)1<<8),(u16)1<<8);CHECK(rng32==seed,1);CHECK(UNIT_CELL(&st.unit[U_RAMZA]),first);restore_surface();
}
static void melee_trials(void){source_melee();executions();}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);melee_trials();printf("melee_collision %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();melee_trials();BENCH_VALUE(sizeof(Unit));BENCH_VALUE(sizeof(State));BENCH_VALUE(sizeof(Work));BENCH_VALUE(sizeof(Save));BENCH_VALUE(sizeof(ModCtx));BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
