// Sequential imitation uses the ordinary primary and response owners.
#define COMBAT_UI_CHECK
#include "check_combat.c"
#include "x/core-mime/oracle/rotations.h"
static u8 next_mime(void){
#ifdef FFT_MODULES
 return (u8)mod_call(MOD_NATURE,9,0);
#else
 return mime_next();
#endif
}
static void mime_arena(void){combat_arena();}
static void add_mime(u8 who,u8 cell,u8 face)
{
 st.unit[who]=st.unit[U_RAMZA];unit_place(&st.unit[who],cell);st.unit[who].job=93;st.unit[who].set=24;st.unit[who].learned=0;st.unit[who].face=face;
 st.unit[who].slot=255;st.unit[who].pa=8;st.unit[who].ma=12;
}
static void drain(void)
{
 u16 i;for(i=0;i<150&&(st.action.phase!=CTX_IDLE||W->mime.active||W->mime.pending);i++){
  if(st.action.phase!=CTX_IDLE&&st.act_t<ACT_HIT)impact();else end_anim();
 }
 CHECK(i<150,1);CHECK(st.action.phase,CTX_IDLE);CHECK(W->mime.active,0);CHECK(W->mime.pending,0);
}
static void reach_copy(void)
{
 u8 i;for(i=0;i<30&&!W->mime.active&&W->mime.pending;i++){
  if(st.act_t<ACT_HIT)impact();else end_anim();
 }
 CHECK(W->mime.active,1);CHECK(st.action.phase!=CTX_IDLE,1);
}
static void basic(void)
{
 u16 jp,hp,ct;u8 eq,oldact;u32 seed;
 mime_arena();add_mime(8,third,1);add_mime(9,first,0);st.unit[U_RAMZA].face=0;st.unit[U_RAMZA].eq[EQ_WEAPON]=19;
 jp=st.unit[8].jp;eq=st.unit[8].eq[EQ_WEAPON];ct=st.unit[8].ct;oldact=st.act;
 attack(U_RAMZA,10);CHECK(W->mime.pending,((u16)1<<8)|((u16)1<<9));CHECK(W->mime.face,0);
 st.hit=1;st.push=0;st.dmg=1;impact();reach_copy();CHECK(W->mime.who,8);CHECK(st.atk,8);CHECK(st.tgt,10);CHECK(st.abil,NO_ABIL);
 CHECK(st.unit[8].eq[EQ_WEAPON],19);CHECK(st.action.root,8);CHECK(st.act,oldact);CHECK(st.moved,0);CHECK(st.acted,1);CHECK(st.unit[8].ct,ct);
 hp=st.unit[10].hp;st.hit=1;st.push=0;st.dmg=13;impact();CHECK(st.unit[10].hp,hp-13);seed=rng32;impact();CHECK(st.unit[10].hp,hp-13);CHECK(rng32==seed,1);
 while(W->mime.who==8&&W->mime.active)end_anim();CHECK(W->mime.who,9);CHECK(st.atk,9);CHECK(st.tgt,10);CHECK(st.unit[8].eq[EQ_WEAPON],eq);CHECK(st.unit[8].set,24);CHECK(st.unit[8].jp>jp,1);
 drain();CHECK(st.act,oldact);CHECK(st.mode,M_MENU);CHECK(st.unit[9].eq[EQ_WEAPON],255);CHECK(st.unit[9].set,24);
}
static void eligibility(void)
{
 u8 sid;
 for(sid=0;sid<40;sid++){
  mime_arena();add_mime(8,third,1);status(8,sid);CHECK(reaction_live(8),source_mime_live[sid]);
 }

 u8 mode;u32 seed;
 for(mode=0;mode<8;mode++){
  mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].face=0;attack(U_RAMZA,10);CHECK(W->mime.pending,(u16)1<<8);
  if(mode==0)st.unit[8].team=TEAM_ENEMY;
  if(mode==1)status(8,35);
  if(mode==2)status(8,37);
  if(mode==3)st.unit[8].hp=0;
  if(mode==4){st.unit[8].x=255;st.unit[8].dc=DC_GONE;}
  if(mode==5)st.unit[8].job=74;
  if(mode==6)unit_place(&st.unit[10],first); // translated fixed panel is now empty
  if(mode==7)st.unit[8].face=0; // offset from third falls beyond the melee target
  st.action.phase=CTX_IDLE;seed=rng32;CHECK(next_mime(),0);CHECK(rng32==seed,1);CHECK(W->mime.active,0);CHECK(W->mime.pending,0);CHECK(st.unit[8].eq[EQ_WEAPON],255);
 }
 // Guest is an allied control policy, not a separate source camp.
 for(mode=0;mode<4;mode++){
  mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].face=0;
  st.unit[U_RAMZA].team=mode&1?TEAM_GUEST:TEAM_PLAYER;st.unit[8].team=mode&2?TEAM_GUEST:TEAM_PLAYER;
  attack(U_RAMZA,10);reach_copy();CHECK(st.atk,8);drain();
 }
 // A Mime root cannot become a second donor, and enemy Mimes copy enemy roots.
 mime_arena();add_mime(8,third,1);add_mime(9,first,0);attack(8,10);CHECK(W->mime.pending,0);drain();
 mime_arena();add_mime(8,third,1);st.unit[8].team=TEAM_ENEMY;st.unit[U_RAMZA].team=TEAM_ENEMY;st.unit[10].team=TEAM_PLAYER;st.unit[U_RAMZA].face=0;
 attack(U_RAMZA,10);reach_copy();CHECK(st.atk,8);drain();
}
static void resources_and_families(void)
{
 u16 ab,c;u8 mode;
 for(mode=0;mode<2;mode++){
  mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].face=0;st.unit[10].hp=100;st.unit[10].max_hp=999;
  if(!mode){ab=368;st.unit[U_RAMZA].set=6;st.bag[0]=1;}
  else{ab=634;st.unit[U_RAMZA].set=20;st.unit[U_RAMZA].learned=0xfff;battle_save.inv[122]=1;}
  skill(U_RAMZA,10,ab);CHECK(st.mode,M_ACT);CHECK(mode?battle_save.inv[122]:st.bag[0],0);
  impact();reach_copy();CHECK(st.atk,8);CHECK(st.abil,ab);CHECK(can_use(8,ab),1);CHECK(mode?battle_save.inv[122]:st.bag[0],0);
  impact();drain();CHECK(mode?battle_save.inv[122]:st.bag[0],0);CHECK(st.unit[8].set,24);
 }
 // Throw uses the Mime's Move range, independently of a borrowed melee weapon.
 mime_arena();
 for(c=0;c+3<MAP_N;c++)if(TILE_Z[c]==TILE_Z[c+3]&&map_tiles[c].walk==1&&map_tiles[c+1].walk==1&&map_tiles[c+2].walk==1&&map_tiles[c+3].walk==1&&map_tiles[c].stand==map_tiles[c+3].stand)break;
 CHECK(c+3<MAP_N,1);unit_place(&st.unit[U_RAMZA],c);unit_place(&st.unit[10],c+3);add_mime(8,c,0);st.unit[8].move=4;st.unit[U_RAMZA].face=0;
 st.unit[U_RAMZA].set=20;st.unit[U_RAMZA].learned=0xfff;battle_save.inv[122]=1;skill(U_RAMZA,10,634);
 reach_copy();CHECK(st.atk,8);CHECK(st.abil,634);CHECK(battle_save.inv[122],0);drain();
 // Charge and Jump arm once on the donor; Mime executes at the due impact.
 for(mode=0;mode<2;mode++){
  mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].face=0;st.unit[U_RAMZA].set=mode?18:2;st.unit[U_RAMZA].learned=0xfff;ab=mode?394:406;
  skill(U_RAMZA,10,ab);CHECK(W->mime.pending,0);CHECK(st.pending[U_RAMZA].state!=DEFER_NONE,1);
  st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(W->mime.pending,(u16)1<<8);CHECK(st.open_resolving,1);
  reach_copy();CHECK(st.atk,8);CHECK(st.abil,ab);CHECK(st.pending[8].state,DEFER_NONE);CHECK(unit_airborne(&st.unit[8]),0);CHECK(st.open_resolving,1);drain();
 }
 // Math preserves its original selected criterion, with the Mime's own MA.
 mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].set=21;st.unit[U_RAMZA].learned=0xff;st.unit[U_RAMZA].level=99;
 for(ab=1;ab<60;ab++)if(math_available(U_RAMZA,ab)&&abilities[ab].kind==AB_DAMAGE)break;
 CHECK(ab<60,1);CHECK(cast_order(U_RAMZA,7,ab,MATH_BIND),1);CHECK(W->mime.follow,MATH_BIND);CHECK(W->mime.ref,7);
 reach_copy();CHECK(st.atk,8);CHECK(W->cast.follow,MATH_BIND);CHECK(W->cast.ref,7);CHECK(st.unit[8].mp,100);CHECK(st.unit[8].set,21);drain();CHECK(st.unit[8].set,24);
 // A recurring song yields one immediate copied pulse, never a Mime timer.
 mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].set=22;st.unit[U_RAMZA].face=0;
 CHECK(cast_order(U_RAMZA,U_RAMZA,86,1),2);CHECK(W->mime.pending,0);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);
 reach_copy();CHECK(st.atk,8);CHECK(st.abil,86);CHECK(W->cast.phase,CAST_BATCH_READY);CHECK(st.pending[8].state,DEFER_NONE);drain();CHECK(st.pending[8].state,DEFER_NONE);
 // Elements chooses the Mime's terrain; no copied learned-slot requirement.
 mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].set=17;st.unit[U_RAMZA].face=0;
 ab=magic_heal(U_RAMZA,U_RAMZA,126,4);CHECK(ab>=126&&ab<138,1);skill(U_RAMZA,10,ab);
 reach_copy();CHECK(st.atk,8);CHECK(st.abil,magic_heal(8,8,ab,4));drain();
 // A summon is mimicable: its AoE flag is distinct from CANNOT_MIMIC.
 mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].set=11;st.unit[U_RAMZA].face=0;st.open_resolving=1;
 skill(U_RAMZA,10,65);reach_copy();CHECK(st.atk,8);CHECK(st.abil,65);drain();
 // Draw Out needs no second katana; forbidden No-Mimic skills are skipped.
 mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].set=14;st.unit[U_RAMZA].face=0;battle_save.inv[38]=1;st.open_resolving=1;
 skill(U_RAMZA,10,76);reach_copy();CHECK(st.atk,8);CHECK(st.abil,76);CHECK(st.unit[8].mp,100);drain();
 mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].set=11;st.unit[U_RAMZA].face=0;st.open_resolving=1;
 skill(U_RAMZA,10,153);CHECK(W->mime.pending,(u16)1<<8);st.action.phase=CTX_IDLE;CHECK(next_mime(),0);CHECK(W->mime.active,0);
}
static void responses(void)
{
 mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].face=0;st.reaction[10]=REACTION_COUNTER;
 attack(U_RAMZA,10);st.hit=1;st.push=0;st.dmg=1;impact();end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(W->mime.active,0);CHECK(st.atk,10);
 impact();reach_copy();CHECK(st.atk,8);CHECK(st.action.root,8);st.hit=1;st.push=0;st.dmg=1;impact();end_anim();CHECK(st.atk,10);CHECK(st.tgt,8);CHECK(st.action.phase,CTX_SECONDARY);CHECK(W->mime.pending,0);drain();
 // Hamedo preempts the donor before recording; no copy of a cancelled action.
 mime_arena();add_mime(8,third,1);st.reaction[10]=453;st.unit[10].eq[EQ_WEAPON]=19;attack(U_RAMZA,10);CHECK(W->mime.pending,0);CHECK(st.action.cursor,128);drain();
 // Stale borrowed state is discarded by retry/scenario reinitialization.
 mime_arena();add_mime(8,third,1);st.unit[U_RAMZA].face=0;attack(U_RAMZA,10);reach_copy();game_scenario(61);CHECK(W->mime.active,0);CHECK(W->mime.pending,0);
}
static void mime_trials(void){basic();eligibility();resources_and_families();responses();}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);mime_trials();printf("mime %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();mime_trials();BENCH_VALUE(sizeof(Unit));BENCH_VALUE(sizeof(State));BENCH_VALUE(sizeof(Work));BENCH_VALUE(sizeof(Save));BENCH_VALUE(sizeof(ModCtx));BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
