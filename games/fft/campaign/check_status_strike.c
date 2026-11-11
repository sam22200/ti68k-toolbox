// Monster status strikes (formula 0x50, evadeable): the original evade draws, accuracy roll
// and status request replayed through the owned cast (campaign/MONSTER_STATUS_STRIKES.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-status-strike/oracle/source.h"
static void prepare_strike(AbId ab)
{
    u8 k,f;Unit *u,*t;prepare_spell(0x13);u=&st.unit[U_RAMZA];t=&st.unit[10];
    for(u->set=0x40;u->set<0xE0;u->set++){                 // the monster set holding it
        for(k=0;k<SET_N;k++)if(SET_ABILITY(u->set,k)==ab)break;
        if(k<SET_N)break;
    }
    CHECK(u->set<0xE0,1);CHECK(k<4,1);u->learned=(LearnBits)1<<k;
    u->job=94;                                             // a monster: only monsters use these (can_use)
    st.unit[8].hp=0;st.unit[8].dc=DC_GONE;st.unit[8].x=255;
    t->team=TEAM_ENEMY;t->hp=t->max_hp=999;t->job=STRIKE_JOB;u->female=t->female=0;
    for(f=0;f<4;f++){t->face=f;if(attack_side(U_RAMZA,10)==SIDE_FRONT)break;}   // the attacker in front
    CHECK(f<4,1);
    memset(st.affinity,0,sizeof st.affinity);
}
static void strike_profiles(void)
{
    u16 i;u8 k;
    for(i=0;i<sizeof source_strike/sizeof source_strike[0];i++){
        const typeof(source_strike[0]) *p=&source_strike[i];Unit *a,*t;u32 seed,expected;CastBatch b;
        prepare_strike(p->ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p->ma;a->zodiac=p->az;t->zodiac=p->tz;
        st.support[U_RAMZA]=p->up?SUPPORT_ATTACK_UP:0;st.support[10]=p->defense?SUPPORT_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        if(p->gear==2){t->eq[EQ_ACC]=STRIKE_ACC;t->eq[EQ_SHIELD]=STRIKE_SHIELD;}
        if(!p->gear){u8 f;for(f=0;f<4;f++){t->face=f;if(attack_side(U_RAMZA,10)==SIDE_BACK)break;}CHECK(f<4,1);}   // no evade source
        unit_status_refresh(t);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=p->ab;
        seed=rng32;CHECK(cast_forecast(&b),1);CHECK(b.mask,(u16)1<<10);
        CHECK(b.victim[10].hit,p->forecast);CHECK(rng32==seed,1);
        rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
        CHECK(b.victim[10].hit,p->hit);CHECK(rng32==expected,1);
        CHECK(b.victim[10].effect,p->hit?AB_STATUS:AB_NULL);
        if(p->hit){
            u8 byte=(u8)(b.victim[10].amount>>8),mask=(u8)b.victim[10].amount;
            CHECK(byte<5,1);CHECK(mask,p->add[byte<5?byte:0]);
            for(k=0;k<5;k++)if(k!=byte)CHECK(p->add[k],0);
        }
    }
}
// Production: the real cast inflicts the record's status (Blind, Poison, Petrify).
static void strike_production(void)
{
    static const AbId skills[]={0x111,0x11B,0x132};
    static const u8 sid[]={SID_DARK,SID_POISON,SID_PETRIFY};
    u8 i,s;
    for(i=0;i<3;i++){
        u8 hits=0;
        prepare_strike(skills[i]);
        CHECK(abilities[skills[i]].kind,AB_STATUS);CHECK(abilities[skills[i]].flags,AF_CAST|AF_AREA|AF_EVADE);
        CHECK(abilities[skills[i]].range,1);
        for(s=1;s<60&&!hits;s++){
            Unit *t;prepare_strike(skills[i]);t=&st.unit[10];st.unit[U_RAMZA].ma=60;
            memset(&t->statuses,0,sizeof t->statuses);unit_status_refresh(t);rng32=s;
            CHECK(can_use(U_RAMZA,skills[i]),1);
            CHECK(cast_order(U_RAMZA,10,skills[i],1),1);impact();
            if(!(core_status_current(&t->statuses,sid[i]>>3)&(0x80>>(sid[i]&7))))continue;   // evaded
            hits++;CHECK(t->hp,999);
        }
        CHECK(hits,1);
    }
}
static void strike_trials(void){strike_profiles();strike_production();}
#ifndef __m68k__
int main(void){sw_init(61);strike_trials();printf("status_strike %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();strike_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
