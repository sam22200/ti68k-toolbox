// Whole-family AI, response payloads, inventory and Auto-Potion upgrades.
#define RESTORATION_RUNTIME_CHECK
#include "check_restoration.c"
static void resource_ai(void)
{
    u8 j,k,found;u16 steps;
    for(j=0;j<sizeof resources/sizeof resources[0];j++){
        AbId ab=resources[j];prepare_cleanse(ab);
        seed_resources(&st.unit[10],300,100,ab==381?0:100,0,ab==381?2:0);
        st.unit[10].set=11;st.unit[10].learned=1;
        st.moved=1;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,1);CHECK(st.bag[abilities[ab].bag],3);CHECK(st.unit[10].mp,0);
    }
    for(j=0;j<2;j++){
        AbId ab=j?373:371;prepare_cleanse(ab);seed_resources(&st.unit[10],300,100,300,0,0);
        st.unit[10].learned=0;st.moved=1;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,0);
    }
    prepare_cleanse(381);seed_resources(&st.unit[10],300,100,100,0,1);
    st.unit[10].team=TEAM_ENEMY;st.moved=1;W->ai_k=0;steps=0;
    while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==381)found=1;
    CHECK(found,1);CHECK(st.unit[10].hp,100);
}
static void resource_responses(void)
{
    u8 k,item;u16 value;Unit *t;
    prepare_cleanse(373);t=&st.unit[10];seed_resources(t,300,100,100,20,0);
    skill(U_RAMZA,10,373);impact();CHECK(st.effect_mp,100);
    st.reaction[10]=REACTION_COUNTER;t->team=TEAM_ENEMY;
    CHECK(core_action_queue(&st.action,10,REACTION_COUNTER,NUNIT),CTX_OK);
    end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.primary.effect_mp,100);
    CHECK(st.effect_mp,0);impact();end_anim();CHECK(st.effect_kind,AB_HP_MP_HEAL);CHECK(st.effect_mp,100);
    CHECK(st.dmg,300);CHECK(t->hp,300);CHECK(t->mp,100);
    for(item=1;item<3;item++){
        prepare_cleanse(368);t=&st.unit[10];seed_resources(t,100,20,60,0,0);
        st.bag[0]=st.bag[1]=st.bag[2]=0;st.bag[item]=1;t->brave=100;
        st.reaction[10]=REACTION_POTION;act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=1;st.dmg=5;
        impact();CHECK(t->hp,55);CHECK(st.action.pending&((u16)1<<10),(u16)1<<10);
        end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.abil,368+item);
        CHECK(st.effect_kind,AB_HEAL);value=st.dmg;CHECK(value,item==1?70:150);
        impact();CHECK(t->hp,100);CHECK(st.bag[item],0);CHECK(st.action.pending,0);
        for(k=0;k<12&&st.action.phase!=CTX_IDLE;k++){if(st.action.phase==CTX_SECONDARY)impact();end_anim();}
        CHECK(st.action.phase,CTX_IDLE);CHECK(st.bag[item],0);
    }
}
static void resource_guards(void)
{
    u8 j;u32 seed;Unit *t;
    for(j=0;j<sizeof resources/sizeof resources[0];j++){
        AbId ab=resources[j];prepare_cleanse(ab);t=&st.unit[10];
        st.bag[abilities[ab].bag]=0;CHECK(can_use(U_RAMZA,ab),0);seed=rng32;
        skill(U_RAMZA,10,ab);CHECK(st.action.phase,CTX_IDLE);CHECK(rng32==seed,1);
        prepare_cleanse(ab);t=&st.unit[10];change(U_RAMZA,SID_SILENCE);
        seed_resources(t,300,100,ab==381?0:100,20,ab==381?2:0);
        CHECK(can_use(U_RAMZA,ab),1);skill(U_RAMZA,10,ab);CHECK(st.hit,1);CHECK(st.bag[abilities[ab].bag],2);
        prepare_cleanse(ab);t=&st.unit[10];t->dc=DC_CRYSTAL;CHECK(skill_hit(U_RAMZA,10,ab),0);
        t->dc=3;t->statuses.inflicted[0]=4;CHECK(skill_hit(U_RAMZA,10,ab),0);
    }
    prepare_cleanse(368);t=&st.unit[10];seed_resources(t,300,100,100,20,1|32);
    st.reaction[10]=REACTION_POTION;st.bag[0]=3;t->brave=100;
    skill(U_RAMZA,10,368);impact();CHECK(t->hp,70);CHECK(t->statuses.inflicted[4]&16,0);
    CHECK(st.action.pending&((u16)1<<10),(u16)1<<10);
    for(j=0;j<6;j++){
        AbId ab=resources[j];prepare_cleanse(ab);t=&st.unit[10];seed_resources(t,300,100,300,100,0);
        skill(U_RAMZA,10,ab);CHECK(st.hit,1);impact();CHECK(st.gain,0);CHECK(st.jpg,0);
        CHECK(st.bag[abilities[ab].bag],2);
    }
}
static void edge_trials(void){resource_ai();resource_responses();resource_guards();}
#ifndef __m68k__
int main(void){sw_init(61);edge_trials();printf("restoration_edges %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();edge_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
