// Guts: Yell (Speed), Cheer Up (Brave) and Scream (Brave, PA, MA, Speed).
// Source formula/finalizer/clamp oracle, production commits, Silence and AI policy (CORE_GUTS.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-guts/oracle/source.h"
static const u8 guts_modes[6][5]={{0},{0x20,0,0,0,0},{0,0x80,0,0,0},{0x10,0,0,0,0},{0,0,4,0,0},{0x30,0,0,0,0}};
// Guts is Ramza's special set (chapters 2..4), outside the generic jobs.
static void prepare_guts(AbId ab)
{
    u8 j,k;prepare_spell(0x13);
    for(j=1;j<0x5e;j++){
        st.unit[U_RAMZA].set=job_sets[j];
        for(k=0;k<SET_N;k++)if(SET_ABILITY(st.unit[U_RAMZA].set,k)==ab)break;
        if(k<SET_N)break;
    }
    CHECK(j<0x5e,1);st.unit[U_RAMZA].learned=(LearnBits)1<<k;CHECK(known(U_RAMZA,0),ab);
    st.unit[8].hp=0;st.unit[8].dc=DC_GONE;st.unit[8].x=255;
    st.unit[10].team=TEAM_PLAYER;st.unit[10].ct=10;
}
static u8 guts_target(AbId ab){return ab==153?U_RAMZA:10;}   // Scream: the user only
static void profiles(void)
{
    u16 i;u8 j;CastBatch b;
    for(i=0;i<sizeof source_guts/sizeof source_guts[0];i++){
        const u8 *p=source_guts[i];AbId ab=(AbId)p[0];u8 id=guts_target(ab);Unit *t;
        if(p[6])continue;                                // no mounts natively
        if(ab==153&&(p[5]==1||p[5]==2||p[5]==5))continue;   // a KO or petrified user cannot act
        prepare_guts(ab);t=&st.unit[id];
        t->pa=p[1];t->ma=p[2];t->sp=p[3];t->brave=p[4];
        memset(&t->statuses,0,sizeof t->statuses);for(j=0;j<5;j++)t->statuses.inflicted[j]=guts_modes[p[5]][j];
        unit_status_refresh(t);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=id;b.follow=1;b.ability=ab;
        rng32=1;CHECK(cast_forecast(&b),1);CHECK((b.mask>>id&1)&&b.victim[id].hit,p[8]);   // KO/petrified: no hit
        if(!p[8])continue;
        CHECK(b.victim[id].hit,100);CHECK(rng32==1,1);
        CHECK(cast_order(U_RAMZA,id,ab,1),1);impact();CHECK(rng32==1,1);   // no draw
        CHECK(t->brave,p[15]);CHECK(t->pa,p[22]);CHECK(t->ma,p[23]);CHECK(t->sp,p[24]);
    }
}
static void guts(AbId ab){CHECK(cast_order(U_RAMZA,guts_target(ab),ab,1),1);impact();}
static void guts_production(void)
{
    Unit *t;
    // Records: immediate casts, no MP, Yell/Cheer Up range 3, Scream on the user.
    CHECK(abilities[150].kind,AB_SPEED);CHECK(abilities[151].kind,AB_BRAVE);CHECK(abilities[153].kind,AB_BRAVE);
    CHECK(abilities[150].range,3);CHECK(abilities[151].range,3);CHECK(abilities[153].range,0);
    CHECK((s8)abilities[150].y,-1);CHECK((s8)abilities[151].y,-5);CHECK((s8)abilities[153].y,-10);CHECK(abilities[153].element,1);
    // Cheer Up lifts Chicken once Brave reaches 10.
    prepare_guts(151);t=&st.unit[10];t->brave=6;t->statuses.inflicted[2]|=4;unit_status_refresh(t);
    CHECK(core_status_current(&t->statuses,2)&4,4);guts(151);CHECK(t->brave,11);CHECK(core_status_current(&t->statuses,2)&4,0);
    // Yell on an enemy is allowed (any unit in range), Speed caps at 50.
    prepare_guts(150);t=&st.unit[10];t->team=TEAM_ENEMY;t->sp=50;guts(150);CHECK(t->sp,50);
    // Not magic: Silence does not block Guts; a Frog may not use it.
    prepare_guts(151);st.unit[U_RAMZA].statuses.inflicted[1]|=8;unit_status_refresh(&st.unit[U_RAMZA]);
    CHECK(can_use(U_RAMZA,151),1);
    st.unit[U_RAMZA].statuses.inflicted[2]|=2;unit_status_refresh(&st.unit[U_RAMZA]);CHECK(can_use(U_RAMZA,151),0);
}
static u8 plan(u8 who)
{
    u16 steps=0;st.act=who;st.moved=0;st.acted=0;W->ai_k=0;
    while(!ai_think()&&++steps<2048){}
    return steps<2048;
}
// AI: the immediate area policy values neither ally Brave/Speed gains nor Scream (as Praise).
static void guts_ai(void)
{
    static const AbId ids[]={150,151,153};
    u8 j,k,found=0;
    for(j=0;j<3;j++){
        prepare_guts(ids[j]);CHECK(plan(U_RAMZA),1);
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ids[j])found=1;
    }
    CHECK(found,0);
}
static void guts_trials(void){profiles();guts_production();guts_ai();}
#ifndef __m68k__
int main(void){sw_init(61);guts_trials();printf("guts %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();guts_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
