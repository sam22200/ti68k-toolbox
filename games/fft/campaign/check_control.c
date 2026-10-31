// Source control policies and actual player/AI/clock/Charge integration.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main control_runtime_main
#define game_render control_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void control_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
#include "x/core-control/oracle/source.h"
static u16 checks,failures;static u32 digest=17;
static void check(u16 a,u16 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,a,b);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)
static void source(void)
{
    CoreStatus s;u16 k;memset(&s,0,sizeof s);
    for(k=0;k<sizeof source_controls/sizeof source_controls[0];k++){
        // Lost control (CORE_CONTROL_STATUS) is checked separately from these restrictions.
        memcpy(s.inflicted,source_controls[k],5);CHECK(core_status_controls(&s)&~STATUS_NO_CONTROL,source_controls[k][5]);
        // A trait makes the same status effective; cached context stays separate.
        memcpy(s.innate,s.inflicted,5);memset(s.inflicted,0,5);
        CHECK(core_status_controls(&s)&~STATUS_NO_CONTROL,source_controls[k][5]);memset(s.innate,0,5);
    }
    for(k=0;k<sizeof source_ctend/sizeof source_ctend[0];k++){
        const u8 *p=source_ctend[k];memcpy(s.inflicted,p,5);
        CHECK(core_status_end_ct(&s,p[5],p[6],p[7]),p[8]);
    }
}
static void reset(void){game_scenario(61);st.act=U_RAMZA;st.ai=AI_OFF;st.banner=0;st.mode=M_MENU;}
static u16 change(u8 who,u8 id,u8 add)
{
    CoreStatusDelta d;memset(&d,0,sizeof d);
    if(add)d.add[id>>3]=(u8)(0x80>>(id&7));else d.remove[id>>3]=(u8)(0x80>>(id&7));
    return battle_status_apply(who,&d,0);
}
static void menu_enter(void){rt_prev=0;rt_keys=K_ENTER;menu_keys();rt_keys=rt_prev=0;}
static void ai_finish(void)
{
    u16 n=0;W->ai_k=0;st.ai=AI_THINK;
    while(!ai_think()&&++n<1024){}CHECK(n<1024,1);
}
static void integration(void)
{
    Unit *u,*t;u8 i,j;u16 hp,jp,bag;u32 rng;u8 cell;
    reset();CHECK(NUNIT,13);u=&st.unit[U_RAMZA];cell=UNIT_CELL(u);
    compute_reach(U_RAMZA);CHECK(W->from_who,U_RAMZA+1);st.mode=M_TARGET;
    CHECK(change(U_RAMZA,SID_DONT_MOVE,1)!=0,1);
    CHECK(st.mode,M_MENU);CHECK(CURSOR_CELL,cell);CHECK(W->from_who,0);
    CHECK(unit_can_move(u),0);CHECK(unit_can_act(u),1);CHECK(u->status_ct[12],24);
    st.menu=MN_MOVE;menu_enter();CHECK(st.mode,M_MENU);
    compute_reach(U_RAMZA);for(i=0;i<MAP_N;i++)CHECK(st.reach[i],i==cell?0:255);
    CHECK(make_path_cell(U_RAMZA,(u8)(cell?cell-1:1)),0);CHECK(st.path_n,0);
    st.menu=MN_ACT;menu_enter();CHECK(st.mode,M_ACTS);
    CHECK(change(U_RAMZA,SID_DONT_MOVE,0)!=0,1);CHECK(unit_can_move(u),1);
    // Don't Act blocks every start before RNG/items/pending-command mutation.
    reset();u=&st.unit[U_RAMZA];skill(U_RAMZA,10,0x196);CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);
    st.mode=M_AIM;CHECK(change(U_RAMZA,SID_DONT_ACT,1)!=0,1);CHECK(st.mode,M_MENU);
    CHECK(u->status_ct[13],24);CHECK(unit_can_act(u),0);CHECK(unit_can_move(u),1);
    rng=rng32;hp=st.unit[10].hp;jp=u->jp;bag=st.bag[abilities[0x170].bag];
    CHECK(can_use(U_RAMZA,0x170),0);CHECK(skill_hit(U_RAMZA,10,0x170),0);
    attack(U_RAMZA,10);skill(U_RAMZA,10,0x170);defend(U_RAMZA);
    CHECK(rng32==rng,1);CHECK(st.bag[abilities[0x170].bag],bag);CHECK(st.unit[10].hp,hp);CHECK(u->jp,jp);
    CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);CHECK(st.mode,M_MENU);
    attack_range(U_RAMZA);for(i=0;i<MAP_N;i++)CHECK(st.reach[i],255);
    st.menu=MN_ACT;menu_enter();CHECK(st.mode,M_MENU);
    CHECK(change(U_RAMZA,SID_DONT_ACT,0)!=0,1);CHECK(can_use(U_RAMZA,0x170),1);
    // Blocked commands do not masquerade as already spent Move/Act flags.
    reset();u=&st.unit[U_RAMZA];change(U_RAMZA,SID_DONT_MOVE,1);u->ct=0;ct_end(U_RAMZA,0,0);
    CHECK(u->ct,20);CHECK(st.moved,0);CHECK(st.acted,0);
    change(U_RAMZA,SID_DONT_ACT,1);u->ct=0;ct_end(U_RAMZA,0,0);CHECK(u->ct,0);
    change(U_RAMZA,SID_DONT_MOVE,0);u->ct=0;ct_end(U_RAMZA,0,0);CHECK(u->ct,20);
    // Actual AI origins/actions follow the same restrictions, including Defend.
    reset();u=&st.unit[U_RAMZA];cell=UNIT_CELL(u);change(U_RAMZA,SID_DONT_MOVE,1);ai_finish();
    CHECK(st.ai_o,cell);for(i=0;i<W->ai_n;i++)CHECK(W->ai_rank[i].o,cell);
    reset();u=&st.unit[U_RAMZA];u->defend=1;change(U_RAMZA,SID_DONT_ACT,1);ai_finish();
    CHECK(W->ai_n,0);CHECK(st.ai_ab,AI_NONE);
    // Stop retains CT even when already ready, and freezes a pending Charge.
    reset();u=&st.unit[U_RAMZA];skill(U_RAMZA,10,0x196);u->status_flags|=STATUS_IMMORTAL;
    CHECK(change(U_RAMZA,SID_STOP,1)!=0,1);CHECK(u->status_flags&STATUS_CONTEXT,STATUS_IMMORTAL);
    CHECK(u->status_ct[6],20);st.pending[U_RAMZA].ticks=2;
    for(i=0;i<NUNIT;i++){st.unit[i].ct=0;st.unit[i].sp=1;}u->ct=110;st.unit[0].ct=100;
    CHECK(ct_next(),0);CHECK(u->ct,110);CHECK(st.pending[U_RAMZA].ticks,2);
    u->ct=80;u->sp=6;u->status_ct[6]=1;st.unit[0].ct=0;st.unit[0].sp=100;
    CHECK(ct_next(),0);CHECK(u->ct,86);CHECK(u->status_flags&STATUS_CT_FROZEN,0);
    CHECK(st.pending[U_RAMZA].ticks,2);CHECK(core_status_current(&u->statuses,0)&8,8);
    CHECK(ct_next(),0);CHECK(st.pending[U_RAMZA].ticks,1);CHECK(ct_next(),254);
    CHECK(st.pending[U_RAMZA].ticks,0);
    // An active stopped unit yields at a safe choice boundary; no user key is needed.
    reset();u=&st.unit[U_RAMZA];for(i=0;i<NUNIT;i++)st.unit[i].ct=0;st.unit[0].ct=100;change(U_RAMZA,SID_STOP,1);game_update();
    CHECK(st.act,0);CHECK(u->ct,40);CHECK(u->status_ct[6],20);
    // Don't Act remains pending until due; clearing it beforehand permits resolution.
    for(j=0;j<2;j++){
        reset();u=&st.unit[U_RAMZA];t=&st.unit[10];skill(U_RAMZA,10,0x196);
        change(U_RAMZA,SID_DONT_ACT,1);if(j)change(U_RAMZA,SID_DONT_ACT,0);
        st.pending[U_RAMZA].ticks=0;st.open_ct_phase=1;st.unit[0].ct=100;
        hp=t->hp;ZB(18);charge_resolve();ZE(18);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
        if(!j){CHECK(st.open_resolving,0);CHECK(st.mode!=M_ACT,1);CHECK(t->hp,hp);}
        else {CHECK(st.open_resolving,1);CHECK(st.mode,M_ACT);}
    }
    // Source no-evasion cases include Stop, Don't Act and existing Charging.
    for(j=0;j<3;j++){
        reset();u=&st.unit[U_RAMZA];t=&st.unit[10];
        if(j<2)change(10,j?SID_DONT_ACT:SID_STOP,1);else skill(10,U_RAMZA,0x196);
        u->x=t->x+1;u->z=t->z;t->face=0;t->job=0x4a;
        CHECK(attack_side(U_RAMZA,10),SIDE_FRONT);CHECK(hit_chance(U_RAMZA,10),100);
        for(i=0;i<8;i++)CHECK(roll_hit(U_RAMZA,10),1);
    }
    // Source AT first-six entries include the frozen unit's remaining wait.
    for(j=0;j<sizeof source_orders/sizeof source_orders[0];j++){
        reset();for(i=2;i<NUNIT;i++)st.unit[i].hp=0;st.act=0;
        st.unit[0].ct=0;st.unit[0].sp=6;st.unit[1].ct=50;st.unit[1].sp=6;
        change(1,SID_STOP,1);st.unit[1].status_ct[6]=source_orders[j][0];
        ZB(19);turn_order();ZE(19);for(i=0;i<ORDER_N;i++)CHECK(st.turns[i],source_orders[j][i+1]);
    }
    // Whole13-actor turn-list builds: forced cold image, then the warmed call.
    reset();
#ifdef FFT_MODULES
    mod_id=0;
#endif
    ZB(20);turn_order();ZE(20);ZB(21);turn_order();ZE(21);
    // Sleep now shares freeze/eligibility; its damage/wake tests are separate.
    reset();CHECK(change(U_RAMZA,SID_SLEEP,1)!=0,1);
    // Final mask bit: slot12 supports application/cure without extra storage.
    CHECK(change(12,SID_STOP,1)!=0,1);CHECK(st.status_clock&0x1000,0x1000);
    CHECK(change(12,SID_STOP,0)!=0,1);CHECK(st.status_clock&0x1000,0);
}
static void trials(void){source();integration();}
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("control %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){control_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
