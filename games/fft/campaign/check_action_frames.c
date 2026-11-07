// Exact before/after action pictures; no input or hardware emulation.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main action_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
static void trials(void)
{
    u8 kind,i;u16 n;u32 hash;char forecast[32];
    for(kind=0;kind<6;kind++){
        game_scenario(61);st.ai=AI_OFF;st.act=U_RAMZA;
        st.unit[10].hp=kind==3?0:10;unit_status_seed(&st.unit[10],ST_POISON|ST_DARK);
        if(kind==1)st.unit[10].hp=st.unit[10].max_hp-1;
        if(!kind){attack(U_RAMZA,10);st.dmg=4;}
        if(kind==1)skill(U_RAMZA,10,0x170);
        if(kind==2)skill(U_RAMZA,10,0x95);
        if(kind==3){skill(U_RAMZA,10,0x17D);st.dmg=7;}
        if(kind==4){st.unit[U_RAMZA].hp=1;skill(U_RAMZA,10,0x98);}
        if(kind==5)defend(U_RAMZA);
        hash=17;
        if(kind<5){
            forecast_text(U_RAMZA,10,st.abil,forecast);
            for(n=0;forecast[n];n++)hash=(hash<<1|hash>>31)^(u8)forecast[n];
        }
        st.hit=1;st.push=0;
        while(st.act_t<ACT_HIT+3)attack_step();
        for(i=0;i<16;i++){W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;game_render();}
        for(n=0;n<RT_PSIZE;n++){
            hash=(hash<<1|hash>>31)^((const u8 *)rt_light)[n];
            hash=(hash<<1|hash>>31)^((const u8 *)rt_dark)[n];
        }
#ifdef __m68k__
        BENCH_VALUE(hash);
#else
        printf("actionframe %u %08lX\n",kind,(unsigned long)hash);
#endif
    }
}
#ifndef __m68k__
int main(void){sw_init(61);trials();return 0;}
#else
void _main(void){action_runtime_main();trials();}
#endif
