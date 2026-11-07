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
static void check(u16 a,u16 b){checks++;hash=(hash<<1|hash>>31)^a;if(a!=b){failures++;
#ifndef __m68k__
printf("check%u: %u expected%u\n",checks,a,b);
#endif
}}
#define CHECK(a,b) check((u16)(a),(u16)(b))
static void trials(void)
{
    SaveUnit *u=&save.unit[0];Save snapshot;u8 i;
    CHECK(bank!=RT_NULL,1);u->job=1;u->type=2;u->eq[EQ_WEAPON]=19;u->eq[EQ_SHIELD]=0;
    set16(u->support,0);CHECK(can_equip(u,EQ_SHIELD,1),0);
    set16(u->support,SUPPORT_TWO_SWORDS);CHECK(can_equip(u,EQ_SHIELD,1),1);
    save.inv[1]=1;equip(u,EQ_SHIELD,1);CHECK(save.inv[1],0);CHECK(u->eq[EQ_SHIELD],1);
    // Exercise the real support picker: removing the capability returns the
    // incompatible weapon to stock, without changing the other hand.
    mg.screen=S_PICK;mg.back=S_SET;mg.slot=2;mg.row=mg.top=mg.unit=0;mg.pick_n=1;mg.pick[0]=0;
    rt_keys=K_ENTER;rt_prev=0;game_update();rt_keys=0;
    CHECK(b16(u->support),0);CHECK(u->eq[EQ_SHIELD],0);CHECK(u->eq[EQ_WEAPON],19);CHECK(save.inv[1],1);
    set16(u->support,SUPPORT_TWO_SWORDS);equip(u,EQ_SHIELD,1);change_job(u,3);
    CHECK(u->eq[EQ_SHIELD],0);CHECK(save.inv[1],1);
    u->job=89;u->type=0;set16(u->support,0);u->eq[EQ_WEAPON]=1;
    CHECK(can_equip(u,EQ_SHIELD,11),1); // source innate trait, no equipped support
    snapshot=save;
    CHECK(sizeof(SaveUnit),176);CHECK(sizeof(Save),SAVE_V3_BYTES+SAVE_DISCOVERY_BYTES);
    CHECK(cj_valid(common,cj_word(common+CJ_LENGTH)),1);
    for(i=0;i<20;i++)hash=(hash<<1|hash>>31)^cj_section(common,CJ_SPECIAL)[168+CJ_SETS+i];
    CHECK(memcmp(&snapshot,&save,sizeof save),0);
}
#ifndef __m68k__
int main(void){sw_init(0);trials();printf("handsmanage %08lX failures %u checks %u\n",(unsigned long)hash,failures,checks);return failures!=0;}
#else
void _main(void){manage_runtime_main();trials();BENCH_VALUE(hash);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
