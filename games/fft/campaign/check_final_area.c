// Candidate-origin Reflect utility and mutable versus innate Frog cancellation.
#define AREA_UI_CHECK
#include "check_area_ai.c"
#include "../status_ai.h"
static u8 final_area_step(void)
{
#ifdef FFT_MODULES
 return (u8)mod_call(W->ai_k==4||W->ai_k==8?MOD_AI_PREP:MOD_AREA_AI,0,0);
#else
 return area_ai_step();
#endif
}
static void reflect_area(u8 ally,u8 twice,u8 second_origin)
{
 u16 n;u8 done=0;u32 seed;Unit before[UNIT_CAPACITY];AreaAi *q=&W->area_ai;
 fixture(0,0);put_unit(0,0,2,0,TEAM_PLAYER);put_unit(1,1,2,0,TEAM_ENEMY);put_unit(2,2,2,0,ally?TEAM_PLAYER:TEAM_ENEMY);
 st.unit[0].set=11;st.unit[1].statuses.innate[4]=2;unit_status_refresh(&st.unit[1]);
 if(twice){st.unit[2].statuses.innate[4]=2;unit_status_refresh(&st.unit[2]);}
 if(second_origin)put_unit(3,2,3,0,TEAM_PLAYER);
 W->ab=16;W->ab_k=0;W->ab_best.pri=AI_BAD;W->ai_no=second_origin?2:1;
 W->ai_ord[0]=(u8)node_at(0,2,0);W->ai_ord[1]=(u8)node_at(0,1,0);
 memset(W->ai_gain,0,sizeof W->ai_gain);W->ai_base=0;W->ai_self=0;W->ai_k=4;
 seed=rng32;memcpy(before,st.unit,sizeof before);
 for(n=0;n<512&&!done;n++)done=final_area_step();
 CHECK(done,1);CHECK(rng32==seed,1);CHECK(memcmp(before,st.unit,sizeof before)==0,1);
 CHECK(q->reflect&2,2);CHECK(q->hit[1]>0,1);
 CHECK(q->gain[1],q->returned_gain[second_origin?3:2]);
 CHECK(q->gain[1]>0,!ally&&!second_origin);CHECK(q->gain[1]<0,ally||second_origin);
}
static void final_area_trials(void)
{
 u8 ally,twice;f=malloc(sizeof *f);CHECK(f!=RT_NULL,1);if(!f)return;
 for(ally=0;ally<2;ally++)for(twice=0;twice<2;twice++)reflect_area(ally,twice,0);
 reflect_area(0,0,1);
 fixture(0,0);st.unit[1].statuses.inflicted[2]=2;unit_status_refresh(&st.unit[1]);
 CHECK(status_ai_gain(1,0x202,1),48);CHECK(status_ai_gain(1,0x202,0),(u16)-48);
 st.unit[1].statuses.innate[2]=2;CHECK(status_ai_gain(1,0x202,1),0);
 CHECK(chapter_load(1),1);reset();free(f);
}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);final_area_trials();printf("final_area %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();final_area_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
