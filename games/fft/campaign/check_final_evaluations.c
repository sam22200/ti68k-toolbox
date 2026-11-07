// Real planner decisions with collateral, returned_spell weapon spells and innate traits.
#define DIRECT_RENDER_CHECK
#include "check_direct_interception.c"
static s16 evaluate(u8 ally,u8 returned_spell,u8 receiver_reflect)
{
 u16 n;u8 ready=0;Unit before[UNIT_CAPACITY];
 panel_arena(!returned_spell);st.unit[U_RAMZA].move=0;st.unit[U_RAMZA].defend=0;st.unit[U_RAMZA].learned=0;
 if(returned_spell){
  st.unit[U_RAMZA].eq[EQ_WEAPON]=53;st.unit[8]=st.unit[10];unit_place(&st.unit[8],third);
  st.unit[10].statuses.innate[4]|=2;unit_status_refresh(&st.unit[10]);
  if(receiver_reflect){st.unit[8].statuses.innate[4]|=2;unit_status_refresh(&st.unit[8]);}
 }else{st.unit[U_RAMZA].eq[EQ_WEAPON]=19;if(ally)st.unit[8].hp=1;}
 st.unit[8].team=ally?TEAM_PLAYER:TEAM_ENEMY;
 memcpy(before,st.unit,sizeof before);st.act=U_RAMZA;st.ai=AI_THINK;st.moved=st.acted=0;W->ai_k=0;rng32=1;
 for(n=0;n<1024&&!ready;n++)ready=ai_think();
 CHECK(ready,1);CHECK(memcmp(before,st.unit,sizeof before)==0,1);
 CHECK(W->ai_weapon_left,0);CHECK(W->ai_panel_left,0);
 if(!returned_spell&&ally){CHECK(st.ai_ab,AI_NONE);CHECK(W->ai_n,0);}
 else{CHECK(st.ai_ab,NO_ABIL);CHECK(W->ai_n>0,1);}
 {s16 score=W->ai_n?W->ai_rank[0].pri:AI_BAD;panel_restore();return score;}
}
static void evaluation_trials(void)
{
 s16 hostile,allied,twice;
 CHECK(evaluate(0,0,0)>0,1);CHECK(evaluate(1,0,0),AI_BAD);
 hostile=evaluate(0,1,0);allied=evaluate(1,1,0);twice=evaluate(1,1,1);
 CHECK(hostile>allied,1);CHECK(allied,twice); // No second bounce at an innate Reflect receiver.
}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);evaluation_trials();printf("final_evaluations %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();evaluation_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
