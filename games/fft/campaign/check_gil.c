// One economic effect behind Gil Taking's physical and Negotiate's Talk accuracy.
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-gil/oracle/source.h"
static const u8 gil_status[6][5]={{0},{0,0,0,0x20,0},{0,0,0,0,0x10},{8,0,0,0,0},{0,0,4,0,0},{0,0,2,0,0}};
static void prepare_gil(AbId ab)
{
    prepare_cleanse(ab);st.unit[10].team=TEAM_ENEMY;st.unit[10].ct=10;
    st.unit[10].hp=st.unit[10].max_hp=999;
    qput(battle_save.money,2000);
}
static void funds_equal(u32 expected)
{
    u32 actual=qlong(battle_save.money);CHECK(actual>>16,expected>>16);CHECK(actual,expected);
}
static void profiles(void)
{
    u16 i;u8 k;CastBatch b;
    for(i=0;i<sizeof source_gil/sizeof source_gil[0];i++){
        const typeof(source_gil[0]) *p=&source_gil[i];Unit *a,*t;u32 seed;
        prepare_gil(p->ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->sp=p->sp;a->level=p->level;a->ma=p->ma;a->zodiac=p->az;t->zodiac=p->tz;t->female=0;
        a->team=p->ap?TEAM_PLAYER:TEAM_ENEMY;t->team=p->tp?TEAM_PLAYER:TEAM_ENEMY;
        st.support[U_RAMZA]=p->up?SUPPORT_ATTACK_UP:p->martial?SUPPORT_MARTIAL_ARTS:0;
        memset(&t->statuses,0,sizeof t->statuses);
        for(k=0;k<5;k++)t->statuses.inflicted[k]=gil_status[p->mode][k];unit_status_refresh(t);
        qput(battle_save.money,p->funds);seed=rng32;
        if(p->ab==108){
            u16 chance=charge_query(U_RAMZA,10,108,0,0);
            if(p->hit)CHECK(chance>100?100:chance,p->chance);
            CHECK(skill_hit(U_RAMZA,10,108),p->hit?p->chance:0);
            CHECK(skill_amount(U_RAMZA,10,108,0),p->amount);
            CHECK(rng32==seed,1);funds_equal(p->funds);
            rng32=p->seed;skill(U_RAMZA,10,108);CHECK(st.hit,p->execute);
            if(st.hit)CHECK(st.dmg,p->amount);
            impact();funds_equal(p->after);impact();funds_equal(p->after);
        }else{
            u32 expected;
            // The isolated formula accepts either team; retail targeting accepts enemies only.
            a->team=p->tp?TEAM_ENEMY:TEAM_PLAYER;
            memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=123;
            CHECK(cast_forecast(&b),1);CHECK(b.victim[10].hit,p->hit?p->chance:0);
            CHECK(b.victim[10].amount,p->amount);CHECK(rng32==seed,1);funds_equal(p->funds);
            rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;
            b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
            CHECK(b.victim[10].hit,p->execute);CHECK(rng32==expected,1);funds_equal(p->funds);
            if(p->execute){CHECK(b.victim[10].effect,AB_STEAL_GIL);CHECK(b.victim[10].amount,p->amount);}
            // Production ordering/ownership is checked below; commit this frozen oracle packet.
            a->team=p->ap?TEAM_PLAYER:TEAM_ENEMY;
            CHECK(st.action.phase,CTX_ROOT);
            st.atk=U_RAMZA;st.abil=123;CHECK(cast_batch(CAST_BATCH_COMMIT,&b)&1,1);
            funds_equal(p->after);CHECK(cast_batch(CAST_BATCH_COMMIT,&b),0);funds_equal(p->after);
        }
    }
}
static void gil_production(void)
{
    u8 j;AbId ab;Unit *a,*t;u32 seed;
    for(j=0;j<2;j++){
        ab=j?123:108;prepare_gil(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->sp=12;a->level=50;a->ma=99;
        CHECK(abilities[ab].kind,AB_STEAL_GIL);CHECK(skill_cost(U_RAMZA,ab),0);CHECK(can_use(U_RAMZA,ab),1);
        seed=rng32;CHECK(skill_amount(U_RAMZA,10,ab,0),600);CHECK(rng32==seed,1);
        ZB(61);
        if(j){CHECK(cast_order(U_RAMZA,10,ab,1),1);CHECK(W->cast.victim[10].amount,600);}
        else skill(U_RAMZA,10,ab);
        ZE(61);
        CHECK(st.mode,M_ACT);if(!j)CHECK(st.hit,1);funds_equal(2000);
        // Freeze the transfer amount; later stat changes must not change the prepared packet.
        a->sp=1;a->level=1;ZB(62);impact();ZE(62);funds_equal(2600);CHECK(t->hp,999);CHECK(t->mp,999);
        CHECK(st.primary.reward||st.gain,1);impact();funds_equal(2600);
        prepare_gil(ab);a=&st.unit[U_RAMZA];a->sp=12;a->level=50;a->ma=99;
        a->team=TEAM_ENEMY;st.unit[10].team=TEAM_PLAYER;qput(battle_save.money,19);
        if(j)CHECK(cast_order(U_RAMZA,10,ab,1),1);else skill(U_RAMZA,10,ab);
        impact();funds_equal(0);
        prepare_gil(ab);st.unit[10].team=TEAM_PLAYER;qput(battle_save.money,0);
        CHECK(skill_hit(U_RAMZA,10,ab),0);funds_equal(0);
        // The source Silence flag blocks Negotiate, not Gil Taking; Frog blocks both.
        prepare_gil(ab);change(U_RAMZA,SID_SILENCE);CHECK(can_use(U_RAMZA,ab),j?0:1);
        change(U_RAMZA,SID_FROG);CHECK(can_use(U_RAMZA,ab),0);
        prepare_gil(ab);CHECK(abilities[ab].range,j?3:1);CHECK(abilities[ab].vert,j?6:2);
        {char text[28];forecast_text(U_RAMZA,10,ab,text);CHECK(memcmp(text,"Steal ",6),0);}
        // A forced miss cannot grant a success reward or alter the funds.
        st.unit[U_RAMZA].team=TEAM_ENEMY;st.unit[10].team=TEAM_PLAYER;qput(battle_save.money,0);
        if(j)CHECK(cast_order(U_RAMZA,10,ab,1),1);else skill(U_RAMZA,10,ab);
        impact();funds_equal(0);CHECK(st.primary.reward|st.gain,0);
        // Guest is a control policy, not a different source camp: Gariland's
        // Delita has ENTD team flags 0x84 (blue camp), despite native TEAM_GUEST.
        prepare_gil(ab);a=&st.unit[U_RAMZA];a->team=TEAM_GUEST;a->sp=12;a->level=50;a->ma=99;
        if(j)CHECK(cast_order(U_RAMZA,10,ab,1),1);else skill(U_RAMZA,10,ab);
        impact();funds_equal(2600);
        prepare_gil(ab);a=&st.unit[U_RAMZA];a->team=TEAM_ENEMY;a->sp=12;a->level=50;a->ma=99;
        st.unit[10].team=TEAM_GUEST;qput(battle_save.money,19);
        CHECK(skill_amount(U_RAMZA,10,ab,0),19);
        if(j)CHECK(cast_order(U_RAMZA,10,ab,1),1);else skill(U_RAMZA,10,ab);
        impact();funds_equal(0);
    }
    prepare_gil(123);change(10,SID_SLEEP);seed=rng32;
    CHECK(magic_hit(U_RAMZA,10,123,1),0);CHECK(rng32==seed,1);
}
static void ai_policy(void)
{
    u8 j,k,found;u16 steps;
    for(j=0;j<2;j++){
        prepare_gil(j?123:108);st.act=U_RAMZA;st.moved=st.acted=W->ai_k=0;steps=0;found=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==(j?123:108))found=1;
        CHECK(found,0);
    }
}
static void gil_trials(void){profiles();gil_production();ai_policy();}
#ifndef __m68k__
int main(void){sw_init(61);gil_trials();printf("gil %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();gil_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
