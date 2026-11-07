// Full native13-slot envelope: twelve occupants of one adjacent panel.
#define PANEL_RENDER_CHECK
#include "check_panel_victims.c"
static void full_panel(void)
{
 u8 old_count,i,target=0;u16 hit_count=0;u32 seed;
#ifdef __m68k__
 u32 start;
#endif
 panel_arena(0);old_count=encounter_units;encounter_units=13;
 for(i=0;i<NUNIT;i++)if(i!=U_RAMZA){st.unit[i]=st.unit[10];unit_place(&st.unit[i],second);st.reaction[i]=0;}
 st.act=U_RAMZA;rng32=1;seed=rng32;
#ifdef FFT_MODULES
 mod_id=0;
#endif
#ifdef __m68k__
 start=BENCH_CYCLES;
#endif
 attack(U_RAMZA,10);
#ifdef __m68k__
 BENCH_VALUE(BENCH_CYCLES-start);
#endif
 CHECK(st.action.left,WEAPON_PANEL_MORE);CHECK(st.tgt,0);
 CHECK(rng32!=seed,1);seed=rng32;
 for(i=0;i<12;i++){
  if(target==U_RAMZA)target++;
  CHECK(st.tgt,target);CHECK(st.hand,0);panel_no_push();impact();hit_count++;
  CHECK(st.unit[target].hp<999,1);
  if(i<11){CHECK(st.gain,0);end_anim();CHECK(rng32==seed,1);}
  target++;
 }
 CHECK(hit_count,12);CHECK(st.action.left,0);CHECK(W->weapon_panel.todo,0);panel_finish();
 encounter_units=old_count;
#ifdef FFT_MODULES
 mod_id=0;
#endif
 panel_restore();
}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);full_panel();printf("panel_victims_capacity %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();full_panel();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
