// All fourteen recurring performances: original formulas and production lifecycle.
#define CLEANSE_RUNTIME_CHECK
#define FFT_CHECK_NATIVE_ORDINAL
#include "check_cleanse.c"
#include "x/core-performance/oracle/source.h"
static void prepare_performance(AbId ab)
{
    u8 i;prepare_cleanse(ab);
    for(i=0;i<NUNIT;i++)if(i!=U_RAMZA&&i!=10){st.unit[i].x=255;st.unit[i].dc=DC_GONE;}
    if(ab<93){st.unit[0]=st.unit[10];st.unit[0].team=TEAM_ENEMY;unit_place(&st.unit[0],third);}
    st.unit[10].team=ab<93?TEAM_PLAYER:TEAM_ENEMY;
    st.unit[U_RAMZA].pa=17;st.unit[U_RAMZA].ma=23;st.unit[U_RAMZA].brave=70;
    CHECK(can_use(U_RAMZA,ab),1);
}
static void formula_profiles(void)
{
    u16 i;u8 j;u32 seed,expected;CastBatch b;Unit *a,*t;
    static const u8 mode_sid[]={0,35,3,2,8,31};
    for(i=0;i<sizeof source_performance/sizeof source_performance[0];i++){
        const typeof(source_performance[0]) *p=&source_performance[i];const u8 *r;
        prepare_performance(p->ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->pa=p->pa;a->ma=p->ma;a->brave=p->brave;r=cj_section(common,CJ_STATUS)+6*cj_range(common,p->ab)[11];
        if(p->mode&&p->mode<6)t->statuses.inflicted[mode_sid[p->mode]>>3]|=128>>(mode_sid[p->mode]&7);
        if(p->mode==6)memcpy(t->statuses.immune,r+1,5);
        if(p->mode==7)memcpy(t->statuses.inflicted,r+1,5);
        if(p->mode==8)t->statuses.inflicted[SID_FROG>>3]|=128>>(SID_FROG&7);
        unit_status_refresh(t);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=first;b.ability=p->ab;
        seed=rng32;CHECK(cast_forecast(&b),1);CHECK(b.victim[10].hit,p->preview_hit?p->preview_chance:0);CHECK(rng32==seed,1);
        CHECK(t->hp,999);CHECK(t->mp,999);
        rng32=p->seed;for(j=0;j<p->draws;j++)rand15();expected=rng32;rng32=p->seed;b.phase=CAST_BATCH_EMPTY;
        ZB(61);CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);ZE(61);
        CHECK(b.victim[10].hit,p->hit);CHECK(rng32==expected,1);
        if(p->hit){CHECK(b.victim[10].effect,abilities[p->ab].kind);CHECK(b.victim[10].amount,p->amount);}
        else CHECK(b.victim[10].effect,AB_NULL);
        CHECK(t->hp,999);CHECK(t->mp,999);CHECK(a->mp,999);
        if(p->mode==8){
            CHECK(cast_batch(CAST_BATCH_COMMIT,&b),1);
            CHECK((core_status_current(&t->statuses,SID_FROG>>3)&2)!=0,p->packed!=0x0202);
        }
    }
}
static void settle_performance(void)
{
    u8 j;st.open_resolving=0; // effect-stage barrier; clock scheduling has separate real-turn checks
    for(j=0;j<12&&st.action.phase!=CTX_IDLE;j++)end_anim();
    CHECK(st.action.phase,CTX_IDLE);CHECK(W->cast.phase,CAST_BATCH_EMPTY);
}
static void pulse(void)
{
    st.pending[U_RAMZA].ticks=0;
    ZB(62);CHECK(charge_resolve(),1);ZE(62);
    CHECK(st.mode,M_ACT);CHECK(st.pending[U_RAMZA].state,DEFER_PERFORMING);
    CHECK(st.pending[U_RAMZA].ticks,cj_range(common,st.abil)[12]);
}
static void lifecycle(void)
{
    AbId ab;u8 j;u16 hp,mp;u32 seed;Unit *a,*t;
    for(ab=86;ab<100;ab++){
        prepare_performance(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];a->mp=0;
        t->hp=t->mp=500;st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
        seed=rng32;CHECK(cast_order(U_RAMZA,first,ab,0),2);
        CHECK(st.pending[U_RAMZA].state,DEFER_PERFORMING);CHECK(st.pending[U_RAMZA].ticks,cj_range(common,ab)[12]);
        CHECK(core_status_current(&a->statuses,0)&15,1);CHECK(rng32==seed,1);CHECK(st.mode,M_MENU);
        CHECK(t->hp,500);CHECK(t->mp,500);CHECK(st.acted,1);
        CHECK(core_status_end_ct(&a->statuses,0,0,0),20);
        for(j=0;j<2;j++){
            rng32=1;pulse();hp=t->hp;mp=t->mp;seed=rng32;
            CHECK(W->cast.mask,ab<93?((1U<<U_RAMZA)|(1U<<10)):(1U<<10));
            ZB(63);impact();ZE(63);CHECK(rng32==seed,1);CHECK(a->mp,ab==86?(j+1)*43:0);
            if(ab==86)CHECK(t->mp,mp+43);
            if(ab==87)CHECK(t->hp,hp+33);
            if(ab==93)CHECK(t->mp,mp-28);
            if(ab==94)CHECK(t->hp,hp-28);
            hp=t->hp;mp=t->mp;impact();CHECK(t->hp,hp);CHECK(t->mp,mp);
            settle_performance();CHECK(st.pending[U_RAMZA].state,DEFER_PERFORMING);
            CHECK(core_status_current(&a->statuses,0)&1,1);
        }
    }
    // A moving performer retains the clock. A new command replaces it.
    prepare_performance(94);CHECK(cast_order(U_RAMZA,first,94,0),2);
    unit_place(&st.unit[U_RAMZA],third);CHECK(pending_next(),255);CHECK(st.pending[U_RAMZA].state,DEFER_PERFORMING);
    attack(U_RAMZA,10);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(core_status_current(&st.unit[U_RAMZA].statuses,0)&1,0);
    // Status cancellation belongs to source bitmaps; Stop pauses without losing the performance.
    for(j=0;j<4;j++){
        static const u8 sid[]={SID_SLEEP,SID_FROG,SID_BERSERK,SID_PETRIFY};
        prepare_performance(94);CHECK(cast_order(U_RAMZA,first,94,0),2);change(U_RAMZA,sid[j]);
        CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(core_status_current(&st.unit[U_RAMZA].statuses,0)&1,0);
    }
    prepare_performance(94);CHECK(cast_order(U_RAMZA,first,94,0),2);change(U_RAMZA,SID_STOP);
    CHECK(st.pending[U_RAMZA].state,DEFER_PERFORMING);st.pending[U_RAMZA].ticks=0;CHECK(pending_next(),255);
    prepare_performance(94);change(U_RAMZA,SID_SILENCE);CHECK(can_use(U_RAMZA,94),1);
    // The next pulse reads the current camp, positions, stats and immunity.
    prepare_performance(94);CHECK(cast_order(U_RAMZA,first,94,0),2);st.unit[10].team=TEAM_PLAYER;
    pulse();CHECK(W->cast.mask,0);impact();settle_performance();st.unit[10].team=TEAM_ENEMY;st.unit[U_RAMZA].pa=30;
    pulse();CHECK(W->cast.victim[10].amount,51);st.unit[U_RAMZA].pa=1;hp=st.unit[10].hp;impact();CHECK(st.unit[10].hp,hp-51);
}
static void clock_edges(void)
{
    u8 i,n;prepare_performance(94);
    for(i=0;i<NUNIT;i++){st.unit[i].sp=0;st.unit[i].ct=0;}
    CHECK(cast_order(U_RAMZA,first,94,0),2);n=0;
    while(ct_next()!=CT_DUE&&n++<8){}CHECK(n<8,1);CHECK(st.pending[U_RAMZA].ticks,0);
    CHECK(charge_resolve(),1);CHECK(st.pending[U_RAMZA].ticks,6);
    // Quick may interrupt after a whole pulse settles, never in the live root.
    st.unit[10].ct=255;CHECK(charge_resolve(),1);CHECK(st.atk,U_RAMZA);
    impact();st.primary.reward=0;st.unit[U_RAMZA].ct=100;
    end_anim();CHECK(st.act,10);CHECK(st.unit[10].ct,0);CHECK(st.pending[U_RAMZA].ticks,6);
}
static void source_geometry(void)
{
    u8 k,i;u16 c;
    static const u8 ids[]={255,5,2,8,31,35,30,1,15,10,12};
    for(k=0;k<sizeof source_performance_masks/sizeof source_performance_masks[0];k++){
        const u16 *p=source_performance_masks[k];prepare_performance(p[0]);c=0;
        for(i=0;i<NUNIT;i++){
            Unit *u=&st.unit[i];
            // The original geometry fixture has eleven actors; the capacity bank has thirteen.
            if(i>=sizeof ids){u->x=255;u->dc=DC_GONE;continue;}
            while(c<MAP_N&&map_tiles[c].walk!=1)c++;
            CHECK(c<MAP_N,1);unit_place(u,(u8)c++);u->team=i%3;u->hp=999;u->dc=3;
            memset(&u->statuses,0,sizeof u->statuses);
            if(p[1]&&ids[i]!=255){u->statuses.inflicted[ids[i]>>3]|=128>>(ids[i]&7);if(ids[i]==1)u->dc=DC_CRYSTAL;if(ids[i]==15)u->dc=DC_CHEST;}
            if(p[1]==2&&i==10){u->x=255;u->dc=DC_GONE;}
            unit_status_refresh(u);
        }
        CHECK(cast_targets(0,UNIT_CELL(&st.unit[0]),p[0],0),p[2]);
    }
}
static void source_clocks(void)
{
    u8 k,j,i,waits;
    for(k=0;k<sizeof source_performance_clocks/sizeof source_performance_clocks[0];k++){
        const u8 *p=source_performance_clocks[k];prepare_performance(p[0]);
        for(i=0;i<NUNIT;i++){st.unit[i].sp=0;st.unit[i].ct=0;}
        CHECK(cast_order(U_RAMZA,first,p[0],0),2);st.pending[U_RAMZA].ticks=1;
        for(j=0;j<3;j++){
            waits=0;while(ct_next()!=CT_DUE&&waits++<8){}CHECK(waits<8,1);
            CHECK(st.pending[U_RAMZA].ticks,0);CHECK(charge_resolve(),1);
            CHECK(st.pending[U_RAMZA].ticks,p[1]);CHECK(core_status_end_ct(&st.unit[U_RAMZA].statuses,0,0,0),p[2]);
            unit_place(&st.unit[U_RAMZA],j&1?first:third);CHECK(st.pending[U_RAMZA].state,DEFER_PERFORMING);CHECK(p[3],0);
            // Like the source clock oracle, isolate the event from its effect dispatcher.
            st.action.phase=CTX_IDLE;W->cast.phase=CAST_BATCH_EMPTY;st.open_resolving=0;
        }
    }
}
static void stat_bounds(void)
{
    AbId ab;u8 i;Unit *t;u8 *value,max;u16 before;
    for(ab=88;ab<=97;ab++)if(ab<=90||ab>=95){
        for(i=0;i<2;i++){
            prepare_performance(ab);t=&st.unit[10];value=ab==88||ab==95?&t->sp:ab==89||ab==96?&t->pa:&t->ma;
            max=ab==88||ab==95?50:99;*value=i?max:1;before=*value;
            CHECK(cast_order(U_RAMZA,first,ab,0),2);rng32=1;pulse();impact();
            CHECK(*value,ab<93?(before<max?before+1:max):(before>1?before-1:1));
            impact();CHECK(*value,ab<93?(before<max?before+1:max):(before>1?before-1:1));
        }
    }
}
static void performance_ai(void)
{
    AbId ab;u8 k,found;u16 steps;
    for(ab=86;ab<100;ab++){
        prepare_performance(ab);st.unit[10].hp=1;st.unit[10].mp=100;st.unit[10].ct=80;
        st.unit[10].set=11;st.unit[10].learned=0xffff;
        st.act=U_RAMZA;st.moved=1;st.acted=0;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,1);
    }
    prepare_performance(94);CHECK(cast_order(U_RAMZA,first,94,0),2);
    st.act=U_RAMZA;st.acted=0;st.moved=1;st.unit[U_RAMZA].defend=1;W->ai_k=0;steps=0;
    while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);CHECK(W->ai_n,0);
    CHECK(st.ai_ab,AI_NONE);CHECK(st.pending[U_RAMZA].state,DEFER_PERFORMING);CHECK(st.pending[U_RAMZA].ticks,6);
}
static u8 wave_source_slot(u8 id){return id==0?0:id==U_RAMZA?12:6;}
static void performance_wave(void)
{
    u8 i;prepare_performance(94);st.unit[0]=st.unit[U_RAMZA];unit_place(&st.unit[0],third);
    for(i=0;i<NUNIT;i++){st.unit[i].sp=0;st.unit[i].ct=0;}
    CHECK(cast_order(0,third,94,0),2);CHECK(cast_order(U_RAMZA,first,94,0),2);
    st.pending[0].ticks=st.pending[U_RAMZA].ticks=1;
    CHECK(ct_next(),CT_DUE);CHECK(charge_resolve(),1);
    CHECK(0x200|wave_source_slot(st.atk),source_performance_wave[0]);CHECK(st.pending[0].ticks,6);
    st.unit[10].ct=255;CHECK(charge_resolve(),1);CHECK(st.atk,0);
    impact();st.primary.reward=0;end_anim();CHECK(0x100|wave_source_slot(st.act),source_performance_wave[1]);
    CHECK(st.pending[U_RAMZA].ticks,0);turn_end(1,1);
    CHECK(0x200|wave_source_slot(st.atk),source_performance_wave[2]);CHECK(st.pending[0].ticks,6);
    CHECK(st.pending[U_RAMZA].ticks,6);
    // Don't Act skips a pulse while retaining its next period; Stop freezes it.
    prepare_performance(94);CHECK(cast_order(U_RAMZA,first,94,0),2);change(U_RAMZA,SID_DONT_ACT);
    st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),0);CHECK(st.pending[U_RAMZA].ticks,6);
    CHECK(st.pending[U_RAMZA].state,DEFER_PERFORMING);
    prepare_performance(94);CHECK(cast_order(U_RAMZA,first,94,0),2);change(U_RAMZA,SID_STOP);
    for(i=0;i<NUNIT;i++){st.unit[i].sp=0;st.unit[i].ct=0;}
    CHECK(ct_next(),CT_WAIT);CHECK(st.pending[U_RAMZA].ticks,6);
    CHECK(st.unit[U_RAMZA].status_ct[SID_STOP-SID_POISON],12);
}
static void performance_text(void)
{
    static const AbId ids[]={88,89,90,92,95,96,97,99};
    static const char *expected[]={"Sp+1 50%","PA+1 50%","MA+1 50%","Quick 50%",
                                  "Sp-1 50%","PA-1 50%","MA-1 50%","CT 0 34%"};
    u8 k,j,n;u32 seed;char text[28];
    for(k=0;k<sizeof ids/sizeof ids[0];k++){
        prepare_performance(ids[k]);st.act=U_RAMZA;st.abil=ids[k];cursor_place(first);
        n=0;while(expected[k][n])n++;
        seed=rng32;forecast_text(U_RAMZA,10,ids[k],text);CHECK(memcmp(text,expected[k],n+1)==0,1);
        area_control(AREA_BEGIN);CHECK(st.mode,M_AREA);
#ifdef FFT_MODULES
        mod_call(MOD_AREA_UI,AREA_DRAW,0);
#else
        area_draw(0);
#endif
        for(j=0;j<W->area_ui.count;j++)if(W->area_ui.ids[j]==10)break;
        CHECK(j<W->area_ui.count,1);CHECK(memcmp(W->area_ui.line[j],expected[k],n+1)==0,1);
        CHECK(rng32==seed,1);CHECK(st.acted,0);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
    }
}
#ifndef PERFORMANCE_MATH_ONLY
static void performance_trials(void){
#ifndef PERFORMANCE_RUNTIME_ONLY
    formula_profiles();
#endif
    lifecycle();clock_edges();source_geometry();source_clocks();stat_bounds();performance_ai();performance_wave();performance_text();}
#else
static void performance_trials(void){formula_profiles();}
#endif
#ifndef PERFORMANCE_LABEL
#define PERFORMANCE_LABEL "performance"
#endif
#ifndef __m68k__
int main(void){sw_init(61);performance_trials();printf(PERFORMANCE_LABEL " %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();performance_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
