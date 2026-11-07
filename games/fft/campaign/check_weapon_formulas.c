#define FFT_WEAPON_TEST_API // direct test adapters; absent from size-bound renderer carriers
// Source primitives and actual command/response RNG integration.
#define COMBAT_UI_CHECK
#include "check_combat.c"
#include "x/core-weapon-formulas/oracle/source.h"
static void source_scalars(void)
{
 u16 size,i,j,v[SOURCE_WEAPON_DAMAGE_FIELDS];const u8 *data=rt_file("fwform",&size);u32 seed;Unit *a,*t;
 CHECK(data!=RT_NULL,1);CHECK(size>=SOURCE_WEAPON_DAMAGE_CASES*SOURCE_WEAPON_DAMAGE_FIELDS*2U,1);
 for(i=0;i<SOURCE_WEAPON_DAMAGE_CASES;i++){
  for(j=0;j<SOURCE_WEAPON_DAMAGE_FIELDS;j++)v[j]=cj_word(data+((u16)i*SOURCE_WEAPON_DAMAGE_FIELDS+j)*2U);
  combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->eq[EQ_WEAPON]=v[0];a->eq[EQ_SHIELD]=255;
  a->pa=v[1];a->ma=v[2];a->sp=v[3];a->brave=v[4];st.support[U_RAMZA]=v[6];a->female=t->female=0;a->zodiac=t->zodiac=0;
  if(v[7]!=255)status(U_RAMZA,v[7]);if(v[8]!=255)status(10,v[8]);seed=rng32;
  ZB(54);CHECK(weapon_amount(U_RAMZA,10,v[0],v[16],v[11],0,v[5],v[12]?16384:v[9]),v[13]*v[14]);ZE(54);
  if(v[9]==16384||!weapon_variable(v[0]==255?0:cj_equipment_item(common,v[0])[0])){
   if(v[5])CHECK(charge_query(U_RAMZA,10,0x198,(u8)(v[16]<<2),v[11]),v[15]);
   else CHECK(attack_damage(U_RAMZA,10,v[16],v[11]),v[15]);
  }
  CHECK(rng32==seed,1);
 }
}
static void expected_strike(u8 actor,u8 target,u8 charge,u16 *damage,u8 *critical,u32 *after)
{
 u8 weapon=current_weapon(actor),i;u16 base=16384,amp=0;u32 seed=rng32;
 if(!weapon_plain_gun(weapon))for(i=0;i<4;i++)rand15();
 if(weapon!=255&&weapon_variable(cj_equipment_item(common,weapon)[0]))base=rand15();
 *critical=(u16)(muls16((s16)rand15(),100)>>15)<4;if(*critical)amp=rand15();
 if(cj_weapon(common,weapon)[2]<=2)rand15();
 *after=rng32;*damage=weapon_amount(actor,target,weapon,*critical,amp,0,charge,base);if(*damage>999)*damage=999;rng32=seed;
}
static void execution(void)
{
 static const u8 weapons[]={38,48,59,67,71,95,107,115};u8 i,charge,crit;u16 amount;u32 after,seed;
 for(i=0;i<sizeof weapons;i++)for(charge=0;charge<2;charge++){
  combat_arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=weapons[i];st.unit[10].face=0;rng32=1;
  // Make evasion zero while retaining the ordinary four source draws.
  st.unit[10].statuses.inflicted[4]=16;unit_status_refresh(&st.unit[10]);
  expected_strike(U_RAMZA,10,charge?1:0,&amount,&crit,&after);
  ZB(55);
  if(charge){skill(U_RAMZA,10,0x196);CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);}
  else attack(U_RAMZA,10);
  ZE(55);CHECK(st.hit,1);CHECK(st.crit,crit);CHECK(st.dmg,amount);CHECK(rng32==after,1);
  seed=rng32;attack_prediction(U_RAMZA,10);CHECK(rng32==seed,1);
 }
 // Gun hit chance ignores ordinary evasion and Defend, but keeps Blade Grasp.
 combat_arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=71;st.unit[10].eq[EQ_SHIELD]=128;
 status(10,6);CHECK(hit_chance(U_RAMZA,10),100);st.reaction[10]=REACTION_BLADE_GRASP;st.unit[10].brave=100;
 CHECK(hit_chance(U_RAMZA,10),0);seed=rng32;rand15();after=rng32;rng32=seed;attack(U_RAMZA,10);CHECK(st.hit,0);CHECK(rng32==after,1);
 // A left-only staff and Frog select the same source hand as ordinary attacks.
 combat_arena();st.unit[U_RAMZA].eq[EQ_SHIELD]=59;st.unit[U_RAMZA].ma=11;
 CHECK(attack_damage(U_RAMZA,10,0,0),11*items[59].wp);status(U_RAMZA,22);CHECK(hand_weapon(&st.unit[U_RAMZA],0),255);
 CHECK(attack_damage(U_RAMZA,10,0,0),1*(st.unit[U_RAMZA].pa*st.unit[U_RAMZA].brave/100));
}
static void response_rolls(void)
{
 u16 amount;u8 crit,kind;u32 seed,after;
 for(kind=0;kind<2;kind++){
  combat_arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=19;st.unit[10].eq[EQ_WEAPON]=48;
  st.reaction[10]=kind?REACTION_HAMEDO:REACTION_COUNTER;rng32=1;
  if(kind){
   status(U_RAMZA,4);seed=rng32;rand15();expected_strike(10,U_RAMZA,0,&amount,&crit,&after);rng32=seed;
   attack(U_RAMZA,10);
  }else{
   attack(U_RAMZA,10);st.hit=0;impact();status(U_RAMZA,35);
   expected_strike(10,U_RAMZA,0,&amount,&crit,&after);end_anim();
  }
  CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.hit,1);CHECK(st.crit,crit);CHECK(st.dmg,amount);CHECK(rng32==after,1);
 }
 // A pre-formula guard miss cannot consume the variable-power or critical draw.
 combat_arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=48;st.reaction[10]=REACTION_BLADE_GRASP;
 seed=rng32;rand15();after=rng32;rng32=seed;attack(U_RAMZA,10);CHECK(st.hit,0);CHECK(rng32==after,1);
 // Exercise a real variable-power critical, with independent base/amplitude draws.
 for(seed=1;seed<4096;seed++){
  combat_arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=48;status(10,35);rng32=seed;
  expected_strike(U_RAMZA,10,0,&amount,&crit,&after);if(crit)break;
 }
 CHECK(seed<4096,1);attack(U_RAMZA,10);CHECK(st.crit,1);CHECK(st.dmg,amount);CHECK(rng32==after,1);
}
static void expanded_hands(void)
{
 u8 i,k,crit;u16 amount,hp;u32 after;Unit *a,*t;
 for(i=0;i<sizeof source_weapon_dual/sizeof source_weapon_dual[0];i++){
  const u8 *row=source_weapon_dual[i];combat_arena();a=&st.unit[U_RAMZA];
  a->eq[EQ_WEAPON]=a->eq[EQ_SHIELD]=row[0];st.support[U_RAMZA]=SUPPORT_TWO_SWORDS;
  CHECK(hand_count(U_RAMZA),row[2]);CHECK(hand_weapon(a,0),row[3]);CHECK(hand_weapon(a,1),row[4]);
 }
 for(i=0;i<2;i++){
  combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->eq[EQ_WEAPON]=i?33:67;a->eq[EQ_SHIELD]=i?59:38;
  a->pa=5;a->brave=60;st.support[U_RAMZA]=SUPPORT_TWO_SWORDS;t->statuses.innate[4]=16;unit_status_refresh(t);rng32=1;hp=t->hp;
  CHECK(hand_count(U_RAMZA),2);CHECK(attack_prediction(U_RAMZA,10)>0,1);
  expected_strike(U_RAMZA,10,0,&amount,&crit,&after);attack(U_RAMZA,10);
  for(k=0;k<2;k++){
   CHECK(st.hand,k);CHECK(st.hit,1);CHECK(st.crit,crit);CHECK(st.dmg,amount);CHECK(rng32==after,1);
   impact();if(!k){st.hand=1;expected_strike(U_RAMZA,10,0,&amount,&crit,&after);st.hand=0;}end_anim();
  }
  CHECK(t->hp<hp,1);for(k=0;k<8&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.action.phase,CTX_IDLE);
 }
}
static void facing_refactor(void)
{
 s16 x,z,ax,az,c;u8 face;static const s8 dx[4]={1,-1,0,0},dz[4]={0,0,1,-1};
 combat_arena();st.unit[10].x=st.unit[10].z=8;
 for(face=0;face<4;face++)for(x=-8;x<=7;x++)for(z=-8;z<=7;z++){
  st.unit[10].face=face;st.unit[U_RAMZA].x=8+x;st.unit[U_RAMZA].z=8+z;ax=x<0?-x:x;az=z<0?-z:z;c=0;
  if(ax>=az)c+=dx[face]*(x<0?-1:x>0);if(az>=ax)c+=dz[face]*(z<0?-1:z>0);
  CHECK(attack_side(U_RAMZA,10),!ax&&!az?SIDE_BACK:c>0?SIDE_FRONT:c<0?SIDE_BACK:SIDE_SIDE);
 }
}
static void trials_weapon_formulas(void){source_scalars();execution();response_rolls();expanded_hands();facing_refactor();}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);trials_weapon_formulas();printf("weapon_formulas %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();trials_weapon_formulas();BENCH_VALUE(sizeof(Unit));BENCH_VALUE(sizeof(State));BENCH_VALUE(sizeof(Work));BENCH_VALUE(sizeof(Save));BENCH_VALUE(sizeof(ModCtx));BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
