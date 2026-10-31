// Punch Art strikes: Wave Fist and Repeating Fist. Source damage/critical oracle
// with its own draws replayed, production execution, knockback and AI use (CORE_PUNCH_ART.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-punch-strikes/oracle/source.h"
static const u8 strike_status[7][5]={{0},{0,0,0,0x20,0},{0,0,0,0,0x10},{8,0,0,0,0},{0,0,4,0,0},{0,0,2,0,0},{0,0,4,0x20,0x10}};
static void prepare_strike(AbId ab)
{
    prepare_cleanse(ab);st.unit[10].team=TEAM_ENEMY;st.unit[10].ct=10;
    st.unit[10].hp=st.unit[10].max_hp=999;
}
// Original draws: four evade rolls, the critical roll, its amount, then Repeating Fist's hits
// (Wave Fist: a status-proc draw instead). Native evasion keeps Attack's model.
static void profiles(void)
{
    u16 i;u8 j,crit,draws;u16 cr,mult,damage;
    for(i=0;i<sizeof source_strike/sizeof source_strike[0];i++){
        const s16 *p=source_strike[i];AbId ab=(AbId)p[0];Unit *a,*t;
        prepare_strike(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->pa=(u8)p[1];a->zodiac=(u8)p[2];t->zodiac=(u8)p[3];t->female=(u8)p[4];
        st.support[U_RAMZA]=p[5]?SUPPORT_ATTACK_UP:p[6]?SUPPORT_MARTIAL_ARTS:0;
        memset(&t->statuses,0,sizeof t->statuses);
        for(j=0;j<5;j++)t->statuses.inflicted[j]=strike_status[p[7]][j];
        unit_status_refresh(t);
        rng32=(u32)p[8];for(j=0;j<4;j++)rand15();draws=5;
        crit=(u8)(((u32)rand15()*100>>15)<4);cr=0;
        if(crit){cr=rand15();draws++;}
        CHECK(crit,p[9]);
        damage=skill_amount(U_RAMZA,10,ab,0x8000|cr);
        if(ab==101){
            mult=(u16)(((u32)rand15()*cj_range(common,ab)[9]>>15)+1);draws++;
            CHECK(mult,p[11]);damage=(u16)(damage*mult)>999?999:(u16)(damage*mult);
        }
        if(ab!=101)draws++;   // 0x31's conditional status proc draws after the damage (no status)
        CHECK(damage,p[10]);CHECK(draws,p[12]);
    }
}
static void strike_production(void)
{
    u8 s,crits=0,base;u16 hp,amount;Unit *t;
    // Wave Fist: range 3, physical evasion, the shared prediction, damage committed.
    prepare_strike(102);st.unit[U_RAMZA].pa=20;t=&st.unit[10];
    CHECK(skill_hit(U_RAMZA,10,102),hit_chance(U_RAMZA,10));
    amount=skill_amount(U_RAMZA,10,102,0);CHECK(amount,20*11);
    for(s=1;s<200;s++){
        prepare_strike(102);st.unit[U_RAMZA].pa=20;t=&st.unit[10];hp=t->hp;
        rng32=s;skill(U_RAMZA,10,102);CHECK(st.mode,M_ACT);
        if(!st.hit)continue;
        CHECK(st.dmg>=amount&&st.dmg<=2*amount,1);
        if(st.crit){crits++;CHECK(st.push,1);}else{CHECK(st.dmg,amount);CHECK(st.push,0);}
        impact();CHECK(t->hp,hp-st.dmg);
    }
    CHECK(crits>0,1);
    // Repeating Fist: (PA + (PA+Y)/2) x 1..9 hits, never a Brave knockback.
    prepare_strike(101);st.unit[U_RAMZA].pa=20;base=(u8)skill_amount(U_RAMZA,10,101,0);CHECK(base,30);
    CHECK(skill_amount(U_RAMZA,10,101,0x7fff),270);
    for(s=1;s<60;s++){
        prepare_strike(101);st.unit[U_RAMZA].pa=20;rng32=s;skill(U_RAMZA,10,101);
        if(!st.hit||st.crit)continue;
        CHECK(st.dmg%base,0);CHECK(st.dmg/base>=1&&st.dmg/base<=9,1);CHECK(st.push,0);
    }
}
// Spin Fist: radius 1 on the user (excluded), per-victim physical evasion and a 4%
// critical without knockback, through the owned area batch.
static void spin_fist(void)
{
    u8 s,hits=0,crits=0;u16 base,hp;CastBatch b;Unit *t;
    prepare_strike(100);st.unit[U_RAMZA].pa=20;t=&st.unit[10];base=skill_amount(U_RAMZA,10,100,0);CHECK(base,200);
    memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=U_RAMZA;b.follow=1;b.ability=100;
    CHECK(cast_forecast(&b),1);CHECK(b.mask,(u16)1<<10);
    CHECK(b.victim[10].hit,hit_chance(U_RAMZA,10));CHECK(b.victim[10].amount,base);CHECK(b.victim[10].effect,AB_DAMAGE);
    for(s=1;s<120;s++){
        prepare_strike(100);st.unit[U_RAMZA].pa=20;t=&st.unit[10];hp=t->hp;
        rng32=s;CHECK(cast_order(U_RAMZA,U_RAMZA,100,1),1);
        if(W->cast.victim[10].hit){
            u16 n=W->cast.victim[10].amount;hits++;
            CHECK(n>=base&&n<=2*base,1);if(n>base)crits++;
            impact();CHECK(t->hp,hp-n);CHECK(UNIT_CELL(t),second);   // no knockback in an area
        }else{impact();CHECK(t->hp,hp);}
        CHECK(st.unit[U_RAMZA].hp,999);
    }
    CHECK(hits>0,1);CHECK(crits>0,1);
}
static void strike_gates(void)
{
    static const AbId gated[]={103,104};
    u8 j;
    // Earth Slash (line) and Secret Fist (physical then percentage status) stay gated.
    for(j=0;j<sizeof gated/sizeof gated[0];j++)CHECK(abilities[gated[j]].kind,255);
    CHECK(abilities[101].kind,AB_DAMAGE);CHECK(abilities[102].kind,AB_DAMAGE);
    CHECK(abilities[102].flags,AF_EVADE);CHECK(abilities[102].range,3);
    // Not magic: Silence does not block it; a Frog may not use it.
    prepare_strike(102);st.unit[U_RAMZA].statuses.inflicted[1]|=8;unit_status_refresh(&st.unit[U_RAMZA]);
    CHECK(can_use(U_RAMZA,102),1);
    st.unit[U_RAMZA].statuses.inflicted[2]|=2;unit_status_refresh(&st.unit[U_RAMZA]);CHECK(can_use(U_RAMZA,102),0);
}
static u8 plan(u8 who)
{
    u16 steps=0;st.act=who;st.moved=0;st.acted=0;W->ai_k=0;
    while(!ai_think()&&++steps<2048){}
    return steps<2048;
}
static void strike_ai(void)
{
    u8 k,found=0;
    prepare_strike(102);st.unit[U_RAMZA].pa=40;CHECK(plan(U_RAMZA),1);
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==102)found=1;
    CHECK(found,1);
}
static void strike_trials(void){profiles();strike_production();spin_fist();strike_gates();strike_ai();}
#ifndef __m68k__
int main(void){sw_init(61);strike_trials();printf("punch_strikes %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();strike_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
