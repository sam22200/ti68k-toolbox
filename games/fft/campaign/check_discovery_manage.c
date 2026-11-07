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
#include "deploy.c"
static u16 checks,failures;static u32 hash=17;
static void check(u16 a,u16 b){checks++;hash=(hash<<1|hash>>31)^a;if(a!=b){failures++;
#ifndef __m68k__
printf("check%u: %u expected%u\n",checks,a,b);
#endif
}}
#define CHECK(a,b) check((u16)(a),(u16)(b))
static void trials(void)
{
 SaveUnit *u;SaveUnit before;Stats values;UnitCard card;u8 c;u16 i;
 CHECK(bank!=RT_NULL,1);CHECK(flowbank!=RT_NULL,1);u=&save.unit[7];*u=save.unit[1];save.n=8;
 u->type=3;u->job=94;u->gfx=255;memset(u->eq,0,5);before=*u;
 CHECK(!memcmp(unit_name(u),"Chocobo",8),1);CHECK(picture(u,0),255);
 for(c=0;c<20;c++)CHECK(unlocked(u,c,RT_NULL),0);
 change_job(u,1);CHECK(memcmp(u,&before,sizeof before),0);CHECK(learn(u,0),0);equip(u,EQ_WEAPON,19);CHECK(memcmp(u,&before,sizeof before),0);
 values=stats(u);CHECK(values.move,cj_section(common,CJ_JOBS)[14U*94+10]);management_card(&card,u,&values);CHECK(card.portrait==RT_NULL,1);
 mg.unit=7;mg.screen=S_MENU;mg.row=3;rt_keys=K_ENTER;rt_prev=0;management_update();CHECK(mg.screen,S_MENU);CHECK(memcmp(u,&before,sizeof before),0);
 mg.screen=S_STATUS;management_render();for(i=0;i<RT_PSIZE;i++){hash=(hash<<1|hash>>31)^((u8 *)rt_light)[i];hash=(hash<<1|hash>>31)^((u8 *)rt_dark)[i];}
 dp.unit=7;CHECK(deploy_place(),0);CHECK(dp.note,6);CHECK(deploy_where(7),255);game_render();for(i=0;i<RT_PSIZE;i++){hash=(hash<<1|hash>>31)^((u8 *)rt_light)[i];hash=(hash<<1|hash>>31)^((u8 *)rt_dark)[i];}
 CHECK(sizeof(Save),SAVE_V3_BYTES+SAVE_DISCOVERY_BYTES);
}
#ifndef __m68k__
int main(void){sw_init(0);trials();printf("discovery_manage %08lX failures %u checks %u\n",(unsigned long)hash,failures,checks);return failures!=0;}
#else
void _main(void){manage_runtime_main();trials();BENCH_VALUE(hash);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
