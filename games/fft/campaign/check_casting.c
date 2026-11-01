// Source casting policy plus real shared MP eligibility/commit with a test carrier.
#define REACTION_SEQUENCE_CHECK
#include "check_reactions.c"
#include "x/core-casting/oracle/source.h"
static u32 state_hash(void)
{
    const u8 *p=(const u8 *)&st;u16 n;u32 h=17;
    for(n=0;n<sizeof st;n++)h=(h<<1|h>>31)^*p++;
    return h;
}
static void source_rules(void)
{
    u16 i;u8 cost;
    for(i=0;i<sizeof source_cast_payment/sizeof source_cast_payment[0];i++){
        const u16 *p=source_cast_payment[i];u16 mp=p[4];
        cost=core_cast_cost(p[0],p[1],p[2],p[3]);
        if(p[6]==255){
            CHECK(cost,p[8]);CHECK(mp<cost?3:0,p[9]);
            if(mp>=cost&&!p[5])mp-=cost;
            CHECK(mp,p[7]);
        }else{CHECK(p[7],p[4]);CHECK(p[8],0);CHECK(p[9]!=0,1);}
    }
    for(i=0;i<sizeof source_cast_preparation/sizeof source_cast_preparation[0];i++){
        const u16 *p=source_cast_preparation[i];u8 ticks=core_cast_ticks(p[0],p[1],p[2],p[3]);
        CHECK(!ticks,p[5]);CHECK(p[4]?ticks:255,p[6]);CHECK(p[7],0);
        CHECK(p[8],p[4]);CHECK(p[9],999);
    }
    // Native normalization strips the high timing flag; no support can change
    // a persevering performance into an instant or half_mp-duration one.
    CHECK(core_cast_ticks(0x87,0,1,0),4);CHECK(core_cast_ticks(0xff,1,1,1),127);
}
static void prepared(void)
{
    arena();st.unit[U_RAMZA].set=SET_BASIC;CHECK(cj_valid(common,cj_word(common+CJ_LENGTH)),1);
    CHECK(cj_range(common,0x93)[13],7);CHECK(st.unit[U_RAMZA].set,SET_BASIC);
    st.unit[U_RAMZA].mp=10;st.unit[U_RAMZA].eq[EQ_SHIELD]=255;
}
static void eligibility(void)
{
    u8 who,half_mp;u32 hash,seed;Unit *a;char text[32];
    for(who=0;who<2;who++)for(half_mp=0;half_mp<2;half_mp++){
        u8 id=who?10:U_RAMZA;prepared();a=&st.unit[id];a->set=SET_BASIC;
        st.support[id]=half_mp?SUPPORT_HALF_MP:0;a->mp=half_mp?2:6;
        hash=state_hash();seed=rng32;
        CHECK(skill_cost(id,0x93),half_mp?3:7);CHECK(can_use(id,0x93),0);
        CHECK(state_hash()==hash,1);
        CHECK(skill_hit(id,id,0x93),0);forecast_text(id,id,0x93,text);
        // Native preview writes the formula request scratch, not world resources.
        CHECK(a->mp,half_mp?2:6);CHECK(a->hp,999);CHECK(rng32==seed,1);
        hash=state_hash();
        skill(id,id,0x93);CHECK(state_hash()==hash,1);CHECK(rng32==seed,1);
        a->mp++;CHECK(can_use(id,0x93),1);
        CHECK(skill_hit(id,id,0x93)>0,1);
    }
    prepared();a=&st.unit[U_RAMZA];a->mp=999;
    CHECK(can_use(U_RAMZA,38),0);CHECK(abilities[38].kind,255); // Float (deferred movement) remains gated; Death since CORE_TERMINAL.
    CHECK(skill_cost(U_RAMZA,0x170),0);CHECK(skill_cost(U_RAMZA,0x196),0);
    a->set=8;CHECK(skill_cost(U_RAMZA,0x93),0);a->set=0xe0;CHECK(skill_cost(U_RAMZA,0x93),0);
    a->set=SET_BASIC;a->job=SOURCE_JOB_MIME;a->mp=0;
    CHECK(skill_cost(U_RAMZA,0x93),0);CHECK(can_use(U_RAMZA,0x93),1);
}
static void ai_resources(void)
{
    u8 who,half_mp,affordable,k,found,id;u32 seed;
    for(who=0;who<2;who++)for(half_mp=0;half_mp<2;half_mp++)for(affordable=0;affordable<2;affordable++){
        prepared();id=who?10:U_RAMZA;st.act=id;st.unit[id].set=SET_BASIC;
        st.unit[id].learned=0xffff;st.support[id]=half_mp?SUPPORT_HALF_MP:0;
        st.unit[id].mp=(half_mp?3:7)-(affordable?0:1);
        st.acted=0;st.moved=1;W->ai_k=0;seed=rng32;
        CHECK(ai_think(),0);found=0;
        for(k=0;k<W->ai_nab;k++)if(W->ai_list[k]==0x93)found=1;
        CHECK(found,affordable);CHECK(rng32==seed,1);
        CHECK(st.unit[id].mp,(half_mp?3:7)-(affordable?0:1));
    }
}
static void transactions(void)
{
    u8 half_mp;u32 hash,seed;u16 hp;
    for(half_mp=0;half_mp<2;half_mp++){
        prepared();st.support[U_RAMZA]=half_mp?SUPPORT_HALF_MP:0;
        hp=st.unit[10].hp;ZB(35);skill(U_RAMZA,10,0x93);ZE(35);
        CHECK(st.unit[U_RAMZA].mp,half_mp?7:3);CHECK(st.action.phase,CTX_ROOT);
        hash=state_hash();seed=rng32;skill(U_RAMZA,10,0x93);
        CHECK(state_hash()==hash,1);CHECK(rng32==seed,1);
        st.hit=0;st.push=0;impact();impact();CHECK(st.unit[U_RAMZA].mp,half_mp?7:3);
        CHECK(st.unit[10].hp,hp);end_anim();CHECK(st.action.phase,CTX_IDLE);
        CHECK(st.unit[U_RAMZA].mp,half_mp?7:3);
        st.unit[U_RAMZA].mp=14;skill(U_RAMZA,10,0x93);
        CHECK(st.unit[U_RAMZA].mp,half_mp?11:7);
    }
    // Monsters/enemies do not get free MP merely because enemy items are free.
    prepared();st.unit[10].set=SET_BASIC;st.unit[10].mp=7;
    skill(10,U_RAMZA,0x93);CHECK(st.unit[10].mp,0);CHECK(st.action.root,10);
    // Reject before discarding an independently armed Charge or formula request.
    prepared();skill(U_RAMZA,10,0x196);st.unit[U_RAMZA].mp=6;
    hash=state_hash();seed=rng32;skill(U_RAMZA,10,0x93);
    CHECK(state_hash()==hash,1);CHECK(rng32==seed,1);
    CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);
    // The test carrier's silence flag is synthetic, not retail Dash behavior.
    prepared();change(U_RAMZA,SID_SILENCE);hash=state_hash();seed=rng32;
    skill(U_RAMZA,10,0x93);CHECK(state_hash()==hash,1);CHECK(rng32==seed,1);
    CHECK(st.unit[U_RAMZA].mp,10);CHECK(can_use(U_RAMZA,0x93),0);
    prepared();change(U_RAMZA,SID_DONT_ACT);hash=state_hash();skill(U_RAMZA,10,0x93);
    CHECK(state_hash()==hash,1);CHECK(st.unit[U_RAMZA].mp,10);
    // Existing Charge remains a zero-MP deferred command: no debit on arm/due.
    prepared();st.unit[U_RAMZA].mp=0;skill(U_RAMZA,10,0x196);
    CHECK(st.pending[U_RAMZA].ticks,4);CHECK(st.unit[U_RAMZA].mp,0);
    st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);
    CHECK(st.action.phase,CTX_ROOT);CHECK(st.unit[U_RAMZA].mp,0);
}
static void cold_range(void)
{
    u8 sequence;
    prepared();sequence=hl_seq;ZB(36);skill_range(U_RAMZA,0x93);ZE(36);
    CHECK(hl_seq,(u8)(sequence+1));CHECK(st.reach[second],1);
    CHECK(st.reach[first],255);CHECK(st.unit[U_RAMZA].mp,10);
    CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
    // COMMAND has returned before FORMULA/AI may query the same candidate.
    CHECK(skill_hit(U_RAMZA,10,0x93),100);CHECK(st.unit[U_RAMZA].mp,10);
}
static void casting_trials(void){source_rules();eligibility();ai_resources();transactions();cold_range();}
#ifndef INCANTATION_CHECK
#ifndef __m68k__
int main(void){sw_init(61);casting_trials();printf("casting %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();casting_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
