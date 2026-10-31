// Punch Art support: Stigma Magic, Chakra and Revive. Source formula/finalizer
// oracle, immediate production casts, gates and AI use (CORE_PUNCH_ART.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-punch-art/oracle/source.h"
static void prepare_punch(AbId ab)
{
    prepare_cleanse(ab);st.unit[10].team=TEAM_PLAYER;st.unit[10].ct=10;
}
static void seed_target(Unit *t,const u8 *r,u8 mode,u16 max_hp,u16 max_mp)
{
    u8 j,dead=mode==2||mode==3;
    memset(&t->statuses,0,sizeof t->statuses);
    for(j=0;j<5;j++)t->statuses.inflicted[j]=mode==1||mode==5?r[j+1]:0;
    if(mode==5)t->statuses.inflicted[1]|=0x80;
    if(dead)t->statuses.inflicted[0]|=0x20;
    if(mode==3||mode==4)t->statuses.inflicted[0]|=0x10;
    if(mode==6){t->statuses.inflicted[2]|=0x20;t->statuses.inflicted[4]|=1;}
    t->max_hp=max_hp;t->hp=dead?0:max_hp/3+1;t->max_mp=max_mp;t->mp=max_mp/4;t->dc=3;
    unit_status_refresh(t);
}
static void profiles(void)
{
    u16 i;u8 k,j,ok;u32 expected;CastBatch b;CastVictim v;
    for(i=0;i<sizeof source_punch/sizeof source_punch[0];i++){
        const s16 *p=source_punch[i];AbId ab=(AbId)p[0];Unit *a,*t;const u8 *r;u8 ref;
        // Frog may only Attack or Frog and Berserk only Attack (CORE_CONTROL_STATUS):
        // the source attacker XA rows are unreachable for Punch Art.
        if(p[7])continue;   // not emitted; kept as a guard
        prepare_punch(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];r=cj_section(common,CJ_STATUS)+6*cj_range(common,ab)[11];
        a->pa=(u8)p[1];a->zodiac=(u8)p[2];t->zodiac=(u8)p[3];t->female=(u8)p[4];
        st.support[U_RAMZA]=p[5]?SUPPORT_ATTACK_UP:p[6]?SUPPORT_MARTIAL_ARTS:0;
        seed_target(t,r,(u8)p[8],(u16)p[9],(u16)p[10]);
        ref=ab==107?10:U_RAMZA;   // Revive aims at a panel; Stigma/Chakra centre on the user
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=ref;b.follow=1;b.ability=ab;
        rng32=1;CHECK(cast_forecast(&b),1);v=b.victim[10];CHECK(b.mask>>10&1,1);
        CHECK(v.hit,p[11]?p[12]:0);CHECK(rng32==1,1);
        rng32=1;for(k=0;k<p[23];k++)rand15();expected=rng32;rng32=1;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);v=b.victim[10];
        CHECK(v.hit,p[13]);CHECK(rng32==expected,1);
        if(!p[13])continue;
        if(ab==105){
            CHECK(v.effect,AB_CLEANSE);CHECK(v.amount,core_cleanse_pack(&t->statuses,r+1));
            for(ok=1,j=0;j<5;j++)if((core_status_current(&t->statuses,j)&r[j+1])!=p[18+j])ok=0;
            CHECK(ok,1);
        }else if(ab==106){
            CHECK(v.effect,AB_HP_MP_HEAL);CHECK(v.amount,p[15]);CHECK(v.amount>>1,p[16]);
        }else if(p[17]&0x80){
            CHECK(v.effect,AB_DAMAGE);CHECK(v.amount,p[14]);
        }else{
            CHECK(v.effect,AB_REVIVE);CHECK(v.amount,p[15]);CHECK(p[18]&0x20,0x20);
        }
    }
}
static void punch_production(void)
{
    Unit *a,*t;const u8 *r;u8 j;
    // Chakra: immediate (no CT, no MP), radius 1 around the user, the user included.
    prepare_punch(106);a=&st.unit[U_RAMZA];t=&st.unit[10];a->pa=20;
    a->hp=100;a->mp=10;t->hp=200;t->mp=0;
    CHECK(cast_order(U_RAMZA,U_RAMZA,106,1),1);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);impact();
    CHECK(a->hp,200);CHECK(a->mp,60);CHECK(t->hp,300);CHECK(t->mp,50);CHECK(st.acted,1);
    // Revive: a KO ally rises with 20% max HP; its death counter resets.
    prepare_punch(107);a=&st.unit[U_RAMZA];t=&st.unit[10];a->pa=99;
    seed_target(t,RT_NULL,2,999,50);t->dc=1;
    CHECK(cast_order(U_RAMZA,10,107,1),1);impact();
    CHECK(t->hp,200);CHECK(t->dc,3);CHECK(core_status_current(&t->statuses,0)&0x20,0);
    // On an Undead the same 20% is damage.
    prepare_punch(107);a=&st.unit[U_RAMZA];t=&st.unit[10];a->pa=99;
    seed_target(t,RT_NULL,4,999,50);
    CHECK(cast_order(U_RAMZA,10,107,1),1);impact();CHECK(t->hp,334-200);
    // Stigma Magic cancels its whole set from the user and its neighbours.
    prepare_punch(105);a=&st.unit[U_RAMZA];t=&st.unit[10];
    r=cj_section(common,CJ_STATUS)+6*abilities[105].y;seed_target(t,r,1,999,50);
    CHECK(cast_order(U_RAMZA,U_RAMZA,105,1),1);impact();
    for(j=0;j<5;j++)CHECK(t->statuses.inflicted[j]&r[j+1],0);
}
static void gates(void)
{
    static const AbId gated[]={103,104};
    u8 j;Unit *a;
    // Earth Slash (line) and Secret Fist (0x50) stay gated; Spin, Wave and
    // Repeating Fist are core-punch-strikes-check's.
    for(j=0;j<sizeof gated/sizeof gated[0];j++)CHECK(abilities[gated[j]].kind,255);
    // A Frog may use none of them.
    prepare_punch(106);a=&st.unit[U_RAMZA];CHECK(can_use(U_RAMZA,106),1);
    a->statuses.inflicted[2]|=2;unit_status_refresh(a);CHECK(can_use(U_RAMZA,106),0);
    // Punch Art is not magic: Silence leaves it usable and Reflect does not bounce it.
    prepare_punch(105);a=&st.unit[U_RAMZA];a->statuses.inflicted[1]|=8;unit_status_refresh(a);
    CHECK(can_use(U_RAMZA,105),1);
    st.unit[10].statuses.inflicted[4]|=2;unit_status_refresh(&st.unit[10]);CHECK(magic_reflects(10,105),0);
}
static u8 plan(u8 who)
{
    u16 steps=0;st.act=who;st.moved=0;st.acted=0;W->ai_k=0;
    while(!ai_think()&&++steps<2048){}
    return steps<2048;
}
static u8 ranked(AbId ab)
{
    u8 k;for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)return 1;
    return 0;
}
// Immediate AI: Chakra on a hurt ally, Revive on a KO ally.
static void ai(void)
{
    prepare_punch(106);st.unit[U_RAMZA].pa=20;st.unit[10].hp=100;CHECK(plan(U_RAMZA),1);CHECK(ranked(106),1);
    prepare_punch(106);CHECK(plan(U_RAMZA),1);CHECK(ranked(106),0);
    prepare_punch(107);st.unit[U_RAMZA].pa=99;seed_target(&st.unit[10],RT_NULL,2,999,50);
    CHECK(plan(U_RAMZA),1);CHECK(ranked(107),1);
}
static void punch_trials(void){profiles();punch_production();gates();ai();}
#ifndef __m68k__
int main(void){sw_init(61);punch_trials();printf("punch %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();punch_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
