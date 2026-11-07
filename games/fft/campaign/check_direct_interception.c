// Retail routing controls and actual direct-shot preparation/settlement.
#define PANEL_RENDER_CHECK
#include "check_panel_victims.c"
#include "x/core-direct-interception/oracle/source.h"
static void direct_arena(void)
{
 u16 c,i;panel_arena(0);
 for(c=0;c+4<MAP_N;c++)if(TILE_Z[c]==TILE_Z[c+4]){
  for(i=0;i<5;i++)if(!map_tiles[c+i].walk)break;if(i==5)break;
 }
 CHECK(c+4<MAP_N,1);first=c;second=c+2;third=c+4;
 unit_place(&st.unit[U_RAMZA],first);unit_place(&st.unit[10],third);
 st.unit[U_RAMZA].eq[EQ_WEAPON]=71;W->surface_flags|=SURFACE_HIT_UPPER;
}
static u16 direct_column(u8 cell){return (u16)(W->grid_row[TILE_Z[cell]]+TILE_X[cell])*8;}
static u8 direct_preview(u8 a,u8 t)
{
 char text[28];u32 seed=rng32;State before=st;u8 n=panel_preview(a,t,NO_ABIL,text);
 CHECK(text[n],0);CHECK(rng32==seed,1);CHECK(memcmp(&before,&st,sizeof st)==0,1);
 return W->preview_target;
}
static void direct_source(void)
{
 u16 size,i;const u8 *data=rt_file("fdir",&size),*r;u8 old_count,j;
 CHECK(data!=RT_NULL,1);CHECK(size>=DIRECT_SOURCE_CASES*4,1);
 for(i=0;i<DIRECT_SOURCE_CASES;i++){
  r=data+i*4;direct_arena();old_count=encounter_units;encounter_units=13;
  for(j=0;j<NUNIT;j++){st.unit[j]=st.unit[10];st.unit[j].x=255;st.unit[j].dc=DC_GONE;}
  st.unit[1].dc=st.unit[r[0]].dc=3;unit_place(&st.unit[1],first);unit_place(&st.unit[r[0]],third);
  st.unit[1].eq[EQ_WEAPON]=71;
  if(!r[1]){
   if(r[2]==255)panel_surface[direct_column(second)]=20;
   else if(r[2]!=r[0]){st.unit[r[2]].dc=3;unit_place(&st.unit[r[2]],second);}
  }
#ifdef FFT_MODULES
  mod_id=0;
#endif
  CHECK(direct_preview(1,r[0]),r[3]);CHECK(battle_line_clear(1,r[0]),r[3]!=255);
  encounter_units=old_count;
#ifdef FFT_MODULES
  mod_id=0;
#endif
  panel_restore();
 }
}
static void direct_geometry(void)
{
 u8 mode;u16 hp,at;
 // Nearest position wins before unit-slot order. All occupants of that panel
 // are subsequently frozen in slot order by the existing shared owner.
 direct_arena();st.unit[0]=st.unit[9]=st.unit[10];unit_place(&st.unit[0],second+1);unit_place(&st.unit[9],first+1);
 CHECK(direct_preview(U_RAMZA,10),9);hp=st.unit[10].hp;rng32=1;
 attack(U_RAMZA,10);CHECK(st.tgt,9);CHECK(st.action.cell,third);impact();CHECK(st.unit[9].hp<999,1);CHECK(st.unit[0].hp,999);CHECK(st.unit[10].hp,hp);panel_finish();panel_restore();
 // Terrain and bodies compete in travel order; a wall beyond the intercepted
 // unit cannot erase its hit. Either terrain layer can stop an earlier sample.
 for(mode=0;mode<3;mode++){
  direct_arena();st.unit[9]=st.unit[10];unit_place(&st.unit[9],mode==0?second:first+1);
  at=direct_column(mode==0?first+1:second);panel_surface[at+(mode==2?4:0)]=20;
  if(mode==2)panel_surface[at+7]=20;
  CHECK(direct_preview(U_RAMZA,10),mode==0?255:9);panel_restore();
 }
 // Frog/Chicken boxes are shorter. A flat normal shot passes exactly above
 // their top, while lowering its endpoints makes the same body intercept.
 for(mode=0;mode<2;mode++){
  direct_arena();st.unit[9]=st.unit[10];unit_place(&st.unit[9],second);status(9,mode?21:22);
  CHECK(direct_preview(U_RAMZA,10),10);W->hs[first]=W->hs[third]=7;
  CHECK(direct_preview(U_RAMZA,10),9);panel_restore();
 }
 // KO boxes can intercept a low shot, but crystal/chest/gone and airborne
 // actors are not projectile bodies. KO then rejects ordinary damage.
 direct_arena();st.unit[9]=st.unit[10];st.unit[9].hp=0;unit_place(&st.unit[9],second);W->hs[first]=W->hs[third]=6;
 CHECK(direct_preview(U_RAMZA,10),10);W->hs[first]=W->hs[third]=5;CHECK(direct_preview(U_RAMZA,10),9);
 rng32=1;attack(U_RAMZA,10);CHECK(st.tgt,9);impact();CHECK(st.hit,0);CHECK(st.unit[10].hp,999);panel_finish();panel_restore();
 for(mode=0;mode<4;mode++){
  direct_arena();st.unit[9]=st.unit[10];unit_place(&st.unit[9],second);
  if(mode==0)st.unit[9].dc=DC_GONE;else if(mode==1)st.unit[9].dc=DC_CRYSTAL;else if(mode==2)st.unit[9].dc=DC_CHEST;
  else{st.pending[9].state=DEFER_AIRBORNE;unit_status_action(&st.unit[9],4);}
  CHECK(direct_preview(U_RAMZA,10),10);panel_restore();
 }
}
static void direct_sequence(void)
{
 u8 mode;u16 hp,stock;u32 seed;
 for(mode=0;mode<2;mode++){
  direct_arena();st.unit[8]=st.unit[9]=st.unit[10];unit_place(&st.unit[8],second);unit_place(&st.unit[9],second);
  if(mode)st.unit[8].team=st.unit[9].team=TEAM_PLAYER;
  st.reaction[8]=st.reaction[9]=mode?REACTION_POTION:0;st.bag[0]=3;st.bag[1]=st.bag[2]=0;
  CHECK(direct_preview(U_RAMZA,10),8);CHECK(W->preview_count,2);rng32=1;hp=st.unit[10].hp;
  attack(U_RAMZA,10);CHECK(st.tgt,8);CHECK(st.hand,0);impact();seed=rng32;end_anim();
  CHECK(st.tgt,9);CHECK(st.hand,0);CHECK(rng32==seed,1);impact();CHECK(st.bag[0],3);
  CHECK(st.unit[10].hp,hp);stock=st.bag[0];panel_finish();CHECK(st.bag[0],stock-(mode?2:0));panel_restore();
 }
 // An interceptor enters after Charge was accepted. Its due strike resolves
 // the live recipient while retaining the originally selected panel.
 direct_arena();st.unit[U_RAMZA].set=8;st.unit[U_RAMZA].learned=1;
 CHECK(can_use(U_RAMZA,0x196),1);skill(U_RAMZA,10,0x196);CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);
 st.unit[9]=st.unit[10];unit_place(&st.unit[9],second);st.pending[U_RAMZA].ticks=0;rng32=1;charge_resolve();
 CHECK(st.tgt,9);CHECK(st.abil,0x196);CHECK(st.action.cell,third);impact();CHECK(st.unit[9].hp<999,1);CHECK(st.unit[10].hp,999);panel_finish();panel_restore();
}
static void direct_returns(void)
{
 u16 hp;
 // The actual victim offers Counter. The intended victim offers nothing.
 direct_arena();st.unit[9]=st.unit[10];unit_place(&st.unit[9],first+3);st.unit[9].eq[EQ_WEAPON]=71;
 st.reaction[9]=REACTION_COUNTER;hp=st.unit[U_RAMZA].hp;rng32=1;
 attack(U_RAMZA,10);CHECK(st.tgt,9);impact();CHECK(st.action.pending,(u16)1<<9);end_anim();
 CHECK(st.atk,9);CHECK(st.tgt,U_RAMZA);CHECK(st.action.phase,CTX_SECONDARY);impact();CHECK(st.unit[U_RAMZA].hp<hp,1);
 panel_finish();CHECK(st.unit[10].hp,999);panel_restore();
 // Hamedo preflight accepts a return which hits a third unit. It still
 // cancels the unpaid primary and cannot trigger a recursive return.
 direct_arena();st.unit[9]=st.unit[10];unit_place(&st.unit[9],second);st.unit[10].eq[EQ_WEAPON]=71;
 st.reaction[10]=REACTION_HAMEDO;st.reaction[9]=REACTION_COUNTER;rng32=1;hp=st.unit[U_RAMZA].hp;
 attack(U_RAMZA,10);CHECK(st.atk,10);CHECK(st.tgt,9);CHECK(st.action.phase,CTX_SECONDARY);
 impact();CHECK(st.unit[9].hp<999,1);CHECK(st.action.pending,0);panel_finish();
 CHECK(st.unit[10].hp,999);CHECK(st.unit[U_RAMZA].hp,hp);panel_restore();
}
static void direct_ai(void)
{
 u16 n;u32 seed;u8 home;
 direct_arena();st.unit[9]=st.unit[10];st.unit[9].team=TEAM_PLAYER;unit_place(&st.unit[9],second);
 home=UNIT_CELL(&st.unit[U_RAMZA]);memset(W->ai_m,0,sizeof W->ai_m);W->ai_m[home]=1;
 W->ai_weapon_left=512;W->ai_melee_at=0;st.act=U_RAMZA;seed=rng32;
 for(n=0;n<128&&W->ai_weapon_left;n++){
  ZB(61);
#ifdef FFT_MODULES
  mod_call(MOD_PLANNER,0,0);
#else
  melee_cache();
#endif
  ZE(61);
 }
 CHECK(W->ai_weapon_left,0);CHECK(W->ai_melee_mask[home]&((u16)1<<10),0);
 CHECK(UNIT_CELL(&st.unit[U_RAMZA]),home);CHECK(rng32==seed,1);panel_restore();
}
static void direct_trials(void){direct_source();direct_geometry();direct_sequence();direct_returns();direct_ai();}
#ifndef DIRECT_RENDER_CHECK
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);direct_trials();printf("direct_interception %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();direct_trials();BENCH_VALUE(sizeof(Unit));BENCH_VALUE(sizeof(State));BENCH_VALUE(sizeof(Work));BENCH_VALUE(sizeof(Save));BENCH_VALUE(sizeof(ModCtx));BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
