// Production catalogue, real learned lists, commands, impacts and area AI.
// No normalized spell overrides: every entry comes from the shipping compiler.
#define REACTION_SEQUENCE_CHECK
#include "check_reactions.c"
static const AbId spells[]={0x13,0x17,0x1b,0x2c,0x3d,0x3e,0x3f,
                            0x40,0x43,0x44,0x45,0x46,0x4a,0x4b};
static void prepare_spell(AbId ab)
{
    u8 i,k;arena();
    for(i=0;i<NUNIT;i++){
        Unit *u=&st.unit[i];memset(&u->statuses,0,sizeof u->statuses);
        memset(st.affinity[i],0,5);memset(u->eq,255,sizeof u->eq);
        st.reaction[i]=st.support[i]=0;u->status=u->status_flags=0;
        u->ma=5;u->faith=100;u->zodiac=0;u->mp=u->max_mp=999;
        u->learned=0;u->defend=0;
    }
    st.unit[8]=st.unit[10];unit_place(&st.unit[8],third);st.unit[8].team=TEAM_PLAYER;
    st.unit[U_RAMZA].set=ab<0x20?11:ab<0x3d?12:13;
    for(k=0;k<SET_N;k++)if(SET_ABILITY(st.unit[U_RAMZA].set,k)==ab)break;
    CHECK(k<SET_N,1);st.unit[U_RAMZA].learned=(LearnBits)1<<k;
    CHECK(known(U_RAMZA,0),ab);CHECK(known(U_RAMZA,1),NO_ABIL);
    st.acted=st.moved=st.open_resolving=st.open_ct_phase=0;st.weather=0;
}
static void batch_execution(void)
{
    u8 j,follow,k;u16 mask,hp,expected;const u8 *r;AbId ab;
    for(j=0;j<sizeof spells/sizeof spells[0];j++)for(follow=0;follow<2;follow++){
        ab=spells[j];prepare_spell(ab);r=cj_range(common,ab);
        CHECK(abilities[ab].formula,8);CHECK(can_use(U_RAMZA,ab),1);
        mask=(u16)1<<10;
        if(!(r[4]&64))mask|=(u16)1<<8;
        if(!(r[4]&65))mask|=(u16)1<<U_RAMZA;
        CHECK(cast_targets(U_RAMZA,follow?10:second,ab,follow),mask);
        ZB(71);CHECK(cast_order(U_RAMZA,follow?10:second,ab,follow),2);ZE(71);
        CHECK(st.pending[U_RAMZA].ticks,r[12]);CHECK(st.unit[U_RAMZA].mp,999);
        st.pending[U_RAMZA].ticks=0;
        ZB(72);CHECK(charge_resolve(),1);ZE(72);
        CHECK(st.unit[U_RAMZA].mp,999-r[13]);CHECK(W->cast.mask,mask);
        expected=5*r[10];CHECK(W->cast.victim[10].amount,expected);
        CHECK(W->cast.victim[10].hit,1);CHECK(W->cast.victim[10].effect,AB_DAMAGE);
        hp=st.unit[10].hp;impact();CHECK(st.unit[10].hp,hp-expected);
        CHECK(st.unit[8].hp,(mask&((u16)1<<8))?999-expected:999);
        CHECK(st.unit[U_RAMZA].hp,(mask&((u16)1<<U_RAMZA))?999-expected:999);
        impact();CHECK(st.unit[10].hp,hp-expected);CHECK(st.unit[U_RAMZA].mp,999-r[13]);
        for(k=0;k<8&&st.action.phase!=CTX_IDLE;k++)end_anim();
        CHECK(st.action.phase,CTX_IDLE);CHECK(W->cast.phase,CAST_BATCH_EMPTY);
        CHECK(st.unit[U_RAMZA].mp,999-r[13]);
    }
}
static void eligibility_batch(void)
{
    u8 j;AbId ab;const u8 *r;u32 seed;
    for(j=0;j<sizeof spells/sizeof spells[0];j++){
        ab=spells[j];prepare_spell(ab);r=cj_range(common,ab);
        st.unit[U_RAMZA].mp=r[13]-1;seed=rng32;
        CHECK(cast_order(U_RAMZA,10,ab,1),0);CHECK(st.acted,0);
        CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(rng32==seed,1);
        st.support[U_RAMZA]=SUPPORT_HALF_MP;CHECK(skill_cost(U_RAMZA,ab),r[13]>>1);
        st.unit[U_RAMZA].mp=r[13]>>1;CHECK(cast_order(U_RAMZA,10,ab,1),2);
        st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(st.unit[U_RAMZA].mp,0);
        prepare_spell(ab);change(U_RAMZA,SID_SILENCE);seed=rng32;
        CHECK(cast_order(U_RAMZA,10,ab,1),0);CHECK(st.acted,0);CHECK(rng32==seed,1);
        prepare_spell(ab);CHECK(cast_order(U_RAMZA,10,ab,1),2);
        change(U_RAMZA,SID_SILENCE);st.pending[U_RAMZA].ticks=0;
        CHECK(charge_resolve(),0);CHECK(st.unit[U_RAMZA].mp,999);
    }
}
static void ai_batch(void)
{
    u8 j,k,found;u16 steps;AbId ab;
    for(j=0;j<sizeof spells/sizeof spells[0];j++){
        ab=spells[j];prepare_spell(ab);st.moved=1;W->ai_k=0;steps=0;
        ZB(73);while(!ai_think()&&++steps<2048){}ZE(73);
        CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,1);CHECK(st.unit[U_RAMZA].mp,999);CHECK(st.unit[10].hp,999);
    }
}
static void targeting_batch(void)
{
    u8 j,k;u16 mask;
    // Same radius/tolerance but real source ally filtering, no per-summon branch.
    for(j=0;j<sizeof spells/sizeof spells[0];j++){
        prepare_spell(spells[j]);st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
        CHECK(cast_order(U_RAMZA,10,spells[j],1),1);
        CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(W->cast.phase,CAST_BATCH_READY);
        mask=W->cast.mask;CHECK(mask&((u16)1<<10),(u16)1<<10);
        for(k=0;k<NUNIT;k++)if(mask&((u16)1<<k))CHECK(W->cast.victim[k].amount,5*cj_range(common,spells[j])[10]);
    }
    // The second batch now shares the source Reflect gate.
    for(j=0xf;j<=0x1f;j++)if(cj_range(common,j)[8]==8&&(cj_range(common,j)[5]&128))
        CHECK(abilities[j].kind,AB_DAMAGE);
}
static void spell_trials(void){batch_execution();eligibility_batch();targeting_batch();ai_batch();}
#ifndef REFLECT_CHECK
#ifndef __m68k__
int main(void){sw_init(61);spell_trials();printf("offensive_magic %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();spell_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
