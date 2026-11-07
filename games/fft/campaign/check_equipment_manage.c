// Left-hand equipment, capability removal and unchanged serialized records.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main manage_runtime_main
#define game_render no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "manage.c"
static u16 checks,failures;static u32 hash=17;
static void check(u16 a,u16 b){checks++;hash=(hash<<1|hash>>31)^a;if(a!=b)failures++;}
#define CHECK(a,b) check((u16)(a),(u16)(b))
static void trials(void)
{
 SaveUnit *u=&save.unit[0];u8 ability,job,slot,k;u16 id;u8 found;
 CHECK(bank!=RT_NULL,1);
 for(ability=0;ability<8;ability++){
  found=0;
  for(job=74;job<94&&!found;job++)for(id=1;id<255&&!found;id++)for(slot=0;slot<5&&!found;slot++){
   memset(u->eq,0,5);u->job=job;u->type=0;set16(u->support,0);
   if(can_equip(u,slot,(u8)id))continue;
   set16(u->support,454+ability);
   if(!can_equip(u,slot,(u8)id))continue;
   found=1;save.inv[id]=1;equip(u,slot,(u8)id);CHECK(u->eq[slot],id);CHECK(save.inv[id],0);
   mg.screen=S_PICK;mg.back=S_SET;mg.slot=2;mg.row=mg.top=mg.unit=0;mg.pick_n=1;mg.pick[0]=0;
   rt_keys=K_ENTER;rt_prev=0;game_update();rt_keys=0;
   CHECK(b16(u->support),0);CHECK(u->eq[slot],0);CHECK(save.inv[id],1);
  }
  CHECK(found,1);
 }
 memset(u->eq,0,5);u->job=75;u->type=0;set16(u->support,456);u->eq[3]=19;change_job(u,1);CHECK(u->eq[3],19);
 set16(u->support,0);u->eq[3]=0;save.inv[19]=0;equip(u,3,19);CHECK(save.inv[19],0);CHECK(u->eq[3],0);
 CHECK(sizeof(SaveUnit),176);CHECK(sizeof(Save),SAVE_V3_BYTES+SAVE_DISCOVERY_BYTES);CHECK(cj_valid(common,cj_word(common+CJ_LENGTH)),1);
}

#ifndef __m68k__
int main(void){sw_init(0);trials();printf("equipment_manage %08lX failures %u checks %u\n",(unsigned long)hash,failures,checks);return failures!=0;}
#else
void _main(void){manage_runtime_main();trials();BENCH_VALUE(hash);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
