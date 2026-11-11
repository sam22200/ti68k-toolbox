// Production cancellation family; source removals packed into existing events.
#define REFLECT_CHECK
#include "check_offensive_magic.c"
#include "x/core-cleanse/oracle/source.h"
#include "../cleanse_rules.h"
static const AbId cleanses[]={14,56,0xA7,377,378,379,380};
static void prepare_cleanse(AbId ab)
{
    u8 j,k;prepare_spell(0x13);
    for(j=0x4a;j<0x5f;j++){                     // generic jobs, then Ovelia's Holy Magic (0x24)
        st.unit[U_RAMZA].set=j<0x5e?job_sets[j]:0x24;
        for(k=0;k<SET_N;k++)if(SET_ABILITY(st.unit[U_RAMZA].set,k)==ab)break;
        if(k<SET_N)break;
    }
    CHECK(j<0x5f,1);st.unit[U_RAMZA].learned=(LearnBits)1<<k;
    CHECK(known(U_RAMZA,0),ab);CHECK(known(U_RAMZA,1),NO_ABIL);
    st.unit[8].hp=0;st.unit[8].dc=DC_GONE;st.unit[8].x=255;
    st.unit[10].team=ab==56?TEAM_ENEMY:TEAM_PLAYER;
    if(ab>=368)st.bag[abilities[ab].bag]=3;
}
static void seed_profile(Unit *t,const u8 *r,u8 mode)
{
    u8 j;
    for(j=0;j<5;j++){
        t->statuses.inflicted[j]=mode==1||mode==2||mode==3||mode==7?r[j+1]:0;
        t->statuses.innate[j]=mode==2?r[j+1]:mode==3?r[j+1]&0xaa:0;
        t->statuses.immune[j]=mode==3?r[j+1]:0;
    }
    if(mode==4)t->statuses.inflicted[0]|=32;
    if(mode==5)t->statuses.inflicted[1]|=128;
    if(mode==6)t->statuses.inflicted[3]|=1;
    if(mode==7)t->statuses.inflicted[3]|=8;
    unit_status_refresh(t);
}
static void original_profiles(void)
{
    u16 i,c;u8 k;CastBatch b;const u8 *r;u32 expected;
    for(i=0;i<sizeof source_cleanse/sizeof source_cleanse[0];i++){
        const u16 *p=source_cleanse[i];AbId ab=p[0];u8 sid;
        prepare_cleanse(ab);sid=ab<368?abilities[ab].y:cj_section(common,CJ_ITEM_FORMULA)[3*(ab-368)+2];
        r=cj_section(common,CJ_STATUS)+6*sid;
        st.unit[U_RAMZA].faith=p[2];st.unit[10].faith=p[3];seed_profile(&st.unit[10],r,p[1]);
        if(ab>=374&&ab<=376){
            // Existing compact pilot items retain their presentation adapter.
            // Compare the shared removal representation to the same source stage.
            CHECK(magic_target(10)==MAGIC_VALID?core_cleanse_pack(&st.unit[10].statuses,r+1):0,p[6]);
            continue;
        }
        CHECK(abilities[ab].kind,AB_CLEANSE);rng32=1;
        if(ab<368){
            // The original fixture resolves one victim, without a caster AoE roll.
            for(c=0;c<MAP_N;c++)if(abs((s16)TILE_X[c]-st.unit[10].x)+abs((s16)TILE_Z[c]-st.unit[10].z)>4)break;
            CHECK(c<MAP_N,1);unit_place(&st.unit[U_RAMZA],(u8)c);
            memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=ab;
            CHECK(cast_forecast(&b),1);CHECK(b.victim[10].hit,p[4]?p[5]:0);
            CHECK(b.victim[10].amount,p[6]);CHECK(rng32==1,1);
            rng32=1;for(k=0;k<p[11];k++)rand15();expected=rng32;rng32=1;
            b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
            CHECK(b.victim[10].hit,p[8]);CHECK(b.victim[10].amount,p[10]);CHECK(rng32==expected,1);
        }else{
            CHECK(skill_hit(U_RAMZA,10,ab),p[4]?p[5]:0);
            CHECK(magic_applicable(10,ab)?skill_amount(U_RAMZA,10,ab,0):0,p[6]);
            CHECK(rng32==1,1);
        }
    }
}
static void production(void)
{
    u8 j,k,byte,ct;u16 hp,mp,packed;u32 seed;const u8 *r;Unit *t;
    for(j=0;j<sizeof cleanses/sizeof cleanses[0];j++){
        AbId ab=cleanses[j];prepare_cleanse(ab);t=&st.unit[10];
        r=cj_section(common,CJ_STATUS)+6*abilities[ab].y;seed_profile(t,r,1);
        st.unit[U_RAMZA].ma=99;hp=t->hp;mp=t->mp;
        for(k=0;k<16;k++)t->status_ct[k]=7;packed=magic_heal(U_RAMZA,10,ab,0);CHECK(packed!=0,1);
        if(ab<368){CHECK(cast_order(U_RAMZA,10,ab,1),2);CHECK(st.unit[U_RAMZA].mp,999);
            st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(W->cast.victim[10].amount,packed);
        }else{seed=rng32;skill(U_RAMZA,10,ab);CHECK(st.mode,M_ACT);CHECK(st.dmg,packed);
            CHECK(st.bag[abilities[ab].bag],2);CHECK(rng32==seed,1);}
        CHECK(core_cleanse_pack(&t->statuses,r+1),packed);impact();CHECK(t->hp,hp);CHECK(t->mp,mp);
        for(byte=0;byte<5;byte++)CHECK(t->statuses.inflicted[byte]&r[byte+1],0);
        for(k=0;k<16;k++){ct=(r[4+(k>>3)]&(128>>(k&7)))?0:7;CHECK(t->status_ct[k],ct);}
        impact();CHECK(t->hp,hp);CHECK(t->mp,mp);
        if(ab>=368)CHECK(st.bag[abilities[ab].bag],2);
    }
}
static void frozen_and_guarded(void)
{
    u8 j;const u8 *r;Unit *t;u16 hp;u32 seed;
    for(j=0;j<sizeof cleanses/sizeof cleanses[0];j++){
        AbId ab=cleanses[j];prepare_cleanse(ab);t=&st.unit[10];r=cj_section(common,CJ_STATUS)+6*abilities[ab].y;
        seed_profile(t,r,2);CHECK(skill_hit(U_RAMZA,10,ab),0);CHECK(magic_heal(U_RAMZA,10,ab,0),0);
        prepare_cleanse(ab);t=&st.unit[10];hp=t->hp;seed=rng32;
        CHECK(skill_hit(U_RAMZA,10,ab),0);CHECK(rng32==seed,1);
        if(ab>=368){st.bag[abilities[ab].bag]=0;CHECK(can_use(U_RAMZA,ab),0);skill(U_RAMZA,10,ab);CHECK(st.action.phase,CTX_IDLE);}
        CHECK(t->hp,hp);
    }
    prepare_cleanse(14);t=&st.unit[10];t->statuses.inflicted[3]=128;unit_status_refresh(t);
    st.support[U_RAMZA]=SUPPORT_NON_CHARGE;st.unit[U_RAMZA].ma=99;
    CHECK(cast_order(U_RAMZA,10,14,1),1);t->statuses.inflicted[4]|=16;unit_status_refresh(t);
    impact();CHECK(t->statuses.inflicted[3]&128,0);CHECK(t->statuses.inflicted[4]&16,16);
    CHECK(t->hp,999);CHECK(st.action.pending,0);
    prepare_cleanse(378);t=&st.unit[10];t->statuses.inflicted[1]=128;unit_status_refresh(t);
    CHECK(unit_can_act(t),0);CHECK(skill_hit(U_RAMZA,10,378),100);hp=t->hp;
    skill(U_RAMZA,10,378);impact();CHECK(unit_petrified(t),0);CHECK(unit_can_act(t),1);CHECK(t->hp,hp);
}
static void cleansing_trials(void){original_profiles();production();frozen_and_guarded();}
#ifndef CLEANSE_RUNTIME_CHECK
#ifndef __m68k__
int main(void){sw_init(61);cleansing_trials();printf("cleanse %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();cleansing_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
