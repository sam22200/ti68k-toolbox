// Holy Sword (formula 0x2D): the original damage/critical/element oracle with its draws
// replayed, then production: a sword required, physical evasion, the status at 100% on a hit
// (campaign/HOLY_SWORD.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#ifdef DARK_SWORD_CHECK
#include "x/core-holy-sword/oracle/dark.h"
#else
#include "x/core-holy-sword/oracle/source.h"
#endif
static const u8 sword_status[6][5]={{0},{0,0,0,0x20,0},{0,0,0,0,0x10},{8,0,0,0,0},{0,0,4,0,0},{0,0,2,0,0}};
static void prepare_sword(AbId ab,u8 sword)
{
    u8 k;Unit *u;prepare_spell(0x13);u=&st.unit[U_RAMZA];
    u->set=ab==0xA4?0x20:0x1D;                     // Dark Sword (Gafgarion), Holy Sword (Agrias)
    for(k=0;k<SET_N;k++)if(SET_ABILITY(u->set,k)==ab)break;
    CHECK(k<SET_N,1);u->learned=(LearnBits)1<<k;
    u->eq[EQ_WEAPON]=sword;u->eq[EQ_SHIELD]=255;
    st.unit[8].hp=0;st.unit[8].dc=DC_GONE;st.unit[8].x=255;
    st.unit[10].team=TEAM_ENEMY;st.unit[10].hp=st.unit[10].max_hp=999;
    memset(st.affinity,0,sizeof st.affinity);
}
static u32 seed_of(u16 k){return ((u32)k*0x9E3779B1UL)|1;}
#ifndef DARK_SWORD_CHECK
// Original order: the critical roll, then (on a critical) its amplitude; physical evasion is
// Attack's verified model and is zero in the oracle.
static void profiles(void)
{
    u16 i;
    for(i=0;i<sizeof source_sword/sizeof source_sword[0];i++){
        const u8 *p=source_sword[i].in;AbId ab=(AbId)p[0];Unit *a,*t;u8 j,crit,draws,e,kind;u16 cr,amount;
        prepare_sword(ab,(u8)p[1]);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->pa=(u8)p[2];a->brave=70;a->zodiac=(u8)p[3];t->zodiac=(u8)p[4];a->female=t->female=0;
        st.support[U_RAMZA]=p[5]?SUPPORT_ATTACK_UP:0;st.support[10]=0;
        memset(&t->statuses,0,sizeof t->statuses);
        for(j=0;j<5;j++)t->statuses.inflicted[j]=sword_status[p[6]][j];
        unit_status_refresh(t);
        e=cj_weapon(common,(u8)p[1])[6];
        if(e&&p[7]){if(p[7]==5)st.affinity[U_RAMZA][4]=e;else st.affinity[10][p[7]==1?2:p[7]==2?3:p[7]==3?0:1]=e;}
        rng32=seed_of(p[8]);draws=1;
        crit=(u8)(((u32)rand15()*100>>15)<4);cr=0;
        if(crit){cr=rand15();draws++;}
        CHECK(draws,source_sword[i].draws);
        kind=skill_kind(U_RAMZA,10,ab);amount=charge_query(U_RAMZA,10,ab,(u8)(crit<<2),cr);
        if(source_sword[i].healing){CHECK(kind,AB_HEAL);CHECK(amount,source_sword[i].healing);}
        else if(e&&p[7]==4){CHECK(kind,AB_NULL);}
        else{CHECK(kind,AB_DAMAGE);CHECK(amount,source_sword[i].damage);}
    }
}
// Production: the sword gate, a real cast at range, the status set at 100% on a hit.
static void sword_production(void)
{
    static const AbId skills[]={0x9C,0x9D,0x9E};   // Death Sentence, Dead, Silence
    static const u8 status_byte[]={4,0,1},status_bit[]={1,0x20,8};
    u8 i,s,hits;
    prepare_sword(0x9C,255);CHECK(can_use(U_RAMZA,0x9C),0);       // no sword: refused
    prepare_sword(0x9C,40);CHECK(can_use(U_RAMZA,0x9C),0);        // item type 5: not a sword
    for(i=0;i<3;i++){
        hits=0;
        for(s=1;s<40&&hits<3;s++){
            Unit *t;u16 hp;prepare_sword(skills[i],33);t=&st.unit[10];
            CHECK(can_use(U_RAMZA,skills[i]),1);
            memset(&t->statuses,0,sizeof t->statuses);unit_status_refresh(t);t->hp=t->max_hp=999;hp=t->hp;
            rng32=seed_of(s);st.unit[U_RAMZA].mp=999;
            CHECK(cast_order(U_RAMZA,10,skills[i],1),1);impact();
            if(t->hp==hp)continue;                                   // evaded
            hits++;
            if(skills[i]==0x9D){CHECK(t->hp,0);continue;}           // Crush Punch: Dead
            CHECK((core_status_current(&t->statuses,status_byte[i])&status_bit[i])!=0,1);
        }
        CHECK(hits>0,1);
    }
}
// The record's one status (set type 0x20), read from the status table.
static u8 sword_status_byte(AbId ab,u8 *bit)
{
    const u8 *row=cj_section(common,CJ_STATUS)+6*cj_range(common,ab)[11];u8 j;
    for(j=0;j<5;j++)if(row[1+j]){*bit=row[1+j];return j;}
    *bit=0;return 0;
}
// Holy Explosion: Earth Slash's cardinal line (CORE_LINE.md), caster excluded, physical
// evasion, the status on a hit; a critical pushes a sole line victim; a fall adds its damage.
static void explosion_production(void)
{
    u8 s,hits=0,pushes=0,byte,bit;u16 mask;
    prepare_sword(0x9F,33);byte=sword_status_byte(0x9F,&bit);CHECK(bit!=0,1);
    CHECK(abilities[0x9F].flags,AF_CAST|AF_AREA|AF_WEAPON|AF_EVADE);CHECK(abilities[0x9F].kind,AB_DAMAGE);
    CHECK(can_use(U_RAMZA,0x9F),1);
    mask=(u16)cast_targets(U_RAMZA,third,0x9F,0);
    CHECK(!!(mask&((u16)1<<10)),1);CHECK(!!(mask&((u16)1<<U_RAMZA)),0);
    CHECK(cast_order(U_RAMZA,first,0x9F,0),0);                     // as Earth Slash: not a line
    for(s=1;s<250&&!(hits>=3&&pushes);s++){
        Unit *t;u8 x,z;prepare_sword(0x9F,33);t=&st.unit[10];x=t->x;z=t->z;
        memset(&t->statuses,0,sizeof t->statuses);unit_status_refresh(t);t->hp=t->max_hp=999;
        rng32=seed_of(s);st.unit[U_RAMZA].mp=999;
        CHECK(cast_order(U_RAMZA,third,0x9F,0),1);impact();
        if(t->hp==999)continue;                                       // evaded
        hits++;CHECK((core_status_current(&t->statuses,byte)&bit)!=0,1);
        if(t->x!=x||t->z!=z)pushes++;
    }
    CHECK(hits>=3,1);CHECK(pushes>0,1);
}
static void sword_trials(void){profiles();sword_production();explosion_production();}
#else
// Dark Sword 0x2F: the original's MP loss (not capped by the target's MP) and the caster's
// equal MP gain, then a production cast: the drained MP leaves the target and reaches the caster.
static void dark_profiles(void)
{
    u16 i;
    for(i=0;i<sizeof source_dark/sizeof source_dark[0];i++){
        const u8 *p=source_dark[i].in;Unit *a,*t;u8 j,crit,draws;u16 cr;
        prepare_sword(0xA4,p[0]);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->pa=p[1];a->brave=70;a->zodiac=p[2];t->zodiac=p[3];a->female=t->female=0;
        st.support[U_RAMZA]=p[4]?SUPPORT_ATTACK_UP:0;st.support[10]=0;
        memset(&t->statuses,0,sizeof t->statuses);
        for(j=0;j<5;j++)t->statuses.inflicted[j]=sword_status[p[5]][j];
        unit_status_refresh(t);t->mp=p[6]?7:999;t->max_mp=999;
        rng32=seed_of(p[7]);
        draws=1;crit=(u8)(((u32)rand15()*100>>15)<4);cr=0;if(crit){cr=rand15();draws++;}
        CHECK(draws,source_dark[i].draws);CHECK(source_dark[i].hit,1);
        CHECK(skill_kind(U_RAMZA,10,0xA4),AB_MP_DAMAGE);
        CHECK(charge_query(U_RAMZA,10,0xA4,(u8)(crit<<2),cr),source_dark[i].mp_damage);
        CHECK(source_dark[i].mp_healing,source_dark[i].mp_damage);
    }
}
static void dark_production(void)
{
    u8 s,hits=0;
    prepare_sword(0xA4,255);CHECK(can_use(U_RAMZA,0xA4),0);           // no sword: refused
    prepare_sword(0xA4,33);
    CHECK(abilities[0xA4].flags,AF_CAST|AF_AREA|AF_WEAPON|AF_EVADE);CHECK(abilities[0xA4].kind,AB_MP_DAMAGE);
    CHECK(skill_cost(U_RAMZA,0xA4),0);
    for(s=1;s<40&&hits<3;s++){
        Unit *a,*t;u16 drained;prepare_sword(0xA4,33);a=&st.unit[U_RAMZA];t=&st.unit[10];
        CHECK(can_use(U_RAMZA,0xA4),1);
        memset(&t->statuses,0,sizeof t->statuses);unit_status_refresh(t);
        t->hp=t->max_hp=999;t->mp=40;t->max_mp=999;a->mp=10;a->max_mp=999;
        rng32=seed_of(s);
        CHECK(cast_order(U_RAMZA,10,0xA4,1),1);impact();
        if(t->mp==40)continue;                                         // evaded
        hits++;drained=40-t->mp;CHECK(t->hp,999);
        CHECK(t->mp==0||drained<40,1);
        CHECK(a->mp>10,1);                                            // the caster's MP rose
    }
    CHECK(hits>0,1);
}
static void sword_trials(void){dark_profiles();dark_production();}
#endif
#ifndef __m68k__
int main(void){sw_init(61);sword_trials();printf("holy_sword %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();sword_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
