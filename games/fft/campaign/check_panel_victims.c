// Original panel enumeration, frozen weapon hands and real preview recipients.
#define COMBAT_UI_CHECK
#include "check_combat.c"
#include "x/core-panel-victims/oracle/source.h"
static void panel_source(void)
{
 u16 size,i,mask;u8 j,n;const u8 *data=rt_file("fpanel",&size),*r;
 CHECK(data!=RT_NULL,1);CHECK(size>=PANEL_SOURCE_CASES*PANEL_SOURCE_STRIDE,1);
 for(i=0;i<PANEL_SOURCE_CASES;i++){
  r=data+i*PANEL_SOURCE_STRIDE;mask=core_weapon_panel(r,cj_word(r+13),r[15],13);
  CHECK(mask,cj_word(r+16));n=0;
  for(j=0;j<13;j++)if(mask&((u16)1<<j)){CHECK(j,r[19+n]);n++;}
  CHECK(n,r[18]);
 }
}
static u8 panel_surface[2048],saved_hs[MAP_NMAX],saved_flags;static const u8 *saved_surface;
static void panel_arena(u8 crowded)
{
 u16 i;combat_arena();st.golem[0]=st.golem[1]=0;st.unit[U_RAMZA].eq[EQ_WEAPON]=19;
 saved_surface=W->surface;saved_flags=W->surface_flags;memcpy(saved_hs,W->hs,sizeof saved_hs);
 memcpy(panel_surface,W->surface,(u16)MAP_W*MAP_H*8);W->surface=panel_surface;
 for(i=0;i<(u16)MAP_W*MAP_H;i++){panel_surface[i*8]=4;panel_surface[i*8+1]=0;panel_surface[i*8+3]=0;panel_surface[i*8+4]=0;panel_surface[i*8+7]=128;}
 for(i=0;i<MAP_N;i++)W->hs[i]=8;
 if(crowded){st.unit[8]=st.unit[10];unit_place(&st.unit[8],second);}
}
static void panel_restore(void){W->surface=saved_surface;W->surface_flags=saved_flags;memcpy(W->hs,saved_hs,sizeof saved_hs);}
static void panel_no_push(void){u8 i;st.push=0;for(i=0;i<NUNIT;i++)W->weapon_panel.victim[i].push=0;}
static void panel_finish(void)
{
 u16 i;for(i=0;i<200&&st.action.phase!=CTX_IDLE;i++){if(st.act_t<ACT_HIT)impact();else end_anim();}CHECK(st.action.phase,CTX_IDLE);
}
static void panel_capture(WeaponVictim *p)
{
 p->amount=st.dmg;p->packet=st.self;p->kind=st.effect_kind;p->hit=st.hit;p->critical=st.crit;
}
static u32 panel_control(u8 target,u32 seed,WeaponVictim *p)
{
 panel_arena(0);
 if(target==8){st.unit[8]=st.unit[10];st.unit[10].x=255;st.unit[10].dc=DC_GONE;unit_place(&st.unit[8],second);}
 rng32=seed;attack(U_RAMZA,target);panel_capture(p);seed=rng32;panel_restore();return seed;
}
static void frozen_panel(void)
{
 WeaponVictim expected[2];u32 seed,after;u16 hp8,hp10,jp;
 seed=panel_control(8,1,&expected[0]);after=panel_control(10,seed,&expected[1]);
 panel_arena(1);rng32=1;hp8=st.unit[8].hp;hp10=st.unit[10].hp;jp=st.unit[U_RAMZA].jp;
 attack(U_RAMZA,10);panel_no_push();CHECK(st.tgt,8);CHECK(st.action.cell,second);CHECK(st.action.left,WEAPON_PANEL_MORE);
 CHECK(rng32==after,1);CHECK(st.dmg,expected[0].amount);CHECK(st.self,expected[0].packet);CHECK(st.hand,0);
 impact();CHECK(st.unit[8].hp,hp8-expected[0].amount);CHECK(st.unit[10].hp,hp10);CHECK(st.unit[U_RAMZA].jp,jp);CHECK(st.gain,0);
 impact();CHECK(st.unit[8].hp,hp8-expected[0].amount);
 // This late formula change cannot alter a result already prepared for this hand.
 st.support[10]=SUPPORT_DEFENSE_UP;status(10,22);seed=rng32;end_anim();
 CHECK(st.tgt,10);CHECK(st.hand,0);CHECK(st.action.left,0);CHECK(st.dmg,expected[1].amount);CHECK(rng32==seed,1);
 impact();CHECK(st.unit[10].hp,hp10-expected[1].amount);panel_finish();CHECK(st.unit[U_RAMZA].jp>jp,1);panel_restore();
}
static void hands_and_recoil(void)
{
 u8 i;u16 hp8,hp10;
 panel_arena(1);st.unit[U_RAMZA].eq[EQ_SHIELD]=19;st.support[U_RAMZA]=SUPPORT_TWO_SWORDS;rng32=1;
 CHECK(hand_count(U_RAMZA),2);attack(U_RAMZA,10);
 for(i=0;i<4;i++){
  panel_no_push();CHECK(st.tgt,(i&1)?10:8);CHECK(st.hand,i>>1);
  impact();if(i<3){CHECK(st.gain,0);end_anim();}
 }
 CHECK(st.action.left,0);CHECK(st.unit[8].hp<999,1);CHECK(st.unit[10].hp<999,1);panel_finish();panel_restore();
 // Recoil moves the first victim out of the fixed panel for the next hand.
 panel_arena(1);st.unit[U_RAMZA].eq[EQ_SHIELD]=19;st.support[U_RAMZA]=SUPPORT_TWO_SWORDS;rng32=1;
 attack(U_RAMZA,10);panel_no_push();st.push=1;st.push_cell=third;impact();hp8=st.unit[8].hp;
 end_anim();CHECK(st.tgt,10);CHECK(st.hand,0);panel_no_push();impact();hp10=st.unit[10].hp;
 end_anim();CHECK(st.tgt,10);CHECK(st.hand,1);CHECK(st.action.left,0);panel_no_push();impact();CHECK(st.unit[8].hp,hp8);CHECK(st.unit[10].hp<hp10,1);panel_finish();panel_restore();
}
static void responses_and_late_guards(void)
{
 u16 stock,amount;u8 i;
 panel_arena(1);st.unit[8].team=st.unit[10].team=TEAM_PLAYER;st.reaction[8]=st.reaction[10]=REACTION_POTION;
 st.bag[0]=3;st.bag[1]=st.bag[2]=0;rng32=1;attack(U_RAMZA,10);panel_no_push();impact();
 CHECK(st.action.pending,(u16)1<<8);CHECK(st.primary.running,0);end_anim();CHECK(st.tgt,10);CHECK(st.bag[0],3);
 impact();CHECK(st.action.pending,((u16)1<<8)|((u16)1<<10));stock=st.bag[0];panel_finish();CHECK(st.bag[0],stock-2);panel_restore();
 // A disappearing later victim rejects at impact, without a second formula draw.
 panel_arena(1);rng32=1;attack(U_RAMZA,10);panel_no_push();impact();st.unit[10].hp=0;
 end_anim();CHECK(st.tgt,10);impact();CHECK(st.unit[10].hp,0);CHECK(st.hit,0);panel_finish();panel_restore();
 // Each frozen recipient retains its own deferred free spell; no nested overlay.
 panel_arena(1);rng32=1;attack(U_RAMZA,10);panel_no_push();
 st.self=16;st.dmg=1;st.hit=1;W->weapon_panel.victim[10].packet=16;W->weapon_panel.victim[10].amount=1;W->weapon_panel.victim[10].hit=1;
 stock=st.unit[U_RAMZA].mp;impact();CHECK(st.unit[8].hp<998,1);CHECK(st.unit[10].hp,999);
 end_anim();CHECK(st.tgt,10);impact();CHECK(st.unit[10].hp<998,1);CHECK(st.unit[U_RAMZA].mp,stock);panel_finish();panel_restore();
 // Frozen targets finish the hand after reversed drain kills the actor. A
 // later positive drain cannot resurrect that actor without a revive action.
 panel_arena(1);st.unit[U_RAMZA].eq[EQ_WEAPON]=61;st.unit[U_RAMZA].hp=1;status(8,3);rng32=1;
 attack(U_RAMZA,10);panel_no_push();impact();CHECK(st.unit[U_RAMZA].hp,0);
 end_anim();CHECK(st.tgt,10);amount=st.unit[10].hp;impact();CHECK(st.unit[10].hp<amount,1);CHECK(st.unit[U_RAMZA].hp,0);panel_finish();panel_restore();
 (void)i;
}
static u8 panel_preview(u8 a,u8 t,AbId ab,char *text)
{
#ifdef FFT_MODULES
 ModCtx *c;u8 scratch[sizeof st.formula],n;memcpy(scratch,&st.formula,sizeof scratch);
 st.formula.actor=a;st.formula.target=t;st.formula.ability=ab;mod_call(MOD_PLANNER,3,0);
 mod_call(MOD_NUMBERS,255,0);c=mod_context(MOD_NUMBERS);c->forecast_out=text;n=mod_call(MOD_NUMBERS,5,0);
 memcpy(&st.formula,scratch,sizeof scratch);return n;
#else
 return forecast_text(a,t,ab,text);
#endif
}
static void previews_and_interception(void)
{
 char a[28],b[28];u16 hp;u8 n;u32 seed;State before;
 panel_arena(1);st.unit[U_RAMZA].eq[EQ_WEAPON]=99;unit_place(&st.unit[10],third);
 st.unit[9]=st.unit[8];unit_place(&st.unit[9],second);hp=st.unit[10].hp;seed=rng32;before=st;
 n=panel_preview(U_RAMZA,10,NO_ABIL,a);CHECK(W->preview_target,8);CHECK(W->preview_count,2);CHECK(a[n],0);CHECK(rng32==seed,1);CHECK(memcmp(&before,&st,sizeof st)==0,1);
 panel_preview(U_RAMZA,8,NO_ABIL,b);CHECK(memcmp(a,b,n+1)==0,1);CHECK(a[n-2],'x');CHECK(a[n-1],'2');
 rng32=1;attack(U_RAMZA,10);panel_no_push();CHECK(st.tgt,8);CHECK(st.action.cell,third);impact();end_anim();CHECK(st.tgt,9);impact();CHECK(st.unit[10].hp,hp);panel_finish();panel_restore();
 // Blocked previews are terminated and name no recipient.
 panel_arena(0);st.unit[U_RAMZA].eq[EQ_WEAPON]=99;unit_place(&st.unit[10],third);
 panel_surface[((u16)(W->grid_row[TILE_Z[second]]+TILE_X[second])<<3)]=20;
 memset(a,165,sizeof a);seed=rng32;n=panel_preview(U_RAMZA,10,NO_ABIL,a);CHECK(n,7);CHECK(a[0],'M');CHECK(a[n],0);CHECK(W->preview_target,255);CHECK(W->preview_count,0);CHECK(rng32==seed,1);panel_restore();
}
static void direct_panel(void)
{
 u16 c,hp;u8 i,home;u32 seed;char text[28];
 panel_arena(1);
 for(c=0;c+3<MAP_N;c++)if(TILE_Z[c]==TILE_Z[c+3]&&map_tiles[c].walk&&map_tiles[c+1].walk&&map_tiles[c+2].walk&&map_tiles[c+3].walk)break;
 CHECK(c+3<MAP_N,1);home=(u8)c;unit_place(&st.unit[U_RAMZA],home);unit_place(&st.unit[8],c+3);unit_place(&st.unit[10],c+3);
 st.unit[U_RAMZA].eq[EQ_WEAPON]=71;seed=rng32;
 panel_preview(U_RAMZA,10,NO_ABIL,text);CHECK(W->preview_target,8);CHECK(W->preview_count,2);CHECK(rng32==seed,1);
 // Endpoint cooccupants form the selected panel. Intermediate bodies now
 // supply a recipient; they no longer turn an otherwise valid shot into a miss.
 st.unit[9]=st.unit[10];unit_place(&st.unit[9],c+1);CHECK(battle_line_clear(U_RAMZA,10),1);st.unit[9].x=255;st.unit[9].dc=DC_GONE;
 memset(W->ai_m,0,sizeof W->ai_m);W->ai_m[home]=1;W->ai_weapon_left=512;W->ai_melee_at=0;st.act=U_RAMZA;
 for(c=0;c<128&&W->ai_weapon_left;c++){
#ifdef FFT_MODULES
  mod_call(MOD_PLANNER,0,0);
#else
  melee_cache();
#endif
 }
 CHECK(W->ai_weapon_left,0);CHECK(W->ai_melee_mask[home]&((u16)1<<10),(u16)1<<10);CHECK(rng32==seed,1);
 rng32=1;attack(U_RAMZA,10);CHECK(st.tgt,8);hp=st.unit[8].hp;impact();CHECK(st.unit[8].hp<hp,1);
 end_anim();CHECK(st.tgt,10);hp=st.unit[10].hp;impact();CHECK(st.unit[10].hp<hp,1);panel_finish();panel_restore();(void)i;
}
static void panel_trials(void){panel_source();frozen_panel();hands_and_recoil();responses_and_late_guards();previews_and_interception();direct_panel();}
#ifndef PANEL_RENDER_CHECK
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);panel_trials();printf("panel_victims %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();panel_trials();BENCH_VALUE(sizeof(Unit));BENCH_VALUE(sizeof(State));BENCH_VALUE(sizeof(Work));BENCH_VALUE(sizeof(Save));BENCH_VALUE(sizeof(ModCtx));BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
