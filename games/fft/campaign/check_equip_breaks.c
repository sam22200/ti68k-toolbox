// Equipment breaks 0x25 (Head/Armor/Shield/Weapon Break): the source slot choice, hit percent
// and Maintenance, production losses with their bonuses, the default Attack, a lost weapon's
// Charge, forecasts and AI policy (CORE_EQUIP_BREAKS.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-equip-breaks/oracle/source.h"
#include "../equip_break.inc"   // the TI core keeps it in its images (guarded: once on the PC)
static const u8 gear_status[6][5]={{0},{0,0,0,0x20,0},{0,0,0,0,0x10},{8,0,0,0,0},{0,0,4,0,0},{0,0,2,0,0}};
static void prepare_gear_break(AbId ab)
{
    prepare_cleanse(ab);st.unit[10].team=TEAM_ENEMY;st.unit[10].ct=10;
    st.unit[10].hp=st.unit[10].max_hp=999;
}
static void wear(Unit *u,u8 g)   // a source gear row as one item per hand (a shield on the left)
{
    const u8 *s=source_gear[g];
    u->eq[EQ_HEAD]=s[0];u->eq[EQ_BODY]=s[1];u->eq[EQ_ACC]=s[2];u->eq[EQ_WEAPON]=s[3];
    u->eq[EQ_SHIELD]=s[6]!=255?s[6]:s[5];
}
static u8 weapon_with(u8 wp)
{
    u16 i;
    if(!wp)return 255;
    for(i=0;i<256;i++)if(items[i].wp==wp&&!items[i].knife)return (u8)i;
    return 254;
}
static u8 mask_slot(u8 m){return m&0x80?EQ_HEAD:m&0x40?EQ_BODY:m&0x20?EQ_ACC:m&0x10?EQ_WEAPON:m&6?EQ_SHIELD:255;}
static void profiles(void)
{
    u16 i;u8 j;
    for(i=0;i<sizeof source_equip_breaks/sizeof source_equip_breaks[0];i++){
        const u8 *p=source_equip_breaks[i];Unit *a,*t;AbId ab=p[0];u8 slot;
        prepare_gear_break(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->pa=p[1];a->eq[EQ_WEAPON]=weapon_with(p[2]);a->eq[EQ_SHIELD]=255;CHECK(a->eq[EQ_WEAPON]!=254,1);
        a->zodiac=p[3];t->zodiac=p[4];t->female=0;
        st.support[U_RAMZA]=p[5]?SUPPORT_ATTACK_UP:p[6]?SUPPORT_MARTIAL_ARTS:0;
        memset(&t->statuses,0,sizeof t->statuses);
        for(j=0;j<5;j++)t->statuses.inflicted[j]=gear_status[p[7]][j];
        unit_status_refresh(t);wear(t,p[8]);st.support[10]=p[9]?SUPPORT_MAINTENANCE:0;
        slot=equip_break_slot(t,ab);
        if(p[11]==255){CHECK(slot,255);continue;}                   // the source's default Attack
        CHECK(slot,mask_slot(p[12]));CHECK(t->eq[slot],p[13]);
        if(p[9])CHECK(skill_hit(U_RAMZA,10,ab),0);                  // Maintenance: forced failure
        else{u16 c=equip_break_percent(U_RAMZA,10,ab);CHECK(c<100?c:100,p[11]);}
    }
}
static void gear_production(void)
{
    AbId ab;u8 s,found;Unit *t;
    for(ab=0x8a;ab<=0x8d;ab++){
        u8 slot=ab==0x8a?EQ_HEAD:ab==0x8b?EQ_BODY:ab==0x8c?EQ_SHIELD:EQ_WEAPON;
        CHECK(abilities[ab].kind,AB_EQUIP_BREAK);CHECK(abilities[ab].formula,0x25);CHECK(abilities[ab].flags,AF_EVADE);
        for(s=1,found=0;s<200&&!found;s++){
            const Item *it;u16 max_hp;u8 pa;
            prepare_gear_break(ab);t=&st.unit[10];wear(t,0);t->hp=t->max_hp=200;
            it=&items[t->eq[slot]];max_hp=t->max_hp-it->hp;pa=t->pa>it->pa?t->pa-it->pa:1;
            if(ab==0x8d){st.pending[10].state=DEFER_CHARGING;unit_status_action(t,8);}
            rng32=s;skill(U_RAMZA,10,ab);CHECK(st.mode,M_ACT);CHECK(st.abil,ab);
            if(!st.hit){CHECK(t->eq[slot]!=255,1);continue;}
            found=1;impact();CHECK(t->eq[slot],255);CHECK(t->max_hp,max_hp);CHECK(t->pa,pa);
            CHECK(t->hp<=t->max_hp,1);
            if(ab==0x8d){CHECK(st.pending[10].state,DEFER_NONE);CHECK(core_status_current(&t->statuses,0)&8,0);}
        }
        CHECK(found,1);
        // Nothing to break: the ordinary Attack, forecast and executed as one.
        {
            char a[24],b[24];
            prepare_gear_break(ab);t=&st.unit[10];wear(t,1);
            u8 n=forecast_text(U_RAMZA,10,ab,a),k;CHECK(forecast_text(U_RAMZA,10,NO_ABIL,b),n);
            for(k=0;k<n;k++)CHECK(a[k],b[k]);   // (no AMS strcmp under ti-cycles)
            CHECK(skill_hit(U_RAMZA,10,ab),hit_chance(U_RAMZA,10));
            rng32=7;skill(U_RAMZA,10,ab);CHECK(st.mode,M_ACT);CHECK(st.abil,NO_ABIL);
        }
        // Something to break: the forecast names the break.
        {char a[24];prepare_gear_break(ab);wear(&st.unit[10],0);forecast_text(U_RAMZA,10,ab,a);CHECK(a[0],'B');}
        // Maintenance resists every roll.
        for(s=1;s<12;s++){
            prepare_gear_break(ab);wear(&st.unit[10],0);st.support[10]=SUPPORT_MAINTENANCE;
            rng32=s;skill(U_RAMZA,10,ab);CHECK(st.hit,0);
        }
        // Weapon reach as Attack; physical (Silence does not block it); a Frog cannot use it.
        prepare_gear_break(ab);st.act=U_RAMZA;aim(ab);CHECK(st.reach[UNIT_CELL(&st.unit[10])],1);
        st.unit[U_RAMZA].statuses.inflicted[1]|=8;unit_status_refresh(&st.unit[U_RAMZA]);CHECK(can_use(U_RAMZA,ab),1);
        st.unit[U_RAMZA].statuses.inflicted[2]|=2;unit_status_refresh(&st.unit[U_RAMZA]);CHECK(can_use(U_RAMZA,ab),0);
    }
}
static u8 plan(u8 who)
{
    u16 steps=0;st.act=who;st.moved=0;st.acted=0;W->ai_k=0;
    while(!ai_think()&&++steps<2048){}
    return steps<2048;
}
static void gear_ai(void)
{
    u8 k,found=0;
    prepare_gear_break(0x8d);wear(&st.unit[10],0);CHECK(plan(U_RAMZA),1);
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==0x8d)found=1;
    CHECK(found,0);   // conservative AI policy for breaks
}
static void gear_trials(void){profiles();gear_production();gear_ai();}
#ifndef __m68k__
int main(void){sw_init(61);gear_trials();printf("equip_breaks %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();gear_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
