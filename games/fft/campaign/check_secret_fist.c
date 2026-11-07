// Secret Fist: original physical-status formula, frozen packets and countdown.
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-secret-fist/oracle/source.h"
#define SECRET_FIST 104
static void prepare_secret(void)
{
    prepare_cleanse(SECRET_FIST);st.unit[10].team=TEAM_ENEMY;
    st.unit[U_RAMZA].ma=14;st.unit[U_RAMZA].female=0;st.unit[10].female=0;
}
static void source_profiles(void)
{
    u16 i;u8 k;Unit *a,*t;u32 seed,expected;CastBatch b;
    for(i=0;i<sizeof source_secret_fist/sizeof source_secret_fist[0];i++){
        const typeof(source_secret_fist[0]) *p=&source_secret_fist[i];
        prepare_secret();a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p->ma;a->zodiac=p->az;a->faith=p->af;t->zodiac=p->tz;t->faith=p->tf;t->female=p->sex;
        if(p->armed)a->eq[EQ_WEAPON]=0x13;
        st.support[U_RAMZA]=p->up==1?SUPPORT_ATTACK_UP:p->up==2?SUPPORT_MARTIAL_ARTS:p->up==3?SUPPORT_MAGIC_ATTACK_UP:0;
        st.support[10]=p->defense==1?SUPPORT_DEFENSE_UP:p->defense==2?SUPPORT_MAGIC_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        for(k=0;k<5;k++)t->statuses.inflicted[k]=source_secret_status[p->mode][k];
        if(p->mode==10)t->statuses.immune[4]=1;
        if(p->mode==11)t->statuses.innate[4]=1;
        if(p->gear){t->eq[EQ_ACC]=232;t->eq[EQ_SHIELD]=128;t->face=2;}
        unit_status_refresh(t);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=SECRET_FIST;
        seed=rng32;CHECK(cast_forecast(&b),1);CHECK(b.mask,(u16)1<<10);
        CHECK(b.victim[10].hit,p->forecast);CHECK(rng32==seed,1);
        CHECK(skill_hit(U_RAMZA,10,SECRET_FIST),p->forecast);
        rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;
        b.phase=CAST_BATCH_EMPTY;ZB(61);CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);ZE(61);
        CHECK(b.victim[10].hit,p->hit);CHECK(rng32==expected,1);
        CHECK(b.victim[10].effect,p->hit?AB_STATUS:AB_NULL);
        CHECK(b.victim[10].amount,p->hit?0x401:0);CHECK(p->add,p->hit?1:0);
        CHECK(t->hp,999);CHECK(t->mp,999);CHECK(t->statuses.inflicted[4],source_secret_status[p->mode][4]);
    }
}
static u8 sentence_turn(void)
{
#ifdef FFT_MODULES
    return (u8)mod_call(MOD_ORDER,15,10);
#else
    return status_upkeep(10);
#endif
}
static void secret_production(void)
{
    Unit *a,*t;u8 i;u32 seed;
    prepare_secret();a=&st.unit[U_RAMZA];t=&st.unit[10];a->ma=50;
    CHECK(abilities[SECRET_FIST].kind,AB_STATUS);CHECK(abilities[SECRET_FIST].flags,AF_CAST|AF_AREA);
    CHECK(abilities[SECRET_FIST].range,1);CHECK(abilities[SECRET_FIST].vert,0);
    CHECK(skill_cost(U_RAMZA,SECRET_FIST),0);CHECK(skill_hit(U_RAMZA,10,SECRET_FIST),100);
    seed=rng32;CHECK(cast_order(U_RAMZA,10,SECRET_FIST,1),1);
    CHECK(rng32==seed,1);CHECK(W->cast.phase,CAST_BATCH_READY);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
    CHECK(t->status_ct[15],0);CHECK(core_status_current(&t->statuses,4)&1,0);
    CHECK(W->cast.victim[10].amount,0x401);CHECK(W->cast.victim[10].effect,AB_STATUS);
    // Probability/status payload stay frozen when MA or Faith changes before impact.
    a->ma=1;a->faith=t->faith=0;ZB(62);impact();ZE(62);
    CHECK(rng32==seed,1);CHECK(t->hp,999);CHECK(t->mp,999);CHECK(a->mp,999);
    CHECK(t->status_ct[15],3);CHECK(core_status_current(&t->statuses,4)&1,1);
    CHECK(st.primary.reward||st.gain,1);CHECK(st.action.pending,0);
    t->status_ct[15]=1;impact();CHECK(t->status_ct[15],1);CHECK(t->hp,999);
    CHECK(sentence_turn(),1);CHECK(t->hp,0);CHECK(t->kos,1);CHECK(t->dc,3);
    CHECK(core_status_current(&t->statuses,4)&1,0);
    // Real three-turn lifecycle and Undead expiry share Talk Skill's implementation.
    for(i=0;i<2;i++){
        prepare_secret();t=&st.unit[10];st.unit[U_RAMZA].ma=50;
        if(i)change(10,SID_UNDEAD);
        CHECK(cast_order(U_RAMZA,10,SECRET_FIST,1),1);impact();
        CHECK(sentence_turn(),0);CHECK(t->status_ct[15],2);CHECK(t->hp,999);
        CHECK(sentence_turn(),0);CHECK(t->status_ct[15],1);CHECK(t->hp,999);
        CHECK(sentence_turn(),i?0:1);CHECK(t->hp,i?999:0);
        CHECK(core_status_current(&t->statuses,4)&1,0);
    }
    // A pure status strike neither wakes Sleep nor interrupts an existing Charge.
    prepare_secret();t=&st.unit[10];change(10,SID_SLEEP);st.unit[U_RAMZA].ma=99;
    CHECK(cast_order(U_RAMZA,10,SECRET_FIST,1),1);impact();
    CHECK(core_status_current(&t->statuses,4)&16,16);CHECK(t->hp,999);
    prepare_secret();t=&st.unit[10];unit_status_action(t,8);
    st.pending[10].state=DEFER_CAST_TILE;st.pending[10].ticks=5;st.pending[10].ability=15;
    st.unit[U_RAMZA].ma=99;CHECK(cast_order(U_RAMZA,10,SECRET_FIST,1),1);impact();
    CHECK(st.pending[10].state,DEFER_CAST_TILE);CHECK(st.pending[10].ticks,5);
    // Reapplication keeps an existing Death Sentence counter instead of delaying it.
    prepare_secret();t=&st.unit[10];change(10,SID_SENTENCE);t->status_ct[15]=2;
    st.unit[U_RAMZA].ma=50;CHECK(cast_order(U_RAMZA,10,SECRET_FIST,1),1);impact();CHECK(t->status_ct[15],2);
    // Immunity causes a paid no-effect command with no HP, reactions or rewards.
    prepare_secret();t=&st.unit[10];t->statuses.immune[4]=1;st.unit[U_RAMZA].ma=50;
    CHECK(skill_hit(U_RAMZA,10,SECRET_FIST),0);seed=rng32;
    CHECK(cast_order(U_RAMZA,10,SECRET_FIST,1),1);CHECK(rng32==seed,1);impact();
    CHECK(t->hp,999);CHECK(t->mp,999);CHECK(t->status_ct[15],0);CHECK(st.primary.reward,0);CHECK(st.gain,0);
}
static void gates_and_ai(void)
{
    u32 seed;u16 steps,c;u8 k,found;Unit *a;
    prepare_secret();a=&st.unit[U_RAMZA];change(U_RAMZA,SID_SILENCE);CHECK(can_use(U_RAMZA,SECRET_FIST),1);
    a->mp=0;CHECK(can_use(U_RAMZA,SECRET_FIST),1);
    change(U_RAMZA,SID_FROG);CHECK(can_use(U_RAMZA,SECRET_FIST),0);
    prepare_secret();change(U_RAMZA,SID_BERSERK);CHECK(st.unit[U_RAMZA].status_flags&STATUS_NO_CONTROL,STATUS_NO_CONTROL);
    st.moved=1;W->ai_k=0;steps=0;while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==SECRET_FIST)found=1;CHECK(found,0);
    prepare_secret();change(10,SID_REFLECT);CHECK(magic_reflects(10,SECRET_FIST),0);
    // No self target, horizontal distance1 and exactly equal standing height.
    prepare_secret();seed=rng32;CHECK(cast_order(U_RAMZA,U_RAMZA,SECRET_FIST,1),0);
    unit_place(&st.unit[10],third);CHECK(cast_order(U_RAMZA,10,SECRET_FIST,1),0);
    CHECK(st.action.phase,CTX_IDLE);CHECK(rng32==seed,1);
    prepare_secret();a=&st.unit[U_RAMZA];
    for(c=0;c<MAP_N;c++)if(map_tiles[c].walk&&map_tiles[c].stand!=map_tiles[first].stand&&
        abs((s16)TILE_X[c]-a->x)+abs((s16)TILE_Z[c]-a->z)==1)break;
    CHECK(c<MAP_N,1);if(c<MAP_N){unit_place(&st.unit[10],(u8)c);CHECK(cast_order(U_RAMZA,10,SECRET_FIST,1),0);}
    prepare_secret();st.moved=1;W->ai_k=0;steps=0;
    while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==SECRET_FIST)found=1;
    CHECK(found,1);CHECK(st.unit[10].hp,999);CHECK(st.unit[10].status_ct[15],0);
    prepare_secret();st.unit[10].statuses.immune[4]=1;st.moved=1;W->ai_k=0;steps=0;
    while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==SECRET_FIST)found=1;CHECK(found,0);
}
static void secret_trials(void){source_profiles();secret_production();gates_and_ai();}
#ifndef __m68k__
int main(void){sw_init(61);secret_trials();printf("secret_fist %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();secret_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
