// Talk Skill: Persuade, Praise, Threaten, Preach, Solution, Death Sentence, Insult, Mimic Daravon.
// Source formula/finalizer oracle, production commits, gates and AI use (CORE_TALK_SKILL.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-talk-skill/oracle/source.h"
static void prepare_talk(AbId ab)
{
    prepare_cleanse(ab);st.unit[10].team=TEAM_ENEMY;st.unit[10].ct=10;st.unit[10].brave=st.unit[10].faith=50;
}
static void seed_mode(Unit *t,u8 mode)
{
    memset(&t->statuses,0,sizeof t->statuses);
    if(mode==1)t->statuses.inflicted[4]|=0x10;
    if(mode==2)t->statuses.inflicted[2]|=0x08;
    if(mode==3)t->statuses.immune[2]|=0x08;
    if(mode==4)t->statuses.immune[4]|=0x10;
    unit_status_refresh(t);
}
static u16 stat_amount(u8 change){return change&0x80?(u16)(0U-(change&0x7f)):change;}
static void profiles(void)
{
    u16 i;u8 k,j;u32 expected;CastBatch b;CastVictim v;
    for(i=0;i<sizeof source_talk/sizeof source_talk[0];i++){
        const u8 *p=source_talk[i];AbId ab=(AbId)p[0];Unit *a,*t;u16 packed=0;
        prepare_talk(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=(u8)p[1];a->zodiac=(u8)p[2];t->zodiac=(u8)p[3];t->female=(u8)p[4];
        st.support[U_RAMZA]=p[5]?SUPPORT_MAGIC_ATTACK_UP:0;seed_mode(t,(u8)p[6]);
        if(ab==125)unit_place(t,third);   // keep the speaker out of the one-victim source profile
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=ab;
        rng32=1;CHECK(cast_forecast(&b),1);CHECK(b.mask>>10&1,1);v=b.victim[10];
        CHECK(v.hit,p[8]?p[9]:0);CHECK(rng32==1,1);
        rng32=(u32)p[7];for(k=0;k<p[21];k++)rand15();expected=rng32;rng32=(u32)p[7];
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);v=b.victim[10];
        CHECK(v.hit,p[10]);CHECK(rng32==expected,1);
        if(!p[10])continue;
        if(ab==117){CHECK(v.effect,AB_QUICK);CHECK(v.amount,0x100);CHECK(p[12],127);}
        else if(ab<=119){CHECK(v.effect,AB_BRAVE);CHECK(v.amount,stat_amount((u8)p[13]));}
        else if(ab<=121){CHECK(v.effect,AB_FAITH);CHECK(v.amount,stat_amount((u8)p[14]));}
        else{
            for(j=0;j<5;j++)if(p[16+j])packed=(u16)(j<<8|p[16+j]);
            CHECK(v.effect,AB_STATUS);CHECK(v.amount,packed);
        }
    }
}
static void talk(AbId ab,u8 ma)
{
    st.unit[U_RAMZA].ma=ma;CHECK(cast_order(U_RAMZA,10,ab,1),1);impact();
}
static void talk_production(void)
{
    Unit *t;
    prepare_talk(118);t=&st.unit[10];t->brave=98;talk(118,99);CHECK(t->brave,100);
    prepare_talk(119);t=&st.unit[10];t->brave=25;talk(119,99);CHECK(t->brave,5);
    CHECK(core_status_current(&t->statuses,2)&4,4);               // Chicken below 10
    prepare_talk(120);t=&st.unit[10];t->faith=50;talk(120,99);CHECK(t->faith,54);
    prepare_talk(121);t=&st.unit[10];t->faith=12;talk(121,99);CHECK(t->faith,0);
    prepare_talk(117);t=&st.unit[10];t->ct=77;talk(117,99);CHECK(t->ct,0);
    prepare_talk(124);t=&st.unit[10];talk(124,99);
    CHECK(core_status_current(&t->statuses,2)&8,8);CHECK(t->status_flags&STATUS_NO_CONTROL,STATUS_NO_CONTROL);
    // Mimic Daravon: radius 1 around the chosen panel, the speaker included.
    prepare_talk(125);t=&st.unit[10];talk(125,99);
    CHECK(core_status_current(&t->statuses,4)&0x10,0x10);
    CHECK(core_status_current(&st.unit[U_RAMZA].statuses,4)&0x10,0x10);
    // A sleeper never listens: no draw, no effect.
    prepare_talk(119);t=&st.unit[10];t->brave=50;seed_mode(t,1);
    {u32 seed=rng32;CHECK(magic_hit(U_RAMZA,10,119,1),0);CHECK(rng32==seed,1);}
    CHECK(abilities[116].kind,255);CHECK(abilities[123].kind,255);   // Invitation, Negotiate
}
static u8 upkeep(u8 id)
{
#ifdef FFT_MODULES
    return (u8)mod_call(MOD_FORMULA,15,id);
#else
    return status_upkeep(id);
#endif
}
// Source end-of-turn upkeep: Death Sentence's turn counter, its expiry, Undead, Brave +1.
static void sentence(void)
{
    u8 i;Unit *t;
    for(i=0;i<sizeof source_sentence_upkeep/sizeof source_sentence_upkeep[0];i++){
        const u8 *p=source_sentence_upkeep[i];u8 brave;
        prepare_talk(122);t=&st.unit[10];memset(&t->statuses,0,sizeof t->statuses);
        t->statuses.inflicted[4]=1;if(p[0])t->statuses.inflicted[0]=16;
        t->status_ct[15]=p[1];t->brave=p[2];unit_status_refresh(t);
        brave=(u8)(p[2]+(p[6]&0x80?p[6]&0x7f:0));
        {u16 hp=t->hp;CHECK(upkeep(10),p[3]==3);CHECK(t->hp,p[3]==3?0:hp);}CHECK(t->brave,brave);
        CHECK(core_status_current(&t->statuses,4)&1,p[5]?0:1);
        if(!p[5])CHECK(t->status_ct[15],p[7]);
    }
    // Production: the timer starts from the source rules and kills after three turns.
    prepare_talk(122);t=&st.unit[10];talk(122,99);
    CHECK(core_status_current(&t->statuses,4)&1,1);CHECK(t->status_ct[15],cj_status_rules(common)[(SID_SENTENCE<<4)+3]);
    for(i=0;i<8&&t->hp;i++)upkeep(10);
    CHECK(i,cj_status_rules(common)[(SID_SENTENCE<<4)+3]);CHECK(t->hp,0);CHECK(t->dc,3);
}
static u8 plan(u8 who)
{
    u16 steps=0;st.act=who;st.moved=0;st.acted=0;W->ai_k=0;
    while(!ai_think()&&++steps<2048){}
    return steps<2048;
}
static void talk_ai(void)
{
    u8 k,found=0;
    prepare_talk(124);st.unit[U_RAMZA].ma=99;CHECK(plan(U_RAMZA),1);
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==124)found=1;
    CHECK(found,1);
}
static void talk_trials(void){profiles();talk_production();sentence();talk_ai();}
#ifndef __m68k__
int main(void){sw_init(61);talk_trials();printf("talk %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();talk_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
