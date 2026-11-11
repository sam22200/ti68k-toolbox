#define FFT_WEAPON_TEST_API // direct test adapters; absent from size-bound renderer carriers
// Block D: real source jobs, native owners and persistent state, no borrowed art.
#define COMBAT_UI_CHECK
#include "check_combat.c"
#include "save.h"
#include "x/core-discovery/oracle/counts.h"
#ifndef NATURE_DATA
#define NATURE_DATA (cj_section(common,CJ_BATTLE)+CJ_DISCOVERY)
#endif
static void packet(u16 n){act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=1;st.effect_kind=AB_DAMAGE;st.dmg=n;W->cast.phase=CAST_BATCH_EMPTY;}
#ifdef FFT_MODULES
static u8 monster_hit(u8 a,u8 t,AbId ab){st.formula.actor=a;st.formula.target=t;st.formula.ability=ab;return(u8)mod_call(MOD_FORMULA,0,0);}
static u16 monster_amount(u8 a,u8 t,AbId ab,u16 roll){st.formula.actor=a;st.formula.target=t;st.formula.ability=ab;st.formula.roll=roll;return mod_call(MOD_FORMULA,1,0);}
#else
#define monster_hit skill_hit
#define monster_amount skill_amount
#endif
static void refresh(void){
#ifdef FFT_MODULES
 mod_call(MOD_NATURE,1,0);
#else
 monster_refresh();
#endif
}
static u8 find_item(u8 who,u8 map){
#ifdef FFT_MODULES
 st.formula.roll=map;return (u8)mod_call(MOD_NATURE,3,who);
#else
 return discovery_find(who,map);
#endif
}
static void nature_arena(void)
{
 combat_arena();W->nature_pending=W->nature_units=0;st.unit[10].allegiance=0;
 st.golem[0]=st.golem[1]=0;memset(battle_save.discovered,0,sizeof battle_save.discovered);
}
static void readrow(const u8 *p,u16 i,u8 width,u16 *row){u8 j;for(j=0;j<width;j++)row[j]=cj_word(p+((u32)i*width+j)*2);}
static void talk_checks(const u8 *p)
{
 u16 i,v[9],j;u32 seed,after;
 for(i=0;i<DISCOVERY_TALK;i++){
  readrow(p,i,9,v);nature_arena();st.unit[U_RAMZA].ma=20;st.unit[10].job=v[0]?94:74;st.support[U_RAMZA]=v[1];st.reaction[10]=v[2];st.unit[10].brave=v[3];status(10,v[4]);
  act_start(U_RAMZA,10);st.abil=118;if(v[5])st.action.phase=CTX_SECONDARY;
  rng32=v[6];seed=rng32;magic_hit(U_RAMZA,10,118,0);CHECK(rng32==seed,1);
  for(j=0;j<v[8];j++)rand15();after=rng32;rng32=seed;
  CHECK(magic_hit(U_RAMZA,10,118,1),v[7]);CHECK(rng32==after,1);
 }
}
static void train_checks(const u8 *p)
{
 u16 i,v[7],stock;u8 expected;u32 seed;
 for(i=0;i<DISCOVERY_TRAIN;i++){
  readrow(p,i,7,v);nature_arena();st.support[U_RAMZA]=v[0];st.unit[10].job=v[1]?94:74;st.unit[10].hp=v[2];st.unit[10].max_hp=100;
  if(v[4]==1)st.unit[10].statuses.immune[1]=64;else if(v[4]==2)status(10,SID_INVITE);if(v[5])st.unit[10].status_flags|=STATUS_IMMORTAL;
  refresh();packet(v[3]);impact();expected=(v[6]&64)!=0;
  CHECK(st.unit[10].team==TEAM_PLAYER,expected);CHECK(st.unit[10].dc==DC_GONE,(v[6]&32)!=0);
  seed=rng32;stock=battle_save.inv[NATURE_DATA[0]];impact();CHECK(rng32==seed,1);CHECK(battle_save.inv[NATURE_DATA[0]],stock);
 }
 // MP Switch, Golem and misses cannot poach or train through rejected HP impact.
 for(i=0;i<3;i++){
  nature_arena();st.unit[10].job=94;st.unit[10].hp=1;st.support[U_RAMZA]=471;refresh();
  if(i==0)st.reaction[10]=445;if(i==1)st.golem[1]=99;
  packet(20);if(i==2)st.hit=0;impact();CHECK(st.unit[10].hp,1);CHECK(st.unit[10].dc==DC_GONE,0);
 }
}
static void poach_checks(const u8 *p)
{
 u16 i,v[4],j;u32 after;
 for(i=0;i<DISCOVERY_POACH;i++){
  readrow(p,i,4,v);nature_arena();st.unit[10].job=v[0];st.unit[10].hp=1;st.support[U_RAMZA]=471;refresh();
  packet(1);rng32=v[1];for(j=0;j<v[3];j++)rand15();after=rng32;rng32=v[1];impact();
  CHECK(st.unit[10].dc,DC_GONE);CHECK(st.unit[10].x,255);CHECK(battle_save.inv[v[2]],1);CHECK(rng32==after,1);
  impact();CHECK(battle_save.inv[v[2]],1);CHECK(rng32==after,1);
 }
 nature_arena();st.unit[10].job=94;st.unit[10].hp=1;st.support[U_RAMZA]=471;refresh();battle_save.inv[NATURE_DATA[1]]=99;battle_save.inv[NATURE_DATA[0]]=99;
 packet(1);impact();CHECK(st.unit[10].dc,DC_GONE);CHECK(battle_save.inv[NATURE_DATA[0]],99);CHECK(battle_save.inv[NATURE_DATA[1]],99);
}
static void discovery_checks(const u8 *p)
{
 u16 i,v[9],c,n=0,j;u32 after,seed;u8 item;
 for(i=0;i<DISCOVERY_FIND;i++){
  readrow(p,i,9,v);nature_arena();
  for(c=0;c<MAP_N;c++)if(TILE_X[c]==v[2]>>4&&TILE_Z[c]==(v[2]&15)&&node_layer(c)==v[3]>>7)break;
  if(c==MAP_N)continue;
  unit_place(&st.unit[U_RAMZA],c);st.unit[U_RAMZA].brave=v[4];st.movement[U_RAMZA]=24;
  // Only the first eligible source entry at a panel is reachable by an ordinary move.
  for(j=0;j<v[1];j++){const u8 *r=NATURE_DATA+96+v[0]*16+j*4;if(!(r[1]&32)&&(r[2]|r[3])&&r[0]==v[2]&&(r[1]>>7)==(v[3]>>7))break;}
  if(j<v[1])continue;
  rng32=v[5];seed=rng32;rand15();after=rng32;rng32=seed;item=v[6];
  CHECK(find_item(U_RAMZA,v[0]),1);CHECK(battle_save.inv[item],item&&item!=255?1:0);CHECK(battle_save.discovered[v[0]>>1],v[7]);CHECK(rng32==after,1);CHECK(v[8],1);
  CHECK(find_item(U_RAMZA,v[0]),0);CHECK(rng32==after,1);n++;
 }
 CHECK(n>400,1);
 // A full stock sells at source quarter price; all worn items count toward99.
 for(i=0;i<DISCOVERY_FIND;i++){
  readrow(p,i,9,v);if(v[4]!=100||!v[6]||v[6]==255)continue;
  for(c=0;c<MAP_N;c++)if(TILE_X[c]==v[2]>>4&&TILE_Z[c]==(v[2]&15)&&node_layer(c)==v[3]>>7)break;if(c==MAP_N)continue;
  nature_arena();unit_place(&st.unit[U_RAMZA],c);st.unit[U_RAMZA].brave=100;st.movement[U_RAMZA]=24;battle_save.inv[v[6]]=99;qput(battle_save.money,0);rng32=v[5];CHECK(find_item(U_RAMZA,v[0]),1);CHECK(battle_save.inv[v[6]],99);CHECK(qlong(battle_save.money),cj_word(NATURE_DATA+96+2048+64+2*v[6])>>2);break;
 }
 CHECK(i<DISCOVERY_FIND,1);
 nature_arena();seed=rng32;CHECK(find_item(U_RAMZA,22),0);CHECK(find_item(U_RAMZA,56),0);CHECK(rng32==seed,1);
}
static void monster_checks(void)
{
 u8 i;u16 c;Unit *u,*v;const u8 *row;
 nature_arena();u=&st.unit[U_RAMZA];v=&st.unit[10];u->job=94;u->zodiac=12;v->team=TEAM_PLAYER;st.support[10]=478;refresh();
 CHECK(u->set,176);CHECK(u->learned,15);CHECK(known(U_RAMZA,255),3);CHECK(known(U_RAMZA,0),265);CHECK(known(U_RAMZA,1),269);CHECK(known(U_RAMZA,2),268);CHECK(can_use(U_RAMZA,268),1);
 st.support[10]=0;refresh();CHECK(u->learned,7);CHECK(known(U_RAMZA,255),2);CHECK(can_use(U_RAMZA,268),0);
 st.support[10]=478;status(10,37);refresh();CHECK(u->learned,7); // Source PREVENT_REACTION includes Don't Act.
 status(10,35);refresh();CHECK(u->learned,7);
 v->statuses.inflicted[4]=0;unit_status_refresh(v);v->hp=0;refresh();CHECK(u->learned,7);
 v->hp=100;v->team=TEAM_ENEMY;refresh();CHECK(u->learned,7);v->team=TEAM_PLAYER;
 unit_place(v,third);refresh();CHECK(u->learned,7);
 // Check original total-height differences, Float and body-size reduction.
 unit_place(v,second);refresh();CHECK(u->learned,15);status(10,17);refresh();CHECK(u->learned,15);status(10,17);refresh();CHECK(u->learned,15);
 // Actual monster command pipeline: no borrowed weapon, learned-set holes kept.
 nature_arena();u=&st.unit[U_RAMZA];u->job=94;u->zodiac=12;u->ma=14;u->faith=0;refresh();u->hp=100;u->max_hp=200;
 skill(U_RAMZA,U_RAMZA,269);CHECK(W->cast.phase,CAST_BATCH_READY);impact();CHECK(u->hp,142);CHECK(u->mp,100);
 nature_arena();u=&st.unit[U_RAMZA];v=&st.unit[10];u->job=94;u->zodiac=12;v->team=TEAM_PLAYER;st.support[10]=478;status(U_RAMZA,24);refresh();
 skill(U_RAMZA,U_RAMZA,268);CHECK(W->cast.phase,CAST_BATCH_READY);st.hit=1;W->cast.victim[U_RAMZA].hit=1;impact();CHECK(core_status_current(&u->statuses,3)&128,0);
 nature_arena();u=&st.unit[U_RAMZA];u->job=94;u->zodiac=12;refresh();skill(U_RAMZA,10,265);CHECK(st.mode,M_ACT);CHECK(st.abil,265);CHECK(st.dmg>0,1);impact();CHECK(st.unit[10].hp<999,1);
 // The ordinary physical protection hook applies to the monster constructor too.
 nature_arena();u=&st.unit[U_RAMZA];u->job=94;u->zodiac=12;refresh();st.golem[1]=99;
 skill(U_RAMZA,10,265);CHECK(st.mode,M_ACT);st.hit=1;st.dmg=20;st.push=0;impact();CHECK(st.unit[10].hp,999);CHECK(st.golem[1],79);CHECK(st.effect_kind,AB_GOLEM);
 // Choco Attack forecasts use the same physical evades as its actual roll.
 nature_arena();u=&st.unit[U_RAMZA];v=&st.unit[10];u->job=94;u->zodiac=12;refresh();v->eq[EQ_ACC]=232;v->eq[EQ_SHIELD]=128;v->eq[EQ_WEAPON]=19;
 for(i=0;i<3;i++){u32 before=rng32;v->face=i;CHECK(monster_hit(U_RAMZA,10,265),magic_hit(U_RAMZA,10,NO_ABIL,0));CHECK(rng32==before,1);CHECK(monster_amount(U_RAMZA,10,265,0),attack_prediction(U_RAMZA,10));}
 // Monster copies of human records (common_tables): Small Bomb = Spin Fist's formula0x31 (Y0),
 // Shine Lover = Gil Taking, Gather Power = Accumulate with Y2. Same numbers, same rolls.
 CHECK(monster_amount(U_RAMZA,10,0xE1,0),monster_amount(U_RAMZA,10,0x64,0));CHECK(monster_hit(U_RAMZA,10,0xE1),monster_hit(U_RAMZA,10,0x66));
 CHECK(abilities[0xE1].range,1);CHECK(abilities[0xE1].vert,0);
 CHECK(monster_amount(U_RAMZA,10,0x133,0),monster_amount(U_RAMZA,10,0x6C,0));CHECK(monster_hit(U_RAMZA,10,0x133),monster_hit(U_RAMZA,10,0x6C));
 CHECK(monster_amount(U_RAMZA,U_RAMZA,0x143,0),2);CHECK(monster_amount(U_RAMZA,U_RAMZA,0x92,0),1);
 // Choco Ball: the same strike, range 4 at any height.
 CHECK(monster_amount(U_RAMZA,10,0x10A,0),monster_amount(U_RAMZA,10,0x64,0));CHECK(abilities[0x10A].range,4);CHECK(abilities[0x10A].vert,255);
 CHECK(monster_hit(U_RAMZA,10,0x10A),monster_hit(U_RAMZA,10,0x66));
 // Cat Kick / Tail Swing = Dash's formula0x37 with Y 8/15, evadeable (common_tables.dash_like).
 CHECK(monster_amount(U_RAMZA,10,0x119,0),monster_amount(U_RAMZA,10,0x93,0));
 CHECK(monster_amount(U_RAMZA,10,0x119,0x7FFF),2*monster_amount(U_RAMZA,10,0x93,0x7FFF));
 CHECK(abilities[0x119].flags&AF_EVADE,AF_EVADE);CHECK(abilities[0x93].flags&AF_EVADE,0);CHECK(abilities[0x150].y,15);
 CHECK(monster_hit(U_RAMZA,10,0x119)<=monster_hit(U_RAMZA,10,0x93),1);
 // Monster basic attacks share Choco Attack's source record (common_tables.monster_attack_record).
 {static const u16 same[]={0x10E,0x113,0x118,0x11D,0x122,0x12C,0x131,0x140,0x145,0x14A,0x14F};
  for(i=0;i<sizeof same/sizeof*same;i++){
   const Ability *b=&abilities[same[i]];CHECK(b->kind,0);CHECK(b->formula,1);CHECK(b->flags,10);CHECK(b->range,1);
   CHECK(monster_hit(U_RAMZA,10,same[i]),monster_hit(U_RAMZA,10,265));CHECK(monster_amount(U_RAMZA,10,same[i],0),monster_amount(U_RAMZA,10,265,0));}}
 // Support helper's source mask and total-height boundaries are exported fixtures.
 (void)i;(void)c;(void)row;
}
static void supporter_checks(const u8 *p)
{
 u16 i,v[5],base_a,base_t;u8 terrain[2048];const u8 *original;Unit *a,*t;s16 h;
 for(i=0;i<DISCOVERY_SUPPORTERS;i++){
  readrow(p,i,5,v);nature_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->job=94;t->team=v[1]?TEAM_ENEMY:TEAM_PLAYER;st.support[10]=v[3]?478:0;status(10,v[2]);
  original=W->surface;memcpy(terrain,original,(u16)MAP_W*MAP_H*8);W->surface=terrain;
  base_a=((W->grid_row[a->z]+a->x)<<3)+4*node_layer(UNIT_CELL(a));base_t=((W->grid_row[t->z]+t->x)<<3)+4*node_layer(UNIT_CELL(t));
  terrain[base_a]=12;terrain[base_a+1]=0;h=24+(s16)v[0];terrain[base_t]=(u8)(h>>1);terrain[base_t+1]=h&1;
  refresh();
  CHECK((a->learned&8)!=0,v[4]);W->surface=original;
 }
}
static void choco_checks(const u8 *p)
{
 u16 i,v[10],j;u32 seed,after;
 for(i=0;i<DISCOVERY_CHOCO;i++){
  readrow(p,i,10,v);nature_arena();st.unit[U_RAMZA].job=94;st.unit[U_RAMZA].zodiac=12;st.unit[U_RAMZA].ma=v[1];st.support[U_RAMZA]=v[2]?SUPPORT_MAGIC_ATTACK_UP:0;st.unit[U_RAMZA].faith=st.unit[10].faith=v[3];if(v[4])status(10,3);
  if(v[0]==269){CHECK(magic_heal(U_RAMZA,10,269,0),v[4]?v[6]:v[7]);CHECK(skill_kind(U_RAMZA,10,269),v[4]?AB_DAMAGE:AB_HEAL);}
  else {CHECK(magic_heal(U_RAMZA,10,268,1),v[8]);rng32=1;seed=rng32;for(j=0;j<v[9];j++)rand15();after=rng32;rng32=seed;CHECK(magic_hit(U_RAMZA,10,268,1),v[5]);CHECK(rng32==after,1);}
 }
}
static void flush_private(void){
#ifdef __m68k__
 save_write();sv_data=0;
#endif
}
static void save_checks(void)
{
 Save back,legacy;u8 i;
 nature_arena();memcpy(battle_save.magic,SAVE_MAGIC,4);battle_save.discovered[0]=128;battle_save.discovered[63]=1;
 CHECK(rt_save("dfsave",&battle_save,sizeof battle_save),1);flush_private();CHECK(save_load_from("dfsave",&back),1);CHECK(back.discovered[0],128);CHECK(back.discovered[63],1);
 legacy=back;memcpy(legacy.magic,"FSV3",4);CHECK(rt_save("dfsave",&legacy,SAVE_V3_BYTES),1);flush_private();CHECK(save_load_from("dfsave",&back),1);CHECK(memcmp(back.unit,legacy.unit,sizeof back.unit)==0,1);for(i=0;i<64;i++)CHECK(back.discovered[i],0);
 memcpy(legacy.magic,"FSV2",4);CHECK(rt_save("dfsave",&legacy,SAVE_V2_BYTES),1);flush_private();CHECK(save_load_from("dfsave",&back),1);CHECK(qlong(back.money),2000);CHECK(back.discovered[0],0);
 // Victory recruitment keeps source monster identity, never a human career.
 nature_arena();st.party_in=1;memset(&battle_save,0,sizeof battle_save);memcpy(battle_save.magic,SAVE_MAGIC,4);st.support[U_RAMZA]=470;st.unit[10].job=94;st.unit[10].hp=21;st.unit[10].max_hp=100;st.unit[10].slot=255;st.unit[10].gfx=255;refresh();packet(1);impact();
#ifdef FFT_MODULES
 mod_call(MOD_SAVE,0,0);
#else
 save_campaign();
#endif
 CHECK(battle_save.n,1);CHECK(battle_save.unit[0].type,3);CHECK(battle_save.unit[0].job,94);
}
static void discovery_trials(void)
{
 const u8 *p;u16 bytes;p=rt_file("ffdn0",&bytes);CHECK(p!=RT_NULL,1);CHECK(bytes>=DISCOVERY_BYTES&&bytes<=DISCOVERY_BYTES+6,1);
 talk_checks(p+DISCOVERY_TALK_OFFSET);train_checks(p+DISCOVERY_TRAIN_OFFSET);poach_checks(p+DISCOVERY_POACH_OFFSET);discovery_checks(p+DISCOVERY_FIND_OFFSET);monster_checks();supporter_checks(p+DISCOVERY_SUPPORTERS_OFFSET);choco_checks(p+DISCOVERY_CHOCO_OFFSET);save_checks();
}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);discovery_trials();printf("discovery %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();discovery_trials();BENCH_VALUE(sizeof(Unit));BENCH_VALUE(sizeof(State));BENCH_VALUE(sizeof(Work));BENCH_VALUE(sizeof(Save));BENCH_VALUE(sizeof(ModCtx));BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
