// The seven production resource items; five newly enabled, no normalized overrides.
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-restoration/oracle/source.h"
static const AbId resources[]={368,369,370,371,372,373,381};
static void seed_resources(Unit *t,u16 mhp,u16 mmp,u16 hp,u16 mp,u8 flags)
{
    memset(&t->statuses,0,sizeof t->statuses);memset(t->status_ct,0,sizeof t->status_ct);
    t->max_hp=mhp;t->max_mp=mmp;t->hp=hp;t->mp=mp;
    if(flags&1)t->statuses.inflicted[0]|=16;
    if(flags&2)t->statuses.inflicted[0]|=32;
    if(flags&4)t->statuses.innate[0]|=32;
    if(flags&8)t->statuses.inflicted[1]|=128;
    if(flags&16)t->statuses.inflicted[3]|=1;
    if(flags&32)t->statuses.inflicted[4]|=16;
    unit_status_refresh(t);
}
static void original_resources(void)
{
    u16 i;u8 k;u32 expected;
    for(i=0;i<sizeof source_restoration/sizeof source_restoration[0];i++){
        const u16 *p=source_restoration[i];AbId ab=p[0];Unit *t;
        prepare_cleanse(ab);t=&st.unit[10];seed_resources(t,p[1],p[2],p[3],p[4],p[5]);
        CHECK(restore_ability(ab),1);CHECK(can_use(U_RAMZA,ab),1);
        rng32=1;for(k=0;k<p[10];k++)rand15();expected=rng32;rng32=1;
        skill(U_RAMZA,10,ab);CHECK(st.mode,M_ACT);CHECK(st.hit,p[6]);
        CHECK(st.effect_kind,p[7]);CHECK(st.dmg,p[8]);CHECK(st.effect_mp,p[9]);CHECK(rng32==expected,1);
        CHECK(st.bag[abilities[ab].bag],2);CHECK(t->hp,p[3]);CHECK(t->mp,p[4]);
    }
}
static void production_resources(void)
{
    u8 j,dead;u16 amount,hp,mp,actor_hp;Unit *t;AbId ab;
    for(j=0;j<sizeof resources/sizeof resources[0];j++)for(dead=0;dead<2;dead++){
        ab=resources[j];prepare_cleanse(ab);t=&st.unit[10];
        seed_resources(t,300,100,ab==381&&!dead?0:100,20,dead?1:ab==381?2:0);
        actor_hp=st.unit[U_RAMZA].hp;skill(U_RAMZA,10,ab);CHECK(st.hit,1);
        amount=st.dmg;hp=t->hp;mp=t->mp;
        if(effect_damage(st.effect_kind)){hp=amount>=hp?0:hp-amount;if(st.effect_kind==AB_HP_DAMAGE_MP_HEAL)mp=100;}
        else if(st.effect_kind==AB_MP_HEAL)mp=mp+amount>100?100:mp+amount;
        else{hp=hp+amount>300?300:hp+amount;if(st.effect_kind==AB_HP_MP_HEAL)mp=100;}
        impact();CHECK(t->hp,hp);CHECK(t->mp,mp);CHECK(st.unit[U_RAMZA].hp,actor_hp);
        CHECK(st.bag[abilities[ab].bag],2);impact();CHECK(t->hp,hp);CHECK(t->mp,mp);
        if(ab==381&&!dead){CHECK(core_status_current(&t->statuses,0)&32,0);CHECK(t->dc,3);}
    }
}
static void frozen_resources(void)
{
    Unit *t;u16 hp;u32 rng;char line[32];
    prepare_cleanse(373);t=&st.unit[10];seed_resources(t,300,100,100,0,0);
    W->fc_key=((373+1)<<4)|11;
    skill(U_RAMZA,10,373);CHECK(W->fc_key,0);CHECK(st.dmg,300);CHECK(st.effect_mp,100);
    t->max_hp=600;t->max_mp=200;impact();CHECK(t->hp,400);CHECK(t->mp,100);
    CHECK(st.unit[U_RAMZA].hp,999); // MP payload must never become caster HP cost.
    prepare_cleanse(373);seed_resources(&st.unit[U_RAMZA],300,100,1,0,0);
    skill(U_RAMZA,U_RAMZA,373);impact();CHECK(st.unit[U_RAMZA].hp,300);CHECK(st.unit[U_RAMZA].mp,100);
    prepare_cleanse(371);t=&st.unit[10];seed_resources(t,300,100,100,20,32);
    hp=t->hp;rng=rng32;forecast_text(U_RAMZA,10,371,line);CHECK(memcmp(line,"+20MP 100%",10),0);
    CHECK(rng32==rng,1);skill(U_RAMZA,10,371);CHECK(rng32==rng,1);impact();
    CHECK(t->hp,hp);CHECK(t->mp,40);CHECK(t->statuses.inflicted[4]&16,16);
    prepare_cleanse(373);t=&st.unit[10];seed_resources(t,300,100,100,20,1);
    forecast_text(U_RAMZA,10,373,line);CHECK(memcmp(line,"KO +80MP 100%",12),0);
    skill(U_RAMZA,10,373);impact();CHECK(t->hp,0);CHECK(t->mp,100);
}
static void restoration_trials(void){original_resources();production_resources();frozen_resources();}
#ifndef RESTORATION_RUNTIME_CHECK
#ifndef __m68k__
int main(void){sw_init(61);restoration_trials();printf("restoration %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();restoration_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
