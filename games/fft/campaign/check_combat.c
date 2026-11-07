// Block C source records and real scalar/batch/secondary ownership.
#define FFT_PRODUCTION_CORE
#define REACTION_SEQUENCE_CHECK
#include "check_reactions.c"
#include "x/core-combat/oracle/counts.h"
static void combat_arena(void)
{
 u8 i;arena();st.morph_mask=0;W->rebuild=0;
 for(i=0;i<NUNIT;i++){
  Unit *u=&st.unit[i];st.reaction[i]=st.support[i]=st.movement[i]=0;
  memset(&u->statuses,0,sizeof u->statuses);u->status_flags=0;u->job=74;u->female=0;u->zodiac=0;
  memset(u->eq,255,5);u->mp=u->max_mp=100;u->ma=9;u->faith=70;u->brave=100;u->sp=9;
 }
 memset(battle_save.inv,0,256);
}
static void status(u8 who,u8 sid)
{
 if(sid!=255)st.unit[who].statuses.inflicted[sid>>3]|=128>>(sid&7);
 unit_status_refresh(&st.unit[who]);
}
static void frozen(u8 a,u8 t,u8 kind,u16 amount,AbId ab)
{
 CastBatch *b=&W->cast;act_start(a,t);st.abil=ab;st.hit=1;st.effect_kind=kind;st.dmg=amount;
 memset(b,0,sizeof *b);b->phase=CAST_BATCH_READY;b->actor=a;b->ability=ab;b->mask=(u16)1<<t;
 b->victim[t].effect=kind;b->victim[t].hit=1;b->victim[t].amount=amount;
}
static void complete(void)
{
 u8 i;for(i=0;i<30&&st.action.phase!=CTX_IDLE;i++){if(st.action.phase==CTX_SECONDARY)impact();end_anim();}
 CHECK(st.action.phase,CTX_IDLE);
}
static void evades(const u8 *rows)
{
 u16 i,j;for(i=0;i<COMBAT_EVADES;i++){
  u16 v[14];u8 hit,side;u32 seed,after;AbId ab=NO_ABIL;
  for(j=0;j<14;j++)v[j]=cj_word(rows+i*28+2*j);
  combat_arena();unit_place(&st.unit[U_RAMZA],first);unit_place(&st.unit[10],second);
  st.unit[10].eq[3]=19;st.unit[10].eq[4]=v[1];st.unit[10].eq[2]=v[2];st.unit[U_RAMZA].eq[3]=v[0];
  st.reaction[10]=v[3];st.support[U_RAMZA]=v[4];st.unit[10].brave=v[7];status(10,v[6]);
  for(side=0;side<4;side++){st.unit[10].face=side;if(attack_side(U_RAMZA,10)==v[5])break;}
  CHECK(side<4,1);
  if(v[9]==1){for(ab=512;ab<NABIL;ab++)if(abilities[ab].formula==99)break;CHECK(ab<NABIL,1);}
  else if(v[9]>=2)ab=v[9]==2?148:147;
  act_start(U_RAMZA,10);st.abil=ab;if(v[10])st.action.phase=CTX_SECONDARY;
  rng32=v[8];seed=rng32;CHECK(magic_hit(U_RAMZA,10,ab,0),v[11]);CHECK(rng32==seed,1);
  for(j=0;j<v[13];j++)rand15();after=rng32;rng32=seed;
  hit=magic_hit(U_RAMZA,10,ab,1);CHECK(hit,v[12]);CHECK(rng32==after,1);
 }
}
static void resources(const u8 *rows)
{
 u16 i,j;for(i=0;i<COMBAT_RESOURCES;i++){
  u16 v[11],rid;u8 heal;u32 seed,after;Unit *t;
  for(j=0;j<11;j++)v[j]=cj_word(rows+i*22+2*j);
  combat_arena();t=&st.unit[10];t->hp=70;t->max_hp=100;t->mp=11;t->brave=v[2];st.reaction[10]=v[0];status(10,v[4]);heal=v[0]==444;
  frozen(U_RAMZA,10,heal?AB_HEAL:AB_DAMAGE,v[1],1);
  if(v[5])st.action.phase=CTX_SECONDARY;
  rng32=v[3];seed=rng32;for(j=0;j<v[10];j++)rand15();after=rng32;rng32=seed;
  impact();rid=st.action.pending&((u16)1<<10)?reaction_id(st.action.reaction[10]):0;
  CHECK(rid,v[6]==445?0:v[6]);
  if(rid)CHECK(st.action.reaction[10]&1023,v[7]);
  CHECK(t->hp,heal?(70+v[1]>100?100:70+v[1]):v[8]>=70?0:70-v[8]);
  CHECK(t->mp,v[9]>=11?0:11-v[9]);CHECK(rng32==after,1);
  {State once=st;impact();CHECK(memcmp(&once,&st,sizeof st)==0,1);}
 }
}
static void sharing(void)
{
 Unit *a,*t,*other;u16 jp;u32 seed;
 combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];st.reaction[10]=446;t->hp=70;t->max_hp=100;
 root(17);CHECK(t->hp,53);CHECK(st.action.reaction[10],0x2009);jp=t->jp;end_anim();CHECK(st.abil,446);CHECK(st.dmg,9);
 impact();CHECK(a->hp,990);CHECK(t->hp,62);CHECK(t->jp,jp);CHECK(st.action.pending,0);complete();
 combat_arena();st.reaction[10]=446;st.unit[10].hp=1;root(17);end_anim();CHECK(st.primary.running,0);CHECK(st.unit[U_RAMZA].hp,999);
 // Overflow is divided among injured initial allies. Wall participates in the
 // divisor but rejects its own result; no second Distribute is staged.
 combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];other=&st.unit[8];
 t->team=TEAM_PLAYER;t->hp=90;t->max_hp=100;st.reaction[10]=444;
 *other=*a;unit_place(other,third);other->hp=100;other->max_hp=999;status(8,31);a->hp=100;st.reaction[U_RAMZA]=444;
 frozen(U_RAMZA,10,AB_HEAL,31,1);impact();CHECK(t->hp,100);CHECK(st.action.reaction[10],0x1015);
 end_anim();CHECK(st.abil,444);CHECK(W->cast.mask,((u16)1<<U_RAMZA)|((u16)1<<8));CHECK(W->cast.victim[U_RAMZA].amount,11);
 seed=rng32;impact();CHECK(a->hp,111);CHECK(other->hp,100);CHECK(st.action.pending,0);CHECK(rng32==seed,1);complete();
 // Self overflow is a legal primary trigger, with no recursive secondary.
 combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->hp=90;a->max_hp=100;t->hp=50;t->max_hp=100;t->team=TEAM_PLAYER;st.reaction[U_RAMZA]=444;
 frozen(U_RAMZA,U_RAMZA,AB_HEAL,20,1);impact();CHECK(st.action.pending,(u16)1<<U_RAMZA);end_anim();CHECK(st.primary.running,0);CHECK(t->hp,50);complete();
}
static void guards_and_items(void)
{
 u16 item,stock,ab;u8 mode;State before;u32 seed;
 for(mode=0;mode<3;mode++){
  combat_arena();st.unit[U_RAMZA].set=20;st.unit[U_RAMZA].learned=0xfff;st.unit[10].team=TEAM_PLAYER;st.reaction[10]=450;
  for(ab=512;ab<NABIL;ab++)if(abilities[ab].formula==99&&abilities[ab].kind!=255)break;CHECK(ab<NABIL,1);item=abilities[ab].bag;battle_save.inv[item]=mode==2?99:2;
  if(mode==1)st.unit[10].team=TEAM_ENEMY;
  // Accepted Throw pays one source item; Catch returns it only to the blue bag.
  skill(U_RAMZA,10,ab);CHECK(st.mode,M_ACT);CHECK(st.effect_kind,AB_CATCH);CHECK(st.dmg,item);CHECK(battle_save.inv[item],mode==2?98:1);
  stock=battle_save.inv[item];impact();CHECK(st.unit[10].hp,999);CHECK(battle_save.inv[item],stock+(mode!=1));
  before=st;stock=battle_save.inv[item];impact();CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(battle_save.inv[item],stock);complete();
 }
 combat_arena();seed=rng32;before=st;CHECK(ability_range(U_RAMZA,0x170),1);CHECK(rng32==seed,1);CHECK(memcmp(&before,&st,sizeof st)==0,1);
 st.support[U_RAMZA]=474;CHECK(ability_range(U_RAMZA,0x170),4);st.support[U_RAMZA]=0;st.unit[U_RAMZA].job=75;CHECK(ability_range(U_RAMZA,0x170),4);
}
static void offensive(void)
{
 u16 hp,mp,jp;u32 seed;u8 i;
 // Hamedo replaces both primary hands, never awards XP and preserves the
 // defender's independently armed action. It also cancels on a forced miss.
 for(i=0;i<3;i++){
  combat_arena();st.reaction[10]=453;st.unit[10].eq[3]=19;st.pending[10].state=DEFER_CHARGING;st.pending[10].ticks=7;
  if(i==2)st.unit[10].hp=0;
  jp=st.unit[U_RAMZA].jp;hp=st.unit[10].hp;mp=st.unit[U_RAMZA].mp;
  CHECK(attack_sequence(U_RAMZA,10,2),1);
  if(i==2){CHECK(st.atk,U_RAMZA);continue;}
  CHECK(st.atk,10);CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.action.cursor,128);
  if(i==1)st.hit=0;
  impact();CHECK(st.unit[10].hp,hp);CHECK(st.primary.reward,0);complete();CHECK(st.unit[10].hp,hp);
  CHECK(st.unit[U_RAMZA].jp,jp);CHECK(st.unit[U_RAMZA].mp,mp);CHECK(st.pending[10].state,DEFER_CHARGING);CHECK(st.pending[10].ticks,7);
 }
 combat_arena();st.reaction[10]=434;st.unit[10].hp=30;st.unit[10].max_hp=100;root(11);CHECK(st.unit[10].hp,19);end_anim();CHECK(st.abil,434);CHECK(st.dmg,100);
 hp=st.unit[U_RAMZA].hp;impact();CHECK(st.unit[U_RAMZA].hp,hp-100);complete();
 // A Meatbone reactor displaced outside reach must expire, without falling
 // through to the older self-resource (Critical Quick) constructor.
 combat_arena();st.reaction[10]=434;st.unit[10].hp=30;st.unit[10].max_hp=100;root(11);unit_place(&st.unit[10],third);
 end_anim();CHECK(st.primary.running,0);CHECK(st.unit[10].ct,37);CHECK(st.unit[U_RAMZA].hp,999);
 // Capture an eligible incoming Fire even on a miss. The return pays once,
 // ignores learning/range and uses the frozen incoming spell ID.
 combat_arena();st.unit[10].set=3;st.unit[10].learned=0;st.reaction[10]=435;
 for(i=0;i<60;i++)if((abilities[i].flags&(AF_CAST|AF_AREA))==(AF_CAST|AF_AREA)&&abilities[i].kind==AB_DAMAGE&&(cj_range(common,i)[6]&64))break;
 CHECK(i<60,1);frozen(U_RAMZA,10,AB_DAMAGE,9,i);W->cast.victim[10].hit=0;impact();CHECK(reaction_id(st.action.reaction[10]),435);
 mp=st.unit[10].mp;end_anim();CHECK(st.abil,i);CHECK(st.unit[10].mp,mp-cj_range(common,i)[13]);CHECK(W->cast.phase,CAST_BATCH_READY);
 seed=rng32;impact();CHECK(st.action.pending,0);CHECK(rng32==seed,1);mp=st.unit[10].mp;complete();CHECK(st.unit[10].mp,mp);
}
static void counter_contracts(const u8 *rows)
{
 u16 i,j;u32 seed,after;
 for(i=0;i<COMBAT_COUNTERS;i++){
  u16 v[8];for(j=0;j<8;j++)v[j]=cj_word(rows+i*16+2*j);
  combat_arena();st.reaction[10]=435;st.unit[10].brave=v[1];status(10,v[2]);st.unit[U_RAMZA].set=v[4]?21:11;
  frozen(U_RAMZA,10,AB_DAMAGE,1,v[0]);W->cast.victim[10].hit=v[5];if(v[3])st.action.phase=CTX_SECONDARY;
  rng32=32767;seed=rng32;for(j=0;j<v[7];j++)rand15();after=rng32;rng32=seed;impact();
  CHECK(st.action.pending&((u16)1<<10)?reaction_id(st.action.reaction[10]):0,v[6]);CHECK(rng32==after,1);
 }
 rows+=COMBAT_COUNTERS*16;
 for(i=0;i<COMBAT_COSTS;i++){
  u16 v[7],mp;for(j=0;j<7;j++)v[j]=cj_word(rows+i*14+2*j);
  combat_arena();st.unit[10].job=v[4]?SOURCE_JOB_MIME:74;st.unit[10].mp=v[1];st.support[10]=v[2]?SUPPORT_HALF_MP:0;
  frozen(U_RAMZA,10,AB_DAMAGE,1,v[0]);W->cast.victim[10].hit=0;impact();
  CHECK(core_action_queue(&st.action,10,435,NUNIT),CTX_OK);st.action.reaction[10]=0x0800U|v[0];st.reaction[10]=435;status(10,v[3]);
  mp=st.unit[10].mp;end_anim();
  CHECK(st.primary.running,v[5]);CHECK(st.unit[10].mp,v[5]?v[6]:mp);
 }
}
static void command_order(void)
{
 u16 jp,mp;u8 i;
 // A Throw intercepted before payment keeps the source item, including misses.
 for(i=0;i<2;i++){
  combat_arena();st.unit[U_RAMZA].set=20;st.unit[U_RAMZA].learned=0xfff;battle_save.inv[122]=2;
  st.reaction[10]=453;st.unit[10].eq[3]=19;mp=st.unit[U_RAMZA].mp;jp=st.unit[U_RAMZA].jp;
  skill(U_RAMZA,10,634);CHECK(st.atk,10);CHECK(st.action.phase,CTX_SECONDARY);CHECK(battle_save.inv[122],2);
  if(i)st.hit=0;impact();complete();CHECK(battle_save.inv[122],2);CHECK(st.unit[U_RAMZA].mp,mp);CHECK(st.unit[U_RAMZA].jp,jp);
 }
 // Delayed Charge reaches the same pre-emption after its clock expires.
 combat_arena();st.unit[U_RAMZA].set=2;st.unit[U_RAMZA].learned=0xffff;st.reaction[10]=453;st.unit[10].eq[3]=19;
 skill(U_RAMZA,10,0x196);CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);st.pending[U_RAMZA].ticks=0;
 CHECK(charge_resolve(),1);CHECK(st.atk,10);CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.open_resolving,1);impact();
 CHECK(st.unit[10].hp,999);
 // MP Switch changes the owned presentation packet at impact; a prepared HP
 // glyph expires so AREA_UI rebuilds the MP glyph from the applied result.
 combat_arena();st.reaction[10]=445;frozen(U_RAMZA,10,AB_DAMAGE,17,16);W->area_impact.ready=(u16)1<<10;W->area_impact.next=NUNIT+1;
 impact();CHECK(W->cast.victim[10].effect,AB_MP_DAMAGE);CHECK(W->area_impact.ready,0);CHECK(W->area_impact.next,0);
}
static void combat_trials(void)
{
 const u8 *rows;u16 bytes;rows=rt_file("ffco0",&bytes);CHECK(rows!=RT_NULL,1);CHECK(bytes>=COMBAT_EVADES*28+COMBAT_RESOURCES*22,1);
 evades(rows);resources(rows+COMBAT_EVADES*28);sharing();guards_and_items();offensive();
 rows=rt_file("ffco1",&bytes);CHECK(rows!=RT_NULL,1);counter_contracts(rows);command_order();
}
#ifndef COMBAT_UI_CHECK
#ifndef __m68k__
int main(void){sw_init(61);combat_trials();printf("combat %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();combat_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif

#endif
