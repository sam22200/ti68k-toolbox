// Source hand/scalar policy and actual primary/Counter dual-strike ownership.
#define REACTION_SEQUENCE_CHECK
#include "check_reactions.c"
#include "x/core-hands/oracle/source.h"
static void source_policy(void)
{
    u16 i;Unit *a,*t;
    for(i=0;i<sizeof source_hands/sizeof source_hands[0];i++){
        const u16 *p=source_hands[i];arena();a=&st.unit[U_RAMZA];
        a->eq[EQ_WEAPON]=p[0];a->eq[EQ_SHIELD]=p[1];st.support[U_RAMZA]=p[2]?SUPPORT_TWO_SWORDS:0;
        CHECK(hand_count(U_RAMZA),p[3]);CHECK(hand_weapon(a,0),p[4]?p[4]:255);CHECK(hand_weapon(a,1),p[5]?p[5]:255);
    }
    for(i=0;i<sizeof source_hand_damage/sizeof source_hand_damage[0];i++){
        const u16 *p=source_hand_damage[i];arena();a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->eq[EQ_WEAPON]=p[0];a->eq[EQ_SHIELD]=255;a->pa=p[1];a->sp=p[2];a->brave=p[3];
        a->zodiac=0;a->female=t->female=0;t->zodiac=p[5];if(p[4])change(10,SID_SLEEP);
        CHECK(attack_damage(U_RAMZA,10,0,0),p[6]);
    }
    arena();a=&st.unit[U_RAMZA];a->job=89;a->eq[EQ_WEAPON]=a->eq[EQ_SHIELD]=255;
    CHECK(hand_count(U_RAMZA),2);a->job=1;CHECK(hand_count(U_RAMZA),1);
    a->eq[EQ_SHIELD]=0x80;st.support[U_RAMZA]=SUPPORT_TWO_SWORDS;CHECK(hand_count(U_RAMZA),2);
}
static void primary_pair(void)
{
    u8 sleeping;u16 hp,jp;char text[32];Unit *a,*t;u8 eq[EQ_N];
    for(sleeping=0;sleeping<3;sleeping++){
        arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->eq[EQ_WEAPON]=1;a->eq[EQ_SHIELD]=11;
        st.support[U_RAMZA]=SUPPORT_TWO_SWORDS;a->pa=5;a->sp=9;t->face=0;
        if(sleeping)change(10,SID_SLEEP);
        if(sleeping==2)t->statuses.innate[4]|=0x10;
        memcpy(eq,a->eq,EQ_N);hp=t->hp;jp=a->jp;
        CHECK(attack_prediction(U_RAMZA,10),sleeping==0?77:sleeping==1?86:110);
        forecast_text(U_RAMZA,10,NO_ABIL,text);
        CHECK(text[0],'-');CHECK(st.action.phase,CTX_IDLE);CHECK(t->hp,hp);
        attack(U_RAMZA,10);CHECK(st.action.left,1);CHECK(st.hand,0);
        st.hit=1;st.crit=st.push=0;st.dmg=attack_damage(U_RAMZA,10,0,0);
        CHECK(st.dmg,sleeping?30:21);impact();CHECK(a->jp,jp);
        ZB(29);end_anim();ZE(29);CHECK(st.hand,1);CHECK(st.action.phase,CTX_ROOT);
        st.hit=1;st.crit=st.push=0;st.dmg=attack_damage(U_RAMZA,10,0,0);
        CHECK(st.dmg,sleeping==2?80:56);impact();end_anim();end_anim();
        CHECK(st.action.phase,CTX_IDLE);CHECK(hp-t->hp,sleeping==0?77:sleeping==1?86:110);
        CHECK(memcmp(eq,a->eq,EQ_N),0);CHECK(a->jp>jp,1);
    }
}
static void dual_counter(void)
{
    u8 lethal;Unit *a,*t;u16 jp;
    for(lethal=0;lethal<3;lethal++){
        arena();a=&st.unit[U_RAMZA];t=&st.unit[10];
        t->eq[EQ_WEAPON]=1;t->eq[EQ_SHIELD]=11;st.support[10]=SUPPORT_TWO_SWORDS;st.reaction[10]=REACTION_COUNTER;
        jp=a->jp;root(1);end_anim();CHECK(st.action.actor,10);CHECK(st.hand,0);CHECK(st.action.left,1);
        st.hit=1;st.push=0;st.dmg=lethal==1?a->hp:1;impact();end_anim();
        if(lethal!=1){CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.hand,1);st.hit=1;st.push=0;st.dmg=lethal==2?a->hp:1;impact();end_anim();}
        if(st.gain)end_anim();CHECK(st.action.phase,CTX_IDLE);CHECK(st.hand,0);
        CHECK(t->gexp,0);CHECK(t->gjp,0);CHECK(st.end,lethal?END_DEFEAT:END_NONE);
        CHECK(a->jp>jp,!lethal);
    }
}
static void unsupported(void)
{
    u16 ab;arena();st.support[U_RAMZA]=SUPPORT_TWO_SWORDS;
    st.unit[U_RAMZA].eq[EQ_WEAPON]=1;st.unit[U_RAMZA].eq[EQ_SHIELD]=11;
    CHECK(can_use(U_RAMZA,0x196),1);CHECK(can_use(U_RAMZA,0x9b),0);CHECK(can_use(U_RAMZA,0x170),1);
    for(ab=1;ab<128;ab++)if(!(items[ab].pad&HAND_DUAL)&&items[ab].wp)break;
    CHECK(ab<128,1);st.unit[U_RAMZA].eq[EQ_SHIELD]=ab;CHECK(hand_count(U_RAMZA),1);
}
static void ai_pair(void)
{
    u8 dual;u16 steps,hp;u32 seed;s16 rank[2];
    for(dual=0;dual<2;dual++){
        arena();st.acted=0;st.moved=1;st.unit[U_RAMZA].learned=0;st.unit[U_RAMZA].defend=0;
        st.unit[U_RAMZA].eq[EQ_WEAPON]=1;st.unit[U_RAMZA].eq[EQ_SHIELD]=11;
        st.support[U_RAMZA]=dual?SUPPORT_TWO_SWORDS:0;st.unit[10].face=0;
        hp=st.unit[10].hp;seed=rt_seed;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<1024){}
        CHECK(steps<1024,1);CHECK(W->ai_n>0,1);CHECK(st.ai_ab,NO_ABIL);
        rank[dual]=W->ai_rank[0].pri;
        CHECK(st.unit[10].hp,hp);CHECK(rt_seed==seed,1);CHECK(UNIT_CELL(&st.unit[U_RAMZA]),first);
    }
    CHECK(rank[1]>rank[0],1);
}
static void saved_support(void)
{
    reset();party_load();CHECK(st.support[U_RAMZA],SUPPORT_TWO_SWORDS);
    CHECK(st.unit[U_RAMZA].eq[EQ_WEAPON],1);CHECK(st.unit[U_RAMZA].eq[EQ_SHIELD],11);
    CHECK(hand_count(U_RAMZA),2);
}
static void left_charge(void)
{
    arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=255;st.unit[U_RAMZA].eq[EQ_SHIELD]=11;
    st.support[U_RAMZA]=SUPPORT_TWO_SWORDS;st.unit[U_RAMZA].pa=5;st.unit[U_RAMZA].sp=9;
    CHECK(hand_count(U_RAMZA),1);skill(U_RAMZA,10,0x196);
    CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);CHECK(st.pending[U_RAMZA].weapon,11);
    CHECK(skill_amount(U_RAMZA,10,0x196,0),64);
    st.unit[U_RAMZA].eq[EQ_SHIELD]=19;st.pending[U_RAMZA].ticks=0;pending_next();
    CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
}
static void trials_hands(void){saved_support();source_policy();primary_pair();dual_counter();unsupported();ai_pair();left_charge();}
#ifndef __m68k__
int main(void){sw_init(61);trials_hands();printf("hands %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();trials_hands();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
