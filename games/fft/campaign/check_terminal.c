// Four production decisive effects, bounded source oracle and actual lifecycle.
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-terminal/oracle/source.h"
static const AbId terminal[]={30,41,51,59};
static void prepare_terminal(AbId ab)
{
    prepare_cleanse(ab);st.unit[10].team=ab==41?TEAM_PLAYER:TEAM_ENEMY;
    st.unit[10].ct=10;
}
static void profiles(void)
{
    u16 i,c;u8 k;u32 expected;CastBatch b;CastVictim v;
    for(i=0;i<sizeof source_terminal/sizeof source_terminal[0];i++){
        const u16 *p=source_terminal[i];AbId ab=p[0];Unit *a,*t;const u8 *r;
        prepare_terminal(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p[1];a->faith=p[2];t->faith=p[3];st.support[U_RAMZA]=p[4]?SUPPORT_MAGIC_ATTACK_UP:0;
        st.support[10]=p[5]?SUPPORT_MAGIC_DEFENSE_UP:0;if(p[6])t->statuses.inflicted[3]|=16;
        a->zodiac=p[7];t->zodiac=p[8];t->female=p[9];t->eq[EQ_ACC]=p[11]?232:255;
        r=cj_section(common,CJ_STATUS)+6*cj_range(common,ab)[11];
        if(p[10]==1)memcpy(t->statuses.immune,r+1,5);
        if(p[10]==2||p[10]==3)memcpy(t->statuses.inflicted,r+1,5);
        if(p[10]==3)memcpy(t->statuses.innate,r+1,5);
        if(p[10]==4)t->statuses.inflicted[0]|=32;
        if(p[10]==5)t->statuses.inflicted[1]|=128;
        if(p[10]==6)t->statuses.inflicted[3]|=1;
        if(p[10]==7)t->statuses.inflicted[0]|=16;
        if(p[10]==8)t->status_flags|=STATUS_IMMORTAL;
        if(p[10]==9){t->statuses.inflicted[0]|=8;t->statuses.inflicted[3]|=8;t->statuses.inflicted[4]|=16;}
        unit_status_refresh(t);
        for(c=0;c<MAP_N;c++)if(abs((s16)TILE_X[c]-t->x)+abs((s16)TILE_Z[c]-t->z)>4)break;
        CHECK(c<MAP_N,1);unit_place(a,(u8)c);memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=ab;
        rng32=1;CHECK(cast_forecast(&b),1);v=b.victim[10];
        CHECK(v.hit,p[12]?p[13]:0);CHECK(v.effect,p[14]==AB_DAMAGE&&ab==30?AB_DEATH:p[14]);CHECK(v.amount,p[15]);CHECK(rng32==1,1);
        rng32=1;for(k=0;k<p[21];k++)rand15();expected=rng32;rng32=1;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);v=b.victim[10];CHECK(v.hit,p[17]);CHECK(v.effect,p[19]==AB_DAMAGE&&ab==30?AB_DEATH:p[19]);CHECK(v.amount,p[20]);CHECK(rng32==expected,1);
    }
}
static void terminal_production(void)
{
    u8 j,k,follow;Unit *t;u16 hp,cell,ct;
    for(j=0;j<sizeof terminal/sizeof terminal[0];j++)for(follow=0;follow<2;follow++){
        AbId ab=terminal[j];prepare_terminal(ab);t=&st.unit[10];st.unit[U_RAMZA].ma=99;
        st.unit[U_RAMZA].faith=t->faith=100;hp=t->hp;cell=UNIT_CELL(t);ct=t->ct;
        CHECK(cast_order(U_RAMZA,follow?10:second,ab,follow),2);CHECK(st.unit[U_RAMZA].mp,999);
        st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(W->cast.victim[10].hit,1);
        CHECK(st.unit[U_RAMZA].mp,999-cj_range(common,ab)[13]);impact();
        if(ab==30){CHECK(t->hp,0);CHECK(t->dc,3);CHECK(t->kos,1);}
        if(ab==41){CHECK(t->ct,255);CHECK(t->hp,hp);}
        if(ab==51){CHECK(core_status_current(&t->statuses,0)&16,16);CHECK(t->hp,hp);}
        if(ab==59){CHECK(unit_petrified(t),128);CHECK(t->hp,hp);CHECK(t->ct,ct);CHECK(unit_can_act(t),0);CHECK(unit_can_move(t),0);}
        CHECK(UNIT_CELL(t),cell);impact();CHECK(UNIT_CELL(t),cell);
        for(k=0;k<16&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.action.phase,CTX_IDLE);
    }
}
static void ai_and_guards(void)
{
    u8 j,k,found;u16 steps;CastBatch b;u32 seed;
    for(j=0;j<sizeof terminal/sizeof terminal[0];j++){
        AbId ab=terminal[j];prepare_terminal(ab);st.unit[U_RAMZA].ma=99;st.moved=1;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;CHECK(found,1);
        prepare_terminal(ab);st.unit[U_RAMZA].mp=0;seed=rng32;CHECK(cast_order(U_RAMZA,10,ab,1),0);CHECK(rng32==seed,1);
        prepare_terminal(ab);change(U_RAMZA,SID_SILENCE);CHECK(cast_order(U_RAMZA,10,ab,1),0);
        prepare_terminal(ab);memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=ab;
        st.unit[10].x=255;CHECK(cast_forecast(&b),0);CHECK(b.mask&((u16)1<<10),0);
    }
}
static void terminal_trials(void){profiles();terminal_production();ai_and_guards();}
#ifndef TERMINAL_RUNTIME_CHECK
#ifndef __m68k__
int main(void){sw_init(61);terminal_trials();printf("terminal %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();terminal_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
