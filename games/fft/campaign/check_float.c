// Float's production status, source formulas, terrain costs and once-only ownership.
#define CLEANSE_RUNTIME_CHECK
#define FFT_CHECK_NATIVE_ORDINAL
#include "check_cleanse.c"
#include "x/core-float/oracle/source.h"
static void prepare_float(void)
{
    u8 i;prepare_cleanse(38);
    for(i=0;i<NUNIT;i++)if(i!=U_RAMZA&&i!=10){st.unit[i].dc=DC_GONE;st.unit[i].x=255;}
}
static void profiles(void)
{
    u16 i,c;u8 j;CastBatch b;u32 after;
    for(i=0;i<sizeof source_float/sizeof source_float[0];i++){
        const u16 *p=source_float[i];Unit *a,*t;prepare_float();a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p[1];a->faith=p[2];t->faith=p[3];st.support[U_RAMZA]=p[4]?SUPPORT_MAGIC_ATTACK_UP:0;
        st.support[10]=p[5]?SUPPORT_MAGIC_DEFENSE_UP:0;if(p[6])t->statuses.inflicted[3]|=16;
        a->zodiac=p[7];t->zodiac=p[8];t->female=p[9];t->eq[EQ_ACC]=p[11]?232:255;
        if(p[10]==1)t->statuses.immune[2]|=64;
        if(p[10]==2)t->statuses.inflicted[2]|=64;
        if(p[10]==3)t->statuses.innate[2]|=64;
        unit_status_refresh(a);unit_status_refresh(t);
        for(c=0;c<MAP_N;c++)if(abs((s16)TILE_X[c]-t->x)+abs((s16)TILE_Z[c]-t->z)>4)break;
        CHECK(c<MAP_N,1);unit_place(a,(u8)c);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=38;
        rng32=1;ZB(61);CHECK(cast_forecast(&b),1);ZE(61);
        CHECK(b.mask,(u16)1<<10);CHECK(b.victim[10].hit,p[12]?p[13]:0);
        CHECK(b.victim[10].amount,p[14]);CHECK(rng32==1,1);
        rng32=1;for(j=0;j<p[19];j++)rand15();after=rng32;rng32=1;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
        CHECK(b.victim[10].hit,p[16]);CHECK(b.victim[10].amount,p[18]);CHECK(rng32==after,1);
        CHECK(b.victim[10].effect,p[16]?AB_STATUS:AB_NULL);
    }
}
static void finish_float(void)
{
    u8 n;for(n=0;n<20&&st.action.phase!=CTX_IDLE;n++)end_anim();CHECK(st.action.phase,CTX_IDLE);
}
static void producer(void)
{
    u8 follow,j;u32 seed;Unit *a,*t;u16 cost;
    for(follow=0;follow<2;follow++){
        prepare_float();a=&st.unit[U_RAMZA];t=&st.unit[10];a->faith=t->faith=100;
        CHECK(abilities[38].kind,AB_STATUS);CHECK(abilities[38].range,4);CHECK(abilities[38].vert,255);
        cost=skill_cost(U_RAMZA,38);CHECK(cost,8);
        CHECK(cast_order(U_RAMZA,follow?10:second,38,follow),2);
        CHECK(st.pending[U_RAMZA].ticks,2);CHECK(a->mp,999);
        st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(a->mp,999-cost);
        CHECK(W->cast.victim[10].amount,0x240);CHECK(t->statuses.inflicted[2]&64,0);
        seed=rng32;ZB(62);impact();ZE(62);CHECK(t->statuses.inflicted[2]&64,64);
        CHECK(rng32==seed,1);CHECK(t->hp,999);CHECK(t->mp,999);CHECK(st.pending[10].state,DEFER_NONE);
        for(j=0;j<16;j++)CHECK(t->status_ct[j],0); // Float has no CT expiry slot.
        impact();CHECK(t->statuses.inflicted[2]&64,64);CHECK(a->mp,999-cost);finish_float();
        CHECK(skill_hit(U_RAMZA,10,38),0); // Existing permanent Float is no new infliction.
        act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=1;st.dmg=1;st.push=0;impact();
        CHECK(t->hp,998);CHECK(t->statuses.inflicted[2]&64,64);finish_float();
        {CoreStatusDelta d;memset(&d,0,sizeof d);d.remove[2]=64;
         CHECK(battle_status_apply(10,&d,0)!=STATUS_REJECTED,1);CHECK(t->statuses.inflicted[2]&64,0);}
    }
    prepare_float();st.unit[U_RAMZA].mp=7;CHECK(cast_order(U_RAMZA,10,38,1),0);CHECK(st.acted,0);
    st.support[U_RAMZA]=SUPPORT_HALF_MP;st.unit[U_RAMZA].mp=4;
    CHECK(cast_order(U_RAMZA,10,38,1),2);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);
    CHECK(st.unit[U_RAMZA].mp,0);impact();finish_float();
    prepare_float();change(U_RAMZA,SID_SILENCE);CHECK(can_use(U_RAMZA,38),0);
    prepare_float();change(U_RAMZA,SID_FROG);CHECK(can_use(U_RAMZA,38),0);
    prepare_float();st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
    CHECK(cast_order(U_RAMZA,10,38,1),1);impact();finish_float();CHECK(st.unit[10].statuses.inflicted[2]&64,64);
    // Real producer makes the existing Earth/null query visible to later spells.
    CHECK(skill_kind(U_RAMZA,10,0x40),AB_NULL); // Titan is Earth.
    CHECK(skill_hit(U_RAMZA,10,0x40),0);
    st.acted=0;st.unit[10].team=TEAM_ENEMY; // Source Dispel uses the shared cancellation packet.
    CHECK(cast_order(U_RAMZA,10,56,1),1);impact();finish_float();
    CHECK(core_status_current(&st.unit[10].statuses,2)&64,0);
    CHECK(skill_kind(U_RAMZA,10,0x40),AB_DAMAGE);
}
static void ai_and_reach(void)
{
    u8 k,found=0,*baseline;u16 steps=0,c,extra=0;
    prepare_float();st.moved=1;W->ai_k=0;
    while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==38)found=1;CHECK(found,1);
    prepare_float();baseline=malloc(MAP_N);CHECK(baseline!=RT_NULL,1);if(!baseline)return;
    ZB(59);compute_reach(U_RAMZA);ZE(59);memcpy(baseline,st.reach,MAP_N);
    change(U_RAMZA,SID_FLOAT);CHECK(W->from_who,0);ZB(60);compute_reach(U_RAMZA);ZE(60);
    for(c=0;c<MAP_N;c++){
        if(baseline[c]!=255)CHECK(st.reach[c]<=baseline[c],1);
        if(st.reach[c]!=255&&st.reach[c]<baseline[c])extra++;
    }
    CHECK(extra>0,1);
    {CoreStatusDelta d;memset(&d,0,sizeof d);d.remove[2]=64;battle_status_apply(U_RAMZA,&d,0);}
    compute_reach(U_RAMZA);CHECK(memcmp(baseline,st.reach,MAP_N)==0,1);free(baseline);
}
static void source_moves(void)
{
    u16 n;u8 mode,*dist;
    prepare_float();dist=malloc(MAP_N);CHECK(dist!=RT_NULL,1);if(!dist)return;
    for(n=0;n<sizeof source_float_movement/sizeof source_float_movement[0];n++)for(mode=0;mode<2;mode++){
        const u8 *p=source_float_movement[n];Unit *a=&st.unit[U_RAMZA];
        if(!p[0])continue; // Normal source controls retain their own provenance; existing native water adaptation is separate.
        memset(W->en,0,MAP_N);memset(W->hw,1,MAP_N);W->hw[0]|=p[1]<<1;W->hw[1]|=p[2]<<1;
        EDGE_ROW(0)[0]=1;EDGE_ROW(0)[1]=p[2]<<1;W->en[0]=2;W->ej=step_jump_cap(a->jump);
        a->statuses.inflicted[2]=p[0]?64:0;unit_status_refresh(a);
        ZB(63);spread_start(U_RAMZA,0,p[3],dist,RT_NULL,mode?CORE_NAV_WIDE:0,RT_NULL,0);
        while(!spread_go(1)){}ZE(63);
        CHECK(!!(W->nav.out&CORE_NAV_FLOAT),p[0]);CHECK(dist[1],p[4]);
    }
    free(dist);
}
static void float_trials(void){profiles();producer();ai_and_reach();source_moves();}
#ifndef __m68k__
int main(void){sw_init(61);float_trials();printf("floating %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();float_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
