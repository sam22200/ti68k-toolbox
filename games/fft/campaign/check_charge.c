// Shared engine source-preview, scheduler and bounded menu checks on PC/68000.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main chapter_runtime_main
#define game_render charge_no_render // rules and menu-cache checks do not run the full compositor
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void charge_no_render(void) {}
#undef _main
#define _main __main
#undef off
#endif
#include "gariland.c"
#include "x/charge/source.h"
#ifndef __m68k__
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif

static u16 failures;
static u32 digest = 17;
static void check(u16 actual, u16 expected, u16 line)
{
    digest = (digest << 1 | digest >> 31) ^ actual;
    if (actual != expected) {
        failures++;
#ifndef __m68k__
        printf("line %u: %u != %u\n", line, actual, expected);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)

static void trials(void)
{
    u8 i,k;
    for (i=0;i<sizeof(charge_oracle)/sizeof(charge_oracle[0]);i++) {
        Unit *a,*t;
        AbId ab=charge_oracle[i].ability;
        game_scenario(61);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->pa=charge_oracle[i].pa;a->sp=charge_oracle[i].sp;a->brave=charge_oracle[i].br;
        a->eq[EQ_WEAPON]=charge_oracle[i].weapon;a->zodiac=charge_oracle[i].az;a->female=charge_oracle[i].af;
        t->zodiac=charge_oracle[i].tz;t->female=charge_oracle[i].tf;t->face=2;
        st.pending[10].state=charge_oracle[i].charging;
        unit_status_action(t,charge_oracle[i].charging?8:0);
        CHECK(skill_hit(U_RAMZA,10,ab),charge_oracle[i].hit);
        CHECK(skill_amount(U_RAMZA,10,ab,0),charge_oracle[i].amount);
#ifdef __m68k__
        // The resident-AI path and the standalone FORMULA image must agree.
        mod_call(MOD_AI,2,0);
#endif
        CHECK(skill_amount(U_RAMZA,10,ab,0),charge_oracle[i].amount);
        st.pending[10].state=0;unit_status_action(t,0);st.act=U_RAMZA;st.ai=0;
        skill(U_RAMZA,10,ab);
        CHECK(st.pending[U_RAMZA].ability,ab);
        CHECK(st.pending[U_RAMZA].ticks,charge_oracle[i].ct);
        CHECK(st.moved | st.acted<<1,3);
    }
    // Every source power, including nonconsecutive +7/+10/+20, retains a tile,
    // waits on CT rather than displayed frames, and can be interrupted.
    for (k=0;k<8;k++) {
        u8 ticks=charge_oracle[k*6].ct;
        AbId ab=(AbId)(0x196+k);
        game_scenario(61);st.ai=0;st.act=U_RAMZA;skill(U_RAMZA,10,ab);
        st.mode=M_MENU;
        for(i=0;i<20;i++) { game_update(); CHECK(st.pending[U_RAMZA].ticks,ticks); }
        for(i=0;i<NUNIT;i++) { st.unit[i].sp=1;st.unit[i].ct=0; }
        st.unit[10].ct=100;
        CHECK(ct_next(),10);CHECK(st.pending[U_RAMZA].ticks,ticks);
        CHECK(ct_next(),254);CHECK(st.pending[U_RAMZA].ticks,0);
        charge_resolve();CHECK(st.tgt,10);CHECK(st.abil,ab);CHECK(st.open_resolving,1);
        game_scenario(61);st.ai=0;skill(U_RAMZA,10,ab);unit_place(&st.unit[10],(u8)node_at(st.unit[10].x+1,st.unit[10].z,0));
        for(i=0;i<NUNIT;i++) { st.unit[i].sp=1;st.unit[i].ct=0; }
        turn_start();CHECK(st.open_resolving,0);CHECK(st.pending[U_RAMZA].state,0);
        for(i=0;i<3;i++) {
            game_scenario(61);st.ai=0;skill(U_RAMZA,10,ab);
            if(!i)st.unit[U_RAMZA].hp=0;
            else if(i==1)unit_place(&st.unit[U_RAMZA],(u8)node_at(st.unit[U_RAMZA].x+1,st.unit[U_RAMZA].z,0));
            else st.unit[U_RAMZA].eq[EQ_WEAPON]=1;
            CHECK(pending_next(),255);CHECK(st.pending[U_RAMZA].state,0);
        }
    }
    // Eight actions, all cursor positions, downward and wrap/upward. The menu
    // keeps its existing 47-row buffers and shows the selected source ID.
    game_scenario(61);st.unit[U_RAMZA].set=8;st.unit[U_RAMZA].learned=255;
    st.mode=M_SKILLS;st.skill=0;
    for(k=0;k<16;k++) {
        u8 top;
        CHECK(st.skill,k<8?k:15-k);
        top=(u8)SKILL_TOP;CHECK(st.skill>=top && st.skill<top+SKILL_ROWS,1);
        W->win_k[0]=M_SKILLS;W->win_k[1]=top;W->win_k[2]=U_RAMZA;W->win_n=0;
        make_window();CHECK(W->win_h<=WIN_ROWS,1);CHECK(W->win_m,SKILL_ROWS+1);
        while(W->win_n<W->win_m)make_window();
        // Compare every visible cache byte on PC/68000, including both
        // continuation triangles and their absence at either list endpoint.
        {
            u8 plane,row,column;
            for(plane=0;plane<2;plane++)for(row=0;row<W->win_h;row++)
                for(column=0;column<8;column++)
                    digest=(digest<<1|digest>>31)^((const u8 *)W->win[plane])[row*RT_PBYTES+column];
        }
        CHECK(known(U_RAMZA,st.skill),0x196+st.skill);
        rt_prev=0;rt_keys=k<7?K_DOWN:k>7&&k<15?K_UP:0;
        acts_keys();
    }
    rt_prev=0;rt_keys=K_UP;acts_keys();CHECK(st.skill,7);
    rt_keys=K_DOWN;acts_keys();CHECK(st.skill,0);
    rt_keys=K_UP;acts_keys();
    rt_keys=K_ENTER;acts_keys();CHECK(st.mode,M_AIM);CHECK(st.abil,0x19D);
    rt_keys=0;
}

#ifndef __m68k__
int main(void)
{
    sw_init(61);trials();
    printf("charge %08lX failure %u\n",(unsigned long)digest,failures);
    return failures?1:0;
}
#else
void _main(void)
{
    // --frames 0 initializes the actual core/modules without advancing it.
    chapter_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);
}
#endif
