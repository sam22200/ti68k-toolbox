// Incremental graphs equal complete graphs, even across cold owner reloads.
#define AREA_UI_CHECK
#include "check_area_ai.c"
static u32 graph_hash(void)
{
 u16 cell,j;u32 h=17;
 for(cell=0;cell<MAP_N;cell++){
  h=(h<<1|h>>31)^W->en[cell];h=(h<<1|h>>31)^W->hw[cell];
  for(j=0;j<W->en[cell];j++)h=(h<<1|h>>31)^EDGE_ROW(cell)[j];
  for(j=0;j<4;j++)h=(h<<1|h>>31)^W->via[cell][j];
 }
 return h;
}
static void graph_trials(void)
{
 u8 mode,ready;u16 steps;u32 full,partial,seed;Unit before[UNIT_CAPACITY];
 f=malloc(sizeof *f);CHECK(f!=RT_NULL,1);if(!f)return;
 for(mode=0;mode<4;mode++){
  fixture(0,1);st.movement[0]=mode==1?502-485:mode==2?506-485:0;
  if(mode==3){st.unit[0].statuses.inflicted[2]=2;unit_status_refresh(&st.unit[0]);st.weather=3;}
  W->ej=255;W->move_who=0;movement_ready(0);full=graph_hash();
  // The live builder emits at most16 links across both landing layers.
  // A tighter reserve changes storage only, including node255 and via rows.
  CHECK(nav_reserve(MAP_N,3),1);CHECK(W->edge_shift,5);
  W->ej=255;W->move_who=0;movement_ready(0);CHECK(graph_hash()==full,1);
  {u16 node;for(node=0;node<MAP_N;node++)CHECK(W->en[node]<=32,1);}
  W->ej=255;W->move_who=0;W->edge_phase=0;seed=rng32;memcpy(before,st.unit,sizeof before);
  for(steps=0,ready=0;steps<260&&!ready;steps++){
#ifdef FFT_MODULES
   mod_id=0; // configuration must live in Work, not in an executing image
#endif
   ZB(60);ready=movement_poll(0);ZE(60);
  }
  CHECK(ready,1);CHECK(steps<=130,1);partial=graph_hash();CHECK(partial==full,1);
  CHECK(rng32==seed,1);CHECK(memcmp(before,st.unit,sizeof before)==0,1);
 }
 // A changed weather key cancels partial old work before it becomes searchable.
 fixture(0,1);W->ej=255;W->move_who=0;
 for(steps=0;steps<5;steps++)CHECK(movement_poll(0),0);
 st.weather=3;
 for(steps=0,ready=0;steps<260&&!ready;steps++)ready=movement_poll(0);
 CHECK(ready,1);partial=graph_hash();W->ej=255;W->move_who=0;movement_ready(0);CHECK(graph_hash()==partial,1);
 // A full-screen page may overwrite idle AI/ceiling scratch after Auto is
 // cancelled. Leaving it must discard an interrupted graph before reuse.
 fixture(0,1);W->ej=255;W->move_who=0;
 for(steps=0;steps<5;steps++)CHECK(movement_poll(0),0);
 CHECK(W->edge_phase!=0,1);memset(W->status,0x5a,sizeof W->status);
 st.mode=M_STATUS;st.back=M_MENU;st.page=st.status_info=0;rt_prev=0;rt_keys=K_ESC;
 status_keys();rt_keys=0;CHECK(st.mode,M_MENU);CHECK(W->edge_phase,0);CHECK(W->ej,255);CHECK(W->ai_k,0);
 CHECK(chapter_load(1),1);reset();free(f);
}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);graph_trials();printf("final_graph %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();graph_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
