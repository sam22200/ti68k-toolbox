// Actual clock/AT adapters and public timed-status requests; no new spell record.
#define TEMPO_CHECK
#include "check_scheduler.c"
#include "x/core-haste-slow/oracle/source.h"
static void bare_rate(Unit *u,u8 rate,u8 innate)
{
    u->statuses.inflicted[3]=rate;u->statuses.innate[3]=innate;unit_status_refresh(u);
}
static void clock_gain(void)
{
    u16 k;u8 i;
    for(k=0;k<sizeof source_tempo_gain/sizeof source_tempo_gain[0];k++){
        const u16 *p=source_tempo_gain[k];wave();
        for(i=0;i<NUNIT;i++){st.unit[i].ct=0;st.unit[i].sp=0;}
        st.unit[0].ct=p[1];st.unit[0].sp=p[0];st.unit[0].hp=p[3]?0:999;
        bare_rate(&st.unit[0],p[2]|(p[4]?2:0),0);
        st.unit[12].ct=254;
        // Compare only the original ADD_CT phase; expiry is tested separately.
        st.open_ct_phase=1;st.status_clock=0;
        ZB(44);CHECK(ct_next(),p[5]);ZE(44);CHECK(st.unit[0].ct,p[6]);CHECK(st.unit[0].sp,p[0]);
    }
}
static void at_profiles(void)
{
    u16 k;u8 i;
    for(k=0;k<sizeof source_tempo_order/sizeof source_tempo_order[0];k++){
        const u16 *p=source_tempo_order[k];Unit *u;wave();
        for(i=2;i<NUNIT;i++)st.unit[i].hp=0;
        st.act=0;u=&st.unit[0];u->sp=p[0];u->ct=p[1];
        bare_rate(u,p[2]|(p[6]?2:0),p[5]);
        u->status_ct[SID_HASTE-SID_POISON]=p[3];u->status_ct[SID_SLOW-SID_POISON]=p[4];
        u->status_ct[SID_STOP-SID_POISON]=p[6];u->status_ct[SID_SLEEP-SID_POISON]=p[7];
        if(p[7]){u->statuses.inflicted[4]=0x10;unit_status_refresh(u);}
        st.unit[1].sp=6;st.unit[1].ct=50;
        turn_order();for(i=0;i<ORDER_N;i++)CHECK(st.turns[i],p[8+i]);
        CHECK(u->ct,p[1]);CHECK(u->sp,p[0]);
        CHECK(u->status_ct[SID_HASTE-SID_POISON],p[3]);CHECK(u->status_ct[SID_SLOW-SID_POISON],p[4]);
    }
}
static void timed_requests(void)
{
    u8 sid;Unit *u;u16 life;CoreStatusDelta d;
    for(sid=SID_HASTE;sid<=SID_SLOW;sid++){
        wave();u=&st.unit[12];life=cj_status_rules(common)[((u16)sid<<4)+3];
        change(12,sid);CHECK(core_status_current(&u->statuses,3),sid==SID_HASTE?8:4);
        CHECK(u->status_ct[sid-SID_POISON],life);CHECK(st.status_clock,0x1000);
        u->status_ct[sid-SID_POISON]=1;change(12,sid);
        CHECK(u->status_ct[sid-SID_POISON],life); // repeated mutable applications refresh
        memset(&d,0,sizeof d);d.remove[3]=sid==SID_HASTE?8:4;
        CHECK(battle_status_apply(12,&d,1)!=0,1);CHECK(core_status_current(&u->statuses,3),0);
        CHECK(st.status_clock,0);CHECK(u->status_ct[sid-SID_POISON],0);
        u->statuses.immune[3]=sid==SID_HASTE?8:4;d.add[3]=d.remove[3];d.remove[3]=0;
        CHECK(battle_status_apply(12,&d,0),0);CHECK(core_status_current(&u->statuses,3),0);
    }
    wave();u=&st.unit[12];change(12,SID_HASTE);change(12,SID_SLOW);
    CHECK(core_status_current(&u->statuses,3),4);CHECK(u->status_ct[SID_HASTE-SID_POISON],0);
    change(12,SID_HASTE);CHECK(core_status_current(&u->statuses,3),8);CHECK(u->status_ct[SID_SLOW-SID_POISON],0);
    // Protected innate tempo survives a cure and retains no expiry-clock bit.
    wave();u=&st.unit[12];u->statuses.innate[3]=8;unit_status_refresh(u);
    memset(&d,0,sizeof d);d.remove[3]=8;CHECK(battle_status_apply(12,&d,1),0);
    CHECK(core_status_current(&u->statuses,3),8);CHECK(st.status_clock,0);
}
static void expiry_clock(void)
{
    u8 sid,i;
    for(sid=SID_HASTE;sid<=SID_SLOW;sid++)for(i=1;i<=2;i++){
        Unit *u;freeze_all(SID_STOP);u=&st.unit[U_RAMZA];u->statuses.innate[3]=0;unit_status_refresh(u);
        change(U_RAMZA,sid);u->status_ct[sid-SID_POISON]=i;u->ct=90;u->sp=5;
        CHECK(ct_next(),U_RAMZA);CHECK(u->ct,i==1?0:2);
        CHECK(core_status_current(&u->statuses,3),0);CHECK(st.status_clock,0);
    }
    // Haste expires during Stop, before the resumed ordinary gain.
    wave();for(i=0;i<NUNIT;i++){st.unit[i].sp=0;st.unit[i].ct=0;}
    change(U_RAMZA,SID_HASTE);change(U_RAMZA,SID_STOP);
    st.unit[U_RAMZA].status_ct[SID_HASTE-SID_POISON]=1;
    st.unit[U_RAMZA].status_ct[SID_STOP-SID_POISON]=2;
    st.unit[U_RAMZA].ct=90;st.unit[U_RAMZA].sp=5;st.unit[12].sp=50;
    CHECK(ct_next(),12);CHECK(st.unit[U_RAMZA].ct,95);
    CHECK(core_status_current(&st.unit[U_RAMZA].statuses,3),0);
    // Real Slow1 is idle rather than a fake minimum gain; expiry wakes it safely.
    freeze_all(SID_STOP);st.unit[U_RAMZA].statuses.innate[3]=0;unit_status_refresh(&st.unit[U_RAMZA]);
    st.unit[U_RAMZA].sp=1;change(U_RAMZA,SID_SLOW);st.unit[U_RAMZA].status_ct[SID_SLOW-SID_POISON]=20;
    turn_start();CHECK(st.mode,M_CLOCK);CHECK(st.unit[U_RAMZA].ct,0);
    CHECK(st.unit[U_RAMZA].status_ct[SID_SLOW-SID_POISON],12);
    for(i=0;i<16&&st.mode==M_CLOCK;i++)turn_start();
    CHECK(st.mode,M_MENU);CHECK(st.act,U_RAMZA);CHECK(st.unit[U_RAMZA].ct,0);CHECK(st.status_clock,0);
}
static void charge_and_ko(void)
{
    u8 i,j;
    for(i=0;i<sizeof source_tempo_charge/sizeof source_tempo_charge[0];i++){
        wave();for(j=0;j<NUNIT;j++){st.unit[j].ct=0;st.unit[j].sp=0;}
        arm(0,cells[0],2);arm(12,cells[12],1);
        bare_rate(&st.unit[0],source_tempo_charge[i][0],0);
        CHECK(ct_next(),CT_DUE);CHECK(st.pending[0].ticks,source_tempo_charge[i][1]);
        CHECK(st.pending[12].ticks,0);
    }
    wave();arm(U_RAMZA,cells[10],4);change(U_RAMZA,SID_HASTE);
    CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);CHECK(st.pending[U_RAMZA].ticks,4);
    // Ordinary KO cancels mutable Haste and its timer; revival does not restore it.
    arena();change(10,SID_HASTE);st.unit[10].hp=1;root(1);
    CHECK(st.unit[10].hp,0);CHECK(core_status_current(&st.unit[10].statuses,3)&12,0);
    CHECK(st.unit[10].status_ct[SID_HASTE-SID_POISON],0);
    core_action_close(&st.action);skill(U_RAMZA,10,0x17d);st.dmg=10;impact();
    CHECK(st.unit[10].hp,10);CHECK(core_status_current(&st.unit[10].statuses,3)&12,0);
}
static u32 units_hash(void)
{
    u16 n;u32 h=17;const u8 *p=(const u8 *)st.unit;
    for(n=0;n<sizeof st.unit;n++)h=(h<<1|h>>31)^*p++;
    return h;
}
static void thirteen(void)
{
    u8 i;u32 before,seed;
    wave();for(i=0;i<NUNIT;i++){
        st.unit[i].sp=i==12?50:5;st.unit[i].ct=i==12?24:90;
        change(i,i&1?SID_SLOW:SID_HASTE);st.unit[i].status_ct[(i&1?SID_SLOW:SID_HASTE)-SID_POISON]=i==12?255:1;
    }
    before=units_hash();seed=rng32;
#ifdef FFT_MODULES
    mod_id=0;
#endif
    ZB(41);turn_order();ZE(41);ZB(42);turn_order();ZE(42);
    CHECK(units_hash()==before,1);CHECK(rng32==seed,1);
    for(i=0;i<ORDER_N;i++)CHECK(st.turns[i]<NUNIT,1);
    st.unit[12].status_ct[SID_HASTE-SID_POISON]=1;
#ifdef FFT_MODULES
    mod_id=0;ZB(43);mod_call(MOD_FORMULA,4,0);ZE(43);
#else
    status_tick();
#endif
    CHECK(st.status_clock,0);for(i=0;i<NUNIT;i++)CHECK(core_status_current(&st.unit[i].statuses,3)&12,0);
}
static void tempo_trials(void){clock_gain();at_profiles();timed_requests();expiry_clock();charge_and_ko();thirteen();}
#ifndef __m68k__
int main(void){sw_init(61);tempo_trials();printf("haste_slow %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();tempo_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
