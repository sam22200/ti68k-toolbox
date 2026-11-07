// Real source footprints plus the shared native command/range/response owners.
#define COMBAT_UI_CHECK
#include "check_combat.c"
#include "../weapon_range.h"
#include "x/core-weapon-reach/oracle/source.h"
static void range_request(u8 who,AbId ab)
{
#ifdef FFT_MODULES
 st.formula.actor=who;st.formula.ability=ab;
 BENCH_BEGIN(61);mod_call(ab==NO_ABIL?MOD_TURN:MOD_MOVEMENT,ab==NO_ABIL?2:5,0);BENCH_END(61);
#else
 if(ab==NO_ABIL)attack_range(who);else skill_range(who,ab);
#endif
}
static void shape_checks(void)
{
 u16 size,i,k;u8 x,z,layer;const u8 *data=rt_file("fwreach",&size);
 CHECK(data!=RT_NULL,1);CHECK(size>=SOURCE_WEAPON_PROFILES*338UL,1);
 for(i=0;i<SOURCE_WEAPON_PROFILES;i++){
  for(layer=0;layer<2;layer++)for(z=0;z<13;z++)for(x=0;x<13;x++){
   k=(u16)layer*169+(u16)z*13+x;
   CHECK(core_weapon_reaches(source_weapon[i].flags,source_weapon[i].range,(s16)x-6,(s16)z-6,source_weapon[i].height,(u16)source_weapon[i].target<<1),data[source_weapon[i].offset+k]);
  }
 }
}
static void finish_weapon(void)
{
 u8 i;for(i=0;i<80&&(st.action.phase!=CTX_IDLE||W->mime.active||W->mime.pending);i++){
  if(st.action.phase!=CTX_IDLE&&st.act_t<ACT_HIT)impact();else end_anim();
 }
 CHECK(i<80,1);CHECK(st.action.phase,CTX_IDLE);
}
static void live_checks(void)
{
 u16 c,i;u8 d,flags;u32 seed;Unit *a,*t;
 combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->eq[EQ_WEAPON]=99;unit_place(t,third);
 CHECK(cj_equipment_item(common,99)[2]&64,64);CHECK(ranged_reach(U_RAMZA,first,third),1);
 range_request(U_RAMZA,NO_ABIL);for(c=0;c<MAP_N;c++)CHECK(st.reach[c]!=255,ranged_reach(U_RAMZA,first,c));
 range_request(U_RAMZA,406);for(c=0;c<MAP_N;c++)CHECK(st.reach[c]!=255,ranged_reach(U_RAMZA,first,c));
 // Cardinal two-tile reach; diagonal panels are rejected even at distance2.
 for(c=0;c<MAP_N;c++)if(map_tiles[c].walk&&TILE_X[c]==a->x+1&&TILE_Z[c]==a->z+1){CHECK(ranged_reach(U_RAMZA,first,c),0);break;}
 CHECK(c<MAP_N,1);status(U_RAMZA,22);CHECK(WEAPON_REACH(a),1);CHECK(ranged_reach(U_RAMZA,first,third),0);CHECK(ranged_reach(U_RAMZA,first,second),1);
 combat_arena();a=&st.unit[U_RAMZA];a->eq[EQ_SHIELD]=99;CHECK(WEAPON_REACH(a),2);CHECK(ranged_reach(U_RAMZA,first,third),1);
 // Hamedo and ordinary Counter share the same newly legal two-tile return.
 for(d=0;d<2;d++){
  combat_arena();unit_place(&st.unit[10],third);st.unit[U_RAMZA].eq[EQ_WEAPON]=st.unit[10].eq[EQ_WEAPON]=99;st.reaction[10]=d?REACTION_COUNTER:REACTION_HAMEDO;rng32=1;
  attack(U_RAMZA,10);if(d){st.hit=0;impact();end_anim();}
  CHECK(st.atk,10);CHECK(st.tgt,U_RAMZA);CHECK(st.action.phase,CTX_SECONDARY);finish_weapon();
 }
 // Bow uphill penalty uses truncation toward zero; direct weapons have no height bonus.
 combat_arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=83;
 CHECK(core_weapon_reaches(16,5,5,0,20,24),0);CHECK(core_weapon_reaches(16,5,5,0,20,23),1);CHECK(core_weapon_reaches(16,5,6,0,20,16),1);
 CHECK(core_weapon_reaches(32,8,8,0,20,0),1);CHECK(core_weapon_reaches(32,8,9,0,20,0),0);CHECK(core_weapon_reaches(32,8,2,0,20,20),0);
 seed=rng32;for(i=0;i<10;i++)ranged_reach(U_RAMZA,first,third);CHECK(rng32==seed,1);
 (void)flags;
}
static void height_checks(void)
{
 u8 terrain[2048],i;const u8 *saved,*r;u16 at;
 combat_arena();saved=W->surface;memcpy(terrain,saved,(u16)MAP_W*MAP_H*8);W->surface=terrain;
 at=((u16)(W->grid_row[st.unit[U_RAMZA].z]+st.unit[U_RAMZA].x)<<3)+4*node_layer(first);
 for(i=0;i<sizeof source_weapon_heights/sizeof source_weapon_heights[0];i++){
  r=source_weapon_heights[i];terrain[at]=r[0];terrain[at+1]=(r[2]<<5)|r[1];st.movement[U_RAMZA]=r[3];
  memset(&st.unit[U_RAMZA].statuses,0,sizeof st.unit[U_RAMZA].statuses);if(r[4])st.unit[U_RAMZA].statuses.inflicted[2]=64;
  CHECK(weapon_origin_height(U_RAMZA,first),r[5]);
 }
 W->surface=saved;
}
static void ai_weapon_checks(void)
{
 u16 i;u8 frog;
 for(frog=0;frog<2;frog++){
  combat_arena();unit_place(&st.unit[10],third);st.unit[10].eq[EQ_WEAPON]=99;st.unit[10].learned=0;st.unit[10].move=0;st.unit[10].pa=20;
  st.act=10;st.ai=AI_THINK;st.acted=st.moved=0;st.ai_t=AI_HOLD_SKIP;W->ai_k=0;if(frog)status(10,22);
  for(i=0;i<1000&&!ai_think();i++){}CHECK(i<1000,1);CHECK(st.ai_ab,frog?AI_NONE:NO_ABIL);
 }
}
static void range_matrix_checks(void)
{
 static const u8 weapons[4]={19,99,83,71};u8 i,warm;u16 c;u32 seed;
 for(i=0;i<4;i++){
  combat_arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=weapons[i];
  for(warm=0;warm<2;warm++){
#ifdef FFT_MODULES
   if(!warm)mod_id=0;
   BENCH_BEGIN(50+i);
#endif
   seed=rng32;range_request(U_RAMZA,NO_ABIL);
#ifdef FFT_MODULES
   BENCH_END(50+i);
#endif
   CHECK(rng32==seed,1);
   for(c=0;c<MAP_N;c++)CHECK(st.reach[c]!=255,ranged_reach(U_RAMZA,first,c));
  }
 }
}
static void clock_checks(void)
{
 u8 result;u32 seed;combat_arena();st.unit[U_RAMZA].ct=0;st.unit[U_RAMZA].sp=1;st.unit[10].ct=st.unit[10].sp=0;
 st.status_clock=st.open_ct_phase=0;seed=rng32;
#ifdef FFT_MODULES
 mod_id=0;
#endif
#ifdef __m68k__
 BENCH_BEGIN(59);
#endif
 result=ct_next();
#ifdef __m68k__
 BENCH_END(59);
#endif
 CHECK(result,U_RAMZA);CHECK(st.unit[U_RAMZA].ct,0);CHECK(rng32==seed,1);
 st.unit[U_RAMZA].ct=100;
#ifdef __m68k__
 BENCH_BEGIN(60);
#endif
 result=ct_next();
#ifdef __m68k__
 BENCH_END(60);
#endif
 CHECK(result,U_RAMZA);CHECK(st.unit[U_RAMZA].ct,0);CHECK(rng32==seed,1);
}
static void weapon_trials(void){shape_checks();live_checks();height_checks();ai_weapon_checks();range_matrix_checks();clock_checks();}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);weapon_trials();printf("weapon_reach %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();weapon_trials();BENCH_VALUE(sizeof(Unit));BENCH_VALUE(sizeof(State));BENCH_VALUE(sizeof(Work));BENCH_VALUE(sizeof(Save));BENCH_VALUE(sizeof(ModCtx));BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
