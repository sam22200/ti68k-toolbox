// Equipment steals 0x26 (Steal Helmet/Armor/Shield/Weapon/Accessory): the source slot choice,
// hit percent and Maintenance, production losses with their bonuses, the stolen item kept for
// the player's inventory, nothing to steal, forecasts and AI policy (CORE_STEAL_EQUIP.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-steal-equip/oracle/source.h"
#include "../equip_break.inc"   // the TI core keeps it in its images (guarded: once on the PC)
static const u8 steal_status[6][5]={{0},{0,0,0,0x20,0},{0,0,0,0,0x10},{8,0,0,0,0},{0,0,4,0,0},{0,0,2,0,0}};
static void prepare_steal(AbId ab)
{
    prepare_cleanse(ab);st.unit[10].team=TEAM_ENEMY;st.unit[10].ct=10;
    st.unit[10].hp=st.unit[10].max_hp=999;st.nstolen=0;
}
static void wear(Unit *u,u8 g)   // a source gear row as one item per hand (a shield on the left)
{
    const u8 *s=source_gear[g];
    u->eq[EQ_HEAD]=s[0];u->eq[EQ_BODY]=s[1];u->eq[EQ_ACC]=s[2];u->eq[EQ_WEAPON]=s[3];
    u->eq[EQ_SHIELD]=s[6]!=255?s[6]:s[5];
}
static u8 mask_slot(u8 m){return m&0x80?EQ_HEAD:m&0x40?EQ_BODY:m&0x20?EQ_ACC:m&0x10?EQ_WEAPON:m&6?EQ_SHIELD:255;}
static void profiles(void)
{
    u16 i;u8 j;
    for(i=0;i<sizeof source_steal_equip/sizeof source_steal_equip[0];i++){
        const u8 *p=source_steal_equip[i];Unit *a,*t;AbId ab=p[0];u8 slot;
        prepare_steal(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->sp=p[1];
        if(!p[2])a->eq[EQ_WEAPON]=a->eq[EQ_SHIELD]=255;
        else if(a->eq[EQ_WEAPON]==255){a->eq[EQ_WEAPON]=0;a->eq[EQ_SHIELD]=255;}   // any weapon
        a->zodiac=p[3];t->zodiac=p[4];t->female=0;
        st.support[U_RAMZA]=p[5]?SUPPORT_ATTACK_UP:p[6]?SUPPORT_MARTIAL_ARTS:0;
        memset(&t->statuses,0,sizeof t->statuses);
        for(j=0;j<5;j++)t->statuses.inflicted[j]=steal_status[p[7]][j];
        unit_status_refresh(t);wear(t,p[8]);st.support[10]=p[9]?SUPPORT_MAINTENANCE:0;
        slot=equip_break_slot(t,ab);
        if(!p[12]){CHECK(slot,255);CHECK(skill_hit(U_RAMZA,10,ab),0);continue;}   // forced failure
        CHECK(slot,mask_slot(p[12]));CHECK(t->eq[slot],p[13]);
        if(p[9])CHECK(skill_hit(U_RAMZA,10,ab),0);                  // Maintenance: forced failure
        else{u16 c=equip_break_percent(U_RAMZA,10,ab);CHECK(c<100?c:100,p[11]);}
    }
}
static void steal_production(void)
{
    AbId ab;u8 s,found;Unit *t;
    for(ab=0x6e;ab<=0x72;ab++){
        u8 slot=ab==0x6e?EQ_HEAD:ab==0x6f?EQ_BODY:ab==0x70?EQ_SHIELD:ab==0x71?EQ_WEAPON:EQ_ACC;
        CHECK(abilities[ab].kind,AB_STEAL_EQUIP);CHECK(abilities[ab].formula,0x26);CHECK(abilities[ab].flags,AF_EVADE);
        for(s=1,found=0;s<200&&!found;s++){
            const Item *it;u16 max_hp;u8 sp,item;
            prepare_steal(ab);t=&st.unit[10];wear(t,0);t->hp=t->max_hp=200;item=t->eq[slot];
            it=&items[item];max_hp=t->max_hp-it->hp;sp=t->sp>it->sp?t->sp-it->sp:1;
            if(ab==0x71){st.pending[10].state=DEFER_CHARGING;unit_status_action(t,8);}
            rng32=s;skill(U_RAMZA,10,ab);CHECK(st.mode,M_ACT);CHECK(st.abil,ab);
            if(!st.hit){CHECK(t->eq[slot],item);continue;}
            found=1;impact();CHECK(t->eq[slot],255);CHECK(t->max_hp,max_hp);CHECK(t->sp,sp);
            CHECK(st.nstolen,1);CHECK(st.stolen[0],item);           // the player's: saved (save.inc)
            if(ab==0x71){CHECK(st.pending[10].state,DEFER_NONE);CHECK(core_status_current(&t->statuses,0)&8,0);}
        }
        CHECK(found,1);
        // An enemy thief: the player's item is lost, not gained.
        for(s=1,found=0;s<200&&!found;s++){
            prepare_steal(ab);wear(&st.unit[U_RAMZA],0);st.unit[10].team=TEAM_ENEMY;
            rng32=s;skill(10,U_RAMZA,ab);if(!st.hit)continue;
            found=1;impact();CHECK(st.unit[U_RAMZA].eq[slot],255);CHECK(st.nstolen,0);
        }
        CHECK(found,1);
        // Nothing to steal: a forced failure (no Attack), forecast 0%.
        {
            char a[24];
            prepare_steal(ab);t=&st.unit[10];wear(t,1);
            CHECK(skill_hit(U_RAMZA,10,ab),0);forecast_text(U_RAMZA,10,ab,a);CHECK(a[0],'S');
            rng32=7;skill(U_RAMZA,10,ab);CHECK(st.mode,M_ACT);CHECK(st.abil,ab);CHECK(st.hit,0);
        }
        // Maintenance resists every roll.
        for(s=1;s<12;s++){
            prepare_steal(ab);wear(&st.unit[10],0);st.support[10]=SUPPORT_MAINTENANCE;
            rng32=s;skill(U_RAMZA,10,ab);CHECK(st.hit,0);CHECK(st.nstolen,0);
        }
        // Range 1 (adjacent enemy); physical (Silence does not block it); a Frog cannot use it.
        prepare_steal(ab);st.act=U_RAMZA;aim(ab);CHECK(st.reach[UNIT_CELL(&st.unit[10])],1);
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
static void steal_ai(void)
{
    u8 k,found=0;
    prepare_steal(0x71);wear(&st.unit[10],0);CHECK(plan(U_RAMZA),1);
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==0x71)found=1;
    CHECK(found,0);   // conservative AI policy for steals
}
static void steal_trials(void){profiles();steal_production();steal_ai();}
#ifndef __m68k__
int main(void){sw_init(61);steal_trials();printf("steal_equip %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();steal_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
