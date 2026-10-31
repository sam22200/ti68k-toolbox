// Life-cycle, reflected producers and frozen effect packets; real production IDs.
#define TERMINAL_RUNTIME_CHECK
#include "check_terminal.c"
static u8 dead_tick(u8 id)
{
#ifdef FFT_MODULES
    return mod_call(MOD_FORMULA,14,id);
#else
    return status_dead_turn(id);
#endif
}
static void undead_upkeep(void)
{
    u8 i,k,result;u32 expected;Unit *t;
    for(i=0;i<sizeof source_terminal_upkeep/sizeof source_terminal_upkeep[0];i++){
        const u16 *p=source_terminal_upkeep[i];prepare_terminal(51);t=&st.unit[10];
        t->hp=0;t->dc=p[0];t->statuses.inflicted[0]=48;unit_status_refresh(t);
        rng32=p[1];for(k=0;k<p[6];k++)rand15();expected=rng32;rng32=p[1];
        result=dead_tick(10);CHECK(result,p[4]!=0);CHECK(rng32==expected,1);
        if(p[2]==0){CHECK(t->dc,p[3]);CHECK(t->hp,0);}
        else if(p[4]){CHECK(t->hp,p[4]);CHECK(t->dc,3);CHECK(t->statuses.inflicted[0]&32,0);CHECK(st.morph_mask&((u16)1<<10),0);}
        else{CHECK(t->dc,p[5]==1?DC_CRYSTAL:DC_CHEST);CHECK(st.morph_mask&((u16)1<<10),(u16)1<<10);}
    }
    // Frozen dead-unit callback must preserve the active action while updating caches.
    prepare_terminal(51);t=&st.unit[10];t->hp=0;t->dc=0;t->statuses.inflicted[0]=48;
    unit_status_refresh(t);rng32=16;CHECK(dead_tick(10),1);CHECK(t->hp>0,1);
    prepare_terminal(51);t=&st.unit[10];t->hp=0;t->dc=0;t->team=TEAM_GUEST;
    t->statuses.inflicted[0]=48;unit_status_refresh(t);expected=rng32;CHECK(dead_tick(10),0);CHECK(rng32==expected,1);CHECK(t->dc,0);
}
static void petrify_and_cure(void)
{
    Unit *t;u16 cell,hp;u8 k;
    prepare_terminal(59);t=&st.unit[10];st.unit[U_RAMZA].ma=99;
    st.support[U_RAMZA]=SUPPORT_NON_CHARGE;cell=UNIT_CELL(t);hp=t->hp;
    st.pending[10].state=DEFER_CHARGING;unit_status_action(t,8);
    CHECK(cast_order(U_RAMZA,10,59,1),1);impact();CHECK(unit_petrified(t),128);
    CHECK(st.pending[10].state,DEFER_NONE);CHECK(t->hp,hp);CHECK(UNIT_CELL(t),cell);
    CHECK(unit_at_cell((u8)cell),11);CHECK(unit_can_move(t),0);
    for(k=0;k<16&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.action.phase,CTX_IDLE);
    st.acted=0;st.unit[U_RAMZA].set=job_sets[0x4b];
    for(k=0;k<SET_N;k++)if(SET_ABILITY(st.unit[U_RAMZA].set,k)==378)break;
    CHECK(k<SET_N,1);st.unit[U_RAMZA].learned=(LearnBits)1<<k;st.bag[10]=2;
    skill(U_RAMZA,10,378);impact();CHECK(unit_petrified(t),0);CHECK(t->hp,hp);CHECK(UNIT_CELL(t),cell);CHECK(unit_can_act(t),1);
}
static void reflected_producers(void)
{
    u8 j,k;u16 cost;Unit *t;
    for(j=0;j<sizeof terminal/sizeof terminal[0];j++){
        AbId ab=terminal[j];prepare_terminal(ab);t=&st.unit[8];
        *t=st.unit[10];unit_place(t,third);st.unit[U_RAMZA].ma=99;
        change(10,SID_REFLECT);st.support[U_RAMZA]=SUPPORT_NON_CHARGE;cost=skill_cost(U_RAMZA,ab);
        CHECK(cast_order(U_RAMZA,10,ab,1),1);impact();CHECK(st.action.pending&((u16)1<<10),(u16)1<<10);
        end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.tgt,8);CHECK(st.abil,ab);CHECK(st.hit,1);
        change(8,SID_REFLECT);impact();CHECK(st.action.pending&((u16)1<<8),0);
        if(ab==30)CHECK(t->hp,0);
        if(ab==41)CHECK(t->ct,255);
        if(ab==51)CHECK(core_status_current(&t->statuses,0)&16,16);
        if(ab==59)CHECK(unit_petrified(t),128);
        for(k=0;k<20&&st.action.phase!=CTX_IDLE;k++){if(st.action.phase==CTX_SECONDARY)impact();end_anim();}
        CHECK(st.action.phase,CTX_IDLE);CHECK(st.unit[U_RAMZA].mp,999-cost);
    }
}
static void frozen_death(void)
{
    prepare_terminal(30);st.unit[U_RAMZA].ma=99;st.unit[10].hp=st.unit[10].max_hp=100;
    st.support[U_RAMZA]=SUPPORT_NON_CHARGE;CHECK(cast_order(U_RAMZA,10,30,1),1);
    CHECK(W->cast.victim[10].amount,100);CHECK(W->cast.victim[10].effect,AB_DEATH);
    st.unit[10].hp=st.unit[10].max_hp=999;impact();CHECK(st.unit[10].hp,0);
    prepare_terminal(30);st.unit[U_RAMZA].ma=99;st.unit[10].hp=st.unit[10].max_hp=100;
    st.support[U_RAMZA]=SUPPORT_NON_CHARGE;CHECK(cast_order(U_RAMZA,10,30,1),1);
    st.unit[10].hp=st.unit[10].max_hp=999;st.unit[10].statuses.immune[0]|=32;
    impact();CHECK(st.unit[10].hp,899);
}
static void death_undead(void)
{
    prepare_terminal(30);st.unit[U_RAMZA].ma=99;st.unit[10].statuses.inflicted[0]=16;
    unit_status_refresh(&st.unit[10]);st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
    CHECK(cast_order(U_RAMZA,10,30,1),1);CHECK(W->cast.victim[10].effect,AB_HEAL);
    impact();CHECK(st.unit[10].hp,999);CHECK(st.primary.reward,0);CHECK(st.gain,0);
}
static void quick_unchanged(void)
{
    prepare_terminal(41);st.unit[U_RAMZA].ma=99;st.unit[10].ct=255;
    st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
    CHECK(cast_order(U_RAMZA,10,41,1),1);impact();CHECK(st.unit[10].ct,255);
    CHECK(st.primary.reward,0);CHECK(st.gain,0);CHECK(st.jpg,0);
}
static void quick_wave(void)
{
    u8 i,k;prepare_terminal(41);st.unit[U_RAMZA].ma=99;
    st.support[U_RAMZA]=SUPPORT_NON_CHARGE;for(i=0;i<NUNIT;i++)st.unit[i].ct=0;
    st.unit[8]=st.unit[10];unit_place(&st.unit[8],third);st.unit[8].team=TEAM_ENEMY;
    st.open_ct_phase=1;CHECK(cast_order(U_RAMZA,10,41,1),1);
    CHECK(st.unit[10].ct,0);impact();CHECK(st.unit[10].ct,255);
    CHECK(st.act,U_RAMZA);CHECK(st.action.phase,CTX_IMPACT);
    for(k=0;k<20&&st.mode==M_ACT;k++)end_anim();
    turn_end(1,1);CHECK(st.act,10);CHECK(st.unit[10].ct,0);CHECK(st.open_ct_phase,1);
}
static void turn_credits(void)
{
    u8 mode,credit;
    for(mode=0;mode<3;mode++)for(credit=0;credit<4;credit++){
        Unit *u;prepare_terminal(41);u=&st.unit[10];u->ct=0;
        if(mode==1)change(10,SID_DONT_MOVE);if(mode==2)change(10,SID_DONT_ACT);
        ZB(64);ct_end(10,credit&1?2:0,credit&2?4:0);ZE(64);
        CHECK(u->ct,(!(credit&1)&&mode!=1?20:0)+(!(credit&2)&&mode!=2?20:0));
        {u8 ct=u->ct;ZB(65);ct_end(10,2,4);ZE(65);CHECK(u->ct,ct);}
    }
}
static void terminal_edges(void){undead_upkeep();petrify_and_cure();reflected_producers();quick_wave();quick_unchanged();death_undead();frozen_death();turn_credits();}
#ifndef __m68k__
int main(void){sw_init(61);terminal_edges();printf("terminal_edges %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();terminal_edges();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
