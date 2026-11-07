// Quick interrupts only settled roots; bounded idle clocks never invent a unit turn.
#define SCHEDULER_CHECK
#include "check_deferred.c"
#include "x/core-scheduler/oracle/source.h"
static void quick_picks(void)
{
    u8 i,j;
    for(i=0;i<sizeof source_clock_picks/sizeof source_clock_picks[0];i++){
        const u16 *p=source_clock_picks[i];u8 picked;
        wave();for(j=0;j<NUNIT;j++)st.unit[j].ct=0;
        st.unit[0].ct=p[1];st.unit[12].ct=p[2];
        if(p[3])change(0,SID_STOP);
        st.open_ct_phase=p[0]==9;
        picked=ct_next();CHECK(picked,p[4]);CHECK(st.unit[picked].ct,p[5]);
        CHECK(st.open_ct_phase,p[0]==9);
    }
    // Special CT never leaks into an ordinary Speed increment.
    wave();st.unit[0].ct=254;st.unit[0].sp=50;
    CHECK(ct_next(),0);CHECK(st.unit[0].ct,54);
}
static void quick_wave(void)
{
    u8 i;u16 rounds;
    wave();for(i=0;i<NUNIT;i++)st.unit[i].ct=0;
    arm(1,cells[1],1);arm(7,cells[7],1);
    change(U_RAMZA,SID_POISON);st.unit[U_RAMZA].status_ct[0]=2;
    turn_start();CHECK(st.atk,1);CHECK(st.open_ct_phase,1);
    // An in-flight root retains ownership even if Quick becomes ready.
    st.unit[2].ct=st.unit[10].ct=255;rounds=st.rounds;
    CHECK(charge_resolve(),1);CHECK(st.atk,1);CHECK(st.mode,M_ACT);
    st.hit=0;st.push=0;impact();end_anim();
    CHECK(st.act,2);CHECK(st.unit[2].ct,0);CHECK(st.open_ct_phase,1);
    CHECK(st.pending[7].ticks,0);CHECK(st.pending[7].state,DEFER_CHARGING);
    CHECK(st.unit[U_RAMZA].status_ct[0],2);CHECK(st.rounds,rounds);
    // Two ordinary Waits resume the same wave, with no second countdown/expiry.
    turn_end(1,1);CHECK(st.act,10);CHECK(st.unit[10].ct,0);
    CHECK(st.unit[2].ct,0);CHECK(st.unit[U_RAMZA].status_ct[0],2);
    st.unit[12].ct=199;turn_end(1,1);CHECK(st.atk,7);CHECK(st.mode,M_ACT);
    CHECK(st.unit[12].ct,199);CHECK(st.unit[U_RAMZA].status_ct[0],2);
    st.hit=0;st.push=0;impact();end_anim();
    CHECK(st.act,12);CHECK(st.open_ct_phase,0);CHECK(st.unit[12].ct,49);
    CHECK(st.unit[U_RAMZA].status_ct[0],1);
}
static void freeze_all(u8 sid)
{
    u8 i;wave();
    for(i=0;i<NUNIT;i++){
        st.unit[i].ct=0;
        st.unit[i].statuses.innate[sid>>3]|=(u8)(0x80>>(sid&7));
        unit_status_refresh(&st.unit[i]);
    }
}
static void stalled(void)
{
    u8 i,old;u16 rounds;u32 seed;
    for(i=0;i<2;i++){
        freeze_all(i?SID_SLEEP:SID_STOP);seed=rng32;rounds=st.rounds;old=st.act;
        ZB(35);CHECK(ct_next(),CT_WAIT);ZE(35);
        turn_start();CHECK(st.mode,M_CLOCK);CHECK(st.act,old);CHECK(st.act<NUNIT,1);
        CHECK(st.ai,AI_OFF);CHECK(st.rounds,rounds);CHECK(rng32==seed,1);
        CHECK(st.unit[old].ct,0);
    }
    // A stale off-field expiry bit cannot keep the synchronous search alive.
    freeze_all(SID_STOP);st.unit[12].x=255;st.status_clock=0x1000;
    ZB(36);CHECK(ct_next(),CT_WAIT);ZE(36);CHECK(st.status_clock,0x1000);
    // Speed0 makes no real CT progress (AT alone clamps it to1).
    wave();for(i=0;i<NUNIT;i++)st.unit[i].sp=0;
    CHECK(ct_next(),CT_WAIT);turn_start();CHECK(st.mode,M_CLOCK);
    // A guest's KO clock never produces a turn or permanent death.
    wave();for(i=0;i<NUNIT;i++){
        st.unit[i].hp=0;st.unit[i].team=TEAM_GUEST;
    }
    CHECK(ct_next(),CT_WAIT);CHECK(st.morph_mask,0);
    for(i=0;i<NUNIT;i++)CHECK(st.unit[i].dc,3);
    // Defeat is settled before searching an all-KO formation.
    rounds=st.rounds;turn_start();CHECK(st.end,END_DEFEAT);CHECK(st.mode,M_END);
    CHECK(st.rounds,rounds);CHECK(st.unit[U_RAMZA].dc,3);
    wave();for(i=0;i<NUNIT;i++){st.unit[i].hp=0;st.unit[i].dc=DC_GONE;}
    CHECK(ct_next(),CT_WAIT);
}
static void wakes(void)
{
    u8 sid,i,n;
    for(sid=SID_STOP;sid<=SID_SLEEP;sid+=SID_SLEEP-SID_STOP){
        freeze_all(sid);
        st.unit[U_RAMZA].statuses.innate[sid>>3]=0;
        unit_status_refresh(&st.unit[U_RAMZA]);change(U_RAMZA,sid);
        st.unit[U_RAMZA].status_ct[sid-SID_POISON]=9;st.unit[U_RAMZA].ct=80;st.unit[U_RAMZA].sp=6;
        turn_start();CHECK(st.mode,M_CLOCK);CHECK(st.unit[U_RAMZA].ct,80);
        CHECK(st.unit[U_RAMZA].status_ct[sid-SID_POISON],1);
        ZB(37);turn_start();ZE(37);CHECK(st.act,U_RAMZA);CHECK(st.mode,M_MENU);
        CHECK(st.status_clock,0);CHECK(st.unit[U_RAMZA].ct,4);
    }
    // Timer0 deliberately underflows to255 in FFT; bounded resumes still wake.
    wave();for(i=0;i<NUNIT;i++){
        st.unit[i].ct=0;st.unit[i].sp=1;change(i,SID_SLEEP);
        st.unit[i].status_ct[SID_SLEEP-SID_POISON]=0;
    }
    n=0;do{ZB(38);turn_start();ZE(38);n++;CHECK(st.act<NUNIT,1);}while(st.mode==M_CLOCK&&n<48);
    CHECK(n,38);CHECK(st.act,0);CHECK(st.unit[0].ct,0);CHECK(st.status_clock,0);
}
static void wait_input(void)
{
    u8 i;u16 rounds;u32 seed;
    freeze_all(SID_STOP);rounds=st.rounds;seed=rng32;turn_start();
    rt_keys=K_ENTER;rt_prev=0;CHECK(update(0),1);CHECK(st.mode,M_CLOCK);CHECK(st.rounds,rounds);
    rt_keys=K_ESC;rt_prev=0;CHECK(update(0),1);CHECK(st.mode,M_QUIT);CHECK(st.quit,1);CHECK(st.back,M_CLOCK);
    rt_keys=K_ENTER;rt_prev=0;CHECK(update(0),1);CHECK(st.mode,M_CLOCK);
    rt_keys=K_F1;rt_prev=0;CHECK(update(0),1);CHECK(st.turn!=0,1);
    rt_keys=0;for(i=0;i<TURN_FRAMES;i++)CHECK(update(0),1);
    CHECK(st.turn,0);CHECK(st.mode,M_CLOCK);CHECK(rng32==seed,1);CHECK(st.rounds,rounds);
    rt_keys=K_ESC;CHECK(update(0),1);rt_keys=K_UP;CHECK(update(0),1);
    CHECK(st.quit,0);rt_keys=K_A;CHECK(update(0),0);
    // The shared confirmation pauses mutable timers and cancellation resumes.
    wave();for(i=0;i<NUNIT;i++){change(i,SID_SLEEP);st.unit[i].status_ct[SID_SLEEP-SID_POISON]=40;}
    turn_start();CHECK(st.mode,M_CLOCK);CHECK(st.unit[0].status_ct[SID_SLEEP-SID_POISON],32);
    rt_keys=K_ESC;rt_prev=0;CHECK(update(0),1);rt_keys=0;
    for(i=0;i<12;i++)CHECK(update(0),1);
    CHECK(st.unit[0].status_ct[SID_SLEEP-SID_POISON],32);
    rt_keys=K_B;CHECK(update(0),1);CHECK(st.mode,M_CLOCK);
    rt_keys=0;CHECK(update(0),1);CHECK(st.unit[0].status_ct[SID_SLEEP-SID_POISON],24);
}
static void forecast_order(void)
{
    u8 i,j,k;
    for(i=0;i<sizeof source_clock_orders/sizeof source_clock_orders[0];i++){
        const u16 *p=source_clock_orders[i];wave();
        for(j=2;j<NUNIT;j++)st.unit[j].hp=0;
        st.act=0;st.unit[0].sp=p[0];st.unit[0].ct=p[1];st.unit[1].ct=50;st.unit[1].sp=6;
        turn_order();for(k=0;k<ORDER_N;k++)CHECK(st.turns[k],p[k+2]);
        CHECK(st.unit[0].ct,p[1]);
    }
}
static u32 picture(void)
{
    u8 i;u16 n;u32 h=17;
    // The only renderer branch added for M_CLOCK is in draw_hud. Exercise it
    // directly, keeping the native checker below the transport size limit.
    memset(rt_light,0,RT_PSIZE);memset(rt_dark,0,RT_PSIZE);
    for(i=0;i<20;i++){W->left=2*HUD_K;draw_hud();}
    CHECK(W->hud_ok,1);CHECK(W->pan_ok[0],1);
    for(n=0;n<RT_PSIZE;n++){
        h=(h<<1|h>>31)^((const u8 *)rt_light)[n];
        h=(h<<1|h>>31)^((const u8 *)rt_dark)[n];
    }
    return h;
}
static void wait_picture(void)
{
    u32 a,b;u8 i;
    for(i=0;i<2;i++){
        freeze_all(SID_STOP);cursor_place(cells[i?12:U_RAMZA]);
        st.mode=M_BROWSE;a=picture();st.mode=M_CLOCK;b=picture();CHECK(a==b,1);
        CHECK(W->pan_fk[1][0],i?13:0);
#ifdef __m68k__
        BENCH_VALUE(b);
#else
        printf("clockframe %u %08lX\n",i,(unsigned long)b);
#endif
    }
}
static void scheduler_trials(void){quick_picks();quick_wave();stalled();wakes();wait_input();forecast_order();wait_picture();}
#ifndef TEMPO_CHECK
#ifndef __m68k__
int main(void){sw_init(61);scheduler_trials();printf("scheduler %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();scheduler_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
