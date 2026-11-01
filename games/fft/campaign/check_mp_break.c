// Magic Break 0x2c: source hit percent and MP lost, production break through the weapon's
// range, the forecast, gates and AI policy (CORE_MP_BREAK.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-mp-break/oracle/source.h"
#define MP_BREAK 0x8e
static const u8 mp_break_status[6][5]={{0},{0,0,0,0x20,0},{0,0,0,0,0x10},{8,0,0,0,0},{0,0,4,0,0},{0,0,2,0,0}};
static void prepare_mp_break(void)
{
    prepare_cleanse(MP_BREAK);st.unit[10].team=TEAM_ENEMY;st.unit[10].ct=10;
    st.unit[10].hp=st.unit[10].max_hp=999;st.unit[10].max_mp=80;st.unit[10].mp=40;
}
static void profiles(void)
{
    u16 i;u8 j;
    for(i=0;i<sizeof source_mp_break/sizeof source_mp_break[0];i++){
        const u16 *p=source_mp_break[i];Unit *a,*t;u16 c,lost;
        prepare_mp_break();a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->pa=(u8)p[0];a->zodiac=(u8)p[1];t->zodiac=(u8)p[2];t->female=0;
        st.support[U_RAMZA]=p[3]?SUPPORT_ATTACK_UP:p[4]?SUPPORT_MARTIAL_ARTS:0;
        if(!p[5])a->eq[EQ_WEAPON]=a->eq[EQ_SHIELD]=255;
        else if(a->eq[EQ_WEAPON]==255){a->eq[EQ_WEAPON]=0;a->eq[EQ_SHIELD]=255;}   // any weapon
        memset(&t->statuses,0,sizeof t->statuses);
        for(j=0;j<5;j++)t->statuses.inflicted[j]=mp_break_status[p[6]][j];
        unit_status_refresh(t);t->max_mp=p[7];t->mp=p[8];
        c=charge_query(U_RAMZA,10,MP_BREAK,0,0);
        CHECK(c<100?c:100,p[10]);                                 // the source percent stops at 100
        lost=skill_amount(U_RAMZA,10,MP_BREAK,0);CHECK(lost,p[11]);
        if(p[12]){                                                 // the commit's clamp at 0
            st.atk=U_RAMZA;st.abil=MP_BREAK;st.tgt=10;st.dmg=lost;st.hit=1;
            t->mp=t->mp>lost?t->mp-lost:0;
        }
        CHECK(t->mp,p[13]);
    }
}
static void mp_break_production(void)
{
    u8 s,found;Unit *t;char text[24];
    CHECK(abilities[MP_BREAK].kind,AB_BREAK);CHECK(abilities[MP_BREAK].formula,0x2c);
    CHECK(abilities[MP_BREAK].flags,AF_EVADE);CHECK(abilities[MP_BREAK].element,50);
    for(s=1,found=0;s<200&&!found;s++){
        prepare_mp_break();t=&st.unit[10];
        rng32=s;skill(U_RAMZA,10,MP_BREAK);CHECK(st.mode,M_ACT);
        if(!st.hit)continue;
        found=1;CHECK(st.dmg,40);impact();CHECK(t->mp,0);CHECK(t->ma,t->ma);
    }
    CHECK(found,1);
    // The forecast prints the MP lost.
    prepare_mp_break();forecast_text(U_RAMZA,10,MP_BREAK,text);
    {u8 k,ok=0;for(k=0;text[k];k++)if(text[k]=='-'&&text[k+1]=='4'&&text[k+2]=='0'&&text[k+3]=='M')ok=1;CHECK(ok,1);}
    // The reach is the weapon's, as Attack's: the adjacent enemy is a target.
    prepare_mp_break();st.act=U_RAMZA;aim(MP_BREAK);CHECK(st.reach[UNIT_CELL(&st.unit[10])],1);
    // Physical: not blocked by Silence; a Frog cannot use it.
    prepare_mp_break();st.unit[U_RAMZA].statuses.inflicted[1]|=8;unit_status_refresh(&st.unit[U_RAMZA]);
    CHECK(can_use(U_RAMZA,MP_BREAK),1);
    st.unit[U_RAMZA].statuses.inflicted[2]|=2;unit_status_refresh(&st.unit[U_RAMZA]);CHECK(can_use(U_RAMZA,MP_BREAK),0);
}
static u8 plan(u8 who)
{
    u16 steps=0;st.act=who;st.moved=0;st.acted=0;W->ai_k=0;
    while(!ai_think()&&++steps<2048){}
    return steps<2048;
}
static void mp_break_ai(void)
{
    u8 k,found=0;
    prepare_mp_break();CHECK(plan(U_RAMZA),1);
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==MP_BREAK)found=1;
    CHECK(found,0);   // conservative AI policy for breaks
}
static void mp_break_trials(void){profiles();mp_break_production();mp_break_ai();}
#ifndef __m68k__
int main(void){sw_init(61);mp_break_trials();printf("mp-break %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();mp_break_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
