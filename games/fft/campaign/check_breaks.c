// Stat breaks 0x2b (Speed/Power/Mind Break): source hit percent and stat loss, production
// breaks through the weapon's range, gates and AI policy (CORE_BREAKS.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-breaks/oracle/source.h"
static const u8 break_status[6][5]={{0},{0,0,0,0x20,0},{0,0,0,0,0x10},{8,0,0,0,0},{0,0,4,0,0},{0,0,2,0,0}};
static u8 *break_stat(Unit *u,AbId ab){return ab==0x90?&u->pa:ab==0x8f?&u->sp:&u->ma;}
static void prepare_break(AbId ab)
{
    prepare_cleanse(ab);st.unit[10].team=TEAM_ENEMY;st.unit[10].ct=10;
    st.unit[10].hp=st.unit[10].max_hp=999;
}
static void profiles(void)
{
    u16 i;u8 j;
    for(i=0;i<sizeof source_breaks/sizeof source_breaks[0];i++){
        const u8 *p=source_breaks[i];Unit *a,*t;u16 c;AbId ab=p[0];
        prepare_break(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->pa=p[1];a->zodiac=p[2];t->zodiac=p[3];t->female=0;
        st.support[U_RAMZA]=p[4]?SUPPORT_ATTACK_UP:p[5]?SUPPORT_MARTIAL_ARTS:0;
        if(!p[6])a->eq[EQ_WEAPON]=a->eq[EQ_SHIELD]=255;
        else if(a->eq[EQ_WEAPON]==255){a->eq[EQ_WEAPON]=0;a->eq[EQ_SHIELD]=255;}   // any weapon
        memset(&t->statuses,0,sizeof t->statuses);
        for(j=0;j<5;j++)t->statuses.inflicted[j]=break_status[p[7]][j];
        unit_status_refresh(t);*break_stat(t,ab)=p[8];
        c=charge_query(U_RAMZA,10,ab,0,0);
        CHECK(c<100?c:100,p[10]);                                 // the source percent stops at 100CHECK(abilities[ab].y,p[11]);
        if(p[12]){                                                 // the source commit and clamp
            st.atk=U_RAMZA;st.abil=ab;
            {u8 *v=break_stat(t,ab);*v=*v>abilities[ab].y?*v-abilities[ab].y:1;}
        }
        CHECK(*break_stat(t,ab),p[13]);
    }
}
static void break_production(void)
{
    AbId ab;u8 s,found;Unit *t;
    for(ab=0x8f;ab<=0x91;ab++){
        CHECK(abilities[ab].kind,AB_BREAK);CHECK(abilities[ab].formula,0x2b);CHECK(abilities[ab].flags,AF_EVADE);
        for(s=1,found=0;s<200&&!found;s++){
            prepare_break(ab);t=&st.unit[10];*break_stat(t,ab)=20;
            rng32=s;skill(U_RAMZA,10,ab);CHECK(st.mode,M_ACT);
            if(!st.hit)continue;
            found=1;impact();CHECK(*break_stat(t,ab),20-abilities[ab].y);
        }
        CHECK(found,1);
        // The reach is the weapon's, as Attack's: the adjacent enemy is a target.
        prepare_break(ab);st.act=U_RAMZA;aim(ab);CHECK(st.reach[UNIT_CELL(&st.unit[10])],1);
        // Physical: not blocked by Silence; a Frog cannot use it.
        prepare_break(ab);st.unit[U_RAMZA].statuses.inflicted[1]|=8;unit_status_refresh(&st.unit[U_RAMZA]);
        CHECK(can_use(U_RAMZA,ab),1);
        st.unit[U_RAMZA].statuses.inflicted[2]|=2;unit_status_refresh(&st.unit[U_RAMZA]);CHECK(can_use(U_RAMZA,ab),0);
    }
}
static u8 plan(u8 who)
{
    u16 steps=0;st.act=who;st.moved=0;st.acted=0;W->ai_k=0;
    while(!ai_think()&&++steps<2048){}
    return steps<2048;
}
static void break_ai(void)
{
    u8 k,found=0;
    prepare_break(0x90);CHECK(plan(U_RAMZA),1);
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==0x90)found=1;
    CHECK(found,0);   // conservative AI policy for stat breaks
}
static void break_trials(void){profiles();break_production();break_ai();}
#ifndef __m68k__
int main(void){sw_init(61);break_trials();printf("breaks %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();break_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
