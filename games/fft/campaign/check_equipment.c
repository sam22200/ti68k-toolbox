// Block B source fixtures and actual battle owners, without rendering.
#define FFT_PRODUCTION_CORE
#define FFT_WEAPON_TEST_API // this diagnostic explicitly calls attack_damage
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main equipment_runtime_main
#define game_render no_equipment_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void no_equipment_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
#include "../reward_value.h"
static u32 checks,failures,digest=17;
static void equal(u16 a,u16 b,u16 line){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b){failures++;
#ifndef __m68k__
if(failures<30)printf("line%u: %u expected%u\n",line,a,b);
#else
if(failures<10)BENCH_VALUE(line);
#endif
}}
#define CHECK(a,b) equal((u16)(a),(u16)(b),__LINE__)
static void arena(void)
{
 u8 i;Unit *u;game_scenario(61);st.ai=AI_OFF;st.banner=st.morph_mask=0;st.act=st.sel=U_RAMZA;st.mode=M_MENU;
 st.moved=st.acted=st.pick_t=0;W->rebuild=0;
 for(i=0;i<NUNIT;i++){u=&st.unit[i];st.support[i]=st.reaction[i]=st.movement[i]=0;
  memset(&u->statuses,0,sizeof u->statuses);u->status_flags=0;u->zodiac=0;u->level=1;u->female=0;u->job=74;
  u->hp=u->max_hp=999;u->mp=u->max_mp=100;u->dc=3;u->pa=5;u->sp=9;u->brave=100;u->team=TEAM_PLAYER;
  memset(u->eq,255,5);}
 memset(battle_save.inv,0,256);qput(battle_save.money,1000);
}
static void earn(u8 e);
static void source_rules(void)
{
 const u8 *cats,*rows,*p;u16 bytes,i,type,j;Unit *a=&st.unit[U_RAMZA],*t=&st.unit[10];u16 support;
 cats=rt_file("ffeq0",&bytes);CHECK(cats!=RT_NULL,1);CHECK(bytes>=23040,1);
 for(i=0;i<2880;i++){
  p=cats+(u16)(i<<3);
  for(type=0;type<32;type++)CHECK(core_equipment_category(common,p[0],p[1],cj_word(p+2),(u8)type)!=0,(p[4+(type>>3)]&(128>>(type&7)))!=0);
 }
 rows=rt_file("ffeq1",&bytes);CHECK(rows!=RT_NULL,1);CHECK(bytes>=10256,1);arena();
 for(i=0;i<384;i++){
  u16 v[8];p=rows+(i<<4);for(j=0;j<8;j++)v[j]=cj_word(p+2*j);
  if(v[3]&&v[4])continue; // equipment has one support slot; do not synthesize impossible pairs
  a->eq[3]=(u8)v[0];a->pa=v[1];a->brave=60;support=v[3]?476:v[4]==1?465:v[4]==2?472:0;st.support[U_RAMZA]=support;
  st.support[10]=v[5]?466:0;t->statuses.inflicted[3]=v[6]?32:0;
  CHECK(attack_weapon_damage(U_RAMZA,10,(u8)v[0],0,0,0,(u8)v[2]),v[7]);
 }
 for(i=0;i<108;i++){
  u16 v[6];p=rows+6144+i*12;for(j=0;j<6;j++)v[j]=cj_word(p+2*j);
  a->level=v[0];t->level=v[1];t->kos=v[3];st.support[U_RAMZA]=v[4]?464:0;
  CHECK(earned_exp(U_RAMZA,10,(u8)v[2]),v[5]);
 }
 for(i=0;i<128;i++){
  u16 v[11],hp,mp,exp,jp; p=rows+7440+i*22;for(j=0;j<11;j++)v[j]=cj_word(p+2*j);
  arena();a=&st.unit[U_RAMZA];a->hp=a->mp=1;a->max_hp=v[1];a->max_mp=v[2];a->exp=0;a->jp=0;
  st.path_n=v[3]+1;st.movement[U_RAMZA]=v[0]-485;st.support[U_RAMZA]=v[4]?(v[0]==495?464:463):0;
  if(v[5]!=255)a->statuses.inflicted[v[5]>>3]=128>>(v[5]&7);
  hp=a->hp;mp=a->mp;exp=a->exp;jp=a->jp;movement_settle(U_RAMZA);
  CHECK(a->hp,v[6]==65535?hp:hp+v[7]>a->max_hp?a->max_hp:hp+v[7]);
  CHECK(a->mp,v[6]==65535?mp:mp+v[8]>a->max_mp?a->max_mp:mp+v[8]);
  CHECK(a->exp,exp+v[9]);CHECK(a->jp,jp+v[10]);
 }
 for(i=0;i<36;i++){
  static const u16 thresholds[8]={0,100,200,350,550,800,1150,1550};u16 v[6];
  p=rows+10304+i*12;for(j=0;j<6;j++)v[j]=cj_word(p+2*j);
  if(!v[3])continue; // no producer invokes the reward owner without earned EXP
  arena();a=&st.unit[U_RAMZA];a->level=v[0];a->tjp=thresholds[v[1]];st.support[U_RAMZA]=v[2]?463:0;
  st.unit[10].jp=0;earn((u8)v[3]);CHECK(st.jpg,v[4]);CHECK(st.unit[10].jp,v[5]);CHECK(W->rebuild,0);
 }
 // A shield blocks Two Hands; queries leave RNG and inventory untouched.
 arena();a=&st.unit[U_RAMZA];a->eq[3]=19;st.support[U_RAMZA]=476;
 {u16 both=attack_damage(U_RAMZA,10,0,0),jump=magic_heal(U_RAMZA,10,394,0);u32 seed=rng32;
  a->eq[4]=128;CHECK(attack_damage(U_RAMZA,10,0,0)*2,both);CHECK(magic_heal(U_RAMZA,10,394,0),jump);CHECK(rng32==seed,1);}

}
static u8 commit(const u8 *eq)
{
#ifdef FFT_MODULES
 memcpy(W->ai_tt,eq,5);return (u8)mod_call(MOD_EQUIPMENT,5,st.act);
#else
 return equipment_commit(st.act,eq);
#endif
}
static void earn(u8 e)
{
#ifdef FFT_MODULES
 st.primary.reward=e;st.action.root=U_RAMZA;mod_call(MOD_RESPONSE,3,0);
#else
 award_rewards(U_RAMZA,e);
#endif
}
static void transactions(void)
{
 Unit *u;u8 eq[5],old[5];u16 hp,mp;u32 seed;State snapshot;u8 stock[256];
 arena();u=&st.unit[U_RAMZA];u->job=76;st.support[U_RAMZA]=480;memset(eq,255,5);eq[3]=19;
 battle_save.inv[19]=1;seed=rng32;CHECK(commit(eq),1);CHECK(u->eq[3],19);CHECK(battle_save.inv[19],0);CHECK(st.acted,1);CHECK(rng32==seed,1);
 CHECK(commit(eq),0);CHECK(battle_save.inv[19],0);
 arena();u=&st.unit[U_RAMZA];u->job=76;st.support[U_RAMZA]=480;u->eq[3]=19;u->hp=3;u->mp=2;
 memcpy(eq,u->eq,5);eq[3]=255;eq[4]=19;hp=u->hp;mp=u->mp;CHECK(commit(eq),1);CHECK(u->eq[4],19);CHECK(battle_save.inv[19],0);CHECK(u->hp,hp);CHECK(u->mp,mp);
 arena();u=&st.unit[U_RAMZA];u->job=76;st.support[U_RAMZA]=480;u->eq[3]=19;battle_save.inv[19]=99;
 memcpy(eq,u->eq,5);eq[3]=255;snapshot=st;memcpy(stock,battle_save.inv,256);CHECK(commit(eq),0);CHECK(memcmp(&snapshot,&st,sizeof st)==0,1);CHECK(memcmp(stock,battle_save.inv,256)==0,1);
 arena();st.support[U_RAMZA]=480;memset(eq,255,5);eq[3]=19;snapshot=st;CHECK(commit(eq),0);CHECK(memcmp(&snapshot,&st,sizeof st)==0,1);
 arena();st.support[U_RAMZA]=480;st.unit[U_RAMZA].statuses.inflicted[4]=4;unit_status_refresh(&st.unit[U_RAMZA]);battle_save.inv[19]=1;CHECK(commit(eq),0);CHECK(battle_save.inv[19],1);
 // The actor boost never changes spillover, including a level-up rebuild.
 arena();u=&st.unit[U_RAMZA];u->exp=95;u->jp=u->tjp=0;st.unit[10].jp=0;st.support[U_RAMZA]=463;hp=u->hp;mp=u->mp;earn(10);
 CHECK(st.jpg,12);CHECK(st.unit[10].jp,2);CHECK(u->level,2);CHECK(u->hp<=hp,1);CHECK(u->mp<=mp,1);CHECK(W->rebuild,0);
 arena();u=&st.unit[U_RAMZA];u->jp=u->tjp=9998;st.support[U_RAMZA]=463;earn(10);CHECK(u->jp,9999);CHECK(u->tjp,9999);
 // Actual walk completion settles once and disables undo, rather than farming movement.
 arena();u=&st.unit[U_RAMZA];st.movement[U_RAMZA]=493-485;u->hp=100;u->max_hp=101;st.path_n=2;
 st.path[0]=UNIT_CELL(u);st.path[1]=st.path[0];st.path_i=0;st.walk_t=0;st.mode=M_WALK;
 while(st.mode==M_WALK)game_update();CHECK(u->hp,111>101?101:111);CHECK(st.mode,M_PICK);
 {u8 i;for(i=0;i<PICK_FRAMES;i++)game_update();}CHECK(st.mode,M_MENU);hp=u->hp;
 rt_prev=0;rt_keys=K_B;game_update();rt_keys=0;CHECK(st.moved,1);CHECK(u->hp,hp);
 // The native hand layout gates right-hand shields; an actual offhand weapon
 // still supplies its reach, and a real item254 is never a resource message.
 arena();u=&st.unit[U_RAMZA];st.support[U_RAMZA]=480;u->job=76;memset(eq,255,5);eq[3]=128;
 battle_save.inv[128]=1;CHECK(commit(eq),0);CHECK(battle_save.inv[128],1);
 for(hp=1;hp<128;hp++)if(items[hp].knife==2&&items[hp].wp)break;
 CHECK(hp<128,1);u->eq[3]=255;u->eq[4]=(u8)hp;CHECK(WEAPON_REACH(u),items[hp].reach);
 st.movement[U_RAMZA]=493-485;st.path_n=2;movement_settle(U_RAMZA);
 memset(st.unit[10].eq,254,5);st.unit[10].dc=DC_CHEST;pick_up(U_RAMZA,10);
 CHECK(st.pick,254);CHECK(st.abil,NO_ABIL);CHECK(st.nfound,1);

}
static void gilgame(void)
{
 Unit *u;u16 jp;u32 funds;State snapshot;
 arena();u=&st.unit[10];st.reaction[10]=REACTION_GILGAME;u->brave=100;u->team=TEAM_PLAYER;jp=u->jp;
 act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=1;st.dmg=17;st.push=st.self=0;
#ifdef FFT_MODULES
 mod_call(MOD_ACTION,0,0);
#else
 action_commit();
#endif
 CHECK(st.action.pending,(u16)1<<10);CHECK(st.action.reaction[10],0x4011);
 {u8 kind;
#ifdef FFT_MODULES
 kind=(u8)mod_call(MOD_RESULTS,3,0);mod_call(MOD_RESPONSE,1,kind);
#else
 kind=reaction_select();reaction_launch(kind);
#endif
 CHECK(kind,5);CHECK(st.dmg,17);CHECK(st.abil,439);}
#ifdef FFT_MODULES
 mod_call(MOD_ACTION,0,0);
#else
 action_commit();
#endif
 CHECK(qlong(battle_save.money),1017);CHECK(u->jp,jp);snapshot=st;funds=qlong(battle_save.money);
#ifdef FFT_MODULES
 mod_call(MOD_ACTION,0,0);
#else
 action_commit();
#endif
 CHECK(qlong(battle_save.money),funds);CHECK(memcmp(&snapshot,&st,sizeof st)==0,1);
 {const u8 *rows;u16 bytes,i,j;rows=rt_file("ffeq1",&bytes);
  for(i=0;i<6;i++){
   u16 v[4];u8 kind;for(j=0;j<4;j++)v[j]=cj_word(rows+10256+i*8+2*j);
   arena();u=&st.unit[10];st.reaction[10]=439;act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=0;
#ifdef FFT_MODULES
   mod_call(MOD_ACTION,0,0);
#else
   action_commit();
#endif
   CHECK(core_action_queue(&st.action,10,439,NUNIT),CTX_OK);st.action.reaction[10]=0x4000|v[0];
   if(v[1]!=255)u->statuses.inflicted[v[1]>>3]=128>>(v[1]&7);
#ifdef FFT_MODULES
   kind=(u8)mod_call(MOD_RESULTS,3,0);mod_call(MOD_RESPONSE,1,kind);
#else
   kind=reaction_select();reaction_launch(kind);
#endif
   CHECK(kind,5);CHECK(st.hit,v[2]);CHECK(st.dmg,v[3]);
#ifdef FFT_MODULES
   mod_call(MOD_ACTION,0,0);
#else
   action_commit();
#endif
   CHECK(qlong(battle_save.money),1000+v[3]);
  }
 }

}
static void trials(void){source_rules();transactions();gilgame();}
#ifndef EQUIPMENT_UI_CHECK
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("equipment %08lX failures %lu checks %lu\n",(unsigned long)digest,(unsigned long)failures,(unsigned long)checks);return failures!=0;}
#else
void _main(void){equipment_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif

#endif
