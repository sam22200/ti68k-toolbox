// Reraise and Golem: source formula/finalizer oracle, KO-turn revival, the
// side's Golem pool and its guard against primary Attack/Charge (CORE_PROTECT.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-protect/oracle/source.h"
static void prepare_protect(AbId ab)
{
    prepare_cleanse(ab);st.unit[10].team=TEAM_PLAYER;st.unit[10].ct=10;
}
static u8 dead_tick(u8 id)
{
#ifdef FFT_MODULES
    return mod_call(MOD_FORMULA,14,id);
#else
    return status_dead_turn(id);
#endif
}
static void profiles(void)
{
    u16 i,c;u8 k,ref;u32 expected;CastBatch b;CastVictim v;
    for(i=0;i<sizeof source_protect/sizeof source_protect[0];i++){
        const u16 *p=source_protect[i];AbId ab=p[0];Unit *a,*t;const u8 *r;
        // Golem targets its caster: only the caster-side rows apply to it.
        if(ab==65&&(p[10]||p[3]!=70&&p[3]!=100))continue;
        prepare_protect(ab);a=&st.unit[U_RAMZA];ref=ab==65?U_RAMZA:10;t=&st.unit[ref];
        a->ma=p[1];a->faith=p[2];if(ab!=65)t->faith=p[3];st.support[U_RAMZA]=p[4]?SUPPORT_MAGIC_ATTACK_UP:0;
        if(ab!=65){st.support[10]=p[5]?SUPPORT_MAGIC_DEFENSE_UP:0;if(p[6])t->statuses.inflicted[3]|=16;}
        a->zodiac=p[7];if(ab!=65){t->zodiac=p[8];t->female=p[9];t->eq[EQ_ACC]=p[11]?232:255;}
        r=cj_section(common,CJ_STATUS)+6*cj_range(common,ab)[11];
        if(p[10]==1)memcpy(t->statuses.immune,r+1,5);
        if(p[10]==2||p[10]==3)memcpy(t->statuses.inflicted,r+1,5);
        if(p[10]==3)memcpy(t->statuses.innate,r+1,5);
        if(p[10]==4)t->statuses.inflicted[0]|=32;
        if(p[10]==5)t->statuses.inflicted[1]|=128;
        if(p[10]==6)t->statuses.inflicted[3]|=1;
        if(p[10]==7)t->statuses.inflicted[0]|=16;
        if(p[10]==8)t->status_flags|=STATUS_IMMORTAL;
        if(p[10]==9){t->statuses.inflicted[0]|=8;t->statuses.inflicted[3]|=8;t->statuses.inflicted[4]|=16;}
        unit_status_refresh(t);
        if(ab!=65){
            for(c=0;c<MAP_N;c++)if(abs((s16)TILE_X[c]-t->x)+abs((s16)TILE_Z[c]-t->z)>2)break;
            CHECK(c<MAP_N,1);unit_place(a,(u8)c);
        }
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=ref;b.follow=1;b.ability=ab;
        rng32=1;CHECK(cast_forecast(&b),1);v=b.victim[ref];
        CHECK(v.hit,p[12]?p[13]:0);CHECK(v.effect,p[14]);CHECK(v.amount,p[15]);CHECK(rng32==1,1);
        rng32=1;for(k=0;k<p[21];k++)rand15();expected=rng32;rng32=1;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);v=b.victim[ref];
        CHECK(v.hit,p[17]);CHECK(v.effect,p[19]);CHECK(v.amount,p[20]);CHECK(rng32==expected,1);
    }
}
// Source KO upkeep: Reraise (not Undead) revives with max HP/10 for one draw;
// at 0 HP it is spent, the counter kept. Undead Reraise ticks the counter.
static void reraise_upkeep(void)
{
    u8 i,k;u32 expected;Unit *t;
    for(i=0;i<sizeof source_reraise_upkeep/sizeof source_reraise_upkeep[0];i++){
        const u16 *p=source_reraise_upkeep[i];prepare_protect(7);t=&st.unit[10];
        t->hp=0;t->max_hp=p[1];t->dc=3;t->statuses.inflicted[0]=(u8)(32|(p[0]?16:0));t->statuses.inflicted[2]=32;
        unit_status_refresh(t);rng32=1;for(k=0;k<p[7];k++)rand15();expected=rng32;rng32=1;
        CHECK(dead_tick(10),p[3]!=0);CHECK(rng32==expected,1);CHECK(t->hp,p[3]);CHECK(t->dc,p[6]);
        CHECK(core_status_current(&t->statuses,2)&32,p[5]?0:32);CHECK(core_status_current(&t->statuses,0)&32,p[3]?0:32);
    }
}
static void cast(AbId ab,u8 ref)
{
    u8 k;
    CHECK(cast_order(U_RAMZA,ref,ab,1),2);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);impact();
    for(k=0;k<16&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.action.phase,CTX_IDLE);
}
static void enemy_attack(u16 damage)
{
    attack(10,U_RAMZA);st.hit=1;st.push=0;st.dmg=damage;st.act_t=ACT_HIT;attack_step();
}
static void protect_production(void)
{
    u8 k;Unit *u;
    prepare_protect(7);st.unit[U_RAMZA].ma=99;st.unit[U_RAMZA].faith=st.unit[10].faith=100;
    cast(7,10);CHECK(core_status_current(&st.unit[10].statuses,2)&32,32);
    // Golem: the caster's side pool is its max HP; a primary Attack is wholly absorbed.
    prepare_protect(65);u=&st.unit[U_RAMZA];u->ma=99;u->faith=100;u->max_hp=u->hp=300;
    CHECK(st.golem[0],0);cast(65,U_RAMZA);CHECK(st.golem[0],300);CHECK(st.golem[1],0);
    st.unit[10].team=TEAM_ENEMY;CHECK(attack_prediction(10,U_RAMZA),0);
    enemy_attack(120);CHECK(u->hp,300);CHECK(st.golem[0],180);CHECK(st.effect_kind,AB_GOLEM);
    for(k=0;k<16&&st.action.phase!=CTX_IDLE;k++)end_anim();
    enemy_attack(500);CHECK(u->hp,300);CHECK(st.golem[0],0);
    for(k=0;k<16&&st.action.phase!=CTX_IDLE;k++)end_anim();
    enemy_attack(40);CHECK(u->hp,260);CHECK(st.golem[0],0);
    for(k=0;k<16&&st.action.phase!=CTX_IDLE;k++)end_anim();
    // The enemy side has no pool: its units still take the hit.
    arena();st.golem[0]=500;root(30);CHECK(st.unit[10].hp,969);CHECK(st.golem[0],500);
}
static u8 plan(u8 who)
{
    u16 steps=0;st.act=who;st.moved=0;st.acted=0;W->ai_k=0;
    while(!ai_think()&&++steps<2048){}
    return steps<2048;
}
// Immediate AI: Golem while its side has none, Reraise on an ally.
static void ai(void)
{
    u8 k,found;
    prepare_protect(65);st.unit[U_RAMZA].ma=99;CHECK(plan(U_RAMZA),1);found=0;
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==65)found=1;CHECK(found,1);
    prepare_protect(65);st.golem[0]=100;CHECK(plan(U_RAMZA),1);found=0;
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==65)found=1;CHECK(found,0);
    prepare_protect(7);st.unit[U_RAMZA].ma=99;CHECK(plan(U_RAMZA),1);found=0;
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==7)found=1;CHECK(found,1);
}
static void protect_trials(void){profiles();reraise_upkeep();protect_production();ai();}
#ifndef __m68k__
int main(void){sw_init(61);protect_trials();printf("protect %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();protect_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
