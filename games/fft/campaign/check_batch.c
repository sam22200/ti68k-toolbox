// Caller-owned compound preparation, once-only MP/HP and existing root drain.
#define BATCH_CHECK
#include "check_targets.c"
#include "fixture_monster.h"
static u8 fixture_cast_batch(u8 op,CastBatch *b){if(b)fixture_monster(b->actor,b->ability);return cast_batch(op,b);}
static u8 fixture_cast_forecast(CastBatch *b){if(b)fixture_monster(b->actor,b->ability);return cast_forecast(b);}
#define cast_batch fixture_cast_batch
#define cast_forecast fixture_cast_forecast
static CastBatch batch;
static u32 batch_state_hash(void){return world_hash();}
static void prepare_profile(const u16 *p)
{
    u8 i,k;arena();st.act=0;
    for(i=0;i<NUNIT;i++){
        Unit *u=&st.unit[i];const u16 *r=p+7+14*i;
        unit_place(u,first);u->dc=3;u->hp=r[3]==SID_DEAD?0:100;
        u->max_hp=u->max_mp=999;u->mp=999;u->ma=p[1]==255&&i==0?99:5;
        u->faith=r[4];u->zodiac=0;u->set=SET_BASIC;u->team=i?TEAM_ENEMY:TEAM_PLAYER;
        memset(&u->statuses,0,sizeof u->statuses);u->status_flags=0;
        if(r[3]!=255)u->statuses.inflicted[r[3]>>3]|=0x80>>(r[3]&7);
        if(r[9]==1)u->statuses.inflicted[2]|=128;
        if(r[9]==2)u->statuses.innate[2]|=128;
        memset(u->eq,255,sizeof u->eq);
        u->eq[EQ_ACC]=r[0];u->eq[EQ_WEAPON]=r[1];u->eq[EQ_SHIELD]=r[2];
        memset(st.affinity[i],0,5);
        for(k=0;k<4;k++)if(r[5+k])st.affinity[i][k]=p[2];
        st.reaction[i]=st.support[i]=0;unit_status_refresh(u);
    }
    st.unit[0].mp=7;st.weather=0;st.acted=1;
    memset(&batch,0,sizeof batch);batch.ability=0x110;batch.ref=first;
    rng32=0x2545f491UL^((u32)p[0]<<16|p[0]);
}
static void read_profile(const u8 *src,u16 index,u16 *p)
{
    u16 k;for(k=0;k<189;k++)p[k]=RD16(src+6+378*index,k);
}
static void source_profiles(void)
{
    u16 bytes,n,k,p[189],mask,hps[13];u8 i;u32 seed,hash;
    const u8 *src=rt_file("fba00",&bytes);CHECK(src!=RT_NULL,1);if(!src)return;
    CHECK(sizeof batch,60);CHECK(RD16(src,2),13);
    for(n=0;n<13;n++){
        read_profile(src,n,p);prepare_profile(p);batch.ability+=n;
#ifdef FFT_MODULES
        mod_call(MOD_PARTY,255,0);
#endif
        ZB(61);CHECK(cast_batch(CAST_BATCH_PREPARE,&batch),1);ZE(61);
        CHECK(batch.phase,CAST_BATCH_READY);CHECK(st.unit[0].mp,0);
        CHECK(rng32==((u32)p[4]<<16|p[5]),1);mask=0;
        for(i=0;i<NUNIT;i++){
            const u16 *r=p+7+14*i;
            if(r[10])mask|=(u16)1<<i;
            CHECK(batch.victim[i].hit,r[11]);CHECK(batch.victim[i].amount,r[12]);
            CHECK(batch.victim[i].effect,r[13]);CHECK(st.unit[i].hp,r[3]==SID_DEAD?0:100);
            hps[i]=magic_hp(r[13]&EFFECT_KIND_MASK,st.unit[i].hp,999,r[12]);
        }
        CHECK(batch.mask,mask);CHECK(st.action.pending,0);CHECK(st.unit[0].gexp,0);
        hash=batch_state_hash();seed=rng32;
        CHECK(cast_batch(CAST_BATCH_PREPARE,&batch),0);CHECK(batch_state_hash()==hash,1);CHECK(rng32==seed,1);
        // Pure predictions and later live scalar changes cannot replace prepared results.
        for(i=0;i<13;i++)st.unit[i].ma=1;
        battle_weather(3,0);skill_amount(0,1,batch.ability,0);
        ZB(62);CHECK(cast_batch(CAST_BATCH_COMMIT,&batch),1);ZE(62);
        CHECK(batch.phase,CAST_BATCH_APPLIED);CHECK(st.action.phase,CTX_IMPACT);
        for(i=0;i<NUNIT;i++)CHECK(st.unit[i].hp,hps[i]);
        CHECK(st.unit[0].mp,0);CHECK(st.unit[0].gexp,0);CHECK(rng32==seed,1);
        hash=batch_state_hash();CHECK(cast_batch(CAST_BATCH_COMMIT,&batch),0);
        CHECK(batch_state_hash()==hash,1);CHECK(rng32==seed,1);
        // Normal root completion settles the maximum reward once, then terminal checks.
        for(k=0;st.action.phase!=CTX_IDLE&&k<32;k++)end_anim();
        CHECK(st.action.phase,CTX_IDLE);CHECK(k<32,1);CHECK(st.unit[0].mp,0);
    }
}
static void rejected_and_empty(void)
{
    u16 size,p[189];u32 seed,hash;const u8 *src=rt_file("fba00",&size);u8 i;
    read_profile(src,2,p);prepare_profile(p);batch.ability+=2;
    st.unit[0].mp=6;fixture_monster(0,batch.ability);hash=batch_state_hash();seed=rng32;   // the caster's set before the hash
    CHECK(cast_batch(CAST_BATCH_PREPARE,&batch),0);CHECK(batch_state_hash()==hash,1);CHECK(rng32==seed,1);
    CHECK(batch.phase,CAST_BATCH_EMPTY);CHECK(batch.mask,0);
    CHECK(cast_batch(2,&batch),0);CHECK(cast_batch(0,RT_NULL),0);CHECK(cast_batch(1,&batch),0);
    st.unit[0].mp=7;batch.actor=13;hash=batch_state_hash();CHECK(cast_batch(0,&batch),0);CHECK(batch_state_hash()==hash,1);
    batch.actor=0;batch.follow=2;CHECK(cast_batch(0,&batch),0);CHECK(batch.phase,0);
    batch.follow=0;batch.ability=431;CHECK(cast_batch(0,&batch),0);CHECK(abilities[431].kind,255); // Reaction IDs remain unavailable as commands
    batch.ability=0x11e;CHECK(cast_batch(0,&batch),0);CHECK(batch.phase,0);
    batch.ability=0x11f;CHECK(cast_batch(0,&batch),0);CHECK(batch.phase,0);
    CHECK(rng32==seed,1);
    // Private raw flag removes the caster; all other actors are absent.
    prepare_profile(p);batch.ability=0x11d;
    for(i=1;i<NUNIT;i++){st.unit[i].x=255;st.unit[i].dc=DC_GONE;}
    seed=rng32;CHECK(cast_batch(0,&batch),1);CHECK(batch.mask,0);CHECK(rng32==seed,1);
    CHECK(st.unit[0].mp,0);CHECK(cast_batch(1,&batch),1);CHECK(st.primary.reward,0);
    end_anim();CHECK(st.action.phase,CTX_IDLE);CHECK(st.unit[0].gexp,0);
    // A populated but entirely nullified area still pays once, with no reward.
    prepare_profile(p);batch.ability=0x112;
    for(i=0;i<NUNIT;i++)st.affinity[i][1]=128;
    seed=rng32;CHECK(cast_batch(0,&batch),1);CHECK(batch.mask!=0,1);
    CHECK(rng32==seed,1);CHECK(cast_batch(1,&batch),1);
    CHECK(st.primary.reward,0);CHECK(st.action.pending,0);CHECK(st.unit[0].mp,0);
    end_anim();CHECK(st.action.phase,CTX_IDLE);CHECK(st.unit[0].gexp,0);

}
static void responses_and_late_protection(void)
{
    u16 size,p[189],hp;u8 i;const u8 *src=rt_file("fba00",&size);
    read_profile(src,2,p);prepare_profile(p);batch.ability+=2;
    for(i=0;i<12;i++)st.bag[i]=0;st.bag[0]=2;
    st.reaction[8]=st.reaction[12]=REACTION_POTION;
    st.unit[8].brave=st.unit[12].brave=100;
    st.unit[8].team=st.unit[12].team=TEAM_PLAYER;
    st.unit[8].statuses.innate[2]=0;st.unit[8].statuses.inflicted[2]|=128;
    CHECK(cast_batch(0,&batch),1);CHECK(batch.victim[8].effect&EFFECT_OIL,EFFECT_OIL);
    // Innate protection added after launch is rechecked at commit.
    st.unit[8].statuses.innate[2]|=128;
    CHECK(cast_batch(1,&batch),1);CHECK(st.unit[8].statuses.inflicted[2]&128,128);
    CHECK(st.action.pending,((u16)1<<8)|((u16)1<<12));CHECK(st.bag[0],2);
    hp=st.unit[8].hp;end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.action.actor,8);
    impact();CHECK(st.unit[8].hp,hp+30>999?999:hp+30);end_anim();
    CHECK(st.action.actor,12);CHECK(st.action.phase,CTX_SECONDARY);impact();end_anim();
    for(i=0;st.action.phase!=CTX_IDLE&&i<16;i++)end_anim();
    CHECK(st.action.phase,CTX_IDLE);CHECK(st.bag[0],0);CHECK(st.unit[0].mp,0);
    CHECK(st.unit[0].gexp>0,1);
}
static void batch_trials(void){source_profiles();rejected_and_empty();responses_and_late_protection();}
#ifndef FORECAST_CHECK
#ifndef __m68k__
int main(void){sw_init(61);batch_trials();printf("batch %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();batch_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif // FORECAST_CHECK
