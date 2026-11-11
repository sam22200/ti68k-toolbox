// Ruins (Arc Knight): the original 0x1A/0x1B chance, draws, MP loss and stat loss replayed,
// then production casts on radius 1 (campaign/RUINS.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-ruin/oracle/source.h"
static const u8 ruin_status[4][5]={{0},{0,0,0,0x10,0},{0,0,2,0,0},{0,0,4,0,0}};
static void prepare_ruin(AbId ab)
{
    u8 k;Unit *u;prepare_spell(0x13);u=&st.unit[U_RAMZA];
    for(u->set=0x1D;u->set<0x50;u->set++){                 // the first set holding it (Arc Knight 0x4B)
        for(k=0;k<SET_N;k++)if(SET_ABILITY(u->set,k)==ab)break;
        if(k<SET_N)break;
    }
    CHECK(u->set<0x50,1);u->learned=(LearnBits)1<<k;
    u->eq[EQ_WEAPON]=33;u->eq[EQ_SHIELD]=255;               // a sword is required
    st.unit[8].hp=0;st.unit[8].dc=DC_GONE;st.unit[8].x=255;
    st.unit[10].team=TEAM_ENEMY;st.unit[10].hp=st.unit[10].max_hp=999;
    st.unit[10].eq[EQ_ACC]=232;st.unit[10].eq[EQ_SHIELD]=128;st.unit[10].face=2;   // never evaded (flags_4 lacks 0x02)
    memset(st.affinity,0,sizeof st.affinity);
}
static u32 seed_of(u16 k){return ((u32)k*0x9E3779B1UL)|1;}
static void ruin_profiles(void)
{
    u16 i;
    for(i=0;i<sizeof source_ruin/sizeof source_ruin[0];i++){
        const RuinProfile *r=&source_ruin[i];const u8 *p=r->in;AbId ab=(AbId)(0xC4+p[0]);
        Unit *a,*t;u8 j,chance,hit,draws=0;
        prepare_ruin(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p[1];a->faith=p[2];t->faith=p[3];a->zodiac=p[4];t->zodiac=p[5];a->female=t->female=0;
        st.support[U_RAMZA]=p[6]?SUPPORT_MAGIC_ATTACK_UP:0;st.support[10]=p[7]?SUPPORT_MAGIC_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        for(j=0;j<5;j++)t->statuses.inflicted[j]=ruin_status[p[8]][j];
        unit_status_refresh(t);t->mp=t->max_mp=(u16)p[9]*10;
        chance=(u8)magic_heal(U_RAMZA,10,ab,1);
        CHECK(chance,r->accuracy>100?100:(u8)r->accuracy);
        rng32=seed_of(p[10]);hit=1;
        if(chance<100){draws=1;hit=(u16)(muls16((s16)rand15(),100)>>15)<chance;}
        CHECK(draws,r->draws);CHECK(hit,r->hit);
        if(!hit)continue;
        if(ab==0xC4){CHECK(skill_kind(U_RAMZA,10,ab),AB_MP_DAMAGE);CHECK(magic_heal(U_RAMZA,10,ab,0),r->mp_damage);}
        else{CHECK(skill_kind(U_RAMZA,10,ab),ab==0xC5?AB_SPEED:AB_BREAK);CHECK(magic_heal(U_RAMZA,10,ab,0),r->change);}
    }
}
// Production: a real cast at a unit; on a hit the stat (or MP) falls, at least 1 for stats.
static void ruin_production(void)
{
    AbId ab;
    for(ab=0xC4;ab<=0xC7;ab++){
        u8 s,hits=0;
        prepare_ruin(ab);CHECK(abilities[ab].flags,AF_CAST|AF_AREA);
        st.unit[U_RAMZA].eq[EQ_WEAPON]=255;CHECK(can_use(U_RAMZA,ab),0);   // no sword: refused
        for(s=1;s<40&&hits<2;s++){
            Unit *a,*t;u8 sp,pa,ma;u16 mp;prepare_ruin(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
            a->ma=20;a->faith=t->faith=90;
            memset(&t->statuses,0,sizeof t->statuses);unit_status_refresh(t);
            t->sp=sp=8;t->pa=pa=12;t->ma=ma=11;t->mp=mp=t->max_mp=300;a->mp=999;rng32=seed_of(s);
            CHECK(can_use(U_RAMZA,ab),1);
            if(s==1){CHECK(cast_order(U_RAMZA,10,ab,1),2);continue;}   // the record's CT: a deferred cast
            st.support[U_RAMZA]=SUPPORT_NON_CHARGE;                     // then resolved at once
            CHECK(cast_order(U_RAMZA,10,ab,1),1);impact();
            if(t->sp==sp&&t->pa==pa&&t->ma==ma&&t->mp==mp)continue;      // the percent roll missed
            hits++;
            if(ab==0xC4){CHECK(t->mp,150);CHECK(t->pa,pa);}
            else if(ab==0xC5){CHECK(t->sp,5);CHECK(t->pa,pa);}
            else if(ab==0xC6){CHECK(t->pa,7);CHECK(t->ma,ma);}
            else {CHECK(t->ma,6);CHECK(t->pa,pa);}
        }
        CHECK(hits>0,1);
    }
}
static void ruin_trials(void){ruin_profiles();ruin_production();}
#ifndef __m68k__
int main(void){sw_init(61);ruin_trials();printf("ruin %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();ruin_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
