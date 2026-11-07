// Ranged pre-action cancellation and the source Brave/rejection ordering.
#define COMBAT_UI_CHECK
#include "check_combat.c"
#include "x/core-hamedo/oracle/source.h"
static u8 bow,far_cell;static const u8 *wall_source;
static void ranged_arena(u8 blocked)
{
 u16 c,w;combat_arena();st.golem[0]=st.golem[1]=0;
 // Direct weapons are blocked by solid terrain. Body interception and the
 // separate ballistic upper arc are covered by their own integration suites.
 for(w=0;w<128;w++)if((cj_weapon(common,w)[1]&32)&&items[w].reach>=4)break;
 CHECK(w<128,1);bow=(u8)w;
 for(c=0;c+3<MAP_N;c++)if(TILE_Z[c]==TILE_Z[c+3]&&map_tiles[c].walk==1&&map_tiles[c+1].walk==1&&map_tiles[c+2].walk==1&&map_tiles[c+3].walk==1&&map_tiles[c].stand==map_tiles[c+1].stand&&map_tiles[c].stand==map_tiles[c+2].stand&&map_tiles[c].stand==map_tiles[c+3].stand)break;
 CHECK(c+3<MAP_N,1);far_cell=c+3;unit_place(&st.unit[U_RAMZA],c);unit_place(&st.unit[10],far_cell);st.unit[10].eq[EQ_WEAPON]=bow;
 st.reaction[10]=REACTION_HAMEDO;
 if(blocked)wall_source=solid_panel(c+1);
 CHECK(ranged_reach(10,far_cell,UNIT_CELL(&st.unit[U_RAMZA])),1);CHECK(battle_line_clear(10,U_RAMZA),!blocked);
}
static void finish(void)
{
 u16 i;for(i=0;i<100&&(st.action.phase!=CTX_IDLE||W->mime.active||W->mime.pending);i++){
  if(st.action.phase!=CTX_IDLE&&st.act_t<ACT_HIT)impact();else end_anim();
 }
 CHECK(i<100,1);CHECK(st.action.phase,CTX_IDLE);
}
static void ranged_cases(void)
{
 u8 blocked,miss;u16 hp,jp,mp,ct,stock,c;
 for(blocked=0;blocked<2;blocked++)for(miss=0;miss<2;miss++){
  ranged_arena(blocked);st.pending[10].state=DEFER_CHARGING;st.pending[10].ticks=7;st.unit[U_RAMZA].eq[EQ_WEAPON]=bow;
  hp=st.unit[10].hp;jp=st.unit[U_RAMZA].jp;mp=st.unit[U_RAMZA].mp;ct=st.unit[10].ct;rng32=1;
  CHECK(attack_sequence(U_RAMZA,10,2),1);CHECK(st.atk,blocked?U_RAMZA:10);CHECK(st.action.phase,blocked?CTX_ROOT:CTX_SECONDARY);CHECK(st.action.cursor,blocked?0:128);
  if(!blocked){st.hit=!miss;st.dmg=11;st.push=0;impact();CHECK(st.unit[10].hp,hp);CHECK(st.primary.reward,0);CHECK(st.action.pending,0);}
  finish();CHECK(st.unit[U_RAMZA].mp,mp);CHECK(st.pending[10].state,DEFER_CHARGING);CHECK(st.pending[10].ticks,7);CHECK(st.unit[10].ct,ct);
  if(!blocked){CHECK(st.unit[10].hp,hp);CHECK(st.unit[U_RAMZA].jp,jp);}
  if(blocked)solid_restore(wall_source);
 }
 // Throw stock remains untouched by an intercept, including a forced miss.
 for(blocked=0;blocked<2;blocked++){
  ranged_arena(blocked);st.unit[U_RAMZA].set=20;st.unit[U_RAMZA].learned=0xfff;battle_save.inv[122]=2;rng32=1;
  skill(U_RAMZA,10,634);CHECK(st.atk,blocked?U_RAMZA:10);CHECK(battle_save.inv[122],blocked?1:2);
  st.hit=0;stock=battle_save.inv[122];impact();finish();CHECK(battle_save.inv[122],stock);
  if(blocked)solid_restore(wall_source);
 }
 // Due Charge uses the same preflight; arming never starts Hamedo.
 ranged_arena(0);st.unit[U_RAMZA].set=2;st.unit[U_RAMZA].learned=0xffff;st.unit[U_RAMZA].eq[EQ_WEAPON]=bow;
 skill(U_RAMZA,10,406);CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);CHECK(st.action.phase,CTX_IDLE);st.pending[U_RAMZA].ticks=0;rng32=1;
 CHECK(charge_resolve(),1);CHECK(st.atk,10);CHECK(st.open_resolving,1);st.hit=0;impact();finish();
 // Lethal pre-emption settles before defeat, and cannot record a Mime donor.
 ranged_arena(0);st.unit[U_RAMZA].hp=1;st.unit[8]=st.unit[U_RAMZA];st.unit[8].job=93;for(c=0;c<MAP_N;c++)if(map_tiles[c].walk==1&&TILE_Z[c]!=st.unit[U_RAMZA].z)break;CHECK(c<MAP_N,1);unit_place(&st.unit[8],c);rng32=1;
 attack(U_RAMZA,10);CHECK(W->mime.pending,0);st.hit=1;st.dmg=999;st.push=0;impact();CHECK(st.unit[U_RAMZA].hp,0);CHECK(st.end,END_NONE);finish();
}
static void source_cases(void)
{
 u16 i;u8 accepted,reachable,sid,who,target;u32 expected,seed;
 for(i=0;i<sizeof source_hamedo/sizeof source_hamedo[0];i++){
  accepted=source_hamedo[i].accepted;reachable=source_hamedo[i].reachable;sid=source_hamedo[i].sid;seed=source_hamedo[i].seed;
  combat_arena();st.golem[0]=st.golem[1]=0;if(!reachable)unit_place(&st.unit[U_RAMZA],second);
  st.reaction[10]=REACTION_HAMEDO;st.unit[10].brave=source_hamedo[i].brave;status(10,sid);
  // Compose the unchanged strike RNG with the original outer Brave prefix.
  rng32=seed;rand15();who=accepted?10:U_RAMZA;target=accepted?U_RAMZA:10;
  act_start(who,target);st.abil=NO_ABIL;if(accepted)st.action.phase=CTX_SECONDARY;attack_roll(who,target);expected=rng32;
  combat_arena();st.golem[0]=st.golem[1]=0;if(!reachable)unit_place(&st.unit[U_RAMZA],second);
  st.reaction[10]=REACTION_HAMEDO;st.unit[10].brave=source_hamedo[i].brave;status(10,sid);rng32=seed;
  attack(U_RAMZA,10);CHECK(st.atk,accepted?10:U_RAMZA);CHECK(rng32==expected,1);CHECK(st.action.cursor,accepted?128:0);CHECK(source_hamedo[i].draws,1);
 }
}
static void rejected_orders(void)
{
 State before;u32 seed;u8 mode;
 for(mode=0;mode<2;mode++){
  ranged_arena(0);st.formula.actor=123;st.formula.target=45;st.formula.ability=413;st.formula.roll=888;
  if(mode)st.action.phase=CTX_ROOT;before=st;seed=rng32;
  CHECK(attack_sequence(U_RAMZA,10,mode?1:0),0);CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(rng32==seed,1);
 }
}
static void hamedo_trials(void){source_cases();ranged_cases();rejected_orders();}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);hamedo_trials();printf("hamedo %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();hamedo_trials();BENCH_VALUE(sizeof(Unit));BENCH_VALUE(sizeof(State));BENCH_VALUE(sizeof(Work));BENCH_VALUE(sizeof(Save));BENCH_VALUE(sizeof(ModCtx));BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
