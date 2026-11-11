#define FFT_WEAPON_TEST_API // direct test adapters; absent from size-bound renderer carriers
// Source weapon packets plus root ownership, costs, statuses and secondary reuse.
#define COMBAT_UI_CHECK
#include "check_combat.c"
#include "x/core-weapons/oracle/source.h"
static AbId power_ability(u8 power)
{
 AbId ab;if(!power)return NO_ABIL;
 for(ab=0;ab<512;ab++)if((abilities[ab].flags&AF_CHARGE)&&abilities[ab].y==power)return ab;
 CHECK(0,1);return NO_ABIL;
}
static void fx_prepare(void)
{
#ifdef FFT_MODULES
 mod_call(MOD_PLANNER,1,0);
#else
 weapon_prepare(st.atk,st.tgt);
#endif
}
static u16 fx_forecast(u8 a,u8 t,AbId ab)
{
#ifdef FFT_MODULES
 ModCtx *m;mod_call(MOD_WEAPON,255,0);m=mod_context(MOD_WEAPON);m->cast_actor=a;m->cast_ref=t;m->cast_ability=ab;return mod_call(MOD_WEAPON,5,0);
#else
 return weapon_forecast(a,t,ab,0,0);
#endif
}
static void source_effects(void)
{
 u16 bytes,i,j,v[WEAPON_EFFECT_FIELDS];const u8 *rows=rt_file("fwfx",&bytes);Unit *a,*t;u32 seed,after;AbId ab;
 CHECK(rows!=RT_NULL,1);// The TI reader reports the OTH payload including its six-byte dat trailer.
#ifdef __m68k__
 bytes-=6;
#endif
 CHECK(bytes,WEAPON_EFFECT_CASES*WEAPON_EFFECT_FIELDS*2U);
 for(i=0;i<WEAPON_EFFECT_CASES;i++){
  for(j=0;j<WEAPON_EFFECT_FIELDS;j++)v[j]=cj_word(rows+(u16)(i*WEAPON_EFFECT_FIELDS+j)*2U);
  combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->eq[EQ_WEAPON]=v[0]?v[0]:255;a->eq[EQ_SHIELD]=255;
  a->pa=v[1];a->ma=v[2];a->sp=v[3];a->brave=v[4];a->faith=v[5];t->faith=v[6];ab=power_ability(v[7]);
  st.support[U_RAMZA]=v[8];st.support[10]=v[9];status(U_RAMZA,v[10]);status(10,v[11]);status(10,35);
  a->zodiac=v[12];t->zodiac=v[13];a->female=v[14];t->female=0;st.weather=v[15];
  memset(st.affinity,0,sizeof st.affinity);for(j=0;j<4;j++)st.affinity[10][j]=v[16+j];st.affinity[U_RAMZA][4]=v[20];
  act_start(U_RAMZA,10);st.abil=ab;rng32=v[21];seed=rng32;after=((u32)v[28]<<16)|v[29];
  if(v[22]){State old=st;ZB(56);CHECK(fx_forecast(U_RAMZA,10,ab),v[23]);ZE(56);CHECK(rng32==seed,1);CHECK(memcmp(&old,&st,sizeof st)==0,1);}
  else{
   ZB(57);fx_prepare();ZE(57);
#ifndef __m68k__
   if(st.dmg!=v[23]||(st.effect_kind&EFFECT_KIND_MASK)!=v[24]||st.self!=v[26]||rng32!=after)printf("profile%u weapon%u power%u seed%u: dmg%u kind%u self%u RNG%08lX expected %u/%u/%u/%08lX\n",i,v[0],v[7],v[21],st.dmg,st.effect_kind&EFFECT_KIND_MASK,st.self,(unsigned long)rng32,v[23],v[24],v[26],(unsigned long)after);
#endif
   CHECK(st.dmg,v[23]);CHECK(st.effect_kind&EFFECT_KIND_MASK,v[24]);CHECK(st.crit,v[25]);CHECK(st.self,v[26]);CHECK(rng32==after,1);
  }
 }
}
static void fx_root(u8 weapon,u32 seed)
{
 st.unit[U_RAMZA].eq[EQ_WEAPON]=weapon;status(10,35);rng32=seed;attack(U_RAMZA,10);
}
static u32 fx_proc_seed(u8 weapon)
{
 u32 seed;for(seed=1;seed<2048;seed++){
  combat_arena();fx_root(weapon,seed);if(st.self&255)return seed;
 }
 CHECK(0,1);return 1;
}
static void effects_lifecycle(void)
{
 Unit *a,*t;u16 hp,amount,jp,mp;u8 i,sid;u32 seed;
 // Healing weapons bypass four evades and critical RNG; Undead reverses them.
 for(i=0;i<2;i++){
  combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];t->hp=500;if(i)status(10,SID_UNDEAD);
  fx_root(23,17);CHECK(st.hit,1);CHECK(st.crit,0);CHECK(st.effect_kind,i?AB_DAMAGE:AB_HEAL);hp=t->hp;amount=st.dmg;seed=rng32;
  impact();CHECK(t->hp,i?hp-amount:hp+amount);CHECK(rng32==seed,1);hp=t->hp;impact();CHECK(t->hp,hp);complete();
 }
 // Drain mirrors the prepared scalar on its actor, reverses on Undead and keeps
 // one root reward, including overkill and actor KO.
 for(i=0;i<3;i++){
  combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->hp=i==2?1:300;t->hp=i==0?1:500;if(i)status(10,SID_UNDEAD);
  fx_root(61,17);amount=st.dmg;hp=a->hp;mp=a->mp;jp=a->jp;CHECK(st.self&16384,16384);impact();
  CHECK(a->hp,i?amount>=hp?0:hp-amount:hp+amount);CHECK(t->hp,i?500+amount:0);CHECK(a->mp,mp);hp=a->hp;impact();CHECK(a->hp,hp);complete();CHECK(a->jp>=jp,1);
 }
 // A physical null still consumes base/critical, but never19% or a proc.
 combat_arena();st.affinity[10][1]=64;fx_root(24,1);CHECK(st.hit,0);CHECK(st.dmg,0);CHECK(st.self,0);hp=st.unit[10].hp;impact();CHECK(st.unit[10].hp,hp);complete();
 // Successful status packet commits through immunity/cancellation/timer rules.
 for(i=0;i<3;i++){
  combat_arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=55;status(10,35);
  if(i==1)st.unit[10].statuses.immune[3]=128;
  for(seed=1;seed<2048;seed++){rng32=seed;attack(U_RAMZA,10);if(st.self||i==1)break;st.action.phase=CTX_IDLE;}
  CHECK(seed<2048,1);sid=(st.self>>8)-1;if(i==2)st.unit[10].statuses.immune[3]=128;
  amount=st.dmg;impact();CHECK(st.unit[10].hp,999-amount);CHECK(core_status_current(&st.unit[10].statuses,3)&128,i?0:128);if(!i)CHECK(st.unit[10].status_ct[SID_POISON-SID_POISON]>0,1);complete();
 }
 // Magical Fire gun removes mutable Oil at impact, with Faith and weather.
 combat_arena();status(10,16);st.weather=3;fx_root(75,17);CHECK(st.effect_kind&EFFECT_OIL,EFFECT_OIL);CHECK(core_status_current(&st.unit[10].statuses,2)&128,128);impact();CHECK(core_status_current(&st.unit[10].statuses,2)&128,0);complete();
 // A weapon spell costs no MP/stock, stages no CT, and awards once. Its target
 // is forced single: a neighboring actor is unaffected. Repeated impact is inert.
 seed=fx_proc_seed(53);
 for(i=0;i<4;i++){
  combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->hp=500;mp=a->mp;jp=a->jp;
  if(i==1){st.reaction[10]=REACTION_MP_SWITCH;t->mp=100;}
  if(i==2)t->hp=1;
  if(i==3){st.golem[t->team&1]=999;}
  fx_root(53,seed);CHECK(st.self&255,16);hp=t->hp;amount=st.dmg;impact();
  CHECK(a->mp,mp);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);if(i<2)CHECK(a->jp,jp);
  if(i==0)CHECK(t->hp<hp-amount,1);
  if(i==1)CHECK(t->hp,hp-amount);
  if(i==2)CHECK(t->hp,0);
  if(i==3){CHECK(t->hp,hp);CHECK(st.effect_kind,AB_GOLEM);}
  hp=t->hp;mp=t->mp;seed=rng32;impact();CHECK(t->hp,hp);CHECK(t->mp,mp);CHECK(rng32==seed,1);complete();CHECK(a->mp,100);
  seed=fx_proc_seed(53);
 }
}
static void fx_duals(void)
{
 u8 i,k;u16 hp;u32 seed;Unit *a,*t;
 for(i=0;i<4;i++){
  combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];st.support[U_RAMZA]=SUPPORT_TWO_SWORDS;
  a->eq[EQ_WEAPON]=i==0?23:i==1?24:i==2?53:61;a->eq[EQ_SHIELD]=i==0?19:i==1?19:i==2?54:23;a->hp=500;t->hp=500;status(10,35);
  if(i==1)st.affinity[10][1]=64;
  CHECK(hand_count(U_RAMZA),2);seed=rng32;CHECK(attack_prediction(U_RAMZA,10)!=0,1);CHECK(rng32==seed,1);
  rng32=17;attack(U_RAMZA,10);hp=t->hp;for(k=0;k<2;k++){CHECK(st.hand,k);impact();end_anim();}complete();CHECK(t->hp!=hp,1);
 }
}
static void fx_cancellation(void)
{
 u8 innate;u16 hp;
 for(innate=0;innate<2;innate++){
  combat_arena();status(10,SID_POISON);status(10,SID_SILENCE);
  if(innate)st.unit[10].statuses.innate[3]=128;
  fx_root(113,1);CHECK(st.self,32768U|(26U<<8));hp=st.unit[10].hp;impact();
  CHECK(st.unit[10].hp<hp,1);CHECK(core_status_current(&st.unit[10].statuses,1)&8,0);
  CHECK(core_status_current(&st.unit[10].statuses,3)&128,innate?128:0);complete();
 }
 combat_arena();status(10,SID_SENTENCE);fx_root(60,1);CHECK(st.self,32768U|(28U<<8));impact();CHECK(core_status_current(&st.unit[10].statuses,4)&1,0);complete();
}
static void fx_secondary(void)
{
 u8 i,k;u16 hp,mp,jp;Unit *a,*t;
 for(i=0;i<2;i++){
  combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->hp=t->hp=700;
  t->eq[EQ_WEAPON]=53;st.reaction[10]=i?REACTION_HAMEDO:REACTION_COUNTER;mp=t->mp;jp=t->jp;
  attack(U_RAMZA,10);
  if(!i){st.hit=0;impact();end_anim();}
  CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.abil,NO_ABIL);
  // The separately source-checked prepared proc survives a real secondary owner.
  st.hit=1;st.self=16;st.dmg=1;st.push=0;hp=a->hp;impact();CHECK(a->hp<hp-1,1);CHECK(t->mp,mp);CHECK(t->jp,jp);CHECK(st.action.pending,0);complete();
 }
 // A Mime borrows the donor's effectful weapon only for its ordinary strike.
 combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];st.unit[8]=*a;unit_place(&st.unit[8],third);
 st.unit[8].job=93;st.unit[8].set=24;st.unit[8].face=1;st.unit[8].slot=255;a->face=0;a->eq[EQ_WEAPON]=53;
 attack(U_RAMZA,10);st.hit=0;impact();
 for(k=0;k<30&&!W->mime.active;k++)end_anim();CHECK(W->mime.active,1);CHECK(st.atk,8);CHECK(st.unit[8].eq[EQ_WEAPON],53);
 mp=st.unit[8].mp;st.hit=1;st.self=16;st.dmg=1;st.push=0;hp=t->hp;impact();CHECK(t->hp<hp-1,1);CHECK(st.unit[8].mp,mp);
 for(k=0;k<80&&(st.action.phase!=CTX_IDLE||W->mime.active||W->mime.pending);k++)end_anim();CHECK(k<80,1);CHECK(st.unit[8].eq[EQ_WEAPON],255);
}
static void fx_ai_cache(void)
{
 static const u8 weapons[]={23,24,61,75};u8 i,row;u16 expected,actual;u32 seed;AbId ab;
 for(i=0;i<sizeof weapons;i++){
  combat_arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=weapons[i];status(10,35);seed=rng32;
#ifdef FFT_MODULES
  W->ai_weapon_left=511;W->ai_weapon_at=0;
  while(W->ai_weapon_left)mod_call(MOD_COMMAND,6,0);
#endif
  for(row=0;row<9;row++){
   ab=row?0x195+row:NO_ABIL;expected=fx_forecast(U_RAMZA,10,ab);
#ifdef FFT_MODULES
   st.formula.actor=U_RAMZA;st.formula.target=10;st.formula.ability=row?ab:265;st.formula.roll=0x4000|FORMULA_FORECAST;
   actual=row?mod_call(MOD_AI,3,0):W->ai_weapon_value[0][10];
#else
   actual=row?skill_amount(U_RAMZA,10,ab,0x4000|FORMULA_FORECAST):attack_prediction(U_RAMZA,10);
#endif
   CHECK(actual,expected);CHECK(rng32==seed,1);
  }
 }
}
static void fx_reflect_charge(void)
{
 u8 i;u16 hp,amount,mp;u32 seed=fx_proc_seed(53);Unit *a,*t,*other;
 for(i=0;i<2;i++){
  combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];other=&st.unit[8];status(10,SID_REFLECT);
  if(i){*other=*a;other->slot=255;unit_place(other,third);status(8,SID_REFLECT);}
  fx_root(53,seed);CHECK(st.self&255,16);hp=t->hp;amount=st.dmg;impact();CHECK(t->hp,hp-amount);
  CHECK(st.tgt,10);CHECK(st.abil,NO_ABIL);if(i)CHECK(other->hp<999,1);complete();
 }
 for(i=0;i<4;i++){
  static const u8 weapons[]={23,61,75,53};combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];
  a->eq[EQ_WEAPON]=weapons[i];a->set=2;a->learned=0xffff;status(10,SID_SLEEP);mp=a->mp;
  skill(U_RAMZA,10,0x196);CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);CHECK(st.action.phase,CTX_IDLE);
  st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(st.abil,0x196);CHECK(st.hit,1);CHECK(a->mp,mp);impact();complete();CHECK(a->mp,mp);
 }
}
static void fx_charm_frog(void)
{
 u8 mode,item=255;Unit *t;u16 hp;
 combat_arena();t=&st.unit[10];
 for(mode=0;mode<255;mode++){t->eq[EQ_ACC]=mode;if(cj_allegiance_immune(common,t->job,t->eq)&2){item=mode;break;}}
 CHECK(item!=255,1);
 for(mode=0;mode<3;mode++){
  combat_arena();t=&st.unit[10];if(mode==1)t->eq[EQ_ACC]=item;
  fx_root(94,1);CHECK(st.self,mode==1?0:35U<<8);if(mode==2)t->eq[EQ_ACC]=item;
  hp=t->hp;impact();CHECK(t->hp<hp,1);CHECK(t->team,mode?TEAM_ENEMY:TEAM_PLAYER);
  CHECK(core_status_current(&t->statuses,4)&32,mode?0:32);complete();
  if(!mode){
   CHECK(t->status_ct[SID_CHARM-SID_POISON],32);attack(U_RAMZA,10);st.hit=1;st.dmg=1;st.self=st.push=0;impact();
   CHECK(t->team,TEAM_ENEMY);CHECK(core_status_current(&t->statuses,4)&32,0);complete();
  }
 }
 for(mode=0;mode<3;mode++){
  combat_arena();t=&st.unit[10];status(10,SID_FROG);if(mode==2)t->statuses.innate[2]|=2;
  fx_root(31,1);CHECK(st.self,mode==2?0:32768U|(16U<<8));
  if(mode==1){t->statuses.inflicted[2]&=(u8)~2;unit_status_refresh(t);}
  impact();CHECK(core_status_current(&t->statuses,2)&2,mode==2?2:0);complete();
 }
}
static void fx_late_target_and_order(void)
{
 static const u8 weapons[]={23,61,75};u8 i;u16 hp;
 for(i=0;i<sizeof weapons;i++){
  combat_arena();st.unit[U_RAMZA].hp=500;fx_root(weapons[i],1);st.unit[10].hp=0;hp=st.unit[U_RAMZA].hp;impact();
  CHECK(st.hit,0);CHECK(st.unit[10].hp,0);CHECK(st.unit[U_RAMZA].hp,hp);CHECK(st.primary.reward,0);complete();
 }
 combat_arena();turn_order();fx_root(37,1);CHECK(st.self,9U<<8);impact();CHECK(unit_petrified(&st.unit[10])!=0,1);
 for(i=0;i<ORDER_N;i++)CHECK(st.turns[i]!=10,1);complete();
}
static void trials_weapons(void){source_effects();effects_lifecycle();fx_duals();fx_cancellation();fx_secondary();fx_ai_cache();fx_reflect_charge();fx_charm_frog();fx_late_target_and_order();}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);trials_weapons();printf("weapons %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();trials_weapons();BENCH_VALUE(sizeof(Unit));BENCH_VALUE(sizeof(State));BENCH_VALUE(sizeof(Work));BENCH_VALUE(sizeof(Save));BENCH_VALUE(sizeof(ModCtx));BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
