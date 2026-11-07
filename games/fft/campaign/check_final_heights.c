// Unchanged retail height producer, including live/KO shared-cell donors.
#define DIRECT_RENDER_CHECK
#include "check_direct_interception.c"
#include "../target_height.inc"
#include "x/core-final-combat/oracle/source.h"
static void height_unit(u8 who,u8 state,u8 traits,u8 innate)
{
 Unit *u=&st.unit[who];memset(&u->statuses,0,sizeof u->statuses);
 u->hp=state==1||state==3?0:999;u->dc=state==3?DC_CRYSTAL:3;
 if(state==1)u->statuses.inflicted[0]=32;
 if(state==2)u->statuses.inflicted[1]=128;
 if(state==3)u->statuses.inflicted[0]=64;
 if(innate)u->statuses.innate[2]=traits;else u->statuses.inflicted[2]=traits;
 unit_status_refresh(u);
}
static void height_trials(void)
{
 u16 size,i,at;const u8 *data=rt_file("fheight",&size),*r;MeleeSpan span;u32 seed;
 CHECK(data!=RT_NULL,1);CHECK(size>=HEIGHT_SOURCE_CASES*HEIGHT_SOURCE_STRIDE,1);
 for(i=0;i<HEIGHT_SOURCE_CASES;i++){
  r=data+i*HEIGHT_SOURCE_STRIDE;panel_arena(0);st.unit[8]=st.unit[U_RAMZA];
  if(r[0])unit_place(&st.unit[8],first);else{st.unit[8].x=255;st.unit[8].dc=DC_GONE;}
  height_unit(U_RAMZA,r[1],r[3],i&1);
  if(r[0])height_unit(8,r[2],r[4],i&1);
  {u16 move=r[5]|((u16)r[6]<<8);st.movement[U_RAMZA]=st.movement[8]=move?(u8)(move-485):0;}
  at=direct_column(first);panel_surface[at]=4;panel_surface[at+1]=(r[7]<<5)|1;
  seed=rng32;span=melee_span(U_RAMZA);CHECK(span.id!=255,r[8]);
  if(r[8]){CHECK(span.id,U_RAMZA);CHECK(span.feet,r[9]);CHECK(span.top,r[10]);}
  CHECK(rng32==seed,1);panel_restore();
 }
 // Actual preparation rejects a live target whose borrowed donor is KO.
 panel_arena(1);height_unit(8,1,0,0);rng32=1;seed=rng32;
 attack(U_RAMZA,10);CHECK(st.hit,0);CHECK(rng32==seed,1);panel_finish();panel_restore();
}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);height_trials();printf("final_heights %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();height_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
