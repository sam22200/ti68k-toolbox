// L2 first slice: source-observed primary effects and impact/presentation ownership.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main action_runtime_main
#define game_render action_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void action_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
#include "../oracle.h"
static u16 checks,failures;
static u32 digest=17;
static void check(u16 actual,u16 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^actual;
    if(actual!=expected){failures++;
#ifndef __m68k__
        printf("line %u: %u expected %u\n",line,actual,expected);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(v,e) check((u16)(v),(u16)(e),__LINE__)
static void reset(void)
{
    game_scenario(61);st.ai=AI_OFF;st.act=U_RAMZA;st.moved=st.acted=0;
    st.unit[U_RAMZA].exp=0;
}
static void impact(void)
{
    // A before/after timing check: mutation and rewards happen only at ACT_HIT.
    u16 hp=st.unit[st.tgt].hp,jp=st.unit[st.atk].jp;
    st.act_t=ACT_HIT-1;attack_step();
    CHECK(st.unit[st.tgt].hp,hp);CHECK(st.unit[st.atk].jp,jp);
    ZB(15);attack_step();ZE(15);
    CHECK(st.act_t,ACT_HIT+1);
    hp=st.unit[st.tgt].hp;jp=st.unit[st.atk].jp;
    attack_step();CHECK(st.unit[st.tgt].hp,hp);CHECK(st.unit[st.atk].jp,jp);
}
static void source_effects(void)
{
    static const AbId ids[9]={0x93,0x94,0x95,0x98,0x170,0x176,0x177,0x178,0x17D};
    u8 i,k;
    for(i=0;i<ORACLE_SKILLS;i++){
        Unit *a,*t;AbId ab;u8 hit,kind;
        reset();a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->zodiac=9;t->zodiac=3;a->female=t->female=0;
        a->max_hp=a->hp=52;a->pa=5;a->sp=6;
        t->max_hp=44;t->hp=oracle_skills[i].hp;unit_status_seed(t,oracle_skills[i].status);t->kos=0;
        for(k=0;k<5;k++)st.bag[BAG_SLOT(k)]=bag_init[k];
        ab=oracle_skills[i].abil==255?NO_ABIL:ids[oracle_skills[i].abil];
        kind=ab==NO_ABIL?AB_DAMAGE:abilities[ab].kind;
        hit=ab==NO_ABIL?hit_chance(U_RAMZA,10):skill_hit(U_RAMZA,10,ab);
        CHECK(hit,oracle_skills[i].hit);
        if(ab==NO_ABIL)attack(U_RAMZA,10);else skill(U_RAMZA,10,ab);
        // The original preview and execution use different random rolls. Feed
        // its recorded execution amount, then check actual native commit, item
        // consumption, status changes, self-cost and rewards (no RNG imitation).
        st.hit=oracle_skills[i].hit!=0;st.push=0;
        if(kind==AB_DAMAGE)st.dmg=oracle_skills[i].hp-oracle_skills[i].hp2;
        else if(kind==AB_REVIVE)st.dmg=oracle_skills[i].hp2;
        impact();
        CHECK(t->hp,oracle_skills[i].hp2);CHECK(t->status,oracle_skills[i].status2);
        CHECK(a->hp,oracle_skills[i].ramza);
        for(k=0;k<5;k++)CHECK(st.bag[BAG_SLOT(k)],oracle_skills[i].bag[k]);
        CHECK(st.gain!=0,restore_ability(ab)?(t->hp!=oracle_skills[i].hp||t->status!=oracle_skills[i].status):st.hit!=0);CHECK(t->kos,0);
    }
}
static void boundaries(void)
{
    Unit *a,*t;u8 i;
    // Saturation and exactly-once KO accounting; terminal check waits for
    // presentation/rewards, rather than ending at the first HP assignment.
    reset();a=&st.unit[U_RAMZA];t=&st.unit[10];
    act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=1;st.dmg=t->hp;st.push=0;t->kos=0;
    impact();CHECK(t->hp,0);CHECK(t->kos,1);CHECK(st.end,END_NONE);
    // Reviving a KO resets its death counter without clearing other ailments.
    reset();t=&st.unit[10];t->hp=0;t->dc=1;unit_status_seed(t,ST_POISON|ST_SILENCE);
    skill(U_RAMZA,10,0x17D);st.dmg=999;impact();
    CHECK(t->hp,t->max_hp);CHECK(t->dc,3);CHECK(t->status,ST_POISON|ST_SILENCE);
    reset();t=&st.unit[10];t->hp=t->max_hp-1;skill(U_RAMZA,10,0x170);impact();
    CHECK(t->hp,t->max_hp);
    // Defend is a command sentinel, never an ability-bank subscript.
    reset();a=&st.unit[U_RAMZA];unit_status_seed(a,ST_POISON);defend(U_RAMZA);impact();
    CHECK(a->status,ST_POISON|ST_DEFEND);CHECK(st.gain,0);
    // Existing stat effects keep their floor/cap and never masquerade as HP.
    reset();a=&st.unit[U_RAMZA];a->pa=99;skill(U_RAMZA,U_RAMZA,0x92);impact();
    CHECK(a->pa,99);
    // Find the source-record speed/magic break IDs rather than assume set order.
    for(i=0;i<2;i++){
        AbId ab;reset();t=&st.unit[10];t->sp=t->ma=1;
        for(ab=0;ab<512;ab++)if(abilities[ab].kind==AB_BREAK&&abilities[ab].y==(i?3:2))break;
        CHECK(ab<512,1);if(ab>=512)continue;
        skill(U_RAMZA,10,ab);impact();CHECK(t->sp,1);CHECK(t->ma,1);
    }
    // Wish may KO its user. Settle the whole action before declaring defeat.
    reset();a=&st.unit[U_RAMZA];t=&st.unit[10];a->hp=1;t->hp=1;
    for(i=0;i<NUNIT;i++)if(i!=U_RAMZA)st.unit[i].team=TEAM_ENEMY;
    skill(U_RAMZA,10,0x98);impact();CHECK(a->hp,0);CHECK(st.end,END_NONE);
    while(st.mode==M_ACT)attack_step();CHECK(st.end,END_DEFEAT);
}
static void ai_predictions(void)
{
    u8 trial,k;u16 steps;
    for(trial=0;trial<3;trial++){
        reset();
        if(trial){
            st.act=U_CHEMIST;st.unit[U_CHEMIST].learned=0x2001;
            st.unit[U_RAMZA].hp=trial==1?1:0;
        }
        W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<1024){}
        CHECK(steps<1024,1);CHECK(W->ai_n<=sizeof W->ai_rank/sizeof W->ai_rank[0],1);
        CHECK(st.unit[U_RAMZA].hp,trial==1?1:trial==2?0:st.unit[U_RAMZA].max_hp);
        CHECK(st.bag[abilities[0x170].bag],5);
        // Compare the complete resulting ranked candidates/choice with the
        // captured old build. Pure prediction must not consume HP or stock.
        digest=(digest<<1|digest>>31)^st.ai_ab;
        digest=(digest<<1|digest>>31)^st.ai_o;
        digest=(digest<<1|digest>>31)^st.ai_tgt;
        for(k=0;k<W->ai_n;k++){
            const AiAct *r=&W->ai_rank[k];
            digest=(digest<<1|digest>>31)^r->ab;
            digest=(digest<<1|digest>>31)^(u16)r->pri;
            digest=(digest<<1|digest>>31)^r->hit;
            digest=(digest<<1|digest>>31)^r->o;
            digest=(digest<<1|digest>>31)^r->t;
        }
    }
}
static void forecasts(void)
{
    AbId ab;u8 i;char text[32];
    reset();st.unit[10].hp=10;unit_status_seed(&st.unit[10],ST_POISON|ST_DARK|ST_SILENCE);
    for(ab=0;ab<512;ab++)if(abilities[ab].kind!=255){
        forecast_text(U_RAMZA,10,ab,text);
        for(i=0;text[i];i++)digest=(digest<<1|digest>>31)^(u8)text[i];
    }
}
static void extra_effects(void)
{
    u8 trial,i;AbId ab;
    for(trial=0;trial<2;trial++){
        u8 flag=trial?AF_AREA:AF_DRAIN;
        reset();
        for(i=0;i<NUNIT;i++)st.unit[i].max_hp=st.unit[i].hp=999;
        if(!trial)st.unit[U_RAMZA].hp=1;
        for(ab=0;ab<512;ab++)if(abilities[ab].kind==AB_DAMAGE&&
            (abilities[ab].flags&(AF_AREA|AF_DRAIN|AF_CAST))==flag)break;
        CHECK(ab<512,1);if(ab>=512)continue;
        skill(U_RAMZA,10,ab);st.hit=1;st.push=0;st.dmg=7;
        impact();CHECK(st.unit[10].hp,992);
        if(!trial)CHECK(st.unit[U_RAMZA].hp,8);
        else CHECK(st.unit[U_RAMZA].hp<999,1); // adjacent ally, included in the current cross
        // Frozen old implementation is the reference for every extra target.
        for(i=0;i<NUNIT;i++)digest=(digest<<1|digest>>31)^st.unit[i].hp;
    }
}
static void trials(void){source_effects();boundaries();ai_predictions();forecasts();extra_effects();}
#ifndef __m68k__
int main(void)
{
    sw_init(61);trials();
    printf("actions %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);
    return failures?1:0;
}
#else
void _main(void)
{
    action_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);
}
#endif
