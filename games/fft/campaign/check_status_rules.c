// Existing status hooks through the actual battle/AI; no renderer or hardware.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main status_runtime_main
#define game_render status_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void status_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
#include "x/core-status-rules/oracle/source.h"
static u16 checks,failures;static u32 digest=17;
static void check(u16 actual,u16 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^actual;
    if(actual!=expected){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,actual,expected);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)
static void reset(void){game_scenario(61);st.ai=AI_OFF;st.act=U_RAMZA;}
static void trials(void)
{
    u16 k;u8 i;
    for(k=0;k<sizeof source_evades/sizeof source_evades[0];k++){
        Unit *a,*t;reset();a=&st.unit[U_RAMZA];t=&st.unit[10];
        unit_status_seed(a,source_evades[k].dark?ST_DARK:0);unit_status_seed(t,source_evades[k].defend?ST_DEFEND:0);
        t->face=source_evades[k].side==0?3:source_evades[k].side==1?0:2;
        t->job=0x4a;t->eq[EQ_SHIELD]=source_evades[k].shield;
        CHECK(attack_side(U_RAMZA,10),source_evades[k].side);
        CHECK(status_physical_base(a->status,t->status),source_evades[k].base);
        CHECK(hit_chance(U_RAMZA,10),source_evades[k].hit);
        CHECK(skill_hit(U_RAMZA,10,0x94),source_evades[k].hit);
        CHECK(skill_hit(U_RAMZA,10,0x196),source_evades[k].hit);
        CHECK(skill_hit(U_RAMZA,10,0x93),100); // Dash does not use evasion
        // Committed class-only rolls use the identical status-modified base.
        // This checks the native RNG caller, not PS1 RNG sequence identity.
        if(source_evades[k].shield==255){
            for(i=1;i<=12;i++){
                u8 expected;u16 r,ev=source_evades[k].side==0?jobs[t->job].cev:0;
                // Accessory, weapon and shield stages draw even with zero evade;
                // the class-only comparison therefore uses the fourth draw.
                rand_seed(i);rand15();rand15();rand15();r=rand15();expected=((u32)r*source_evades[k].base>>15)>=ev;
                rand_seed(i);CHECK(roll_hit(U_RAMZA,10),expected);
            }
        }
    }
    reset();
    for(k=0;k<sizeof source_silence/sizeof source_silence[0];k++)
        CHECK(status_blocks_skill(source_silence[k].silence?ST_SILENCE:0,
            source_silence[k].ability,source_silence[k].set),source_silence[k].blocked);
    // None of the current execution records needs a Silence exception.
    // Rejected/unknown IDs never index a nonexistent secondary record.
    CHECK(status_blocks_skill(ST_SILENCE,NO_ABIL,255),0);
    CHECK(status_blocks_skill(ST_SILENCE,1,255),0);
    CHECK(can_use(U_RAMZA,0x100),0);
    // Existing Eye Drop removal restores the real preview/commit base and
    // preserves Silence. Its item is usable while silenced.
    {
        Unit *a=&st.unit[U_RAMZA],*t=&st.unit[10];
        unit_status_seed(a,ST_DARK|ST_SILENCE);unit_status_seed(t,0);t->job=0x4a;t->face=3;t->eq[EQ_SHIELD]=255;
        CHECK(hit_chance(U_RAMZA,10),90);CHECK(can_use(U_RAMZA,0x177),1);
        skill(U_RAMZA,U_RAMZA,0x177);CHECK(a->status,ST_DARK|ST_SILENCE);
        st.act_t=ACT_HIT;attack_step();CHECK(a->status,ST_SILENCE);
        CHECK(hit_chance(U_RAMZA,10),95);
    }
    reset();
    // Inject only the source silence-sensitive flag into an otherwise working
    // Wish. This is an integration fixture, not a claim that retail Wish is
    // silence-sensitive or that unsupported Fire is now implemented.
    {
        const u8 *original=common;u16 bytes=cj_word(common+CJ_LENGTH),steps=0;
        u8 *bank=malloc(bytes);Unit *a=&st.unit[U_RAMZA];u8 n;u32 seed;
        CHECK(bank!=RT_NULL,1);if(!bank)return;
        memcpy(bank,common,bytes);common=bank;
        bank[cj_word(bank+4+2*CJ_RANGE)+14U*0x98+5]|=0x20;
#ifdef FFT_MODULES
        mod_id=0;
#endif
        a->set=0x19;a->learned=0;
        for(i=0;i<16;i++)if(cj_set_ability(common,a->set,i)==0x98)a->learned|=(u16)1<<i;
        CHECK(a->learned!=0,1);unit_status_seed(a,ST_SILENCE);
        CHECK(can_use(U_RAMZA,0x98),0);CHECK(skill_hit(U_RAMZA,10,0x98),0);
        st.mode=M_MENU;st.pending[U_RAMZA].state=0;seed=rng32;
        skill(U_RAMZA,10,0x98);
        CHECK(st.mode,M_MENU);CHECK(st.pending[U_RAMZA].state,0);CHECK(rng32==seed,1);
        W->ai_k=0;st.acted=st.moved=0;
        while(!ai_think()&&++steps<1024){}
        CHECK(steps<1024,1);n=0;
        for(i=0;i<W->ai_nab;i++)if(W->ai_list[i]==0x98)n++;
        CHECK(n,0);
        // An action menu which bypasses Silence uses the source dispatch table.
        a->set=0x11;CHECK(cj_action_menu(common,a->set),4);CHECK(can_use(U_RAMZA,0x98),1);
        a->set=0x19;unit_status_seed(a,0);CHECK(can_use(U_RAMZA,0x98),1);
        unit_status_seed(a,ST_SILENCE);CHECK(can_use(U_RAMZA,0x170),1);
        skill(U_RAMZA,U_RAMZA,0x178);st.act_t=ACT_HIT;attack_step();
        CHECK(a->status,0);CHECK(can_use(U_RAMZA,0x98),1);
        common=original;
#ifdef FFT_MODULES
        mod_id=0;
#endif
        free(bank);
    }
}
#ifndef __m68k__
int main(void)
{
    sw_init(61);trials();printf("statusrules %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);
    return failures?1:0;
}
#else
void _main(void){status_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
